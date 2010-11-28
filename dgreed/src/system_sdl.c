#include "system.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "memory.h"
#include "wav.h"

#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define BUCKET_COUNT 16
#define LINE_BUCKET_SIZE 1024 
#define RECT_BUCKET_SIZE 512
#define MAX_TEXTURES 64
#define FPS_LIMIT 60
#define MS_PER_FRAME (1000 / FPS_LIMIT)

typedef struct {
	char* file;
	uint retain_count;
	uint width, height;
	uint gl_id;
	bool active;
} Texture;	

typedef struct {
	TexHandle tex;
	RectF source;
	RectF dest;
	Color tint;
	float rotation;
} TexturedRectDesc;	

typedef struct {
	Vector2 start;
	Vector2 end;
	Color color;
} LineDesc;	

bool draw_gfx_debug = false;

TexturedRectDesc rect_buckets[BUCKET_COUNT][RECT_BUCKET_SIZE];
uint rect_buckets_sizes[BUCKET_COUNT] = {0};

LineDesc line_buckets[BUCKET_COUNT][LINE_BUCKET_SIZE];
uint line_buckets_sizes[BUCKET_COUNT] = {0};

Texture textures[MAX_TEXTURES];
uint texture_count;

uint frame;

float x_size_factor, y_size_factor;

#ifndef NO_DEVMODE
VideoStats v_stats;

const VideoStats* video_stats(void) {
	return &v_stats;
}
#endif

extern int dgreed_main(int, char**);
#ifdef __APPLE__
int SDL_main(int argc, char** argv) {
#else
#ifdef main
#undef main
#endif
int main(int argc, char** argv) {
#endif
	return dgreed_main(argc, argv);
}

void _radix_pass_rect(TexturedRectDesc* in, TexturedRectDesc* out, uint count,
	uint radix) {
	assert(in);
	assert(out);
	assert(count <= RECT_BUCKET_SIZE);
	assert(radix < 4);

	uint i;
	uint counts[256] = {0};
	uint start_index[256] = {0};

	for(i = 0; i < count; ++i)
		counts[(in[i].tex >> (radix * 8)) & 0xFF]++;
	for(i = 1; i < 256; ++i)
		start_index[i] = start_index[i-1] + counts[i-1];
	for(i = 0; i < count; ++i)
		out[start_index[(in[i].tex >> (radix * 8)) & 0xFF]++] = in[i];
}		

// Sorts single bucket of TexturedRectDescs using radix sort.
void _sort_rect_bucket(uint bucket) {
	assert(bucket < BUCKET_COUNT);
	assert(rect_buckets_sizes[bucket] > 1);

	TexturedRectDesc rect_sorted_temp[RECT_BUCKET_SIZE];

	_radix_pass_rect(rect_buckets[bucket], rect_sorted_temp,
		rect_buckets_sizes[bucket], 0);
	_radix_pass_rect(rect_sorted_temp, rect_buckets[bucket],
		rect_buckets_sizes[bucket], 1);
	_radix_pass_rect(rect_buckets[bucket], rect_sorted_temp,
		rect_buckets_sizes[bucket], 2);
	_radix_pass_rect(rect_sorted_temp, rect_buckets[bucket],
		rect_buckets_sizes[bucket], 3);
}	


void video_init(uint width, uint height, const char* name) {
	video_init_ex(width, height, width, height, name, false);
}	

void video_init_ex(uint width, uint height, uint v_width, uint v_height, const
	char* name, bool fullscreen) {
	assert(width <= 1600 && width >= 320);
	assert(height <= 1200 && height >= 240);
	assert(v_width != 0);
	assert(v_height != 0);

	if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0)
		LOG_ERROR("Unable to initialize SDL");
	SDL_WM_SetCaption(name, NULL);	

	uint flags = SDL_OPENGL | SDL_DOUBLEBUF;
	if(fullscreen)
		flags |= SDL_FULLSCREEN;
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if(SDL_SetVideoMode(width, height, 32, flags) == NULL)
		LOG_ERROR("Unable to set video mode");

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glViewport(0, 0, width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (double)v_width, (double)v_height, 0.0);

	frame = 0;
	texture_count = 0;
	
	uint i;
	for(i = 0; i < MAX_TEXTURES; ++i)
		textures[i].active = false;

	x_size_factor = (float)v_width / (float)width;
	y_size_factor = (float)v_height / (float)height;

	#ifndef NO_DEVMODE
	memset(&v_stats, 0, sizeof(v_stats));	
	v_stats.n_layers = BUCKET_COUNT;
	v_stats.layer_rects = (uint*)MEM_ALLOC(sizeof(uint) * BUCKET_COUNT);
	v_stats.layer_lines = (uint*)MEM_ALLOC(sizeof(uint) * BUCKET_COUNT);
	#endif

	LOG_INFO("Video initialized");
}	

