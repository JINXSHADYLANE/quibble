#include "system.h"

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "memory.h"
#include "wav.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

/*
-------------
--- Sound ---
-------------
*/

#define MAX_SOURCES 8
#define MAX_SOUNDS 64
#define STREAM_BUFFER_SIZE 32768

typedef struct {
	bool is_stream;
	void* decode_buffer; // Null for samples
	stb_vorbis* stream; // Not used for samples
	int stream_source_id, channels, frequency; // Not used for samples
	ALuint buffer; // Only for samples
	float volume;
	bool active;
} Sound;

typedef struct {
	SourceHandle handle;
	SoundHandle sound;
	ALuint buffers[2]; // Only for streams
	ALuint al_source;
	bool loop;
	float volume, pos;
} Source;

Sound sounds[MAX_SOUNDS];
uint sound_count;

ALCdevice* audio_device = NULL;
ALCcontext* audio_context = NULL;
Source sources[MAX_SOURCES];
uint source_count;
uint source_handle_counter;

#ifndef NO_DEVMODE
SoundStats s_stats;

const SoundStats* sound_stats(void) {
	return &s_stats;
}
#endif

void sound_init(void) {
	audio_device = alcOpenDevice(NULL);
	if(!audio_device)
		LOG_ERROR("Unable to open audio device");

	audio_context = alcCreateContext(audio_device, NULL);
	if(!audio_context)
		LOG_ERROR("Unable to create audio context");
	alcMakeContextCurrent(audio_context);

	for(uint i = 0; i < MAX_SOUNDS; ++i)
		sounds[i].active = false;
	sound_count = 0;

	// Init sources
	ALfloat	null_vec[] = {0.0f, 0.0f, 0.0f, 0.0f};
	ALfloat listener_ori[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};

	source_count = 0;
	source_handle_counter = 1;
	memset(&sources, 0, sizeof(sources));

	for(uint i = 0; i < MAX_SOURCES; ++i)
		alGenSources(1, &sources[i].al_source);

	if(alGetError() != AL_NO_ERROR)
		LOG_ERROR("Unable to create sources");

	for(uint i = 0; i < MAX_SOURCES; ++i) {
		alSourcef(sources[i].al_source, AL_PITCH, 1.0f);
		alSourcef(sources[i].al_source, AL_GAIN, 1.0f);
		alSourcefv(sources[i].al_source, AL_POSITION, null_vec);
		alSourcefv(sources[i].al_source, AL_VELOCITY, null_vec);
		alSourcei(sources[i].al_source, AL_LOOPING, false);
	}

	// Set listener params
	alListenerfv(AL_POSITION, null_vec);
	alListenerfv(AL_VELOCITY, null_vec);
	alListenerfv(AL_ORIENTATION, listener_ori);

	if(alGetError() != AL_NO_ERROR)
		LOG_ERROR("Unable to set sources params");

	#ifndef NO_DEVMODE
	memset(&s_stats, 0, sizeof(s_stats));
	#endif

	LOG_INFO("Sound initialized");
}

void sound_close(void) {
	for(uint i = 0; i < MAX_SOURCES; ++i)
		alDeleteSources(1, &sources[i].al_source);

	if(audio_context)
		alcDestroyContext(audio_context);
	if(audio_device)
		alcCloseDevice(audio_device);

	LOG_INFO("Sound closed");
}

