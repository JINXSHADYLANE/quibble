#include "system.h"

#include "memory.h"
#include "darray.h"
#include "wav.h"

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#import "GLESView.h"

// Main function

extern void _async_init(void);
extern void _async_close(void)

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	_async_init();
    int retVal = UIApplicationMain(argc, argv, nil, @"DGreedAppDelegate");
	_async_close();
    [pool release];
    return retVal;
}

/*
-------------
--- Video ---
-------------
*/ 

// Types

typedef struct {
	char* file;
	uint retain_count, width, height;
	uint gl_id;
	bool active;
} Texture;

typedef struct {
	TexHandle tex;
	RectF source, dest;
	Color tint;
	float rotation;
} TexturedRectDesc;

typedef struct {
	Vector2 start, end;
	Color color;
} LineDesc;

typedef struct {
	float x, y, u, v;
	float r, g, b, a;
} Vertex;	

// Globals

#define bucket_count 16
#define max_vertices 1024*4

bool draw_gfx_debug = false;

static DArray rect_buckets[bucket_count];
static DArray line_buckets[bucket_count];
static DArray textures;

static uint frame;
static float x_size_factor, y_size_factor;

static uint radix_counts[256];
static DArray rects_out;
static DArray vertex_buffer;
static uint16 index_buffer[max_vertices/4 * 6];
static uint fps_count = 0;

#ifndef NO_DEVMODE
VideoStats v_stats;

const VideoStats* video_stats(void) {
	return &v_stats;
}
#endif

void _insertion_sort(DArray rect_bucket) {
	assert(rect_bucket.size > 1);
	
	TexturedRectDesc* rects = DARRAY_DATA_PTR(rect_bucket, TexturedRectDesc);
	for(uint i = 1; i < rect_bucket.size; ++i) {
		uint t = i;
		while(t <= i && rects[t].tex > rects[i].tex) t--;
		if(t < i) {
			TexturedRectDesc temp = rects[i];
			rects[i] = rects[t];
			rects[t] = temp;
		}
	}
}

void _sort_rects(DArray rects_in) {
	assert(rects_in.size > 1);
		
	// Insertion sort for small buffers
	if(rects_in.size <= 8) {
		_insertion_sort(rects_in);
		return;
	}	
	
	// Assure out buffer is big enough
	rects_out.size = 0;
	if(rects_out.reserved < rects_in.size)
		darray_reserve(&rects_out, MAX(rects_in.size, rects_out.reserved*2));
	
	assert(rects_out.reserved >= rects_in.size);
	assert(rects_out.item_size == rects_in.item_size);
	
	TexturedRectDesc* r_in = DARRAY_DATA_PTR(rects_in, TexturedRectDesc);
	TexturedRectDesc* r_out = DARRAY_DATA_PTR(rects_out, TexturedRectDesc);
	
	uint i, unique_textures = 0, switches = 0;
	memset(radix_counts, 0, sizeof(radix_counts));
	
	// If there is more than 255 textures our simple count sort won't work
	assert(textures.size < 256);
	
	// Calculate histogram, unsorted texture switches, unique textures
	for(i = 0; i < rects_in.size; ++i) {
		size_t idx = r_in[i].tex & 0xFF;
		radix_counts[idx]++;
		if (radix_counts[idx] == 1)
			unique_textures++;
		if(i > 0 && r_in[i].tex != r_in[i-1].tex)
			switches++;
	}	
	
	// Don't bother sorting if it wouldn't decrease batch count
	if(unique_textures < 3 || unique_textures == switches)
		return;
	
	// Convert histogram to start indices
	for(i = 1; i < 256; ++i)
		radix_counts[i] = radix_counts[i-1] + radix_counts[i];
	
	// Count sort (single pass of radix sort - texture amount is not a large
	// number)
	uint r0 = 0;
	for(i = 0; i < rects_in.size; ++i) {
		size_t idx = r_in[i].tex & 0xFF;
		uint* dest = idx ? &radix_counts[idx-1] : &r0;
		r_out[(*dest)++] = r_in[i];
	}		  
	
	memcpy(r_in, r_out, rects_in.size * rects_in.item_size);	
}

