#include "system.h"

#include "memory.h"
#include "darray.h"
#include "wav.h"

#import <CoreData/CoreData.h>
#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MPMusicPlayerController.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#import "DGreedAppDelegate.h"

/*
-------------
--- Sound ---
-------------
*/

typedef struct {
	SoundHandle handle;
	bool is_stream, is_paused;
	AVAudioPlayer* av_player;
	ALuint buffer;
	float volume;
} Sound;

typedef struct {
	SourceHandle handle;
	SoundHandle sound;
	ALuint buffer;
	ALuint al_source;
	bool loop;
	float volume, pos;
} Source;

#define max_sources 8

static DArray sounds;
static DArray sources;

static uint sound_handle_counter;
static uint source_handle_counter;

static ALCdevice* audio_device = NULL;
static ALCcontext* audio_context = NULL;

#ifndef NO_DEVMODE
SoundStats s_stats;

const SoundStats* sound_stats(void) {
	return &s_stats;
}
#endif

void sound_init(void) {
	NSError* error = nil;
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:&error];
	//[[AVAudioSession sharedInstance] setActive:true error:&error];

	audio_device = alcOpenDevice(NULL);
	if(!audio_device)
		LOG_ERROR("Unable to open audio device");

	audio_context = alcCreateContext(audio_device, NULL);
	if(!audio_context)
		LOG_ERROR("Unable to create audio context");
	alcMakeContextCurrent(audio_context);

	// Init sources
	ALfloat null_vec[] = {0.0f, 0.0f, 0.0f, 0.0f};
	ALfloat listener_ori[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};

	source_handle_counter = sound_handle_counter = 1;

	sounds = darray_create(sizeof(Sound), 0);
	sources = darray_create(sizeof(Source), max_sources);

	for(uint i = 0; i < max_sources; ++i) {
		Source s = {0, 0, 0, 0, false, 0.0f, 0.0f};
		alGenSources(1, &s.al_source);
		alSourcef(s.al_source, AL_PITCH, 1.0f);
		alSourcef(s.al_source, AL_GAIN, 1.0f);
		alSourcefv(s.al_source, AL_POSITION, null_vec);
		alSourcefv(s.al_source, AL_VELOCITY, null_vec);
		alSourcei(s.al_source, AL_LOOPING, false);
		darray_append(&sources, &s);
	}

	if(alGetError() != AL_NO_ERROR)
		LOG_ERROR("Unable to create resources");

	// Set listener params
	alListenerfv(AL_POSITION, null_vec);
	alListenerfv(AL_VELOCITY, null_vec);
	alListenerfv(AL_ORIENTATION, listener_ori);

	if(alGetError() != AL_NO_ERROR)
		LOG_ERROR("Unable to set source params");

#ifndef NO_DEVMODE
	memset(&s_stats, 0, sizeof(s_stats));
#endif

	LOG_INFO("Sound initialized");
}

void sound_close(void) {
	Source* src = DARRAY_DATA_PTR(sources, Source);
	for(uint i = 0; i < max_sources; ++i)
		alDeleteSources(1, &src[i].al_source);

	if(audio_context)
		alcDestroyContext(audio_context);
	if(audio_device)
		alcCloseDevice(audio_device);

	darray_free(&sounds);
	darray_free(&sources);

	LOG_INFO("Sound closed");
}

void sound_update(void) {
	Sound* sound = DARRAY_DATA_PTR(sounds, Sound);
	Source* source = DARRAY_DATA_PTR(sources, Source);

	#ifndef NO_DEVMODE
	memset(&s_stats, 0, sizeof(s_stats));
	#endif
	for(uint i = 0; i < sounds.size; ++i) {
		if(sound[i].is_stream) {
			#ifndef NO_DEVMODE
			s_stats.stream_count++;
			#endif
			if([sound[i].av_player isPlaying]) {
				#ifndef NO_DEVMODE
				s_stats.playing_streams++;
				#endif
			}
			else {
				if(!sound[i].is_paused) {
					for(uint j = 0; j < sources.size; ++j) {
						if(source[j].sound == sound[i].handle) {
							sound_stop_ex(source[j].handle);
							break;
						}
					}
				}
			}
		}
		else {
			#ifndef NO_DEVMODE
			s_stats.sample_count++;
			#endif
		}
	}

	for(uint i = 0; i < sources.size; ++i) {
		if(source[i].buffer != 0) {
			s_stats.playing_samples++;

			ALint state;
			alGetSourcei(source[i].al_source, AL_SOURCE_STATE, &state);
			if(state == AL_STOPPED)
				sound_stop_ex(source[i].handle);
		}
	}
}

