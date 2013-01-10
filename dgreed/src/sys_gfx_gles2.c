#include "sys_gfx.h"
#include "memory.h"
#include "mempool.h"
#include "darray.h"
#include "datastruct.h"
#include "image.h"
#include "gfx_utils.h"

#ifdef TARGET_IOS
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#else
#include <SDL_opengles2.h>
#endif

// Types

// 56 bytes
typedef struct {
	char* file;
	char file_storage[16];
	uint retain_count, width, height, hlog2, wlog2;
	uint gl_id;
	uint scale;
	ListHead list;
} Texture;

// 32 bytes
typedef struct {
	uint32 layer;
	Texture* tex;
	Color tint;
	int32 angle;
	int16 src_l, src_t, src_r, src_b;
	int16 dest_l, dest_t, dest_r, dest_b;
} TexturedRect;

// 16 bytes
typedef struct {
	uint32 layer;
	Color tint;
	int16 start_x, start_y, end_x, end_y;
} Line;

// 12 bytes
typedef struct {
	int16 x, y;
	uint32 color;
	uint16 u, v;
} Vertex;

typedef enum {
	GLSL_ATTRIB_POS = 0,
	GLSL_ATTRIB_COLOR = 1,
	GLSL_ATTRIB_UV = 2
} Attrib;

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

static const float tex_mul = 32768.0f;
static const float pos_mul = 16.0f;
static const float rot_mul = 65536.0f;
static const uint16 tex_scale_1 = 32768;

// Renderer state
float screen_widthf, screen_heightf;
Color clear_color;
static int32 screen_width, screen_height;
static uint frame = 0;
static bool filter_textures = true;
static bool drawing_lines = false;
static bool drawing_rects = false;
static BlendMode blend_mode = ~0;
static Texture* active_texture = NULL;
static int vert_shader_id;
static int frag_shader_id;
static int program_id;

static int glsl_screen_size;
static int glsl_texture;

// Shaders

const char* vert_shader = 
"uniform vec2 screen_size;\n"
"\n"
"attribute vec2 pos;\n"
"attribute vec4 color;\n"
"attribute vec2 uv;\n"
"\n"
"varying vec2 v_uv;\n"
"varying vec4 v_tint;\n"
"\n"
"void main() {\n"
"	v_uv = uv / 32768.0;\n"
"	v_tint = color;\n"
"	vec2 p = (pos / (8.0 * screen_size)) - 1.0;\n"
"	gl_Position = vec4(-p.y, -p.x, 0.0, 1.0);\n"
//"gl_Position = vec4(pos, 0.0, 1.0);\n"
"}";

const char* frag_shader = 
"precision mediump float;\n"
"uniform sampler2D texture;\n"
"\n"
"varying vec2 v_uv;\n"
"varying vec4 v_tint;\n"
"\n"
"void main() {\n"
"	gl_FragColor = v_tint * texture2D(texture, v_uv);\n"
//"	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"}";

static bool _check_extension(const char* name) {
    static const char* exts = NULL;
    if(exts == NULL)
        exts = (char*)glGetString(GL_EXTENSIONS);
    return strfind(name, exts) != -1;
}

static void _check_error(void) {
#ifdef _DEBUG
	int error = glGetError();
	if(error != GL_NO_ERROR)
		LOG_WARNING("OpenGL ES2 error: %d", error);
#endif
}

static uint _compile_shader(const char* source, uint type) {
	uint t = time_ms_current();

	uint gl_id = glCreateShader(type);

	int len = strlen(source);
	glShaderSource(gl_id, 1, &source, &len);
	glCompileShader(gl_id);

	int status;
	glGetShaderiv(gl_id, GL_COMPILE_STATUS, &status);
	if(status) {
		// Success, return new shader!
		LOG_INFO("Compiled glsl shader in %ums", time_ms_current() - t);
		return gl_id;
	}
	else {
		// Failure, log errors
		int log_length;
		glGetShaderiv(gl_id, GL_INFO_LOG_LENGTH, &log_length); 
		char log[1024];
		glGetShaderInfoLog(gl_id, 1024, &log_length, log);
		LOG_ERROR("Compile log: %s", log);
		return gl_id;
	}
}

