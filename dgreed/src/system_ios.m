#include "system.h"

#include "memory.h"
#include "darray.h"
#include "wav.h"
#include "gfx_utils.h"

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenAL/al.h>
#import <OpenAL/alc.h>
#import <Twitter/TWTweetComposeViewController.h>
#import <MediaPlayer/MPMusicPlayerController.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#import "GLESView.h"
#import "GLESViewController.h"

// Main function

extern void _async_init(void);
extern void _async_close(void);

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	_async_init();
    log_init(NULL, LOG_LEVEL_INFO);
    
    int retVal = UIApplicationMain(argc, argv, nil, @"DGreedAppDelegate");
	
    log_close();
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
	float scale;
	bool active;
} Texture;

typedef struct {
	TexHandle tex;
	uint16 src_l, src_t, src_r, src_b;
	RectF dest;
	Color tint;
	float rotation;
} TexturedRectDesc;

typedef struct {
	Vector2 start, end;
	Color color;
} LineDesc;

typedef struct {
	float x, y;
	uint32 color;
	uint16 u, v;
} Vertex;

// Globals

#define bucket_count 16
#define max_vertices 1024*8

bool draw_gfx_debug = false;

static BlendMode last_blend_mode;
static BlendMode blend_modes[bucket_count];
static float* transform[bucket_count];
static DArray rect_buckets[bucket_count];
static DArray line_buckets[bucket_count];
static DArray textures;

static uint frame;
float screen_widthf, screen_heightf;
static float x_size_factor, y_size_factor;
Color clear_color = 0;

static uint radix_counts[256];
static DArray rects_out;
static DArray vertex_buffer;
static uint16 index_buffer[max_vertices/4 * 6];
static uint fps_count = 0;
static bool video_retro_filtering = false;
static bool has_discard_extension = false;
static bool use_bgra = false;

const float tex_mul = 32767.0f;

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

void video_get_native_resolution(uint* width, uint* height) {
    CGRect screen_rect = [[UIScreen mainScreen] bounds];
    *width = MAX(screen_rect.size.width, screen_rect.size.height);
    *height = MIN(screen_rect.size.width, screen_rect.size.height);
    
    CGFloat screen_scale;
    
    if([[UIScreen mainScreen] respondsToSelector:@selector(scale)]) {
        screen_scale = [[UIScreen mainScreen] scale];
    }
    else {
        screen_scale = 1.0f;
    }
    
    *width *= screen_scale;
    *height *= screen_scale;
}

static void _set_blendmode(BlendMode mode) {
	switch(mode) {
		case BM_NORMAL:
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case BM_ADD:
			glBlendFunc(GL_ONE, GL_ONE);
			break;
		case BM_MULTIPLY:
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			break;
	}
}

void video_init(uint width, uint height, const char* name) {
	video_init_ex(width, height, width, height, name, false); 
}

void video_init_exr(uint width, uint height, uint v_width, uint v_height,
				   const char* name, bool fullscreen) {
    video_retro_filtering = true;
    video_init_ex(width, height, v_width, v_height, name, fullscreen);
}

static bool _check_extension(const char* name) {
    static const char* exts = NULL;
    if(exts == NULL)
        exts = (char*)glGetString(GL_EXTENSIONS);
    return strfind(name, exts) != -1;
}