void video_init(uint width, uint height, const char* name) {
	video_init_ex(width, height, width, height, name, false); 
}

void video_init_ex(uint width, uint height, uint v_width, uint v_height,
				   const char* name, bool fullscreen) {
	assert(width >= 480 && width <= 1024);
	assert(height >= 320 && height <= 768);
	assert(v_width != 0 && v_height != 0);
	
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glClearDepthf(1.0f);
	glViewport(0, 0, height, width);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
		
	// Some tricky transformations to properly turn view sideways 
	glOrthof(0.0f, (float)v_width, (float)v_height, 0.0f, -1.0f, 1.0f);
	glTranslatef((float)v_width/2.0f, (float)v_height/2.0f, 0.0f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glTranslatef((float)v_height/-2.0f, (float)v_width/-2.0f, 0.0f);
	glScalef((float)v_height/(float)v_width, (float)v_width/(float)v_height, 1.0f);
	
	frame = 0;
	
	x_size_factor = (float)v_width / (float)width;
	y_size_factor = (float)v_height / (float)height;
	
#ifndef NO_DEVMODE
	memset(&v_stats, 0, sizeof(v_stats));
	v_stats.n_layers = bucket_count;
	v_stats.layer_rects = (uint*)MEM_ALLOC(sizeof(uint) * bucket_count);
	v_stats.layer_lines = (uint*)MEM_ALLOC(sizeof(uint) * bucket_count);
#endif
	
	// Init render buckets
	for(uint i = 0; i < bucket_count; ++i) {
		rect_buckets[i] = darray_create(sizeof(TexturedRectDesc), 32);
		line_buckets[i] = darray_create(sizeof(LineDesc), 32);
	}
	
	// Temp bucket for sorting
	rects_out = darray_create(sizeof(TexturedRectDesc), 32);
	
	textures = darray_create(sizeof(Texture), 16);
	
	// Generate index buffer
	for(uint i = 0; i < max_vertices/4; ++i) {
		index_buffer[i*6 + 0] = i*4 + 0; 
		index_buffer[i*6 + 1] = i*4 + 1; 
		index_buffer[i*6 + 2] = i*4 + 3; 
		index_buffer[i*6 + 3] = i*4 + 2; 
		index_buffer[i*6 + 4] = i*4 + 1; 
		index_buffer[i*6 + 5] = i*4 + 3; 
	}
	
	assert(sizeof(Vertex) == sizeof(float) * 8);
	vertex_buffer = darray_create(sizeof(Vertex), max_vertices);
	
	LOG_INFO("Video initialized");
}

void video_close(void) {
#ifndef NO_DEVMODE
	MEM_FREE(v_stats.layer_rects);
	MEM_FREE(v_stats.layer_lines);
#endif
	
	Texture* tex = DARRAY_DATA_PTR(textures, Texture);
	uint active_textures = 0;
	for(uint i = 0; i < textures.size; ++i)
		if(tex[i].active)
			active_textures++;
	
	if(active_textures > 0)
		LOG_WARNING("%d textures are still active!", textures.size);
	
	darray_free(&vertex_buffer);
	darray_free(&textures);
	darray_free(&rects_out);
	for(uint i = 0; i < bucket_count; ++i) {
		darray_free(&rect_buckets[i]);
		darray_free(&line_buckets[i]);
	}
	
	LOG_INFO("Video closed");
}

void video_present(void) {
	uint i, j;
	
#ifndef NO_DEVMODE
	v_stats.frame = frame+1;
	v_stats.active_textures = textures.size;
	v_stats.frame_layer_sorts = v_stats.frame_batches = v_stats.frame_rects = 0;
	v_stats.frame_lines = v_stats.frame_texture_switches = 0;
	for(i = 0; i < bucket_count; ++i) {
		v_stats.layer_rects[i] = rect_buckets[i].size;
		v_stats.layer_lines[i] = line_buckets[i].size;
	}
#endif
	
	// Sort rects to reduce batch count
	for(i = 0; i < bucket_count; ++i) {
		if(rect_buckets[i].size > 2) {
			_sort_rects(rect_buckets[i]);
#ifndef NO_DEVMODE
			v_stats.frame_layer_sorts++;
#endif
		}	
	}
	
	Vertex* vb = DARRAY_DATA_PTR(vertex_buffer, Vertex);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].u);
	glColorPointer(4, GL_FLOAT, sizeof(Vertex), &vb[0].r);
	
	// Render loop
	Texture* texs = DARRAY_DATA_PTR(textures, Texture);
	uint active_tex = ~0;
	float r, g, b, a;
	for(i = 0; i < bucket_count; ++i) {
		// Draw rects
		TexturedRectDesc* rects = DARRAY_DATA_PTR(rect_buckets[i], TexturedRectDesc);
		for(j = 0; j < rect_buckets[i].size; ++j) {
			if(rects[j].tex != active_tex) {
				if(vertex_buffer.size > 0) {
					assert(vertex_buffer.size % 4 == 0);
					uint tri_count = vertex_buffer.size / 2;
					glDrawElements(GL_TRIANGLES, tri_count*3, GL_UNSIGNED_SHORT, 
								   index_buffer);
					vertex_buffer.size = 0;
#ifndef NO_DEVMODE
					v_stats.frame_batches++;
#endif
				}
				active_tex = rects[j].tex;
				glBindTexture(GL_TEXTURE_2D, texs[active_tex].gl_id);				
#ifndef NO_DEVMODE
				v_stats.frame_texture_switches++;
#endif
			}
			
			COLOR_DECONSTRUCT(rects[j].tint, r, g, b, a);
			r /= 255.0f; g /= 255.0f; b /= 255.0f; a /= 255.0f;
            r *= a; g *= a; b *= a;
			
			size_t k = vertex_buffer.size;
			vertex_buffer.size += 4;
			assert(vertex_buffer.size <= vertex_buffer.reserved);
			
			Vector2 tl = vec2(rects[j].dest.left, rects[j].dest.top);
			Vector2 tr = vec2(rects[j].dest.right, rects[j].dest.top);
			Vector2 br = vec2(rects[j].dest.right, rects[j].dest.bottom);
			Vector2 bl = vec2(rects[j].dest.left, rects[j].dest.bottom);
			
			if(rects[j].rotation != 0.0f) {
				float rot = rects[j].rotation;
				Vector2 cnt = vec2((rects[j].dest.left + rects[j].dest.right) / 2.0f,
								   (rects[j].dest.top + rects[j].dest.bottom) / 2.0f);
				
				tl = vec2_add(vec2_rotate(vec2_sub(tl, cnt), rot), cnt);
				tr = vec2_add(vec2_rotate(vec2_sub(tr, cnt), rot), cnt);
				br = vec2_add(vec2_rotate(vec2_sub(br, cnt), rot), cnt);
				bl = vec2_add(vec2_rotate(vec2_sub(bl, cnt), rot), cnt);
			}
			
			vb[k+0].x = tl.x;
			vb[k+0].y = tl.y;
			vb[k+0].u = rects[j].source.left;
			vb[k+0].v = rects[j].source.top;
			vb[k+0].r = r; vb[k+0].g = g; vb[k+0].b = b; vb[k+0].a = a;
			
			vb[k+1].x = tr.x;
			vb[k+1].y = tr.y;
			vb[k+1].u = rects[j].source.right;
			vb[k+1].v = rects[j].source.top;
			vb[k+1].r = r; vb[k+1].g = g; vb[k+1].b = b; vb[k+1].a = a;
			
			vb[k+2].x = br.x;
			vb[k+2].y = br.y;
			vb[k+2].u = rects[j].source.right;
			vb[k+2].v = rects[j].source.bottom;
			vb[k+2].r = r; vb[k+2].g = g; vb[k+2].b = b; vb[k+2].a = a;
			
			vb[k+3].x = bl.x;
			vb[k+3].y = bl.y;
			vb[k+3].u = rects[j].source.left;
			vb[k+3].v = rects[j].source.bottom;
			vb[k+3].r = r; vb[k+3].g = g; vb[k+3].b = b; vb[k+3].a = a;
		}

		if(vertex_buffer.size > 0) {
			assert(vertex_buffer.size % 4 == 0);
			uint tri_count = vertex_buffer.size / 2;
			glDrawElements(GL_TRIANGLES, tri_count*3, GL_UNSIGNED_SHORT, 
						   index_buffer);
			vertex_buffer.size = 0;
	#ifndef NO_DEVMODE
			v_stats.frame_batches++;
	#endif
		}
		
		// TODO: gfx debug mode

		// Draw lines
		if(line_buckets[i].size) {
			glDisable(GL_TEXTURE_2D);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			
			glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].x);
			glColorPointer(4, GL_FLOAT, sizeof(Vertex), &vb[0].r);
			
			LineDesc* lines = DARRAY_DATA_PTR(line_buckets[i], LineDesc);
			vertex_buffer.size += line_buckets[i].size * 2;
			size_t k = 0;
			for(j = 0; j < line_buckets[i].size; ++j) {
				COLOR_DECONSTRUCT(lines[j].color, r, g, b, a);
				r /= 255.0f; g /= 255.0f; b /= 255.0f; a /= 255.0f;
				vb[k].x = lines[j].start.x;
				vb[k].y = lines[j].start.y;
				vb[k].r = r; vb[k].g = g; vb[k].b = b; vb[k].a = a;
				k++;
				vb[k].x = lines[j].end.x;
				vb[k].y = lines[j].end.y;
				vb[k].r = r; vb[k].g = g; vb[k].b = b; vb[k].a = a;
				k++;
			}
			glDrawArrays(GL_LINES, 0, k);
			vertex_buffer.size = 0;
			glEnable(GL_TEXTURE_2D);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			
			glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].x);
			glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].u);
			glColorPointer(4, GL_FLOAT, sizeof(Vertex), &vb[0].r);
		}	
		
	#ifndef NO_DEVOMDE
		v_stats.frame_rects += rect_buckets[i].size;
		v_stats.frame_lines += line_buckets[i].size;
	#endif
	}
	
	[[GLESView singleton] present];
	frame++;
	fps_count++;
	
	for(i = 0; i < bucket_count; ++i) {
		rect_buckets[i].size = 0;
		line_buckets[i].size = 0;
	}
}

