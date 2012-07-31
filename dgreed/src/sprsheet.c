#include "sprsheet.h"

#include "mml.h"
#include "datastruct.h"
#include "darray.h"
#include "memory.h"
#include "gfx_utils.h"

typedef struct {
	// Common
	const char* tex_name;
	TexHandle tex;
	RectF src;
	Vector2 cntr_off;
	bool loaded;

	// Animation
	uint frames;
	uint grid_w, grid_h;
	Vector2 tex_size;
	Vector2 offset;
} SprDesc;

// Global state
#define SPRSHEET_PREFIX_MAXLEN 64

static bool sprsheet_initialized = false;
static char sprsheet_prefix[SPRSHEET_PREFIX_MAXLEN];
static float sprsheet_scale;
static Dict sprsheet_dict;
static MMLObject sprsheet_mml;
static DArray sprsheet_descs;

static void _parse_img(NodeIdx node) {
	const char* name = mml_getval_str(&sprsheet_mml, node);

	const char* tex_name = NULL;
	RectF src = rectf_null();
	Vector2 cntr = vec2(-1.0f, -1.0f);

	NodeIdx child = mml_get_first_child(&sprsheet_mml, node);
	for(; child != 0; child = mml_get_next(&sprsheet_mml, child)) {
		const char* name = mml_get_name(&sprsheet_mml, child);

		if(strcmp("tex", name) == 0) 
			tex_name = mml_getval_str(&sprsheet_mml, child);

		if(strcmp("src", name) == 0) 
			src = mml_getval_rectf(&sprsheet_mml, child);

		if(strcmp("cntr", name) == 0)
			cntr = mml_getval_vec2(&sprsheet_mml, child);
	}

	if(cntr.x < 0.0f && cntr.y < 0.0f) {
		// Default center
		cntr.x = 0.0f;
		cntr.y = 0.0f;
	}
	else {
		// Override center
		
		Vector2 default_cntr = vec2(
			floorf((src.left + src.right) * 0.5f),
			floorf((src.top + src.bottom) * 0.5f)
		);

		// cntr now becomes offset from default center
		cntr = vec2_scale(vec2_sub(default_cntr, cntr), sprsheet_scale);
	}

	if(!tex_name)
		LOG_ERROR("No 'tex' field in sprsheet img %s", name);

	// Create SprDesc and register it
	SprDesc new = {
		.tex_name = tex_name,
		.tex = 0,
		.src = src,
		.cntr_off = cntr,
		.loaded = false,
		.frames = 1,
		.grid_w = 0,
		.grid_h = 0,
		.tex_size = {0.0f, 0.0f},
		.offset = {0.0f, 0.0f}
	};

	darray_append(&sprsheet_descs, &new);

#ifdef _DEBUG
	assert(dict_insert(&sprsheet_dict, name, (void*)(NULL+sprsheet_descs.size-1)));
#else
	dict_insert(&sprsheet_dict, name, (void*)(NULL+sprsheet_descs.size-1));
#endif
}

