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
#import <MessageUI/MFMailComposeViewController.h>
#import <Twitter/TWTweetComposeViewController.h>
#import <MediaPlayer/MPMusicPlayerController.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#import "DGreedAppDelegate.h"
#import "GLESView.h"
#import "GLESViewController.h"
#import "AutoRotateViewController.h"

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
float x_size_factor, y_size_factor;
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
    FileHandle f = file_open(filename);
    size_t s = file_size(f);
    void* fdata = malloc(s);
    file_read(f, fdata, s);
    file_close(f);

    NSData* img_data = [[NSData alloc] initWithBytesNoCopy:fdata length:s freeWhenDone:YES];

	// Load texture
    UIImage* image = [[UIImage alloc] initWithData:img_data];
    [img_data release];

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

    [image release];

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

TexHandle tex_create(uint width, uint height) {
	assert(width && height);

	// Validate size
	int max_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	if(width > max_size || height > max_size)
		LOG_ERROR("Can't create texture so large!");
	if(!(is_pow2(width) && is_pow2(height)))
		LOG_ERROR("Texture dimensions must be a power of 2");

	// Prep pixel data
	void* data = MEM_ALLOC(4 * width * height);
	memset(data, 0, 4 * width * height);
	uint gl_id = _make_gl_texture(data, width, height);
	MEM_FREE(data);

	// Fill texture struct
	Texture new_tex = {
		.gl_id = gl_id,
		.file = NULL,
		.retain_count = 1,
		.active = true,
		.scale = 1.0f,
        .width = width,
        .height = height
	};

	darray_append(&textures, &new_tex);

	return textures.size - 1;
}

TexHandle tex_load(const char* filename) {
	return tex_load_filter(filename, NULL);
}

TexHandle tex_load_filter(const char* filename, TexFilter filter) {
	assert(filename);

	Texture* tex = DARRAY_DATA_PTR(textures, Texture);

	// Look if texture is already loaded
	for(uint i = 0; i < textures.size; ++i) {
		if(tex[i].active) {
			if(tex[i].file && strcmp(tex[i].file, filename) == 0) {
				tex[i].retain_count++;
				return i;
			}
		}
	}

	LOG_INFO("Loading texture (with filtering) from file %s", filename);

	Texture new_tex;

    CFDataRef image_data;
	void* data = _load_image(filename, &new_tex.width, &new_tex.height, &image_data);

	if(filter)
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

void tex_blit(TexHandle tex, Color* data, uint x, uint y, uint w, uint h) {
	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	t = &t[tex];
	assert(t->active);
	assert((x + w <= t->width) && (y + h <= t->height));

	uint order = use_bgra ? GL_BGRA_EXT : GL_RGBA;

	glBindTexture(GL_TEXTURE_2D, t->gl_id);

	if(w == t->width && h == t->height) {
		glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA,
				w, h, 0, order, GL_UNSIGNED_BYTE, data
		);
	}
	else {
		glTexSubImage2D(
				GL_TEXTURE_2D, 0,
				x, y, w, h,
				GL_RGBA, GL_UNSIGNED_BYTE, data
		);
	}
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
		if(t[tex].file)
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
