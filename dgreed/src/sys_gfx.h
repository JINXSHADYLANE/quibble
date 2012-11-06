#include "utils.h"

/*
----------------
--- Graphics ---
----------------
*/

#ifndef NO_DEVMODE
// This is quite implementation-specific,
// put somewhere else in future
typedef struct {
	uint frame;
	uint active_textures;

	// Per-frame stats
	uint frame_layer_sorts;
	uint frame_batches;
	uint frame_rects;
	uint frame_lines;
	uint frame_texture_switches;

	// Per-layer stats
	uint n_layers;
	uint* layer_rects;
	uint* layer_lines;
} VideoStats;

const VideoStats* video_stats(void);
#endif

typedef enum {
	BM_NORMAL,
	BM_ADD,
	BM_MULTIPLY
} BlendMode;

// Returns native screen resolution, video does not need
// to be initialized for this
void video_get_native_resolution(uint* width, uint* height);
// Initializes video
void video_init(uint width, uint height, const char* name);
// Initializes video, sets virtual resolution
void video_init_ex(uint width, uint height, uint v_width, uint v_height, const
	char* name, bool fullscreen);
// Same as above, but without texture filtering for "retro" look
void video_init_exr(uint width, uint height, uint v_width, uint v_height, const
	char* name, bool fullscreen);
// Deinitializes video
void video_close(void);
// Sets clear color, black by default
void video_clear_color(Color c);
// Flips backbuffer and frontbuffer, sleeps to hit 60fps.
// Returns false if app is closed, true otherwise.
void video_present(void);
// Returns number of frame which is currently being rendered
uint video_get_frame(void);
// Sets a blendmode for a layer
void video_set_blendmode(uint layer, BlendMode bmode);
// Sets a custom 3x2 affine transform matrix for a layer,
// NULL means identity.
void video_set_transform(uint layer, float* matrix);

typedef uint TexHandle;
typedef void (*TexFilter)(Color* texels, uint w, uint h);

// Creates empty texture, filled with transparent white
TexHandle tex_create(uint width, uint height);
// Loads texture from .png file, it must have pow2 dimensions
TexHandle tex_load(const char* filename);
// Loads texture and applies custom filtering
TexHandle tex_load_filter(const char* filename, TexFilter filter);
// Blits image into texture. You are responsible for clipping!
void tex_blit(TexHandle tex, Color* data, uint x, uint y, uint w, uint h);
// Returns width and height of texture in pixels
void tex_size(TexHandle tex, uint* width, uint* height);
// Frees texture which was loaded with tex_load
void tex_free(TexHandle tex);
// Pretend that texture is s times bigger than it actually is
void tex_scale(TexHandle tex, float s);

// Draws textured rectangle. Source can be null rectangle,
// destination can have 0 for right and bottom.
void video_draw_rect(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, Color tint);
// Draws rotated textured rectangle.
// Same rules for source/dest applies as for video_draw_rect.
void video_draw_rect_rotated(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, float rotation, Color tint);
// Draws a line
void video_draw_line(uint layer,
	const Vector2* start, const Vector2* end, Color color);