static void _parse_anim(NodeIdx node) {
	const char* name = mml_getval_str(&sprsheet_mml, node);

	const char* tex_name = NULL;
	RectF src = rectf_null();
	uint frames = 0;
	Vector2 offset = {0.0f, 0.0f};
	uint grid_w = 0, grid_h = 0;

	NodeIdx child = mml_get_first_child(&sprsheet_mml, node);
	for(; child != 0; child = mml_get_next(&sprsheet_mml, child)) {
		const char* name = mml_get_name(&sprsheet_mml, child);

		if(strcmp("tex", name) == 0) 
			tex_name = mml_getval_str(&sprsheet_mml, child);

		if(strcmp("src", name) == 0) 
			src = mml_getval_rectf(&sprsheet_mml, child);

		if(strcmp("frames", name) == 0) 
			frames = mml_getval_uint(&sprsheet_mml, child);

		if(strcmp("offset", name) == 0) 
			offset = mml_getval_vec2(&sprsheet_mml, child);

		if(strcmp("grid", name) == 0) {
			Vector2 temp = mml_getval_vec2(&sprsheet_mml, child);
			grid_w = lrintf(temp.x);
			grid_h = lrintf(temp.y);
		}
	}

	if(!tex_name)
		LOG_ERROR("No 'tex' field in sprsheet img %s", name);
	if(offset.x == 0.0f && offset.y == 0.0f)
		LOG_ERROR("No 'offset' field in sprsheet anim %s", name);
	if(frames < 2)
		LOG_ERROR("No/wrong 'frames' field in sprsheet anim %s", name);

	// Create SprDesc and register it
	SprDesc new = {
		.tex_name = tex_name,
		.tex = 0,
		.src = src,
		.cntr_off = vec2(0.0f, 0.0f),
		.loaded = false,
		.frames = frames,
		.grid_w = grid_w,
		.grid_h = grid_h,
		.tex_size = {0.0f, 0.0f},
		.offset = offset 
	};

	darray_append(&sprsheet_descs, &new);

#ifdef _DEBUG
	assert(dict_insert(&sprsheet_dict, name, (void*)(NULL+sprsheet_descs.size-1)));
#else
	dict_insert(&sprsheet_dict, name, (void*)(NULL+sprsheet_descs.size-1));
#endif
}

static SprDesc* _sprsheet_get(const char* name) {
	assert(name);

	const void* desc = dict_get(&sprsheet_dict, name);
	SprDesc* descs = DARRAY_DATA_PTR(sprsheet_descs, SprDesc);
	assert((size_t)desc < sprsheet_descs.size);
	return &descs[(size_t)desc];
}

static RectF _sprsheet_animframe(SprDesc* desc, uint i) {
	assert(desc);
	assert(desc->frames > 1);
	assert(desc->tex_size.x > 0.0f && desc->tex_size.y > 0.0f);

	if(i >= desc->frames)
		LOG_ERROR("Trying to get %u frame out of %u", i, desc->frames); 

	uint x, y;
	if(desc->grid_w * desc->grid_h == 0) {
		float horiz_frames;
		modff((desc->tex_size.x - desc->src.left) / desc->offset.x, &horiz_frames);

		if(horiz_frames < 1.0f)
			LOG_ERROR("Anim in texture %s has a really weird size/place", desc->tex_name);

		uint hframes = lrintf(horiz_frames);
		x = i % hframes;
		y = i / hframes;
	}
	else {
		assert(desc->grid_w * desc->grid_h >= desc->frames);
		x = i % desc->grid_w;
		y = i / desc->grid_w;
	}

	float w = rectf_width(&desc->src);
	float h = rectf_height(&desc->src);
	RectF src = rectf(desc->src.left + (float)x * w, desc->src.top + (float)y * h, 0.0f, 0.0f);
	src.right = src.left + w;
	src.bottom = src.top + h;

	return src;
}

static void _sprsheet_load(SprDesc* desc) {
	assert(desc);
	assert(!desc->loaded);

	// Construct path
	char path[128];
	assert(strlen(sprsheet_prefix) + strlen(desc->tex_name) < 128);
	strcpy(path, sprsheet_prefix);
	strcat(path, desc->tex_name);

#ifdef TARGET_IOS
	// Try to load dig on iOS
	size_t i = strfind(".png", path);	
	if(i >= 0) {
		// Check if dig file exists
		path[i+1] = 'd'; path[i+2] = 'i';
		if(!file_exists(path)) {
			// Revert back to png
			path[i+1] = 'p'; path[i+2] = 'n';
		}
	}
#endif

	// Load
	desc->tex = tex_load(path);

	// Get texture size for animations
	if(desc->frames > 1) {
		uint tex_w, tex_h;
		tex_size(desc->tex, &tex_w, &tex_h);
		desc->tex_size = vec2((float)tex_w, (float)tex_h);
	}

	desc->loaded = true;
}

