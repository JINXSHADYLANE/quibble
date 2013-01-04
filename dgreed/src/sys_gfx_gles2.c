#include "sys_gfx.h"
#include "memory.h"
#include "mempool.h"
#include "darray.h"
#include "gfx_utils.h"

#include <SDL_opengles2.h>

// Types

// 40 bytes
typedef struct {
	char* file;
	char file_storage[16];
	uint retain_count, width, height;
	uint gl_id;
	float scale;
} Texture;

// 32 bytes
typedef struct {
	uint32 layer;
	Texture* tex;
	Color tint;
	int32 angle;
	uint16 src_l, src_t, src_r, src_b;
	uint16 dest_l, dest_t, dest_r, dest_b;
} TexturedRect;

// 16 bytes
typedef struct {
	uint32 layer;
	Color tint;
	uint16 start_x, start_y, end_x, end_y;
} Line;

// 12 bytes
typedef struct {
	uint16 x, y;
	uint32 color;
	uint16 u, v;
} Vertex;

typedef struct {
	uint32 layer;
	BlendMode mode;
} LayerTag;

bool draw_gfx_debug = false;

// Main renderer data
static MemPool texture_pool;
static DArray tags;
static DArray rects;
static DArray lines;
static DArray vertices;
static DArray indices;

// Renderer state
static Color clear_color = 0;
static int32 screen_width, screen_height;
static int32 scale_x, scale_y;
static uint frame = 0;
static bool filter_textures = true;

static bool _check_extension(const char* name) {
    static const char* exts = NULL;
    if(exts == NULL)
        exts = (char*)glGetString(GL_EXTENSIONS);
    return strfind(name, exts) != -1;
}

// sys_gfx.h interface implementation

extern bool _sys_video_initialized;
extern void _sys_video_init(void);
extern void _sys_video_close(void);
extern void _sys_video_get_native_resolution(uint* width, uint* height);

void video_get_native_resolution(uint* width, uint* height) {
	_sys_video_get_native_resolution(width, height);
}

static void _video_init(uint width, uint height, uint v_width, uint v_height,
		const char* name, bool fullscreen, bool filter_textures) {

	if(!_sys_video_initialized)
		_sys_video_init();

}

void video_init(uint width, uint height, const char* name) {
	
	_video_init(width, height, width, height, name, false, true);
}

void video_init_ex(uint width, uint height, uint v_width, uint v_height,
		const char* name, bool fullscreen) {

	_video_init(width, height, v_width, v_height, name, fullscreen, true);
}

void video_init_exr(uint width, uint height, uint v_width, uint v_height,
		const char* name, bool fullscreen) {

	_video_init(width, height, v_width, v_height, name, fullscreen, false);
}

void video_close(void) {
	_sys_video_close();
}

void video_clear_color(Color c) {
}

void video_present(void) {
}

uint video_get_frame(void) {
	return frame;
}

void video_set_blendmode(uint layer, BlendMode bmode) {
	// Find out if layer is tagged already
	for(uint i = 0; i < tags.size; ++i) {
		LayerTag* tag = darray_get(&tags, i);
		if(tag->layer == layer) {
			// Layer has a tag, overwrite it
			tag->mode = bmode;
			return;
		}
	}

	// Create a new tag
	LayerTag new = {
		.layer = layer,
		.mode = bmode
	};
	darray_append(&tags, &new);
}

void video_set_transform(uint layer, float* matrix) {
}

TexHandle tex_create(uint width, uint height) {
	return 1;
}

TexHandle tex_load(const char* filename) {
	return 1;
}

TexHandle tex_load_filter(const char* filename, TexFilter filter) {
	return 1;
}

void tex_blit(TexHandle tex, Color* data, uint x, uint y, uint w, uint h) {
}

void tex_size(TexHandle tex, uint* width, uint* height) {
	*width = 1;
	*height = 1;
}

void tex_free(TexHandle tex) {
}

void tex_scale(TexHandle tex, float s) {
}

void video_draw_rect(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, Color tint) {
	video_draw_rect_rotated(tex, layer, source, dest, 0.0f, tint);
}

void video_draw_rect_rotated(TexHandle tex, uint layer, const RectF* source, 
		const RectF* dest, float rotation, Color tint) {

}

void video_draw_line(uint layer, const Vector2* start,
		const Vector2* end, Color color) {

}