ALenum _choose_sound_format(const RawSound* sound) {
	if(sound->channels == 1) {
		if(sound->bits == 8)
			return AL_FORMAT_MONO8;
		if(sound->bits == 16)
			return AL_FORMAT_MONO16;
	}
	if(sound->channels == 2) {
		if(sound->bits == 8)
			return AL_FORMAT_STEREO8;
		if(sound->bits == 16)
			return AL_FORMAT_STEREO16;
	}
	LOG_ERROR("Bad sound format");
	return 0;
}

SoundHandle sound_load_sample(const char* filename) {
	assert(filename);

	Sound s = {sound_handle_counter++, false, false, NULL, 0, 1.0f};

	LOG_INFO("Loading sound from file %s", filename);
	RawSound* wave = wav_load(filename);
	alGenBuffers(1, &s.buffer);
	if(alGetError() != AL_NO_ERROR)
		LOG_ERROR("Unable to create sound buffer");
	alBufferData(s.buffer, _choose_sound_format(wave),
				 wave->data, wave->size, wave->frequency);
	wav_free(wave);

	darray_append(&sounds, &s);

	return s.handle;
}

static void _log_nserror(NSError* error) {
	NSArray* detailedErrors = [[error userInfo] objectForKey:NSDetailedErrorsKey];
    if(detailedErrors != nil && [detailedErrors count] > 0)
        for(NSError* detailedError in detailedErrors)
            NSLog(@"  DetailedError: %@", [detailedError userInfo]);
    else
        NSLog(@"  %@", [error userInfo]);
}

SoundHandle sound_load_stream(const char* filename) {
	assert(filename);

	Sound s = {sound_handle_counter++, true, true, NULL, 0, 1.0f};
	char* file = path_change_ext(filename, "m4a");
	NSString* ns_file = [NSString stringWithUTF8String:file];
	NSString* resource_path = [[NSBundle mainBundle] resourcePath];
	NSString* full_path = [resource_path stringByAppendingPathComponent:ns_file];
	NSURL* url = [[NSURL alloc] initFileURLWithPath:full_path];

	MEM_FREE(file);

	NSError* err = nil;
    if([[MPMusicPlayerController iPodMusicPlayer] playbackState] == MPMusicPlaybackStatePlaying) {
        s.av_player = nil;
    }
    else {
       s.av_player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&err];
    }
	if(!s.av_player) {
		_log_nserror(err);
		LOG_WARNING("Unable to make AVAudioPlayer from stream");
	}

	[url release];

	darray_append(&sounds, &s);

	return s.handle;
}

Sound* _get_sound(SoundHandle handle, uint* index) {
	Sound* sound = DARRAY_DATA_PTR(sounds, Sound);
	for(uint i = 0; i < sounds.size; ++i) {
		if(handle == sound[i].handle) {
			if(index)
				*index = i;
			return &sound[i];
		}
	}
	assert(0 && "Unable to find sound handle");
	return &sound[0];
}


Source* _get_source(SourceHandle handle) {
	Source* source = DARRAY_DATA_PTR(sources, Source);
	for(uint i = 0; i < sources.size; ++i) {
		if(handle == source[i].handle) {
			return &source[i];
		}
	}
    return NULL;
}

void sound_free(SoundHandle handle) {
	uint idx;
	Sound* s = _get_sound(handle, &idx);
	Source* src = DARRAY_DATA_PTR(sources, Source);
	assert(s);

	// If not finished playing ...
	for(uint i = 0; i < sources.size; ++i) {
		if(src[i].sound == handle) {
			// Stop
			sound_stop_ex(src[i].handle);
		}
	}

	if(s->is_stream) {
		[s->av_player release];
	}
	else {
		// Delete buffer
		alDeleteBuffers(1, &s->buffer);
		ALuint error = alGetError();
		if(error != AL_NO_ERROR)
			LOG_ERROR("Unable to delete sound buffer");
	}
	darray_remove_fast(&sounds, idx);
}

void sound_play(SoundHandle handle) {
	Sound* sound = _get_sound(handle, NULL);
	assert(sound);
	sound_play_ex(handle, sound->is_stream);
}

void sound_stop(SoundHandle handle) {
	Sound* s = _get_sound(handle, NULL);
	assert(s);

	if(s->is_stream) {
		[s->av_player stop];
		s->is_paused = true;
	}
}