// TODO: Fix this
void video_close(void) {
	SDL_Quit();

	#ifndef NO_DEVMODE
	MEM_FREE(v_stats.layer_rects);
	MEM_FREE(v_stats.layer_lines);
	#endif

	// ...
	LOG_INFO("Video closed");
}	

void _color_to_4f(Color c, float* red, float* green,
	float* blue, float* alpha) {
	*red = (float)(c&0xFF) / 255.0f;
	c >>= 8;
	*green = (float)(c&0xFF) / 255.0f;
	c >>= 8;
	*blue = (float)(c&0xFF) / 255.0f;
	c >>= 8;
	*alpha = (float)(c&0xFF) / 255.0f;
}	

// Forward def for video_present
uint _get_gl_id(TexHandle tex);

void video_present(void) {
	uint i, j;

	#ifndef NO_DEVMODE
	v_stats.frame = frame+1;
	v_stats.active_textures = texture_count;
	v_stats.frame_layer_sorts = v_stats.frame_batches = v_stats.frame_rects = 0;
	v_stats.frame_lines = v_stats.frame_texture_switches = 0;
	memcpy(v_stats.layer_rects, rect_buckets_sizes, sizeof(rect_buckets_sizes));
	memcpy(v_stats.layer_lines, line_buckets_sizes, sizeof(line_buckets_sizes));
	#endif

	// Sort texture rects to minimize texture binding 
	for(i = 0; i < BUCKET_COUNT; ++i) {
		if(rect_buckets_sizes[i] <= 1)
			continue;
		_sort_rect_bucket(i);	
		
		#ifndef NO_DEVMODE
		v_stats.frame_layer_sorts++;
		#endif
	}	

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Not a typo
	uint active_tex = -1;
	bool drawing = false;
	float r, g, b, a;	
	for(i = 0; i < BUCKET_COUNT; ++i) {
		// Draw rects
		for(j = 0; j < rect_buckets_sizes[i]; ++j) {
			TexturedRectDesc rect = rect_buckets[i][j];
			if(active_tex != rect.tex) {
				if(drawing) {
					glEnd();
					drawing = false;

					#ifndef NO_DEVMODE
					v_stats.frame_batches++;
					#endif
				}	
				glBindTexture(GL_TEXTURE_2D, _get_gl_id(rect.tex));
				active_tex = rect.tex;

				#ifndef NO_DEVMODE
				v_stats.frame_texture_switches++;
				#endif
			}	
			if(!drawing) {
				glBegin(GL_QUADS);
				drawing = true;
			}	
			_color_to_4f(rect.tint, &r, &g, &b, &a);

			glColor4f(r, g, b, a);

			if(rect.rotation == 0.0f) {
				glTexCoord2f(rect.source.left, rect.source.top);
				glVertex2f(rect.dest.left, rect.dest.top);
				
				glTexCoord2f(rect.source.right, rect.source.top);
				glVertex2f(rect.dest.right, rect.dest.top);

				glTexCoord2f(rect.source.right, rect.source.bottom);
				glVertex2f(rect.dest.right, rect.dest.bottom);

				glTexCoord2f(rect.source.left, rect.source.bottom);
				glVertex2f(rect.dest.left, rect.dest.bottom);
			}
			else {
				float rot = rect.rotation;
				Vector2 tl = vec2(rect.dest.left, rect.dest.top);
				Vector2 tr = vec2(rect.dest.right, rect.dest.top);
				Vector2 br = vec2(rect.dest.right, rect.dest.bottom);
				Vector2 bl = vec2(rect.dest.left, rect.dest.bottom);
				Vector2 cnt = vec2((rect.dest.left + rect.dest.right) / 2.0f,
					(rect.dest.top + rect.dest.bottom) / 2.0f);

				tl = vec2_add(vec2_rotate(vec2_sub(tl, cnt), rot), cnt);
				tr = vec2_add(vec2_rotate(vec2_sub(tr, cnt), rot), cnt);
				br = vec2_add(vec2_rotate(vec2_sub(br, cnt), rot), cnt);
				bl = vec2_add(vec2_rotate(vec2_sub(bl, cnt), rot), cnt);

				glTexCoord2f(rect.source.left, rect.source.top);
				glVertex2f(tl.x, tl.y);
				
				glTexCoord2f(rect.source.right, rect.source.top);
				glVertex2f(tr.x, tr.y);

				glTexCoord2f(rect.source.right, rect.source.bottom);
				glVertex2f(br.x, br.y);

				glTexCoord2f(rect.source.left, rect.source.bottom);
				glVertex2f(bl.x, bl.y);
			}	
		}
		if(drawing) {
			glEnd();
			drawing = false;

			#ifndef NO_DEVMODE
			v_stats.frame_batches++;
			#endif
		}	

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		if(draw_gfx_debug) {
			for(j = 0; j < rect_buckets_sizes[i]; ++j) {
				TexturedRectDesc rect = rect_buckets[i][j];

				float rot = rect.rotation;
				Vector2 tl = vec2(rect.dest.left, rect.dest.top);
				Vector2 tr = vec2(rect.dest.right, rect.dest.top);
				Vector2 br = vec2(rect.dest.right, rect.dest.bottom);
				Vector2 bl = vec2(rect.dest.left, rect.dest.bottom);
				Vector2 cnt = vec2((rect.dest.left + rect.dest.right) / 2.0f,
					(rect.dest.top + rect.dest.bottom) / 2.0f);

				tl = vec2_add(vec2_rotate(vec2_sub(tl, cnt), rot), cnt);
				tr = vec2_add(vec2_rotate(vec2_sub(tr, cnt), rot), cnt);
				br = vec2_add(vec2_rotate(vec2_sub(br, cnt), rot), cnt);
				bl = vec2_add(vec2_rotate(vec2_sub(bl, cnt), rot), cnt);

				glVertex2f(tl.x, tl.y);
				glVertex2f(tr.x, tr.y);

				glVertex2f(tr.x, tr.y);
				glVertex2f(br.x, br.y);

				glVertex2f(br.x, br.y);
				glVertex2f(bl.x, bl.y);

				glVertex2f(bl.x, bl.y);
				glVertex2f(tl.x, tl.y);
			}
		}

		// Draw lines
		for(j = 0; j < line_buckets_sizes[i]; ++j) {
			LineDesc line = line_buckets[i][j];
			_color_to_4f(line.color, &r, &g, &b, &a);

			glColor4f(r, g, b, a);

			glVertex2f(line.start.x, line.start.y);
			glVertex2f(line.end.x, line.end.y);
		}

		glEnd();
		glEnable(GL_TEXTURE_2D);

		#ifndef NO_DEVMODE
		v_stats.frame_batches++;
		v_stats.frame_rects += rect_buckets_sizes[i];
		v_stats.frame_lines += line_buckets_sizes[i];
		#endif
	}	

	SDL_GL_SwapBuffers();
	frame++;

	for(i = 0; i < BUCKET_COUNT; ++i) {	
		rect_buckets_sizes[i] = 0;
		line_buckets_sizes[i] = 0;
	}	
}