uint video_get_frame(void) {
	return frame;
}

TexHandle tex_load(const char* filename) {
	assert(filename);
	
	Texture* tex = DARRAY_DATA_PTR(textures, Texture);
	
	// Look if texture is already loaded
	for(uint i = 0; i < textures.size; ++i) {
		if(tex[i].active) {
			if(strcmp(tex[i].file, filename) == 0) {
				tex[i].retain_count++;
				return i;
			}
		}
	}
	
	LOG_INFO("Loading texture from file %s", filename);
	
	Texture new_tex;
	
	// Construct proper path to texture
	NSString* ns_filename = [NSString stringWithUTF8String:filename];
	NSString* resource_path = [[NSBundle mainBundle] resourcePath];
	NSString* full_path = [resource_path stringByAppendingPathComponent:ns_filename];
	
	// Load texture
	UIImage* image = [UIImage imageWithContentsOfFile:full_path];
	CGImageRef cg_image = image.CGImage;
	new_tex.width = CGImageGetWidth(cg_image);
	new_tex.height = CGImageGetHeight(cg_image);
	bool has_alpha = CGImageGetAlphaInfo(cg_image) != kCGImageAlphaNone;
	CGColorSpaceRef color_space = CGImageGetColorSpace(cg_image);
	CGColorSpaceModel color_model = CGColorSpaceGetModel(color_space);
	uint bits_per_component = CGImageGetBitsPerComponent(cg_image);
	if(color_model != kCGColorSpaceModelRGB || bits_per_component != 8)
		LOG_ERROR("Bad image color space - please use 24 or 32 bit rgb/rgba");
	if(!(is_pow2(new_tex.width) && is_pow2(new_tex.height)))
		LOG_ERROR("Texture dimensions must be a power of 2");
	CFDataRef image_data = CGDataProviderCopyData(CGImageGetDataProvider(cg_image));
	void* data = (void*)CFDataGetBytePtr(image_data);
	
	// Make gl texture
	uint gl_id;
	uint format = has_alpha ? GL_RGBA : GL_RGB;
	glGenTextures(1, &gl_id);
	glBindTexture(GL_TEXTURE_2D, gl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, new_tex.width, new_tex.height, 0, 
				 format, GL_UNSIGNED_BYTE, data);
	CFRelease(image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	new_tex.gl_id = gl_id;
	new_tex.file = strclone(filename);
	new_tex.retain_count = 1;
	new_tex.active = true;
	
	darray_append(&textures, &new_tex);
	
	return textures.size-1;
}

void tex_size(TexHandle	tex, uint* width, uint* height) {
	assert(tex < textures.size);
	assert(width && height);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	assert(t[tex].active);
	
	*width = t[tex].width;
	*height = t[tex].height;
}

void tex_free(TexHandle tex) {
	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	assert(t[tex].active);
	assert(t[tex].retain_count > 0);
	
	t[tex].retain_count--;
	if(t[tex].retain_count == 0) {
		glDeleteTextures(1, &t[tex].gl_id);
		MEM_FREE(t[tex].file);
		t[tex].active = false;
	}
}

void video_draw_rect_rotated(TexHandle tex, uint layer,
							 const RectF* source, const RectF* dest, float rotation, Color tint) {
	assert(layer < bucket_count);
	assert(dest);
	
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
	
	TexturedRectDesc new_rect;
	new_rect.tex = tex;
	new_rect.source = real_source;
	new_rect.dest = real_dest;
	new_rect.tint = tint;
	new_rect.rotation = rotation;
	
	darray_append(&rect_buckets[layer], &new_rect);
}	

void video_draw_rect(TexHandle tex, uint layer,
					 const RectF* source, const RectF* dest, Color tint) {
	video_draw_rect_rotated(tex, layer, source, dest, 0.0f, tint);
}	

void video_draw_line(uint layer, const Vector2* start,
					 const Vector2* end, Color color) {
	assert(layer < bucket_count);
	assert(start);
	assert(end);
	
	LineDesc new_line;
	new_line.start = *start;
	new_line.end = *end;
	new_line.color = color;
	
	darray_append(&line_buckets[layer], &new_line);
}

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
	[[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategorySoloAmbient error:&error];
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
	s.av_player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&err];
	if(!s.av_player) {
		_log_nserror(err);
		LOG_ERROR("Unable to make AVAudioPlayer from stream");
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
	return NULL;
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
		LOG_WARNING("Skipping sound");
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
	assert(src);
	
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
	assert(src);
	
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
	assert(src);
	
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
	assert(src);
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

float sound_get_volume_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	assert(src);
	
	return src->volume;
}

float sound_get_pos_ex(SourceHandle handle) {
	Source* src = _get_source(handle);
	assert(src);
	
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
	assert(src);
	
	if(!src->buffer) {
		Sound* sound = _get_sound(src->sound, NULL);
		assert(sound);
		[sound->av_player setCurrentTime:pos];
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

bool key_pressed(Key key) {
	return false;
}

bool char_pressed(char c) {
	return false;
}

bool key_down(Key key) {
	return false;
}

bool char_down(char c) {
	return false;
}

bool key_up(Key key) {
	return false;
}

bool char_up(char c) {
	return false;
}

// Current state
uint cmouse_x, cmouse_y;
bool cmouse_pressed, cmouse_up, cmouse_down;

// Cached last frame state
static uint lmouse_x, lmouse_y;
static bool lmouse_pressed, lmouse_up, lmouse_down;

bool mouse_pressed(MouseButton button) {
	return lmouse_pressed;
}

bool mouse_down(MouseButton button) {
	return lmouse_down;
}

bool mouse_up(MouseButton button) {
	return lmouse_up;
}

void mouse_pos(uint* x, uint* y) {
	*x = lmouse_x;
	*y = lmouse_y;
}

Vector2 mouse_vec(void) {
	return vec2((float)lmouse_x, (float)lmouse_y);
}

#define max_touches 11
uint touch_count = 0;
Touch touches[max_touches];

void _touch_down(float x, float y) {
	if(touch_count < max_touches) {
		uint i = touch_count;
		touches[i].hit_time = time_ms() / 1000.0f;
		touches[i].hit_pos = vec2(x, y);
		touches[i].pos = vec2(x, y);
		touch_count++;
	}
}

void _touch_move(float old_x, float old_y, float new_x, float new_y) {
	for(uint i = 0; i < touch_count; ++i) {
		if(touches[i].pos.x == old_x && touches[i].pos.y == old_y) {
			touches[i].pos = vec2(new_x, new_y);
			return;
		}
	}
	assert(0 && "Unable to find the right touch to move!");
}

void _touch_up(float old_x, float old_y) {
	uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count; ++i) {
		float dx = touches[i].pos.x - old_x;
		float dy = touches[i].pos.y - old_y;
		float d = dx*dx + dy*dy;
		if(dx*dx + dy*dy < min_d) {
			min_d = d;
			min_i = i;
		}
	}
	touches[min_i] = touches[--touch_count];
}

uint touches_count(void) {
	return touch_count;
}

Touch* touches_get(void) {
	return touch_count ? &touches[0] : NULL;
}

/*
-----------
--- Time --
-----------
*/

static float t_s = 0.0f, t_d = 0.0f;
static float last_frame_time = 0.0f, last_fps_update = 0.0f;
static uint fps = 0;

float time_ms(void) {
	return t_s;
}

float time_delta(void) {
	// TODO: fix timestep here?
	//return t_d * 1000.0f;
	return 1000.0f / 60.0f;
}

uint time_fps(void) {
	return fps;
}

static float _get_t(void) {
	static mach_timebase_info_data_t info;
	static bool first = true;
	static uint64_t start_t;
	if(first) {
		mach_timebase_info(&info);
		start_t = mach_absolute_time();
		first = false;
	}
	
    uint64_t t = mach_absolute_time();
	
	t = t - start_t;
	
	t *= info.numer;
	t /= info.denom;
	
	return (float)t / 1000000.0f;
}	

void _time_update(float current_time) {
	t_s = current_time;
	t_d = current_time - last_frame_time;
	if(last_fps_update + 1000.0f < current_time) {
		fps = fps_count;
		fps_count = 0;
		last_fps_update = current_time;
	}
	last_frame_time = current_time;
}

/*
------------
--- Misc ---
------------
*/

bool system_update(void) {
	_time_update(_get_t());
	
	lmouse_x = cmouse_x;
	lmouse_y = cmouse_y;
	lmouse_up = cmouse_up;
	lmouse_down = cmouse_down;
	lmouse_pressed = cmouse_pressed;
	
	cmouse_up = false;
	cmouse_down = false;
	
	return true;
}