void video_init_ex(uint width, uint height, uint v_width, uint v_height,
				   const char* name, bool fullscreen) {
	assert(width != 0 && height != 0);
	assert(v_width != 0 && v_height != 0);
    
    screen_widthf = v_width;
    screen_heightf = v_height;
    
    has_discard_extension = _check_extension("GL_EXT_discard_framebuffer");
	
    NSString *reqSysVer = @"3.1.4";
    NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] == NSOrderedAscending) {
		use_bgra = true;
	}
       
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glClearDepthf(1.0f);
	
	glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
	_set_blendmode(BM_NORMAL);
	last_blend_mode = BM_NORMAL;

	glMatrixMode(GL_TEXTURE);
	glScalef(1.0f/tex_mul, 1.0f/tex_mul, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
		
	if(width > height) {
        glViewport(0, 0, height, width);
        // Some tricky transformations to properly turn view sideways 
        glOrthof(0.0f, (float)v_width, (float)v_height, 0.0f, -1.0f, 1.0f);
        glTranslatef((float)v_width/2.0f, (float)v_height/2.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        glTranslatef((float)v_height/-2.0f, (float)v_width/-2.0f, 0.0f);
        glScalef((float)v_height/(float)v_width, (float)v_width/(float)v_height, 1.0f);
    }
    else {
        glViewport(0, 0, width, height);
        glOrthof(0.0f, (float)v_width, (float)v_height, 0.0f, -1.0f, 1.0f);
        //glScalef((float)v_width/(float)v_height, (float)v_height/(float)v_width, 1.0f);
        //glTranslatef(0.0f, (float)v_width/-2.0f, 0.0f);
    }
	
	frame = 0;
	
	x_size_factor = (float)v_width / (float)width;
	y_size_factor = (float)v_height / (float)height;
	
#ifndef NO_DEVMODE
	memset(&v_stats, 0, sizeof(v_stats));
	v_stats.n_layers = bucket_count;
	v_stats.layer_rects = (uint*)MEM_ALLOC(sizeof(uint) * bucket_count);
	v_stats.layer_lines = (uint*)MEM_ALLOC(sizeof(uint) * bucket_count);
#endif
	
	// Init render buckets & blend modes
	for(uint i = 0; i < bucket_count; ++i) {
		rect_buckets[i] = darray_create(sizeof(TexturedRectDesc), 32);
		line_buckets[i] = darray_create(sizeof(LineDesc), 32);
		blend_modes[i] = BM_NORMAL;	
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

static void _color_to_4f(Color c, float* red, float* green,
	float* blue, float* alpha) {
	*red = (float)(c&0xFF) / 255.0f;
	c >>= 8;
	*green = (float)(c&0xFF) / 255.0f;
	c >>= 8;
	*blue = (float)(c&0xFF) / 255.0f;
	c >>= 8;
	*alpha = (float)(c&0xFF) / 255.0f;
}	

void video_clear_color(Color c) {
	float r, g, b, a;
	clear_color = c;
	_color_to_4f(c, &r, &g, &b, &a);
	glClearColor(r, g, b, a);
}

static bool rect_draw_state = false;

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
	
	if(!rect_draw_state) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].x);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vb[0].color);
		glTexCoordPointer(2, GL_SHORT, sizeof(Vertex), &vb[0].u);
		rect_draw_state = true;
	}
	
	// Render loop
	Texture* texs = DARRAY_DATA_PTR(textures, Texture);
	uint active_tex = ~0;
	byte r, g, b, a;
	Color c;
	for(i = 0; i < bucket_count; ++i) {
		// Switch blend modes if neccessary
		if(rect_buckets[i].size > 0 && blend_modes[i] != last_blend_mode) {
			if(vertex_buffer.size > 0) {
				assert(vertex_buffer.size % 4 == 0);
				uint tri_count = vertex_buffer.size / 2;
				glDrawElements(GL_TRIANGLES, tri_count*3, GL_UNSIGNED_SHORT, index_buffer);
				vertex_buffer.size = 0;
				#ifndef NO_DEVMODE
				v_stats.frame_batches++;
				#endif
			}

			_set_blendmode(blend_modes[i]);
			last_blend_mode = blend_modes[i];
		}

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
			r = (r * a) >> 8;
			g = (g * a) >> 8;
			b = (b * a) >> 8;
			c = COLOR_RGBA(r, g, b, a);
			
			size_t k = vertex_buffer.size;
			vertex_buffer.size += 4;
			assert(vertex_buffer.size <= vertex_buffer.reserved);
			
			Vector2 points[4] = {
				{rects[j].dest.left, rects[j].dest.top},
				{rects[j].dest.right, rects[j].dest.top},
				{rects[j].dest.right, rects[j].dest.bottom},
				{rects[j].dest.left, rects[j].dest.bottom}
			};

			if(rects[j].rotation != 0.0f) {
				float rot = rects[j].rotation;
				Vector2 cnt = vec2((rects[j].dest.left + rects[j].dest.right) * 0.5f,
								   (rects[j].dest.top + rects[j].dest.bottom) * 0.5f);
				
				float dx, dy;
				float s = sinf(rot);
				float c = cosf(rot);

				for(uint l = 0; l < 4; ++l) {
					dx = points[l].x - cnt.x;
					dy = points[l].y - cnt.y;
					points[l].x = c * dx - s * dy + cnt.x;
					points[l].y = s * dx + c * dy + cnt.y;
				}
			}

			if(transform[i] != NULL) 
				gfx_matmul(points, 4, transform[i]);

			Vector2 src[4] = {
				{rects[j].src_l, rects[j].src_t},
				{rects[j].src_r, rects[j].src_t},
				{rects[j].src_r, rects[j].src_b},
				{rects[j].src_l, rects[j].src_b}
			};

			for(uint l = 0; l < 4; ++l) {
				vb[k+l].x = points[l].x;
				vb[k+l].y = points[l].y;
				vb[k+l].color = c;
				vb[k+l].u = src[l].x;
				vb[k+l].v = src[l].y;
			}
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
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vb[0].color);
			
			LineDesc* lines = DARRAY_DATA_PTR(line_buckets[i], LineDesc);
			vertex_buffer.size += line_buckets[i].size * 2;
			size_t k = 0;
			for(j = 0; j < line_buckets[i].size; ++j) {
				COLOR_DECONSTRUCT(lines[j].color, r, g, b, a);
				r = (r * a) << 8;
				g = (g * a) << 8;
				b = (b * a) << 8;
				c = COLOR_RGBA(r, g, b, a);
			
				Vector2 points[2] = {
					{lines[j].start.x, lines[j].start.y},
					{lines[j].end.x, lines[j].end.y}
				};

				if(transform[i] != NULL)
					gfx_matmul(points, 2, transform[i]);

				vb[k].x = points[0].x;
				vb[k].y = points[0].y;
				vb[k].color = c;
				k++;
				vb[k].x = points[1].x;
				vb[k].y = points[1].x;
				vb[k].color = c;
				k++;
			}
			glDrawArrays(GL_LINES, 0, k);
			vertex_buffer.size = 0;

			glEnable(GL_TEXTURE_2D);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			
			glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vb[0].x);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vb[0].color);
			glTexCoordPointer(2, GL_SHORT, sizeof(Vertex), &vb[0].u);
		}	
		
	#ifndef NO_DEVOMDE
		v_stats.frame_rects += rect_buckets[i].size;
		v_stats.frame_lines += line_buckets[i].size;
	#endif
	}
    
    [[GLESView singleton] present];
	
    if(has_discard_extension) {
        const GLenum discards[]  = {GL_COLOR_ATTACHMENT0_OES, GL_DEPTH_ATTACHMENT_OES};
        glDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, 2, discards);
    }
  
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