static void _sprsheet_load_desc(const char* desc) {
	assert(sprsheet_descs.item_size == sizeof(SprDesc));

	// Read mml
	char* mml_text = txtfile_read(desc); 
	if(!mml_deserialize(&sprsheet_mml, mml_text))
		LOG_ERROR("Unable to parse sprsheet desc %s", desc);
	MEM_FREE(mml_text);

	NodeIdx root = mml_root(&sprsheet_mml);
	if(strcmp(mml_get_name(&sprsheet_mml, root), "sprsheet") != 0)
		LOG_ERROR("Invalid sprsheet desc %s", desc);

	// Parse mml structure filling data to global state along the way
	NodeIdx child = mml_get_first_child(&sprsheet_mml, root);
	for(; child != 0; child = mml_get_next(&sprsheet_mml, child)) {
		const char* name = mml_get_name(&sprsheet_mml, child);

		if(strcmp("scale", name) == 0)
			sprsheet_scale = mml_getval_float(&sprsheet_mml, child);

		if(strcmp("img", name) == 0) 
			_parse_img(child);

		if(strcmp("anim", name) == 0) 
			_parse_anim(child);

		if(strcmp("prefix", name) == 0) {
			const char* prefix = mml_getval_str(&sprsheet_mml, child);
			assert(strlen(prefix) < SPRSHEET_PREFIX_MAXLEN);

			strcpy(sprsheet_prefix, prefix);
		}
	}

	// Do preloading
	NodeIdx preload = mml_get_child(&sprsheet_mml, root, "preload");	
	if(preload) {
		const char* immutable_pre_list = mml_getval_str(&sprsheet_mml, preload);
		char* pre_list = strclone(immutable_pre_list);
		const char* sprite = strtok(pre_list, ",");
		while(sprite) {
			SprDesc* desc = _sprsheet_get(sprite);
			if(!desc) {
				LOG_ERROR("Item %s in preload list is undefined!", sprite);
            }
            else {
                // Load
                if(!desc->loaded) 
                    _sprsheet_load(desc);
			
                // Continue to next preload list item
                sprite = strtok(NULL, ",");
            }
		}
		MEM_FREE(pre_list);
	}
}

void sprsheet_init(const char* desc) {
	assert(desc);
	assert(sprsheet_initialized == false);

	dict_init(&sprsheet_dict);
	sprsheet_descs = darray_create(sizeof(SprDesc), 0);
	sprsheet_prefix[0] = '\0';
	sprsheet_scale = 1.0f;
	_sprsheet_load_desc(desc);

	sprsheet_initialized = true;
}

void sprsheet_close(void) {
	assert(sprsheet_initialized);

	// Free textures
	SprDesc* descs = DARRAY_DATA_PTR(sprsheet_descs, SprDesc);
	for(uint i = 0; i < sprsheet_descs.size; ++i) {
		if(descs[i].loaded) 
			tex_free(descs[i].tex);
	}

	// Deinit global state
	mml_free(&sprsheet_mml);
	darray_free(&sprsheet_descs);
	dict_free(&sprsheet_dict);

	sprsheet_initialized = false;
}

SprHandle sprsheet_get_handle(const char* name) {
	assert(name);

	SprDesc* desc = _sprsheet_get(name);
	if(!desc)
		LOG_ERROR("Sprite %s does not exist", name);

	return ((void*)desc - sprsheet_descs.data) / sizeof(SprDesc);
}

static SprDesc* _get_desc(SprHandle handle) {
	assert(handle < sprsheet_descs.size);

	SprDesc* descs = DARRAY_DATA_PTR(sprsheet_descs, SprDesc);
	return &descs[handle];
}

void sprsheet_get(const char* name, TexHandle* tex, RectF* src) {
	sprsheet_get_ex_h(sprsheet_get_handle(name), tex, src, NULL);
}

void sprsheet_get_ex(const char* name, TexHandle* tex, RectF* src, Vector2* cntr_off) {
	sprsheet_get_ex_h(sprsheet_get_handle(name), tex, src, cntr_off);
}

void sprsheet_get_h(SprHandle handle, TexHandle* tex, RectF* src) {
	sprsheet_get_ex_h(handle, tex, src, NULL);
}

