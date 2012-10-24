#include "system.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "utils.h"

#include "memory.h"
#include "darray.h"
#include "gfx_utils.h"
#include "image.h"

float x_size_factor, y_size_factor;

typedef struct {
	char* file;
	uint retain_count;
	uint width, height;
	uint gl_id;
	float scale;
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

static BlendMode last_blend_mode;
static float* transform[BUCKET_COUNT];
static BlendMode blend_modes[BUCKET_COUNT];
static DArray rect_buckets[BUCKET_COUNT];
static DArray line_buckets[BUCKET_COUNT];
static DArray textures;

static uint radix_counts[256];
static DArray rects_out;

static uint frame;


static bool retro = false;

static bool sdl_initialized = false;
static uint native_width, native_height;
static Color clear_color = 0;

#ifndef NO_DEVMODE
VideoStats v_stats;

const VideoStats* video_stats(void) {
	return &v_stats;
}
#endif

extern void _async_init(void);
extern void _async_close(void);
extern void _http_check_close(void);
extern int dgreed_main(int, char**);
#ifdef __APPLE__
int SDL_main(int argc, char** argv) {
#else
#ifdef main
#undef main
#endif
int main(int argc, char** argv) {
#endif

	// Log to stderr on windows since it can't provide sane args
#ifndef _WIN32
	// Construct log file name
	assert(argc >= 1);
	const char* prog_name = path_get_file(argv[0]);
	const char* postfix = ".log";
	char logfile[128];
	assert(strlen(prog_name) + strlen(postfix) < 128);
	strcpy(logfile, prog_name);
	strcat(logfile, postfix);
#else
	const char* logfile = "dgreed.log";
#endif

	_async_init();

	log_init(logfile, LOG_LEVEL_INFO);

	int res = dgreed_main(argc, argv);

	log_close();
	_http_check_close();
	_async_close();

#ifdef TRACK_MEMORY
	MemoryStats stats;
	mem_stats(&stats);
	LOG_INFO("Memory usage stats:");
	LOG_INFO(" Total allocations: %u", stats.n_allocations);
	LOG_INFO(" Peak dynamic memory usage: %zuB", stats.peak_bytes_allocated);
	if(stats.bytes_allocated) {
		LOG_INFO(" Bytes still allocted: %zu", stats.bytes_allocated);
		LOG_INFO(" Dumping allocations info to memory.txt");
		mem_dump("memory.txt");
	}
#endif

	return res;
}

static void _insertion_sort(DArray rect_bucket) {
	assert(rect_bucket.size > 1);

	TexturedRectDesc* rects = DARRAY_DATA_PTR(rect_bucket, TexturedRectDesc);

	for(int i = 1; i < rect_bucket.size; ++i) {
		TexturedRectDesc key = rects[i];
		int j = i-1;
		while(j >= 0 && rects[j].tex > key.tex) {
			rects[j+1] = rects[j];
			j--;
		}
		rects[j+1] = key;
	}
}

static void _sort_rects(DArray rects_in) {
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
	for(i = 1; i < ARRAY_SIZE(radix_counts); ++i)
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


static void _init_sdl(void) {
	if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0)
		LOG_ERROR("Unable to initialize SDL");

	const SDL_VideoInfo* info = SDL_GetVideoInfo();
	native_width = info->current_w;
	native_height = info->current_h;

	assert(!sdl_initialized);
	sdl_initialized = true;
}

void video_get_native_resolution(uint* width, uint* height) {
	assert(width && height);

	if(!sdl_initialized)
		_init_sdl();

	*width = native_width;
	*height = native_height;
}

static void _set_blendmode(BlendMode mode) {
	switch(mode) {
		case BM_NORMAL:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case BM_ADD:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case BM_MULTIPLY:
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			break;
	}
}

void video_init(uint width, uint height, const char* name) {
	video_init_ex(width, height, width, height, name, false);
}

void video_init_ex(uint width, uint height, uint v_width, uint v_height, const
	char* name, bool fullscreen) {
	assert(width > 0);
	assert(height > 0);
	assert(v_width != 0);
	assert(v_height != 0);

	if(!sdl_initialized)
		_init_sdl();

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
	_set_blendmode(BM_NORMAL);
	last_blend_mode = BM_NORMAL;
	video_clear_color(clear_color);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (double)v_width, (double)v_height, 0.0);

	frame = 0;

	x_size_factor = (float)v_width / (float)width;
	y_size_factor = (float)v_height / (float)height;

	#ifndef NO_DEVMODE
	memset(&v_stats, 0, sizeof(v_stats));
	v_stats.n_layers = BUCKET_COUNT;
	v_stats.layer_rects = (uint*)MEM_ALLOC(sizeof(uint) * BUCKET_COUNT);
	v_stats.layer_lines = (uint*)MEM_ALLOC(sizeof(uint) * BUCKET_COUNT);
	#endif

	// Init renderer state darrays
	rects_out = darray_create(sizeof(TexturedRectDesc), 32);
	textures = darray_create(sizeof(Texture), 16);
	memset(rect_buckets, 0, sizeof(rect_buckets));
	memset(line_buckets, 0, sizeof(line_buckets));
	for(uint i = 0; i < BUCKET_COUNT; ++i) {
		blend_modes[i] = BM_NORMAL;
		transform[i] = NULL;
	}

	LOG_INFO("Video initialized");
}

void video_init_exr(uint width, uint height, uint v_width, uint v_height,
	const char* name, bool fullscreen) {
	retro = true;
	video_init_ex(width, height, v_width, v_height, name, fullscreen);
}

void video_close(void) {
	SDL_Quit();

	#ifndef NO_DEVMODE
	MEM_FREE(v_stats.layer_rects);
	MEM_FREE(v_stats.layer_lines);
	#endif

	// Look for unfreed textures
	if(textures.size) {
		Texture* tex = DARRAY_DATA_PTR(textures, Texture);
		for(uint i = 0; i < textures.size; ++i) {
			if(tex[i].active)
				LOG_WARNING("Texture %s still unfreed!", tex[i].file);
		}
	}

	// Free renderer state darrays
	darray_free(&textures);
	darray_free(&rects_out);
	for(uint i = 0; i < BUCKET_COUNT; ++i) {
		if(rect_buckets[i].reserved)
			darray_free(&rect_buckets[i]);
		if(line_buckets[i].reserved)
			darray_free(&line_buckets[i]);
	}

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

void video_clear_color(Color c) {
	float r, g, b, a;
	clear_color = c;
	if(sdl_initialized) {
		_color_to_4f(c, &r, &g, &b, &a);
		glClearColor(r, g, b, a);
	}
}

// Forward def for video_present
uint _get_gl_id(TexHandle tex);

void video_present(void) {
	uint i, j;

	#ifndef NO_DEVMODE
	v_stats.frame = frame+1;
	v_stats.active_textures = textures.size;
	v_stats.frame_layer_sorts = v_stats.frame_batches = v_stats.frame_rects = 0;
	v_stats.frame_lines = v_stats.frame_texture_switches = 0;
	for(uint i = 0; i < BUCKET_COUNT; ++i) {
		v_stats.layer_rects[i] = rect_buckets[i].size;
		v_stats.layer_lines[i] = line_buckets[i].size;
	}
	#endif

	// Sort texture rects to minimize texture binding
	for(i = 0; i < BUCKET_COUNT; ++i) {
		if(rect_buckets[i].size > 2) {
			_sort_rects(rect_buckets[i]);
			#ifndef NO_DEVMODE
			v_stats.frame_layer_sorts++;
			#endif
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Not a typo
	uint active_tex = -1;
	bool drawing = false;
	float r, g, b, a;
	for(i = 0; i < BUCKET_COUNT; ++i) {
		// Switch blend modes if neccessary
		if(rect_buckets[i].size > 0 && blend_modes[i] != last_blend_mode) {
			if(drawing) {
				glEnd();
				drawing = false;

				#ifndef NO_DEVMODE
				v_stats.frame_batches++;
				#endif
			}

			_set_blendmode(blend_modes[i]);
			last_blend_mode = blend_modes[i];
		}

		TexturedRectDesc* rects = DARRAY_DATA_PTR(rect_buckets[i], TexturedRectDesc);
		// Draw rects
		for(j = 0; j < rect_buckets[i].size; ++j) {
			TexturedRectDesc rect = rects[j];
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

			Vector2 points[4] = {
				{rect.dest.left, rect.dest.top},
				{rect.dest.right, rect.dest.top},
				{rect.dest.right, rect.dest.bottom},
				{rect.dest.left, rect.dest.bottom}
			};

			if(rect.rotation != 0.0f) {
				float rot = rect.rotation;
				Vector2 cnt = vec2((rect.dest.left + rect.dest.right) / 2.0f,
					(rect.dest.top + rect.dest.bottom) / 2.0f);

				for(uint k = 0; k < 4; ++k)
					points[k] = vec2_add(vec2_rotate(vec2_sub(points[k], cnt), rot), cnt);
			}

			if(transform[i] != NULL)
				gfx_matmul(points, 4, transform[i]);

			glTexCoord2f(rect.source.left, rect.source.top);
			glVertex2f(points[0].x, points[0].y);

			glTexCoord2f(rect.source.right, rect.source.top);
			glVertex2f(points[1].x, points[1].y);

			glTexCoord2f(rect.source.right, rect.source.bottom);
			glVertex2f(points[2].x, points[2].y);

			glTexCoord2f(rect.source.left, rect.source.bottom);
			glVertex2f(points[3].x, points[3].y);

		}
		if(drawing) {
			glEnd();
			drawing = false;

			#ifndef NO_DEVMODE
			v_stats.frame_batches++;
			#endif
		}

		if(line_buckets[i].size || draw_gfx_debug) {
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
			glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
			if(draw_gfx_debug) {
				for(j = 0; j < rect_buckets[i].size; ++j) {
					TexturedRectDesc rect = rects[j];

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
			LineDesc* lines = DARRAY_DATA_PTR(line_buckets[i], LineDesc);
			for(j = 0; j < line_buckets[i].size; ++j) {
				LineDesc line = lines[j];
				_color_to_4f(line.color, &r, &g, &b, &a);
				glColor4f(r, g, b, a);
				Vector2 points[2] = {
					{line.start.x, line.start.y},
					{line.end.x, line.end.y}
				};

				if(transform[i] != NULL)
					gfx_matmul(points, 4, transform[i]);

				glVertex2f(points[0].x, points[0].y);
				glVertex2f(points[1].x, points[1].y);
			}

			glEnd();
			glEnable(GL_TEXTURE_2D);
		}

		#ifndef NO_DEVMODE
		v_stats.frame_batches++;
		v_stats.frame_rects += rect_buckets[i].size;
		v_stats.frame_lines += line_buckets[i].size;
		#endif
	}

	SDL_GL_SwapBuffers();
	frame++;

	for(i = 0; i < BUCKET_COUNT; ++i) {
		rect_buckets[i].size = 0;
		line_buckets[i].size = 0;
	}
}

uint video_get_frame(void) {
	return frame;
}

void video_set_blendmode(uint layer, BlendMode bmode) {
	assert(layer < BUCKET_COUNT);

	blend_modes[layer] = bmode;
}

void video_set_transform(uint layer, float* matrix) {
	assert(layer < BUCKET_COUNT);

	transform[layer] = matrix;
}

static uint _make_gl_texture(void* data, uint width, uint height) {
	// Make gl texture
	uint gl_id;
	glGenTextures(1, &gl_id);
	glBindTexture(GL_TEXTURE_2D, gl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, data);
	GLuint tfilter = retro ? GL_NEAREST : GL_LINEAR;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tfilter);

	uint error = glGetError();
	if(error != GL_NO_ERROR)
		LOG_ERROR("OpenGL error while loading texture, id %u", error);

	return gl_id;
}

static Texture* _alloc_tex(const char* filename, TexHandle* handle) {
	Texture* tex = DARRAY_DATA_PTR(textures, Texture);

	// Look if texture is already loaded
	if(filename) {
		for(uint i = 0; i < textures.size; ++i) {
			if(tex[i].active && tex[i].file && strcmp(tex[i].file, filename) == 0) {
				tex[i].retain_count++;
				*handle = i;
				return NULL;
			}
		}
	}

	// Look for first inactive texture
	for(uint i = 0; i < textures.size; ++i) {
		if(!tex[i].active) {
			*handle = i;
			return &tex[i];
		}
	}

	// Make new texture
	Texture new;
	darray_append(&textures, &new);
	*handle = textures.size - 1;
	tex = DARRAY_DATA_PTR(textures, Texture);
	return &tex[*handle];
}

TexHandle tex_create(uint width, uint height) {
	assert(width && height);

	TexHandle result;
	Texture* new = _alloc_tex(NULL, &result);
	assert(new);

	// Validate size
	int max_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	if(width > max_size || height > max_size)
		LOG_ERROR("Can't create texture so large!");
	if(!(is_pow2(width) && is_pow2(height)))
		LOG_ERROR("Texture dimensions must be power of 2");

	// Prep pixel data, make gl texture
	void* data = MEM_ALLOC(4 * width * height);
	memset(data, 0, 4 * width * height);
	uint gl_id = _make_gl_texture(data, width, height);
	MEM_FREE(data);

	// Fill texture struct
	new->width = width;
	new->height = height;
	new->gl_id = gl_id;
	new->file = NULL;
	new->retain_count = 1;
	new->scale = 1.0f;
	new->active = true;

	return result;
}


TexHandle tex_load(const char* filename) {
	return tex_load_filter(filename, NULL);
}

TexHandle tex_load_filter(const char* filename, TexFilter filter) {
	assert(filename);

	uint t = time_ms_current();

	TexHandle result;
	Texture* new = _alloc_tex(filename, &result);
	if(!new)
		return result;

	uint w, h;
	PixelFormat format;
	byte* decompr_data = image_load(filename, &w, &h, &format);

	if(!(is_pow2(w) && is_pow2(h))) {
		free(decompr_data);
		LOG_ERROR("%s: Texture dimensions is not power of 2", filename);
	}

	if(format != PF_RGBA8888) {
		free(decompr_data);
		LOG_ERROR("%s: Only RGBA8888 pixel format is supported", filename);
	}

	// Apply filter
	if(filter) {
		(*filter)((Color*)decompr_data, w, h);
	}

	uint gl_id = _make_gl_texture(decompr_data, w, h);

	free(decompr_data);

	new->width = w;
	new->height = h;
	new->gl_id = gl_id;
	new->file = strclone(filename);
	new->retain_count = 1;
	new->scale = 1.0f;
	new->active = true;

	LOG_INFO("Loaded texture from file %s in %ums", filename, time_ms_current()-t);

	return result;
}

void tex_blit(TexHandle tex, Color* data, uint x, uint y, uint w, uint h) {
	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	t = &t[tex];
	assert(t->active);
	assert(x + w <= t->width || y + h <= t->height);

	glBindTexture(GL_TEXTURE_2D, t->gl_id);

	glTexSubImage2D(
			GL_TEXTURE_2D, 0,
			x, y, w, h,
			GL_RGBA, GL_UNSIGNED_BYTE, data
	);
}

void tex_size(TexHandle tex, uint* width, uint* height) {
	assert(tex < textures.size);
	assert(width);
	assert(height);

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

uint _get_gl_id(TexHandle tex) {
	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	assert(t[tex].active);
	assert(t[tex].retain_count > 0);

	return t[tex].gl_id;
}

void video_draw_rect_rotated(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, float rotation, Color tint) {
	assert(layer < BUCKET_COUNT);
	assert(dest);

	assert(tex < textures.size);
	Texture* t = DARRAY_DATA_PTR(textures, Texture);
	assert(t[tex].active);
	float scale = t[tex].scale;
	uint texture_width = t[tex].width;
	uint texture_height = t[tex].height;

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

	real_source.left /= (float)texture_width * scale;
	real_source.top /= (float)texture_height * scale;
	real_source.right /= (float)texture_width * scale;
	real_source.bottom /= (float)texture_height * scale;

	TexturedRectDesc new = {tex, real_source, real_dest, tint, rotation};

	if(!rect_buckets[layer].reserved)
		rect_buckets[layer] = darray_create(sizeof(TexturedRectDesc), 32);
	darray_append(&rect_buckets[layer], &new);
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

	LineDesc new = {*start, *end, color};

	if(!line_buckets[layer].reserved)
		line_buckets[layer] = darray_create(sizeof(LineDesc), 32);
	darray_append(&line_buckets[layer], &new);
}