uint video_get_frame(void) {
	return frame;
}	

TexHandle tex_load(const char* filename) {

	bool inactive_found = false;
	uint first_inactive = 0;

	// Look if texture is already loaded
	for(uint i = 0; i < MAX_TEXTURES; ++i) {
		if(textures[i].active) {
			if(strcmp(textures[i].file, filename) == 0) {
				textures[i].retain_count++;
				return i;
			}	
		}	
		else {
			if(!inactive_found) {
				first_inactive = i;
				inactive_found = true;
			}
		}
	}

	LOG_INFO("Loading texture from file %s", filename);

	TexHandle result = first_inactive;
	texture_count++;

	if(texture_count > MAX_TEXTURES || !inactive_found)
		LOG_ERROR("Texture pool overflow");
	
	// Read file to memory
	FileHandle tex_file = file_open(filename);
	uint size = file_size(tex_file);
	byte* buffer = (byte*)MEM_ALLOC(size);
	file_read(tex_file, buffer, size);
	file_close(tex_file);

	// Convert to raw pixel data
	int width, height, components;
	byte* decompr_data = (byte*)stbi_load_from_memory(buffer, size, &width, &height, 
		&components, 4);
	if(!(is_pow2(width) && is_pow2(height))) {
		stbi_image_free(decompr_data);
		MEM_FREE(buffer);
		LOG_ERROR("Texture dimensions is not power of 2");
	}	

	// Make gl texture
	uint gl_id;
	glGenTextures(1, &gl_id);
	glBindTexture(GL_TEXTURE_2D, gl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, decompr_data); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
	
	uint error = glGetError();
	if(error != GL_NO_ERROR)
		LOG_ERROR("OpenGl error while loading texture, id %u", error);

	// Update texture pool item
	textures[result].width = width;
	textures[result].height = height;
	textures[result].gl_id = gl_id;
	textures[result].file = strclone(filename);
	textures[result].retain_count = 1;
	textures[result].active = true;

	stbi_image_free(decompr_data);
	MEM_FREE(buffer);

	return result;
}	