static void _free_shader(uint id) {
	glDeleteShader(id);
}

static uint _link_program(uint vert, uint frag) {
	uint t = time_ms_current();

	uint gl_id = glCreateProgram();
	glAttachShader(gl_id, vert);
	glAttachShader(gl_id, frag);

	glBindAttribLocation(gl_id, GLSL_ATTRIB_POS, "pos");
	glBindAttribLocation(gl_id, GLSL_ATTRIB_COLOR, "color");
	glBindAttribLocation(gl_id, GLSL_ATTRIB_UV, "uv");

	glLinkProgram(gl_id);
	
	int status;
	glGetProgramiv(gl_id, GL_LINK_STATUS, &status);
	if(!status) {
		LOG_ERROR("Can't link glsl program");
	}

	glsl_screen_size = glGetUniformLocation(gl_id, "screen_size");
	glsl_texture = glGetUniformLocation(gl_id, "texture");

	_check_error();

	LOG_INFO("Linked glsl program in %ums", time_ms_current() - t);

	return gl_id;
}

static void _free_program(uint id) {
	glDeleteProgram(id);
}

static void _set_blend_mode(BlendMode mode) {
	assert(mode != blend_mode);

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

	blend_mode = mode;
}

// sys_gfx.h interface implementation

extern bool _sys_video_initialized;
extern void _sys_video_init(void);
extern void _sys_video_close(void);
extern void _sys_set_title(const char* title);
extern void _sys_video_get_native_resolution(uint* width, uint* height);
extern void _sys_present(void);

void video_get_native_resolution(uint* width, uint* height) {
	_sys_video_get_native_resolution(width, height);
}

static void _video_init(uint width, uint height, uint v_width, uint v_height,
		const char* name, bool fullscreen, bool _filter_textures) {

	if(!_sys_video_initialized)
		_sys_video_init();

	_sys_set_title(name);
    
    
    screen_widthf = v_width;
    screen_heightf = v_height;

    LOG_INFO("Vendor     : %s", glGetString(GL_VENDOR));
    LOG_INFO("Renderer   : %s", glGetString(GL_RENDERER));
    LOG_INFO("Version    : %s", glGetString(GL_VERSION));
    LOG_INFO("Extensions : %s", glGetString(GL_EXTENSIONS));
	LOG_INFO("Framebuffer: %d", 
		glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE
	);

	list_init(&textures);
	mempool_init_ex(&texture_pool, sizeof(Texture), sizeof(Texture)*16);
	tags = darray_create(sizeof(LayerTag), 16);
	rects = darray_create(sizeof(TexturedRect), 1024);
	lines = darray_create(sizeof(Line), 16);
	vertices = darray_create(sizeof(Vertex), 4096);
	indices = darray_create(sizeof(uint16), 1024 * 6);
    
    screen_width = v_width;
    screen_height = v_height;

    if(width > height)
        glViewport(0, 0, height, width);
    else
        glViewport(0, 0, width, height);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glDisable(GL_SAMPLE_COVERAGE);

	glEnable(GL_BLEND);
	_set_blend_mode(BM_NORMAL);

	video_clear_color(COLOR_BLACK);

	filter_textures = _filter_textures;

	vert_shader_id = _compile_shader(vert_shader, GL_VERTEX_SHADER);
	frag_shader_id = _compile_shader(frag_shader, GL_FRAGMENT_SHADER);
	program_id = _link_program(vert_shader_id, frag_shader_id);

    glUseProgram(program_id);
	glUniform2f(glsl_screen_size, (float)screen_width, (float)screen_height);

    _check_error();

	glReleaseShaderCompiler();

	_check_error();
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

	_free_shader(vert_shader_id);
	_free_shader(frag_shader_id);
	_free_program(program_id);

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
    clear_color = c;
	byte r, g, b, a;
	COLOR_DECONSTRUCT(c, r, g, b, a);
	glClearColor(
		(float)r / 255.0f, 
		(float)g / 255.0f,
		(float)b / 255.0f,
		1.0f	
	);
}