void video_set_blendmode(uint layer, BlendMode bmode) {
	assert(layer < bucket_count);
	blend_modes[layer] = bmode;
}

void video_set_transform(uint layer, float* matrix) {
	assert(layer < bucket_count);

	transform[layer] = matrix;
}

static void* _load_image(const char* filename, uint* width, uint* height, CFDataRef* image_data) {
	// Construct proper path to texture
	NSString* ns_filename = [NSString stringWithUTF8String:filename];
	NSString* resource_path = [[NSBundle mainBundle] resourcePath];
	NSString* full_path = [resource_path stringByAppendingPathComponent:ns_filename];
	
	// Load texture
	UIImage* image = [UIImage imageWithContentsOfFile:full_path];
	CGImageRef cg_image = image.CGImage;
	*width = CGImageGetWidth(cg_image);
	*height = CGImageGetHeight(cg_image);
    CGColorSpaceRef color_space = CGImageGetColorSpace(cg_image);
	CGColorSpaceModel color_model = CGColorSpaceGetModel(color_space);
	uint bits_per_component = CGImageGetBitsPerComponent(cg_image);
	if(color_model != kCGColorSpaceModelRGB || bits_per_component != 8)
		LOG_ERROR("Bad image color space - please use 24 or 32 bit rgb/rgba");
	if(!(is_pow2(*width) && is_pow2(*height)))
		LOG_ERROR("Texture dimensions must be a power of 2");
	*image_data = CGDataProviderCopyData(CGImageGetDataProvider(cg_image));
	void* data = (void*)CFDataGetBytePtr(*image_data);
	return data;
}

static uint _make_gl_texture(void* data, uint width, uint height) {
	assert(data);

	// Make gl texture
	uint gl_id;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &gl_id);
	glBindTexture(GL_TEXTURE_2D, gl_id);
    
    if(!video_retro_filtering) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

	uint order = use_bgra ? GL_BGRA_EXT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
				 order, GL_UNSIGNED_BYTE, data);

	return gl_id;
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

    CFDataRef image_data;
	void* data = _load_image(filename, &new_tex.width, &new_tex.height, &image_data);
    uint gl_id = _make_gl_texture(data, new_tex.width, new_tex.height);
    CFRelease(image_data);

	new_tex.gl_id = gl_id;
	new_tex.file = strclone(filename);
	new_tex.retain_count = 1;
	new_tex.active = true;
	new_tex.scale = 1.0f;
	
	darray_append(&textures, &new_tex);
	
	return textures.size-1;
}