void tex_size(TexHandle tex, uint* width, uint* height) {
	assert(tex < MAX_TEXTURES);
	assert(textures[tex].active);
	assert(width);
	assert(height);

	*width = textures[tex].width;
	*height = textures[tex].height;
}	

void tex_free(TexHandle tex) {
	assert(tex < MAX_TEXTURES);
	assert(textures[tex].active);
	assert(textures[tex].retain_count > 0);

	textures[tex].retain_count--;
	if(textures[tex].retain_count == 0) {
		glDeleteTextures(1, &textures[tex].gl_id);
		MEM_FREE(textures[tex].file);
		textures[tex].active = false;
	}
}	

uint _get_gl_id(TexHandle tex) {
	assert(tex < MAX_TEXTURES);
	assert(textures[tex].active);

	return textures[tex].gl_id;
}	

void video_draw_rect_rotated(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, float rotation, Color tint) {
	assert(layer < BUCKET_COUNT);
	assert(dest);

	if(rect_buckets_sizes[layer] == RECT_BUCKET_SIZE)
		LOG_ERROR("Rect bucket %u overflow", layer);

	uint texture_width, texture_height;
	tex_size(tex, &texture_width, &texture_height);

	RectF real_source = rectf(0.0f, 0.0f, (float)texture_width,
		(float)texture_height);
	if(source != NULL)
		real_source = *source;

	bool full_dest = dest->right == 0.0f && dest->bottom == 0.0f; 
	RectF real_dest = *dest;

	if(full_dest) {
		float width = real_source.right - real_source.left;
		float height = real_source.bottom - real_source.top;
		real_dest.right = real_dest.left + width;
		real_dest.bottom = real_dest.top + height;
	}	

	real_source.left /= (float)texture_width;
	real_source.top /= (float)texture_height;
	real_source.right /= (float)texture_width;
	real_source.bottom /= (float)texture_height;

	uint rect_idx = rect_buckets_sizes[layer];
	rect_buckets[layer][rect_idx].tex = tex;
	rect_buckets[layer][rect_idx].source = real_source;
	rect_buckets[layer][rect_idx].dest = real_dest;
	rect_buckets[layer][rect_idx].tint = tint;
	rect_buckets[layer][rect_idx].rotation = rotation;
	rect_buckets_sizes[layer]++;
}	

void video_draw_rect(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, Color tint) {
	video_draw_rect_rotated(tex, layer, source, dest, 0.0f, tint);
}	

void video_draw_line(uint layer, const Vector2* start,
	const Vector2* end, Color color) {
	assert(layer < BUCKET_COUNT);
	assert(start);
	assert(end);

	if(line_buckets_sizes[layer] == LINE_BUCKET_SIZE)
		LOG_ERROR("Line bucket %u overflow", layer);

	uint line_idx = line_buckets_sizes[layer];
	line_buckets[layer][line_idx].start = *start;
	line_buckets[layer][line_idx].end = *end;
	line_buckets[layer][line_idx].color = color;
	line_buckets_sizes[layer]++;
}	

/*
-------------
--- Sound ---
-------------
*/