void _sound_update_stream(Source* src) {
	assert(src);

	Sound* sound = &sounds[src->sound];

	ALint state;
	alGetSourcei(src->al_source, AL_SOURCE_STATE, &state);
	if(state == AL_PAUSED)
		return;

	ALint processed_buffers;
	alGetSourcei(src->al_source, AL_BUFFERS_PROCESSED, &processed_buffers);
	while(processed_buffers--) {
		// Decode new data
		int decoded_samples = stb_vorbis_get_samples_short_interleaved(
			sound->stream,
			sound->channels,
			(short*)sound->decode_buffer,
			STREAM_BUFFER_SIZE / sizeof(short)
		);

		int expected_samples = STREAM_BUFFER_SIZE / sizeof(short);
		if(sound->channels == 2)
			expected_samples /= 2;

		if(decoded_samples < expected_samples) {
			LOG_INFO("End of stream");
			// Reached end of stream
			if(src->loop) {
				// Restart stream if we're looping
				stb_vorbis_seek_start(sound->stream);
				size_t end_idx = decoded_samples * sizeof(short);
				if(sound->channels == 2)
					end_idx *= 2;
				memset(sound->decode_buffer + end_idx, 0,
					STREAM_BUFFER_SIZE - end_idx);
				/*
				decoded_samples = stb_vorbis_get_samples_short_interleaved(
					sound->stream,
					sound->channels,
					(short*)sound->decode_buffer + end_idx,
					(STREAM_BUFFER_SIZE - end_idx) / sizeof(short)
				);
				*/
			}
			else {
				sound_stop_ex(src->handle);
				return;
			}
		}

		// Update buffer queue
		ALuint buffer;
		alSourceUnqueueBuffers(src->al_source, 1, &buffer);
		alBufferData(
			buffer,
			sound->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
			sound->decode_buffer,
			STREAM_BUFFER_SIZE,
			sound->frequency
		);
		if(alGetError() != AL_NO_ERROR)
			LOG_ERROR("Streaming error: alBufferData");

		alSourceQueueBuffers(src->al_source, 1, &buffer);

		if(alGetError() != AL_NO_ERROR)
			LOG_ERROR("Streaming error: alSourceQueueBuffers");
	}

	// Resume playing if there was buffer under-run
	if(state == AL_STOPPED)
		alSourcePlay(src->al_source);
}

void sound_update(void) {
	#ifndef NO_DEVMODE
	memset(&s_stats, 0, sizeof(s_stats));

	for(uint i = 0; i < MAX_SOUNDS; ++i) {

		if(sounds[i].active) {
			if(sounds[i].is_stream)
				s_stats.stream_count++;
			else
				s_stats.sample_count++;
		}
	}
	#endif

	for(uint i = 0; i < source_count; ++i) {
		if(sounds[sources[i].sound].is_stream) {
			#ifndef NO_DEVMODE
			s_stats.playing_streams++;
			#endif
			_sound_update_stream(&sources[i]);
		}
		else {
			#ifndef NO_DEVMODE
			s_stats.playing_samples++;
			#endif
			ALint state;
			alGetSourcei(sources[i].al_source, AL_SOURCE_STATE, &state);
			if(state == AL_STOPPED)
				sound_stop_ex(sources[i].handle);
		}
	}
}

ALenum _choose_sound_format(const RawSound* sound) {
	if(sound->channels == 1 && sound->bits == 8)
		return AL_FORMAT_MONO8;
	if(sound->channels == 1 && sound->bits == 16)
		return AL_FORMAT_MONO16;
	if(sound->channels == 2 && sound->bits == 8)
		return AL_FORMAT_STEREO8;
	if(sound->channels == 2 && sound->bits == 16)
		return AL_FORMAT_STEREO16;
	LOG_ERROR("Bad sound format");
	return 0;
}

SoundHandle sound_load_sample(const char* filename) {
	assert(filename);

	// Find free space in sound pool
	SoundHandle result = 0;
	while(sounds[result].active && result < MAX_SOUNDS) {
		result++;
	}
	sound_count++;
	if(sound_count > MAX_SOUNDS || result == MAX_SOUNDS)
		LOG_ERROR("Sound pool overflow");

	// Create open al buffer
	LOG_INFO("Loading sound from file %s", filename);
	RawSound* wave = wav_load(filename);
	ALuint buffer;
	alGenBuffers(1, &buffer);
	if(alGetError() != AL_NO_ERROR)
		LOG_ERROR("Unable to create sound buffer");
	alBufferData(buffer, _choose_sound_format(wave), wave->data,
		wave->size, wave->frequency);
	wav_free(wave);

	sounds[result].is_stream = false;
	sounds[result].stream_source_id = -1;
	sounds[result].buffer = buffer;
	sounds[result].volume = 1.0f;
	sounds[result].active = true;

	return result;
}