TexHandle tex_load_filter(const char* filename, TexFilter filter) {
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
	
	LOG_INFO("Loading texture (with filtering) from file %s", filename);
	
	Texture new_tex;

    CFDataRef image_data;
	void* data = _load_image(filename, &new_tex.width, &new_tex.height, &image_data);

	(*filter)((Color*)data, new_tex.width, new_tex.height);

    uint gl_id = _make_gl_texture(data, new_tex.width, new_tex.height);
    CFRelease(image_data);

	new_tex.gl_id = gl_id;
	new_tex.file = strclone(filename);
	new_tex.retain_count = 1;
	new_tex.active = true;
	new_tex.scale = 1.0f;
	
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

void tex_scale(TexHandle tex, float s) {
	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	assert(t[tex].active);

	t[tex].scale = s;
}

// Fast integer log2 when n is a power of two
static uint _ilog2(uint v) {
	static const uint b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000}; 
	uint r = (v & b[0]) != 0;
	for (uint i = 4; i > 0; i--) 
	{
	  r |= ((v & b[i]) != 0) << i;
	}
	return r;
}

void video_draw_rect_rotated(TexHandle tex, uint layer,
							 const RectF* source, const RectF* dest, float rotation, Color tint) {
	assert(layer < bucket_count);
	assert(dest);
	
	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	assert(t[tex].active);
	uint fixed_scale = lrintf(t[tex].scale * (tex_mul+1.0f));
	uint texture_width = t[tex].width;
	uint texture_height = t[tex].height;

	int16 real_src_l = 0;
	int16 real_src_t = 0;
	int16 real_src_r = texture_width;
	int16 real_src_b = texture_height;

	if(source != NULL) {
		real_src_l = lrintf(source->left);
		real_src_t = lrintf(source->top);
		real_src_r = lrintf(source->right);
		real_src_b = lrintf(source->bottom);
	}

	// This is faster than comparing floats
	bool full_dest = (*((uint*)&dest->right) | *((uint*)&dest->bottom)) == 0U; 
	RectF real_dest = *dest;
	
    int w = real_src_r - real_src_l;
    int h = real_src_b - real_src_t;
    
    if(full_dest) {
		float width = (float)w;
		float height = (float)h;
		real_dest.right = real_dest.left + width;
		real_dest.bottom = real_dest.top + height;
	}
	
    // Don't draw rect if it's not inside screen rect
    float center_x = (real_dest.left + real_dest.right) * 0.5f;
    float center_y = (real_dest.top + real_dest.bottom) * 0.5f;
    float half_sqr_diag = (float)(w*w / 4 + h*h / 4);
    float out_x = center_x;
    float out_y = center_y;
    if((center_x < 0.0f && out_x*out_x > half_sqr_diag) 
       || (center_y < 0.0f && out_y*out_y > half_sqr_diag))
        return;
    out_x = center_x - screen_widthf;
    out_y = center_y - screen_heightf;
    if((center_x >= screen_widthf && out_x*out_x > half_sqr_diag) 
       || (center_y >= screen_heightf && out_y*out_y > half_sqr_diag))
        return;

	assert(is_pow2((texture_width * fixed_scale) >> 15));
	assert(is_pow2((texture_height * fixed_scale) >> 15));

	uint wlog2 = _ilog2((texture_width * fixed_scale) >> 15);
	uint hlog2 = _ilog2((texture_height * fixed_scale) >> 15);

	real_src_l = ((real_src_l << 15) - 1) >> wlog2;
	real_src_t = ((real_src_t << 15) - 1) >> hlog2;
	real_src_r = ((real_src_r << 15) - 1) >> wlog2;
	real_src_b = ((real_src_b << 15) - 1) >> hlog2;

	TexturedRectDesc new_rect = {
		.tex = tex,
		.src_l = real_src_l,
		.src_t = real_src_t,
		.src_r = real_src_r,
		.src_b = real_src_b,
		.dest = real_dest,
		.tint = tint,
		.rotation = rotation
	};
	
    // Do something uglier instead of darray_append, saving a memcpy here is worth it
	//darray_append(&rect_buckets[layer], &new_rect);
    
    
    DArray* bucket = &rect_buckets[layer];
    if(bucket->reserved - bucket->size < 1) {
        bucket->reserved *= 2;
        bucket->data = MEM_REALLOC(bucket->data, bucket->item_size * bucket->reserved);
    }
    
    TexturedRectDesc* rects = DARRAY_DATA_PTR(*bucket, TexturedRectDesc);
    rects[bucket->size] = new_rect;
    bucket->size++;
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

/*
--------------------------
--- Device orientation ---
--------------------------
*/

static DevOrient current_orientation;
static bool orientation_transition_start = false;
static DevOrient orientation_next;
static float orientation_transition_start_t = 0.0f;
static float orientation_transition_len = 0.0f;

DevOrient _uikit_to_dev_orient(UIInterfaceOrientation orient) {
    switch(orient) {
        case UIInterfaceOrientationLandscapeLeft:
            return ORIENT_LANDSCAPE_LEFT;
        case UIInterfaceOrientationLandscapeRight:
            return ORIENT_LANDSCAPE_RIGHT;
        case UIInterfaceOrientationPortrait:
            return ORIENT_PORTRAIT;
        case UIInterfaceOrientationPortraitUpsideDown:
            return ORIENT_PORTRAIT_UPSIDE_DOWN;
    }
    assert(0 && "Bad device orientation");
    return ORIENT_LANDSCAPE_LEFT;
}

void _orientation_set_current(UIInterfaceOrientation orient) {
    current_orientation = _uikit_to_dev_orient(orient);
}

void _orientation_start_transition(UIInterfaceOrientation next, float len) {
    orientation_transition_start = true;
    orientation_next = _uikit_to_dev_orient(next);
    orientation_transition_len = len;
    orientation_transition_start_t = time_ms_current() / 1000.0f;
}


DevOrient orientation_current(void) {
    return current_orientation;
}

bool orientation_change(DevOrient* new, float* anim_start, float* anim_len) {
    if(orientation_transition_start) {
        if(new)
            *new = orientation_next;
        if(anim_start)
            *anim_start = orientation_transition_start_t;
        if(anim_len)
            *anim_len = orientation_transition_len;
        
        orientation_transition_start = false;
        return true;
    }
    return orientation_transition_start;
}

/*
---------------------
--- Running state ---
---------------------
*/

RunStateCallback enter_background_cb = NULL;
RunStateCallback enter_foreground_cb = NULL;

void runstate_background_cb(RunStateCallback cb) {
	enter_background_cb = cb;
}

void runstate_foreground_cb(RunStateCallback cb) {
	enter_foreground_cb = cb;
}

/*
---------------------
--- Accelerometer ---
---------------------
*/

ShakeCallback shake_cb = NULL;

void acc_shake_cb(ShakeCallback cb) {
    shake_cb = cb;
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
	if(!touch_count)
		return;
    uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count && i < max_touches; ++i) {
		float dx = touches[i].pos.x - old_x;
		float dy = touches[i].pos.y - old_y;
		float d = dx*dx + dy*dy;
		if(dx*dx + dy*dy < min_d) {
			min_d = d;
			min_i = i;
		}
	}
    touches[min_i].pos = vec2(new_x, new_y);
}

