#include "system.h"

#include "memory.h"
#include "darray.h"

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "GLESView.h"

// Main function

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, nil, @"DGreedAppDelegate");
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

const VideoStats* Video_stats(void) {
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
	
	// Count sort
	for(i = 0; i < rects_in.size; ++i) {
		size_t idx = r_in[i].tex & 0xFF;
		size_t dest = radix_counts[(256+idx-1)%256]++;
		r_out[dest] = r_in[idx];
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
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
					glDrawElements(GL_TRIANGLES, tri_count*6, GL_UNSIGNED_SHORT, 
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
			
			COLOR_DECONSTRUCT(rects[i].tint, r, g, b, a);
			size_t k = vertex_buffer.size;
			vertex_buffer.size += 4;
			assert(vertex_buffer.size <= vertex_buffer.reserved);
			
			Vector2 tl = vec2(rects[j].dest.left, rects[j].dest.top);
			Vector2 tr = vec2(rects[j].dest.right, rects[j].dest.top);
			Vector2 br = vec2(rects[j].dest.right, rects[j].dest.bottom);
			Vector2 bl = vec2(rects[j].dest.left, rects[j].dest.bottom);
			
			if(rects[i].rotation != 0.0f) {
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
			glDrawElements(GL_TRIANGLES, tri_count*6, GL_UNSIGNED_SHORT, 
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
				COLOR_DECONSTRUCT(rects[i].tint, r, g, b, a);
				vb[k].x = lines[j].start.x;
				vb[k].y = lines[j].start.y;
				vb[k].r = r; vb[k].g = g; vb[k].b = b; vb[k].a = a;
				k++;
				vb[k].x = lines[j].start.x;
				vb[k].y = lines[j].start.y;
				vb[k].r = r; vb[k].g = g; vb[k].b = b; vb[k].a = a;
				k++;
			}
			glDrawArrays(GL_LINES, 0, k/2);
			vertex_buffer.size = 0;
			glEnable(GL_TEXTURE_2D);
		}	
		
	#ifndef NO_DEVOMDE
		v_stats.frame_batches++;
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

/*
------------- 
--- Input ---
-------------
*/ 

/*
-----------
--- Time --
-----------
*/

static float t_s = 0.0f, t_d = 0.0f;
static float last_frame_time = 0.0f, last_fps_update = 0.0f;
static uint fps = 0;

float time_ms(void) {
	return t_s * 1000.0f;
}

float time_delta(void) {
	// TODO: fix timestep here?
	return t_d * 1000.0f;
}

uint time_fps(void) {
	return fps;
}

void _time_update(float current_time) {
	t_s = current_time;
	t_d = current_time - last_frame_time;
	if(last_fps_update + 1.0f < current_time) {
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
	_time_update(CACurrentMediaTime());
	return true;
}