static int _rect_compar(const void* a, const void* b) {
	const TexturedRect* rect_a = a;
	const TexturedRect* rect_b = b;

	if(rect_a->layer != rect_b->layer);
		return rect_a->layer - rect_b->layer;

	if(rect_a->tex != rect_b->tex)
		return (int)(size_t)(rect_a->tex - rect_b->tex);

	// This makes sort stable
	return (int)(size_t)(rect_a - rect_b);
}

static int _line_compar(const void* a, const void* b) {
	const Line* line_a = a;
	const Line* line_b = b;

	if(line_a->layer != line_b->layer)
		return line_a->layer - line_b->layer;

	// This makes sort stable
	return (int)(size_t)(line_a - line_b);
}

static void _draw_rects(uint* count) {
	static Vertex* old_vb = NULL;

	if(!drawing_rects) {
		drawing_rects = true;
		drawing_lines = false;

		glEnableVertexAttribArray(GLSL_ATTRIB_POS);
		glEnableVertexAttribArray(GLSL_ATTRIB_COLOR);
		glEnableVertexAttribArray(GLSL_ATTRIB_UV);
	}

	assert(sizeof(Vertex) == 12);
	Vertex* vb = vertices.data;
	if(old_vb != vb) {
		glVertexAttribPointer(GLSL_ATTRIB_POS, 2, GL_SHORT, GL_FALSE, sizeof(Vertex), &vb->x); 
		glVertexAttribPointer(GLSL_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), &vb->color);
		glVertexAttribPointer(GLSL_ATTRIB_UV, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(Vertex), &vb->u);
		old_vb = vb;
	}

	assert(vertices.size % 4 == 0);

	glDrawElements(GL_TRIANGLES, vertices.size / 2 * 3, GL_UNSIGNED_SHORT, indices.data);

	_check_error();

	*count = 0;
	vertices.size = 0;
} 

static void _draw_lines(uint* count) {
	if(!drawing_lines) {
		drawing_lines = true;
		drawing_rects = false;

		// setup state
	}

	// draw
	*count = 0;
}