#define MAX_SOURCES 8 
#define MAX_SOUNDS 64
#define STREAM_BUFFER_SIZE 16384 

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
			// Reached end of stream
			if(src->loop) {
				// Restart stream if we're looping
				stb_vorbis_seek_start(sound->stream);	
				size_t end_idx = decoded_samples * sizeof(short);
				if(sound->channels == 2)
					end_idx *= 2;

				decoded_samples = stb_vorbis_get_samples_short_interleaved(
					sound->stream,
					sound->channels,
					(short*)sound->decode_buffer + end_idx,
					(STREAM_BUFFER_SIZE - end_idx) / sizeof(short)
				);	
			}
			else {
				sound_stop_ex(src->handle);
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
		alSourceQueueBuffers(src->al_source, 1, &buffer);
		if(alGetError() != AL_NO_ERROR)
			LOG_ERROR("Streaming error");
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
	assert(src);

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
	assert(src);
	assert(volume >= 0.0f && volume <= 1.0f);
	
	src->volume = volume;
	alSourcef(src->al_source, AL_GAIN, volume);
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
	
/*
-------------
--- Input ---
-------------
*/

uint keybindings[8] = {
	SDLK_UP,
	SDLK_DOWN,
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_z,
	SDLK_x,
	SDLK_p,
	SDLK_ESCAPE
};

// Totaly arbitrary. Only thing known - 256 is not enough.
#define N_KEYS 400
#define N_MOUSE_BUTTONS 4

byte old_keystate[N_KEYS];
byte keystate[N_KEYS];

byte old_mousestate[N_MOUSE_BUTTONS];
byte mousestate[N_MOUSE_BUTTONS];
uint mouse_x, mouse_y;

bool key_pressed(Key key) {
	assert(key < KEY_COUNT);
	return keystate[keybindings[key]];
}	

bool char_pressed(char c) {
	return keystate[(size_t)c];
}

bool key_down(Key key) {
	assert(key < KEY_COUNT);
	return keystate[keybindings[key]] && !old_keystate[keybindings[key]];
}	

bool char_down(char c) {
	return keystate[(size_t)c] && !old_keystate[(size_t)c];
}

bool key_up(Key key) {
	assert(key < KEY_COUNT);
	return !keystate[keybindings[key]] && old_keystate[keybindings[key]];
}

bool char_up(char c) {
	return !keystate[(size_t)c] && old_keystate[(size_t)c];
}

bool mouse_pressed(MouseButton button) {
	assert(button < N_MOUSE_BUTTONS);
	return mousestate[button];
}	

bool mouse_down(MouseButton button) {
	assert(button < N_MOUSE_BUTTONS);
	return mousestate[button] && !old_mousestate[button];
}

bool mouse_up(MouseButton button) {
	assert(button < N_MOUSE_BUTTONS);
	return !mousestate[button] && old_mousestate[button];
}

void mouse_pos(uint* x, uint* y) {
	assert(x);
	assert(y);

	*x = (uint)((float)mouse_x * x_size_factor);
	*y = (uint)((float)mouse_y * y_size_factor);
}	

float t_ms = 0.0f, t_d = 0.0f;
uint last_frame_time = 0, last_fps_update = 0;
uint fps_count = 0;
uint fps = 0;
uint last_time = 0;

float time_ms(void) {
	return t_ms;
}

float time_delta(void) {
	//return t_d;
	return (float)MS_PER_FRAME;
}

uint time_fps(void) {
	return fps; 
}	

void _time_update(uint current_time) {
	t_ms = (float)current_time;
	t_d = (float)(current_time - last_frame_time);
	if(last_fps_update + 1000 < current_time) {
		fps = fps_count;
		fps_count = 0;
		last_fps_update = current_time;
	}	
	fps_count++;
	last_frame_time = current_time;
}	

uint _sdl_to_greed_mbtn(uint mbtn_id) {
	switch (mbtn_id) {
		case SDL_BUTTON_LEFT:
			return MBTN_LEFT;
		case SDL_BUTTON_RIGHT:
			return MBTN_RIGHT;
		case SDL_BUTTON_MIDDLE:
			return MBTN_MIDDLE;
		default:
			break;
	}
	return MBTN_ELSE;
}

bool system_update(void) {
	SDL_Event evt;
	int n_keys;
	byte* curr_keystate;

	memcpy(old_mousestate, mousestate, sizeof(mousestate));
	memcpy(old_keystate, keystate, sizeof(keystate));
	while(SDL_PollEvent(&evt)) {
		if(evt.type == SDL_MOUSEMOTION) {
			mouse_x = evt.motion.x;
			mouse_y = evt.motion.y;
		}	
		if(evt.type == SDL_MOUSEBUTTONUP) {
			mousestate[_sdl_to_greed_mbtn(evt.button.button)] = 0;
		}
		if(evt.type == SDL_MOUSEBUTTONDOWN) {
			mousestate[_sdl_to_greed_mbtn(evt.button.button)] = 1;
		}		
		if(evt.type == SDL_QUIT)
			return false;
	}		
	curr_keystate = SDL_GetKeyState(&n_keys);
	memcpy(keystate, curr_keystate, n_keys);

	uint curr_time;	
	do {
		curr_time = SDL_GetTicks();
		if(MS_PER_FRAME - (curr_time - last_time) > 15)
			SDL_Delay(5);
	} while(curr_time - last_time < MS_PER_FRAME);

	last_time = curr_time;

	_time_update(curr_time);

	return true;
}	