SoundHandle sound_load_stream(const char* filename) {
	assert(filename);

	// Find free space in sound pool
	SoundHandle result = 0;
	while(sounds[result].active && result < MAX_SOUNDS) {
		result++;
	}
	sound_count++;
	if(sound_count > MAX_SOUNDS || result == MAX_SOUNDS)
		LOG_ERROR("Sound pool overflow");

	// Initialize stream
	LOG_INFO("Loading sound stream %s", filename);
	int error;
	// stb_vorbis does not accept const string as file, make copy
	//char* filename_clone = strclone(filename);
	char* filename_clone = path_to_resource(filename);
	stb_vorbis* stream = stb_vorbis_open_filename(filename_clone, &error, NULL);
	MEM_FREE(filename_clone);
	if(stream == NULL)
		LOG_ERROR("Unable to open ogg vorbis file %s", filename);
	stb_vorbis_info info = stb_vorbis_get_info(stream);
	assert(info.sample_rate == 22050 || info.sample_rate == 44100);
	assert(info.channels == 1 || info.channels == 2);

	// Allocate decoding buffer
	void* decode_buffer = MEM_ALLOC(STREAM_BUFFER_SIZE);

	sounds[result].is_stream = true;
	sounds[result].stream = stream;
	sounds[result].decode_buffer = decode_buffer;
	sounds[result].stream_source_id = -1; // Will be assigned later
	sounds[result].channels = info.channels;
	sounds[result].frequency = info.sample_rate;
	sounds[result].volume = 1.0f;
	sounds[result].active = true;

	return result;
}

void sound_free(SoundHandle handle) {
	assert(handle < sound_count);
	assert(sounds[handle].active);

	// If not finished playing ...
	for(int i = 0; i < source_count; ++i) {
		if(sources[i].sound == handle) {
			// Stop
			sound_stop_ex(sources[i].handle);
		}
	}

	// Sound stream
	if(sounds[handle].is_stream) {
		MEM_FREE(sounds[handle].decode_buffer);
		stb_vorbis_close(sounds[handle].stream);
		sounds[handle].active = false;
		return;
	}

	// Sound sample

	// Delete buffer
	alDeleteBuffers(1, &sounds[handle].buffer);
	ALuint error = alGetError();
	if(error != AL_NO_ERROR)
		LOG_ERROR("Unable to delete sound buffer");
	sounds[handle].active = false;
}

void sound_play(SoundHandle handle) {
	assert(handle < sound_count);
	assert(sounds[handle].active);

	sound_play_ex(handle, sounds[handle].is_stream);
}


void sound_stop(SoundHandle handle) {
	assert(handle < sound_count);
	assert(sounds[handle].active);

	if(sounds[handle].is_stream) {
		for(uint i = 0; i < source_count; ++i) {
			if(sources[i].sound == handle)
				sound_stop_ex(sources[i].handle);
		}
	}
}

void sound_set_volume(SoundHandle handle, float volume) {
	assert(handle < sound_count);
	assert(sounds[handle].active);
	assert(volume >= 0.0f && volume <= 1.0f);

	sounds[handle].volume = volume;
	if(sounds[handle].is_stream) {
		for(uint i = 0; i < source_count; ++i) {
			if(sources[i].sound == handle)
				sound_set_volume_ex(sources[i].handle, volume);
		}
	}
}

float sound_get_volume(SoundHandle handle) {
	assert(handle < sound_count);
	assert(sounds[handle].active);

	return sounds[handle].volume;
}

float sound_get_length(SoundHandle handle) {
	assert(handle < sound_count);
	assert(sounds[handle].active);

	if(sounds[handle].is_stream) {
		// TODO
		return 0.0f;
	}
	else {
		ALint freq, channels, bits, size;
		alGetBufferi(sounds[handle].buffer, AL_FREQUENCY, &freq);
		alGetBufferi(sounds[handle].buffer, AL_CHANNELS, &channels);
		alGetBufferi(sounds[handle].buffer, AL_BITS, &bits);
		alGetBufferi(sounds[handle].buffer, AL_SIZE, &size);

		return (float)size / (float)(freq * channels * bits/8);
	}
}

