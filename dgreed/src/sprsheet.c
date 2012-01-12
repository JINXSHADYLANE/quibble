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
	bool loaded;

	// Animation
	uint frames;
	Vector2 tex_size;
	Vector2 offset;
} SprDesc;

// Global state
#define SPRSHEET_PREFIX_MAXLEN 64

static bool sprsheet_initialized = false;
static char sprsheet_prefix[SPRSHEET_PREFIX_MAXLEN];
static Dict sprsheet_dict;
static MMLObject sprsheet_mml;
static DArray sprsheet_descs;

static void _parse_img(NodeIdx node) {
	const char* name = mml_getval_str(&sprsheet_mml, node);

	const char* tex_name = NULL;
	RectF src = rectf_null();

	NodeIdx child = mml_get_first_child(&sprsheet_mml, node);
	for(; child != 0; child = mml_get_next(&sprsheet_mml, child)) {
		const char* name = mml_get_name(&sprsheet_mml, child);

		if(strcmp("tex", name) == 0) 
			tex_name = mml_getval_str(&sprsheet_mml, child);

		if(strcmp("src", name) == 0) 
			src = mml_getval_rectf(&sprsheet_mml, child);
	}

	if(!tex_name)
		LOG_ERROR("No 'tex' field in sprsheet img %s", name);

	// Create SprDesc and register it
	SprDesc new = {
		.tex_name = tex_name,
		.tex = 0,
		.src = src,
		.loaded = false,
		.frames = 1,
		.tex_size = {0.0f, 0.0f},
		.offset = {0.0f, 0.0f}
	};

	darray_append(&sprsheet_descs, &new);
	SprDesc* descs = DARRAY_DATA_PTR(sprsheet_descs, SprDesc);

#ifdef _DEBUG
	assert(dict_insert(&sprsheet_dict, name, &descs[sprsheet_descs.size-1]));
#else
	dict_insert(&sprsheet_dict, name, &descs[sprsheet_descs.size-1]);
#endif
}

static void _parse_anim(NodeIdx node) {
	const char* name = mml_getval_str(&sprsheet_mml, node);

	const char* tex_name = NULL;
	RectF src = rectf_null();
	uint frames = 0;
	Vector2 offset = {0.0f, 0.0f};

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
		.loaded = false,
		.frames = frames,
		.tex_size = {0.0f, 0.0f},
		.offset = offset 
	};

	darray_append(&sprsheet_descs, &new);
	SprDesc* descs = DARRAY_DATA_PTR(sprsheet_descs, SprDesc);

#ifdef _DEBUG
	assert(dict_insert(&sprsheet_dict, name, &descs[sprsheet_descs.size-1]));
#else
	dict_insert(&sprsheet_dict, name, &descs[sprsheet_descs.size-1]);
#endif
}

static SprDesc* _sprsheet_get(const char* name) {
	assert(name);

	SprDesc* desc = dict_get(&sprsheet_dict, name);
	return desc;
}

static RectF _sprsheet_animframe(SprDesc* desc, uint i) {
	assert(desc);
	assert(desc->frames > 1);
	assert(desc->tex_size.x > 0.0f && desc->tex_size.y > 0.0f);

	if(i >= desc->frames)
		LOG_ERROR("Trying to get %u frame out of %u", i, desc->frames); 

	float horiz_frames;
	modff((desc->tex_size.x - desc->src.left) / desc->offset.x, &horiz_frames);

	if(horiz_frames < 1.0f)
		LOG_ERROR("Anim in texture %s has a really weird size/place", desc->tex_name);

	uint hframes = lrintf(horiz_frames);
	uint x = i % hframes;
	uint y = i / hframes;

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
			if(!desc) 
				LOG_ERROR("Item %s in preload list is undefined!", sprite);

			// Load
			if(!desc->loaded) 
				_sprsheet_load(desc);
			
			// Continue to next preload list item
			sprite = strtok(NULL, ",");
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

	return desc;
}

void sprsheet_get(const char* name, TexHandle* tex, RectF* src) {
	sprsheet_get_h(sprsheet_get_handle(name), tex, src);
}

void sprsheet_get_h(SprHandle handle, TexHandle* tex, RectF* src) {
	assert(handle && tex && src);

	SprDesc* desc = (SprDesc*)handle;
	if(desc->frames > 1)
		LOG_ERROR("Sprite is an animation");

	if(!desc->loaded)
		_sprsheet_load(desc);

	*tex = desc->tex;
	*src = desc->src;
}

void sprsheet_get_anim(const char* name, uint frame, TexHandle* tex, RectF* src) {
	sprsheet_get_anim_h(sprsheet_get_handle(name), frame, tex, src);
}

void sprsheet_get_anim_h(SprHandle handle, uint frame, TexHandle* tex, RectF* src) {
	assert(handle && tex && src);

	SprDesc* desc = (SprDesc*)handle;
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

	SprDesc* desc = (SprDesc*)handle;
	if(desc->frames < 2)
		LOG_ERROR("Anim is a sprite");

	return desc->frames;
}


// Rendering helpers

void spr_draw(const char* name, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get(name, &tex, &src);
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_h(SprHandle handle, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_h(handle, &tex, &src);
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_anim(const char* name, uint frame, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim(name, frame, &tex, &src);
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_anim_h(SprHandle handle, uint frame, uint layer, RectF dest, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim_h(handle, frame, &tex, &src);
	video_draw_rect(tex, layer, &src, &dest, tint);
}

void spr_draw_cntr(const char* name, uint layer, Vector2 dest, float rot,
		float scale, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get(name, &tex, &src);
	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale, tint); 
}

void spr_draw_cntr_h(SprHandle handle, uint layer, Vector2 dest, float rot,
		float scale, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_h(handle, &tex, &src);
	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale, tint);
}

void spr_draw_cntr_anim(const char* name, uint frame, uint layer, Vector2 dest, 
		float rot, float scale, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim(name, frame, &tex, &src);
	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale, tint);
}

void spr_draw_cntr_anim_h(SprHandle handle, uint frame, uint layer, Vector2 dest, 
		float rot, float scale, Color tint) {
	TexHandle tex;
	RectF src;

	sprsheet_get_anim_h(handle, frame, &tex, &src);
	gfx_draw_textured_rect(tex, layer, &src, &dest, rot, scale, tint);
}