void sound_set_volume(SoundHandle handle, float volume) {
	Sound* s = _get_sound(handle, NULL);
	assert(s);

	s->volume = volume;
	if(s->is_stream) {
		[s->av_player setVolume:volume];
	}
}

float sound_get_volume(SoundHandle handle) {
	Sound* s = _get_sound(handle, NULL);
	assert(s);

	return s->volume;
}

float sound_get_length(SoundHandle handle) {
	Sound* s = _get_sound(handle, NULL);
	assert(s);

	if(s->is_stream) {
		return [s->av_player duration];
	}
	else {
		ALint freq, channels, bits, size;
		alGetBufferi(s->buffer, AL_FREQUENCY, &freq);
		alGetBufferi(s->buffer, AL_CHANNELS, &channels);
		alGetBufferi(s->buffer, AL_BITS, &bits);
		alGetBufferi(s->buffer, AL_SIZE, &size);

		return (float)size / (float)(freq * channels * bits/8);
	}
}

SourceHandle sound_play_ex(SoundHandle handle, bool loop) {
	Source* source = DARRAY_DATA_PTR(sources, Source);
	Source* src = NULL;
	for(uint i = 0; i < sources.size; ++i) {
		if(source[i].handle == 0) {
			src = &source[i];
			break;
		}
	}
	if(src == NULL) {
		return 0;
	}

	Sound* sound = _get_sound(handle, NULL);
	assert(sound);

	src->handle = source_handle_counter++;
	src->sound = handle;
	src->buffer = sound->buffer;
	src->loop = loop;
	src->volume = sound->volume;
	src->pos = 0.0f;

	if(sound->is_stream) {
		[sound->av_player setNumberOfLoops:loop ? -1 : 1];
		[sound->av_player setVolume:sound->volume];
		[sound->av_player play];
		sound->is_paused = false;
	}
	else {
		alSourcei(src->al_source, AL_BUFFER, src->buffer);
		alSourcei(src->al_source, AL_LOOPING, loop);
		alSourcef(src->al_source, AL_GAIN, sound->volume);
		alSourcePlay(src->al_source);
	}

	return src->handle;
}

void sound_pause_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	if(!src)
        return;

	if(src->buffer) {
		alSourcePause(src->al_source);
	}
	else {
		Sound* sound = _get_sound(src->sound, NULL);
		assert(sound);
		[sound->av_player pause];
		sound->is_paused = true;
	}
}

void sound_resume_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	if(!src)
        return;

	if(src->buffer) {
		ALint state;
		alGetSourcei(src->al_source, AL_SOURCE_STATE, &state);
		if(state == AL_PAUSED)
			alSourcePlay(src->al_source);
	}
	else {
		Sound* sound = _get_sound(src->sound, NULL);
		assert(sound);
		[sound->av_player play];
		sound->is_paused = false;
	}
}

void sound_stop_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	if(!src)
		return;

	Sound* sound = _get_sound(src->sound, NULL);
	assert(sound);

	if(sound->is_stream) {
		[sound->av_player stop];
	}
	else {
		alSourceStop(src->al_source);
		alSourcei(src->al_source, AL_BUFFER, AL_NONE);
	}

	src->handle = src->sound = src->buffer = 0;
}

void sound_set_volume_ex(SourceHandle handle, float volume) {
	Source* src = _get_source(handle);
	if(src) {
		assert(volume >= 0.0f && volume <= 1.0f);

		src->volume = volume;
		if(!src->buffer) {
			Sound* sound = _get_sound(src->sound, NULL);
			assert(sound);
			[sound->av_player setVolume:volume];
		}
		else {
			alSourcef(src->al_source, AL_GAIN, volume);
		}
	}
}

float sound_get_volume_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	if(src)
        return src->volume;
    else
        return 0.0f;
}

float sound_get_pos_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	if(!src)
        return 0.0f;

	if(!src->buffer) {
		Sound* sound = _get_sound(src->sound, NULL);
		assert(sound);
		return [sound->av_player currentTime];
	}
	else {
		ALfloat pos;
		alGetSourcef(src->al_source, AL_SEC_OFFSET, &pos);
		return pos;
	}
}

void sound_set_pos_ex(SourceHandle handle, float pos) {
	Source* src = _get_source(handle);
	if(!src)
        return;

	if(!src->buffer) {
		Sound* sound = _get_sound(src->sound, NULL);
		assert(sound);
		[sound->av_player setCurrentTime:pos];
	}
	else {
		alSourcef(src->al_source, AL_SEC_OFFSET, pos);
	}
}