void sprsheet_get_ex_h(SprHandle handle, TexHandle* tex, RectF* src, Vector2* cntr_off) {
	assert(tex && src);

	SprDesc* desc = _get_desc(handle);
	if(desc->frames > 1)
		LOG_ERROR("Sprite is an animation");

	if(!desc->loaded)
		_sprsheet_load(desc);

	*tex = desc->tex;
	*src = desc->src;

	if(cntr_off)
		*cntr_off = desc->cntr_off;
}

void sprsheet_get_anim(const char* name, uint frame, TexHandle* tex, RectF* src) {
	sprsheet_get_anim_h(sprsheet_get_handle(name), frame, tex, src);
}

void sprsheet_get_anim_h(SprHandle handle, uint frame, TexHandle* tex, RectF* src) {
	assert(tex && src);

	SprDesc* desc = _get_desc(handle);
	if(desc->frames < 2)
		LOG_ERROR("Anim is a sprite");

	if(!desc->loaded)
		_sprsheet_load(desc);

	*tex = desc->tex;
	*src = _sprsheet_animframe(desc, frame);
}

uint sprsheet_get_anim_frames(const char* name) {
	return sprsheet_get_anim_frames_h(sprsheet_get_handle(name));
}

uint sprsheet_get_anim_frames_h(SprHandle handle) {

	SprDesc* desc = _get_desc(handle);
	if(desc->frames < 2)
		LOG_ERROR("Anim is a sprite");

	return desc->frames;
}


// Rendering helpers

void spr_draw(const char* name, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get(name, &tex, &src);
	if(dest.right == 0.0f && dest.bottom == 0.0f) {
		dest.right = dest.left + rectf_width(&src) * sprsheet_scale;
		dest.bottom = dest.top + rectf_height(&src) * sprsheet_scale;
	}
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_h(SprHandle handle, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_h(handle, &tex, &src);
	if(dest.right == 0.0f && dest.bottom == 0.0f) {
		dest.right = dest.left + rectf_width(&src) * sprsheet_scale;
		dest.bottom = dest.top + rectf_height(&src) * sprsheet_scale;
	}
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_anim(const char* name, uint frame, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim(name, frame, &tex, &src);
	if(dest.right == 0.0f && dest.bottom == 0.0f) {
		dest.right = dest.left + rectf_width(&src) * sprsheet_scale;
		dest.bottom = dest.top + rectf_height(&src) * sprsheet_scale;
	}
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_anim_h(SprHandle handle, uint frame, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim_h(handle, frame, &tex, &src);
	if(dest.right == 0.0f && dest.bottom == 0.0f) {
		dest.right = dest.left + rectf_width(&src) * sprsheet_scale;
		dest.bottom = dest.top + rectf_height(&src) * sprsheet_scale;
	}
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_cntr(const char* name, uint layer, Vector2 dest, float rot,
		float scale, Color tint) {
	TexHandle tex;
	RectF src;
	Vector2 cntr_off;

	sprsheet_get_ex(name, &tex, &src, &cntr_off);

	if(cntr_off.x != 0.0f || cntr_off.y != 0.0f)
		dest = vec2_add(dest, vec2_rotate(cntr_off, rot));

	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale * sprsheet_scale, tint); 
}

void spr_draw_cntr_h(SprHandle handle, uint layer, Vector2 dest, float rot,
		float scale, Color tint) {
	TexHandle tex;
	RectF src;
	Vector2 cntr_off;

	sprsheet_get_ex_h(handle, &tex, &src, &cntr_off);

	if(cntr_off.x != 0.0f || cntr_off.y != 0.0f)
		dest = vec2_add(dest, vec2_rotate(cntr_off, rot));

	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale * sprsheet_scale, tint);
}

void spr_draw_anim_cntr(const char* name, uint frame, uint layer, Vector2 dest, 
		float rot, float scale, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim(name, frame, &tex, &src);

	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale * sprsheet_scale, tint);
}

void spr_draw_anim_cntr_h(SprHandle handle, uint frame, uint layer, Vector2 dest, 
		float rot, float scale, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim_h(handle, frame, &tex, &src);

	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale * sprsheet_scale, tint);
}