void _touch_up(float old_x, float old_y) {
	if(!touch_count)
		return;
	uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count && i < max_touches; ++i) {
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
float inactive_time = 0.0f;

float time_s(void) {
    return t_s / 1000.0f;
}

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

uint time_ms_current(void) {
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

	return t / 1000000;
}

static float _get_t(void) {
	
	return (float)time_ms_current();
}	

void _time_update(float current_time) {
	t_s = current_time - inactive_time;
	t_d = t_s - last_frame_time;
	if(last_fps_update + 1000.0f < t_s) {
		fps = fps_count;
		fps_count = 0;
		last_fps_update = t_s;
	}
	last_frame_time = t_s;
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
    
    //orientation_transition_start = false;
	
	return true;
}

/*
-----------
--- OS ---
-----------
*/

void ios_open_web_url(const char* url) {
    NSString* ns_url = [NSString stringWithUTF8String:url];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString: ns_url]];
}

void ios_alert(const char* title, const char* text) {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String:title] 
                                                    message:[NSString stringWithUTF8String:text]
                                                   delegate:nil 
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
    [alert release];
}

bool ios_has_twitter(void) {
    NSString *reqSysVer = @"5.0";
    NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending) {
        Class TW = NSClassFromString(@"TWTweetComposeViewController");
        return [TW canSendTweet];
    }
    
    return false;
}

void ios_tweet(const char* msg, const char* url) {
    Class TW = NSClassFromString(@"TWTweetComposeViewController");
    id tw = [[TW alloc] init];
    
    [tw setInitialText:[NSString stringWithUTF8String:msg]];
    if(url)
        [tw addURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
    
    [[GLESViewController singleton] presentModalViewController:tw animated:YES];
}
