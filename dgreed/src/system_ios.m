#include "system.h"

#include "memory.h"
#include "darray.h"

#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

// Main function

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, nil, @"DGreedAppDelegate");
    [pool release];
    return retVal;
}

// Types

typedef struct {
	char* file;
	uint retain_count, width, height;
	uint gl_id;
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

#ifndef NO_DEVMODE
kdeoStats v_stats;

const kdeoStats* kdeo_stats(void) {
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

void kdeo_init(uint width, uint height, const char* name) {
	kdeo_init_ex(width, height, width, height, name, false); 
}

void kdeo_init_ex(uint width, uint height, uint v_width, uint v_height,
				   const char* name, bool fullscreen) {
	assert(width >= 480 && width <= 1024);
	assert(height >= 320 && height <= 768);
	assert(v_width != 0 && v_height != 0);
	
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);
	glClearDepthf(1.0f);
	glkewport(0, 0, width, height);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glMatrixMode(GL_MODELkEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, (float)v_width, (float)v_height, 0.0f, -10.0f, 10.0f);
	
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
		rect_buckets[i] = darray_create(sizeof(TexturedRectDesc, 32));
		line_buckets[i] = darray_create(sizeof(LineDesc, 32));
	}
	
	// Temp bucket for sorting
	rects_out = darray_create(sizeof(TexturedRectDesc, 32));
	
	textures = darray_create(sizeof(textures, 16));
	
	// Generate index buffer
	for(uint i = 0; i < max_vertices/4; ++i) {
		index_buffer[i*6 + 0] = i*4 + 0; 
		index_buffer[i*6 + 1] = i*4 + 1; 
		index_buffer[i*6 + 2] = i*4 + 3; 
		index_buffer[i*6 + 3] = i*4 + 3; 
		index_buffer[i*6 + 4] = i*4 + 2; 
		index_buffer[i*6 + 5] = i*4 + 1; 
	}
	
	assert(sizeof(Vertex) == sizeof(float) * 8);
	vertex_buffer = darray_create(sizeof(Vertex), max_vertices);
	
	LOG_INFO("kdeo initialized");
}

void kdeo_close(void) {
#ifndef NO_DEVMODE
	MEM_FREE(v_stats.layer_rects);
	MEM_FREE(v_stats.layer_lines);
#endif
	
	if(textures.size > 0)
		LOG_WARNING("%d textures are still active!", textures.size);
	
	darray_free(&vertex_buffer);
	darray_free(&textures);
	darray_free(&rects_out);
	for(uint i = 0; i < bucket_count; ++i) {
		darray_free(&rect_buckets[i]);
		darray_free(&rect_lines[i]);
	}
	
	LOG_INFO("kdeo closed");
}

void kdeo_present(void) {
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
	
	// Render loop
	Vertex* vb = DARRAY_DATA_PTR(vertex_buffer, Vertex);
	Texture* texs = DARRAY_DATA_PTR(textures, Texture);
	uint active_tex = ~0;
	float r, g, b, a;
	for(i = 0; i < bucket_count; ++i) {
		// Draw rects
		TexturedRectDesc* rects = DARRAY_DATA_PTR(rect_buckets[i], TexturedRectDesc);
		for(j = 0; j < rect_buckets[i].size; ++j) {
			if(rects[j].tex != active_tex) {
				if(vertex_buffer.size > 0) {
					_draw_rects();
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
			
			Vector2 tl = vec2(rect.dest.left, rect.dest.top);
			Vector2 tr = vec2(rect.dest.right, rect.dest.top);
			Vector2 br = vec2(rect.dest.right, rect.dest.bottom);
			Vector2 bl = vec2(rect.dest.left, rect.dest.bottom);
			
			if(rects[i].rotation != 0.0f) {
				float rot = rect.rotation;
				Vector2 cnt = vec2((rect.dest.left + rect.dest.right) / 2.0f,
								   (rect.dest.top + rect.dest.bottom) / 2.0f);
				
				tl = vec2_add(vec2_rotate(vec2_sub(tl, cnt), rot), cnt);
				tr = vec2_add(vec2_rotate(vec2_sub(tr, cnt), rot), cnt);
				br = vec2_add(vec2_rotate(vec2_sub(br, cnt), rot), cnt);
				bl = vec2_add(vec2_rotate(vec2_sub(bl, cnt), rot), cnt);
			}
			
			vb[k+0].x = tl.x;
			vb[k+0].y = tl.y;
			vb[k+0].u = rects[i].source.left;
			vb[k+0].v = rects[i].source.top;
			vb[k+0].r = r; vb[k+0].g = g; vb[k+0].b = b; vb[k+0].a = a;
			
			vb[k+1].x = tr.x;
			vb[k+1].y = tr.y;
			vb[k+1].u = rects[i].source.right;
			vb[k+1].v = rects[i].source.top;
			vb[k+1].r = r; vb[k+1].g = g; vb[k+1].b = b; vb[k+1].a = a;
			
			vb[k+2].x = br.x;
			vb[k+2].y = br.y;
			vb[k+2].u = rects[i].source.right;
			vb[k+2].v = rects[i].source.bottom;
			vb[k+2].r = r; vb[k+2].g = g; vb[k+2].b = b; vb[k+2].a = a;
			
			vb[k+3].x = bl.x;
			vb[k+3].y = bl.y;
			vb[k+3].u = rects[i].source.left;
			vb[k+3].v = rects[i].source.bottom;
			vb[k+3].r = r; vb[k+3].g = g; vb[k+3].b = b; vb[k+3].a = a;
			
			vertex_buffer.size += 4;
		}

		if(vertex_buffer.size > 0) {
			_draw_rects();
			vertex_buffer.size = 0;
	#ifndef NO_DEVMODE
			v_stats.frame_batches++;
	#endif
		}
		
		// TODO: gfx debug mode

		// Draw lines
		glDisable(GL_TEXTURE_2D);
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
			k++
		}
		_draw_lines();
		vertex_buffer.size = 0;
		glEnable(GL_TEXTURE_2D);
		
	#ifndef NO_DEVOMDE
		v_stats.frame_batches++;
		v_stats.frame_rects += rect_buckets[i].size;
		v_stats.frame_lines += rects_lines[i].size;
	#endif
	}
	
	[[GLESView singleton] present];
	frame++;
	
	for(i = 0; i < bucket_count; ++i) {
		rect_buckets[i].size = 0;
		line_buckets[i].size = 0;
	}
}

uint video_get_frame(void) {
	return frame;
}