void video_present_dummy(void) {
	glClear(GL_COLOR_BUFFER_BIT);

	const GLfloat tri[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

	glUseProgram(program_id);
	glVertexAttribPointer(GLSL_ATTRIB_POS, 2, GL_FLOAT, GL_FALSE, 0, tri);
	glEnableVertexAttribArray(GLSL_ATTRIB_POS);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	_check_error();

	_sys_present();
	frame++;

	rects.size = 0;
	lines.size = 0;
}

// Some fixed point math for rotating rects with fixed point coords

typedef int32 fix16_t;

static const fix16_t fix16_pi = 205887;

static fix16_t fix16_mul(fix16_t inArg0, fix16_t inArg1)
{
    int64_t product = (int64_t)inArg0 * inArg1;
    return product >> 16;
}

static fix16_t fix16_sin(fix16_t inAngle)
{
    fix16_t tempAngle = inAngle % (fix16_pi << 1);

    if(tempAngle > fix16_pi)
        tempAngle -= (fix16_pi << 1);
    else if(tempAngle < -fix16_pi)
        tempAngle += (fix16_pi << 1);

    fix16_t tempAngleSq = fix16_mul(tempAngle, tempAngle);

    fix16_t tempOut = tempAngle;
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut -= (tempAngle / 6);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut += (tempAngle / 120);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut -= (tempAngle / 5040);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut += (tempAngle / 362880);

    return tempOut;
}

fix16_t fix16_cos(fix16_t inAngle)
{
    return fix16_sin(inAngle + (fix16_pi >> 1));
}

void video_present(void) {
	// Sort rects by layer and then by texture
	if(rects.size > 4) {
		sort_heapsort(
			rects.data, rects.size, 
			sizeof(TexturedRect), _rect_compar
		);
	}
	
	// Sort lines by layer
	if(lines.size > 4) {
		sort_heapsort(
			lines.data, lines.size,
			sizeof(Line), _line_compar
		);
	}

	TexturedRect* r = rects.data;
	Line* l = lines.data;

	glClear(GL_COLOR_BUFFER_BIT);

	// Render loop here
	uint r_cur = 0, r_ready = 0;
	uint l_cur = 0, l_ready = 0;

	while(r_cur < rects.size || l_cur < lines.size) {
		uint layer = ~0;
		if(rects.size) {
			layer = r[r_cur].layer;
		}
		if(lines.size) {
			layer = MIN(layer, l[l_cur].layer);
		}

		// Find layer tag
		BlendMode mode = blend_mode;
		for(uint i = 0; i < tags.size; ++i) {
			LayerTag* t = darray_get(&tags, i);
			if(t->layer == layer) {
				mode = t->mode;
				break;
			}
		}

		// Switch blend mode
		if(mode != blend_mode) {
			if(r_ready)
				_draw_rects(&r_ready);
			if(l_ready)
				_draw_lines(&l_ready);

			_set_blend_mode(mode);
		}

		if(layer == r[r_cur].layer) {
			// Flush lines to free up vbuffer
			if(l_ready)
				_draw_lines(&l_ready);

			// Switch texture, flushing currently ready rects
			Texture* tex = r[r_cur].tex;
			if(tex != active_texture) {
				if(r_ready)
					_draw_rects(&r_ready);
				
				glBindTexture(GL_TEXTURE_2D, tex->gl_id);
				active_texture = tex;
			}

			// Fill up vbuffer
			do {
				darray_reserve(&vertices, (l_ready+1)*4);
				Vertex* vb = vertices.data;

				byte cr, cg, cb, ca;
				TexturedRect rect = r[r_cur];

				// Premultiply alpha
				COLOR_DECONSTRUCT(rect.tint, cr, cg, cb, ca);
				Color c = COLOR_RGBA(
					(cr * ca) >> 8, (cg * ca) >> 8, (cb * ca) >> 8, ca
				);

				int16 pos[] = {
					rect.dest_l, rect.dest_t,
					rect.dest_r, rect.dest_t,
					rect.dest_r, rect.dest_b,
					rect.dest_l, rect.dest_b
				};

				int16 src[] = {
					rect.src_l, rect.src_t,
					rect.src_r, rect.src_t,
					rect.src_r, rect.src_b,
					rect.src_l, rect.src_b
				};

				if(rect.angle != 0) {
					const fix16_t a = rect.angle;
					const fix16_t s = fix16_sin(a);
                    const fix16_t c = fix16_cos(a);

					const int16 cx = (rect.dest_l + rect.dest_r) / 2;
					const int16 cy = (rect.dest_t + rect.dest_b) / 2;

					int16 dx, dy;
					for(uint v = 0; v < 4; ++v) {
						dx = pos[v*2+0] - cx;
						dy = pos[v*2+1] - cy;
						pos[v*2+0] = ((c * dx) >> 16) - ((s * dy) >> 16) + cx;
						pos[v*2+1] = ((s * dx) >> 16) + ((c * dy) >> 16) + cy;
					}
				}

				uint k = vertices.size;
				for(uint v = 0; v < 4; ++v) {
					vb[k+v].x = pos[v*2+0];
					vb[k+v].y = pos[v*2+1];
					vb[k+v].color = c;
					vb[k+v].u = src[v*2+0];
					vb[k+v].v = src[v*2+1];
				}
				vertices.size += 4;

				r_ready++;
			} while(
				++r_cur < rects.size && 
				r[r_cur].layer == layer && 
				r[r_cur].tex == tex
			);

			// Make sure index buffer has enough indices
			if(indices.size < r_ready * 6) {
				darray_reserve(&indices, r_ready * 6);
				uint16* idx = indices.data;
				for(uint i = indices.size; i < r_ready * 6; i += 6) {
					idx[i + 0] = (i/6)*4 + 0;
					idx[i + 1] = (i/6)*4 + 1;
					idx[i + 2] = (i/6)*4 + 3;
					idx[i + 3] = (i/6)*4 + 1;
					idx[i + 4] = (i/6)*4 + 3;
					idx[i + 5] = (i/6)*4 + 2;
				}
				indices.size = r_ready * 6;
			}
		}

		if(layer == l[l_cur].layer) {
			// TODO
		}
	}

	if(r_ready)
		_draw_rects(&r_ready);
	if(l_ready)
		_draw_lines(&l_ready);
	
	_check_error();

	_sys_present();
	frame++;

	rects.size = 0;
	lines.size = 0;
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
#ifndef TARGET_IOS
		case PF_ETC1:
			*fmt = GL_ETC1_RGB8_OES;
			*type = -1;
#endif
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
	glActiveTexture(GL_TEXTURE0);
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
#ifndef TARGET_IOS
	if(fmt == GL_ETC1_RGB8_OES) {
		assert(width % 4 == 0 && height % 4 == 0);
		image_size = (width * height * 3) / 6;
	}
#endif
    
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

// Fast integer log2 when n is a power of two
static uint _ilog2(uint v) {
	const uint b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000}; 
	uint r = (v & b[0]) != 0;
	for (uint i = 4; i > 0; i--) 
	{
		r |= ((v & b[i]) != 0) << i;
	}
	return r;
}

static Texture* _new_texture( 
		const char* filename, uint width, uint height, 
		uint gl_id, uint scale) {

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
    new->wlog2 = _ilog2((width * scale) >> 15);
	new->hlog2 = _ilog2((height * scale) >> 15);
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

	Texture* new_tex = _new_texture(NULL, width, height, gl_id, tex_scale_1);

	_check_error();

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

	texture = _new_texture(filename, width, height, gl_id, tex_scale_1);

	_check_error();

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

	_check_error();
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
	t->scale = (uint)(s * (tex_mul+1.0f));
    t->wlog2 = _ilog2((t->width * t->scale) >> 15);
	t->hlog2 = _ilog2((t->height * t->scale) >> 15);
}

void video_draw_rect(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, Color tint) {
	video_draw_rect_rotated(tex, layer, source, dest, 0.0f, tint);
}



void video_draw_rect_rotated(TexHandle tex, uint layer, const RectF* source, 
		const RectF* dest, float rotation, Color tint) {

	Texture* texture = (Texture*)tex;

	TexturedRect rect = {
		.layer = layer,
		.tex = texture,
		.tint = tint,
		.angle = (int32)(rotation * rot_mul),
	};

	if(source != NULL) {
		rect.src_l = (int16)source->left;
		rect.src_t = (int16)source->top;
		rect.src_r = (int16)source->right;
		rect.src_b = (int16)source->bottom;
	}
	else {
		rect.src_l = 0;
		rect.src_t = 0;
		rect.src_r = texture->width;
		rect.src_b = texture->height;
	}

	rect.dest_l = (int16)(dest->left * pos_mul);
	rect.dest_t = (int16)(dest->top * pos_mul);
	rect.dest_r = (int16)(dest->right * pos_mul);
	rect.dest_b = (int16)(dest->bottom * pos_mul);

	if((rect.dest_r | rect.dest_b) == 0) {
		rect.dest_r = rect.dest_l + (rect.src_r - rect.src_l);
		rect.dest_b = rect.dest_t + (rect.src_b - rect.src_t);
	}

	assert(is_pow2((texture->width * texture->scale) >> 15));
	assert(is_pow2((texture->height * texture->scale) >> 15));

	rect.src_l = ((rect.src_l << 15)) >> texture->wlog2;
	rect.src_t = ((rect.src_t << 15)) >> texture->hlog2;
	rect.src_r = ((rect.src_r << 15)) >> texture->wlog2;
	rect.src_b = ((rect.src_b << 15)) >> texture->hlog2;

	darray_append(&rects, &rect);
}

void video_draw_line(uint layer, const Vector2* start,
		const Vector2* end, Color color) {

	Line new = {
		.layer = layer,
		.tint = color,
		.start_x = (int16)(start->x * pos_mul),
		.start_y = (int16)(start->y * pos_mul),
		.end_x = (int16)(end->x * pos_mul),
		.end_y = (int16)(end->y * pos_mul)
	};

	darray_append(&lines, &new);
}