SourceHandle sound_play_ex(SoundHandle handle, bool loop) {
	assert(handle < sound_count);
	assert(sounds[handle].active);

	if(source_count == MAX_SOURCES) {
		LOG_WARNING("Skipping sound");
		return 0;
	}

	Source* src = &sources[source_count++];
	src->handle = source_handle_counter++;
	src->sound = handle;
	src->loop = loop;
	src->volume = sounds[handle].volume;
	src->pos = 0.0f;

	if(sounds[handle].is_stream) {
		for(int i = 0; i < source_count-1; ++i) {
			assert(sources[i].sound != handle);
		}

		alGenBuffers(2, src->buffers);
		if(alGetError() != AL_NO_ERROR)
			LOG_ERROR("Unable to create sound buffers for stream");

		Sound* sound = &sounds[handle];
		assert(sound->decode_buffer);

		for(uint i = 0; i < 2; ++i) {
			stb_vorbis_get_samples_short_interleaved(
				sound->stream,
				sound->channels,
				sound->decode_buffer,
				STREAM_BUFFER_SIZE / sizeof(short)
			);

			alBufferData(
				src->buffers[i],
				sound->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
				sound->decode_buffer,
				STREAM_BUFFER_SIZE,
				sound->frequency
			);
		}

		alSourceQueueBuffers(src->al_source, 2, src->buffers);

	}
	else {
		alSourcei(src->al_source, AL_BUFFER, sounds[handle].buffer);
		alSourcei(src->al_source, AL_LOOPING, loop);
	}

	alSourcef(src->al_source, AL_GAIN, sounds[handle].volume);
	alSourcePlay(src->al_source);


	return src->handle;
}

Source* _find_source(SourceHandle handle) {
	for(uint i = 0; i < source_count; ++i) {
		if(handle == sources[i].handle)
			return &sources[i];
	}
	return NULL;
}

void sound_pause_ex(SourceHandle handle) {
	Source* src = _find_source(handle);
	assert(src);

	alSourcePause(src->al_source);
}

void sound_resume_ex(SourceHandle handle) {
	Source* src = _find_source(handle);
	assert(src);

	ALint state;
	alGetSourcei(src->al_source, AL_SOURCE_STATE, &state);
	if(state == AL_PAUSED)
		alSourcePlay(src->al_source);
}

void sound_stop_ex(SourceHandle handle) {
	Source* src = _find_source(handle);
	if(!src)
		return;

	alSourceStop(src->al_source);
	if(sounds[src->sound].is_stream) {
		// Unqueue buffers
		ALint processed_buffers;
		alGetSourcei(src->al_source, AL_BUFFERS_PROCESSED, &processed_buffers);
		ALuint buffers[2];
		alSourceUnqueueBuffers(src->al_source, processed_buffers, buffers);
		alSourcei(src->al_source, AL_BUFFER, 0);

		alDeleteBuffers(2, src->buffers);
		ALuint error = alGetError();
		if(error != AL_NO_ERROR)
			LOG_ERROR("Unable to delete sound stream buffers");
	}
	else {
		alSourcei(src->al_source, AL_BUFFER, 0);
	}

	Source* last = &sources[source_count-1];
	if(src != last) {
		// Swap
		Source temp = *src;
		*src = *last;
		*last = temp;
	}
	source_count--;
}

void sound_set_volume_ex(SourceHandle source, float volume) {
	Source* src = _find_source(source);
	if(src) {
		assert(volume >= 0.0f && volume <= 1.0f);

		src->volume = volume;
		alSourcef(src->al_source, AL_GAIN, volume);
	}
}

float sound_get_volume_ex(SourceHandle source) {
	Source* src = _find_source(source);
	assert(src);

	return src->volume;
}

float sound_get_pos_ex(SourceHandle source) {
	Source* src = _find_source(source);
	assert(src);

	float pos = 0.0f;
	if(sounds[src->sound].is_stream) {
		// TODO
	}
	else {
		alGetSourcef(src->al_source, AL_SEC_OFFSET, &pos);
	}

	return pos;
}

void sound_set_pos_ex(SourceHandle source, float pos) {
	Source* src = _find_source(source);
	assert(src);

	if(sounds[src->sound].is_stream) {
		// TODO
	}
	else {
		alSourcef(src->al_source, AL_SEC_OFFSET, pos);
	}
}
