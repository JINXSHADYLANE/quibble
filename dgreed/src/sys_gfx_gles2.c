#include "sys_gfx.h"
#include "memory.h"
#include "mempool.h"
#include "darray.h"
#include "datastruct.h"
#include "image.h"
#include "gfx_utils.h"

#include <SDL_opengles2.h>

// Types

// 48 bytes
typedef struct {
	char* file;
	char file_storage[16];
	uint retain_count, width, height;
	uint gl_id;
	float scale;
	ListHead list;
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
static ListHead textures;
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
extern void _sys_set_title(const char* title);
extern void _sys_video_get_native_resolution(uint* width, uint* height);

void video_get_native_resolution(uint* width, uint* height) {
	_sys_video_get_native_resolution(width, height);
}

static void _video_init(uint width, uint height, uint v_width, uint v_height,
		const char* name, bool fullscreen, bool _filter_textures) {

	if(!_sys_video_initialized)
		_sys_video_init();

	_sys_set_title(name);

    LOG_INFO("Vendor     : %s", glGetString(GL_VENDOR));
    LOG_INFO("Renderer   : %s", glGetString(GL_RENDERER));
    LOG_INFO("Version    : %s", glGetString(GL_VERSION));
    LOG_INFO("Extensions : %s", glGetString(GL_EXTENSIONS));

	list_init(&textures);
	mempool_init_ex(&texture_pool, sizeof(Texture), sizeof(Texture)*16);
	tags = darray_create(sizeof(LayerTag), 16);
	rects = darray_create(sizeof(TexturedRect), 1024);
	lines = darray_create(sizeof(Line), 16);
	vertices = darray_create(sizeof(Vertex), 4096);
	indices = darray_create(sizeof(uint16), 1024 * 6);

	filter_textures = _filter_textures;
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

	if(!list_empty(&textures))
		LOG_WARNING("There stil are active textures!");

	mempool_drain(&texture_pool);
	darray_free(&tags);
	darray_free(&rects);
	darray_free(&lines);
	darray_free(&vertices);
	darray_free(&indices);
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
	// Not implemented, currently no game uses it anyway...
}

static void _pf_to_gles(PixelFormat format, GLenum* fmt, GLenum* type) {
	switch(format & PF_MASK_PIXEL_FORMAT) {
		case PF_RGB888:
			*fmt = GL_RGB;
			*type = GL_UNSIGNED_BYTE;
			return;
		case PF_RGB565:
			*fmt = GL_RGB;
			*type = GL_UNSIGNED_SHORT_5_6_5;
			return;
		case PF_RGBA8888:
			*fmt = GL_RGBA;
			*type = GL_UNSIGNED_BYTE;
			return;
		case PF_RGBA4444:
			*fmt = GL_RGBA;
			*type = GL_UNSIGNED_SHORT_4_4_4_4;
			return;
		case PF_RGBA5551:
			*fmt = GL_RGBA;
			*type = GL_UNSIGNED_SHORT_5_5_5_1;
			return;
		case PF_LA88:
			*fmt = GL_LUMINANCE_ALPHA;
			*type = GL_UNSIGNED_BYTE;
			return;
		case PF_PVRTC2:
			*fmt = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
			*type = -1;
			return;
		case PF_PVRTC4:
			*fmt = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
			*type = -1;
			return;
		case PF_ETC1:
			*fmt = GL_ETC1_RGB8_OES;
			*type = -1;
		default:
            *fmt = *type = 0;
			LOG_ERROR("Unknown pixel format");
	}
}

static uint _make_gl_texture(void* data, uint width, uint height, PixelFormat format) {
	assert(data);

	uint gl_id;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &gl_id);
	glBindTexture(GL_TEXTURE_2D, gl_id);

	uint filter = filter_textures ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	GLenum fmt;
	GLenum type;
	_pf_to_gles(format, &fmt, &type);
    
    GLsizei image_size = 0;
    if(fmt == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
        image_size = (MAX(width, 16) * MAX(height, 8) * 2 + 7) / 8;
    if(fmt == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
        image_size = (MAX(width, 8) * MAX(height, 8) * 4 + 7) / 8;
	if(fmt == GL_ETC1_RGB8_OES) {
		assert(width % 4 == 0 && height % 4 == 0);
		image_size = (width * height * 3) / 6;
	}
    
    if(image_size) {
        glCompressedTexImage2D(
            GL_TEXTURE_2D, 0, fmt, width, height, 
            0, image_size, data
        );
    }
    else {
        glTexImage2D(
            GL_TEXTURE_2D, 0, fmt, width, height, 
            0, fmt, type, data
        );
    }

	return gl_id;
}

static Texture* _new_texture( 
		const char* filename, uint width, uint height, 
		uint gl_id, float scale) {

	Texture* new = mempool_alloc(&texture_pool);

	if(filename) {
		// Try to save malloc by fitting filename directly into
		// texture struct
		if(strlen(filename) < 16) {
			strcpy(new->file_storage, filename);
			new->file = new->file_storage;
		}
		else {
			new->file = strclone(filename);
		}
	}
	else {
		new->file = NULL;
	}

	new->retain_count = 1;
	new->width = width;
	new->height = height;
	new->gl_id = gl_id;
	new->scale = scale;

	list_push_front(&textures, &new->list);

	return new;
}

TexHandle tex_create(uint width, uint height) {
	assert(width && height);

#ifdef _DEBUG
	// Validate size
	int max_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	if(width > max_size || height > max_size)
		LOG_ERROR("Can't create texture so large!");

	// Now GLES2 doesn't require pow2 textures,
	// but let's enforce it for now for simplicity with
	// mipmaps & compressed formats.
	if(!(is_pow2(width) && is_pow2(height)))
		LOG_ERROR("Texture dimensions must be power of 2");
#endif

	// Prep pixel data
	size_t s = 4 * width * height;
	void* data = MEM_ALLOC(s);
	memset(data, 0, s);
	uint gl_id = _make_gl_texture(data, width, height, PF_RGBA8888);
	MEM_FREE(data);

	Texture* new_tex = _new_texture(NULL, width, height, gl_id, 1.0f);

	return (TexHandle)new_tex;
}

TexHandle tex_load(const char* filename) {
	return tex_load_filter(filename, NULL);
}

TexHandle tex_load_filter(const char* filename, TexFilter filter) {
	assert(filename);

	uint t = time_ms_current();

	// Look if file is already loaded, incr refcount and return if yes
	Texture* texture = NULL;
	list_for_each_entry(texture, &textures, list) {
		if(texture->file && strcmp(texture->file, filename) == 0) {
			texture->retain_count++;
			return (TexHandle)texture;
		}
	}

	// Read & decompress image data
	uint width, height;
	PixelFormat format;
	void* data = image_load(filename, &width, &height, &format);

	// Apply pixel filter
	if(filter)
		(*filter)((Color*)data, width, height);

	// Make gl texture
	uint gl_id = _make_gl_texture(data, width, height, format);
	// image_load doesn't use MEM_ALLOC because the memory might
	// be actually allocated by some external library
	free(data); 

	texture = _new_texture(filename, width, height, gl_id, 1.0f);

    LOG_INFO("Loaded texture from file %s in %ums", filename, time_ms_current() - t);

	return (TexHandle)texture;
}

void tex_blit(TexHandle tex, Color* data, uint x, uint y, uint w, uint h) {
	Texture* t = (Texture*)tex;
	assert((x + w <= t->width) && (y + h <= t->height));

	glBindTexture(GL_TEXTURE_2D, t->gl_id);

	if(w == t->width && h == t->height) {
		glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA, 
				w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
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

void tex_size(TexHandle tex, uint* width, uint* height) {
	Texture* t = (Texture*)tex;
	*width = t->width;
	*height = t->height;
}

void tex_free(TexHandle tex) {
	Texture* t = (Texture*)tex;

	t->retain_count--;
	if(t->retain_count == 0) {
		glDeleteTextures(1, &t->gl_id);
		if(t->file && strlen(t->file) >= 16)
			MEM_FREE(t->file);
		list_remove(&t->list);
		mempool_free(&texture_pool, t);
	}
}

void tex_scale(TexHandle tex, float s) {
	Texture* t = (Texture*)tex;
	t->scale = s;
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
