#include "objects.h"

#include <gfx_utils.h>
#include <font.h>
#include <memory.h>

#define TEXT_LAYER 15

static const char* object_anim_names[] = { "none", "loop", "blink" };

uint n_object_defs = 0;
ObjectDef* object_defs = NULL;
DArray* objects = NULL;

// All frame source rects are stored in this single blob of memory
static DArray frames_array;
static RectF* frames;

void objects_init(const char* defs_file) {
	assert(defs_file);

	frames_array = darray_create(sizeof(RectF), 0);

	char* defs_text = txtfile_read(defs_file);

	MMLObject defs;
	if(!mml_deserialize(&defs, defs_text))
		LOG_ERROR("Unable to deserialize objects defs file");
	MEM_FREE(defs_text);

	NodeIdx root = mml_root(&defs);
	if(strcmp(mml_get_name(&defs, root), "object_defs") != 0)
		LOG_ERROR("Invalid object defs file");

	n_object_defs = mml_count_children(&defs, root);	
	object_defs = MEM_ALLOC(n_object_defs * sizeof(ObjectDef));
	
	NodeIdx mdef = mml_get_first_child(&defs, root);
	for(uint def_idx = 0;
		mdef != 0;
		mdef = mml_get_next(&defs, mdef), def_idx++) {
		
		// Name
		ObjectDef* def = &object_defs[def_idx];
		assert(strcmp(mml_get_name(&defs, mdef), "obj") == 0);
		def->name = strclone(mml_getval_str(&defs, mdef));

		// Texture (tex_load does internal refcounting, so we're not 
		// bothered with loading same textures twice)
		NodeIdx tex = mml_get_child(&defs, mdef, "tex");
		assert(tex != 0);
		def->texture = tex_load(mml_getval_str(&defs, tex));

		// Animation frames
		NodeIdx frames = mml_get_child(&defs, mdef, "frames");
		assert(frames != 0);
		def->start_frame = frames_array.size;
		def->n_frames = mml_count_children(&defs, frames);
		for(NodeIdx frame = mml_get_first_child(&defs, frames);
			frame != 0;
			frame = mml_get_next(&defs, frame)) {
			
			RectF f = mml_getval_rectf(&defs, frame);
			darray_append(&frames_array, &f);
		}

		// Animation type
		NodeIdx anim = mml_get_child(&defs, mdef, "anim");
		assert(anim != 0);
		const char* anim_type = mml_getval_str(&defs, anim);
		uint t = 0;
		while(
			t < OBJANIM_COUNT && 
			strcmp(anim_type, object_anim_names[t++]) != 0
		);
		def->anim_type = t > OBJANIM_COUNT ? 0 : t - 1;	

		// FPS
		NodeIdx fps = mml_get_child(&defs, mdef, "fps");
		assert(fps != 0);
		def->fps = mml_getval_float(&defs, fps);
	}

	frames = DARRAY_DATA_PTR(frames_array, RectF);

	mml_free(&defs);
}

void objects_close(void) {
	for(uint i = 0; i < n_object_defs; ++i) {
		ObjectDef* def = &object_defs[i];
		MEM_FREE(def->name);
		tex_free(def->texture);
	}
	darray_free(&frames_array);
	MEM_FREE(object_defs);
}

void objects_reset(DArray* objs) {
	assert(objs);
	assert(sizeof(Object) == objs->item_size);

	objects = objs;
}

static uint _get_def(const char* def_name) {
	uint def_id = 0;
	while(
		def_id < n_object_defs &&
		strcmp(def_name, object_defs[def_id++].name) != 0
	);
	assert(def_id <= n_object_defs);
	return def_id-1;
}

#define GET_OBJECT_VAL(type, name, dest) \
	NodeIdx name = mml_get_child(mml, mobj, #name); \
	assert(name != 0); \
	dest = mml_getval_##type(mml, name)

void objects_load(MMLObject* mml, NodeIdx node, DArray* objs) {
	assert(mml);

	uint obj_count = mml_count_children(mml, node);
	*objs = darray_create(sizeof(Object), obj_count);
	
	for(NodeIdx mobj = mml_get_first_child(mml, node);
		mobj != 0;
		mobj = mml_get_next(mml, mobj)) {
		
		Object new;

		// Find def
		const char* def_name = mml_getval_str(mml, mobj);
		new.def = &object_defs[_get_def(def_name)];

		GET_OBJECT_VAL(vec2, pos, new.pos);
		GET_OBJECT_VAL(float, size, new.size);
		GET_OBJECT_VAL(float, rot, new.rot);
		GET_OBJECT_VAL(float, phase, new.phase);
		GET_OBJECT_VAL(uint, layer, new.layer);
		GET_OBJECT_VAL(color, color, new.color);

		darray_append(objs, &new);
	}
}

#define SET_OBJECT_VAL(type, name, src) \
	NodeIdx name = mml_node(mml, #name, ""); \
	mml_setval_##type(mml, name, src); \
	mml_append(mml, obj_node, name)

void objects_save(MMLObject* mml, NodeIdx node, DArray* objs) {
	assert(mml && objs);	

	mml_set_name(mml, node, "objects");

	Object* objects = DARRAY_DATA_PTR(*objs, Object);
	for(uint i = 0; i < objs->size; ++i) {
		Object* obj = &objects[i];

		NodeIdx obj_node = mml_node(mml, "obj", obj->def->name);

		SET_OBJECT_VAL(vec2, pos, obj->pos);
		SET_OBJECT_VAL(float, size, obj->size);
		SET_OBJECT_VAL(float, rot, obj->rot);
		SET_OBJECT_VAL(float, phase, obj->phase);
		SET_OBJECT_VAL(uint, layer, obj->layer);
		SET_OBJECT_VAL(color, color, obj->color);

		mml_append(mml, node, obj_node);
	}
}

static uint _get_frame(Object* obj, float t) {
	assert(obj);

	if(obj->def->anim_type == OBJANIM_NONE)
		return obj->def->start_frame;

	if(obj->def->anim_type == OBJANIM_LOOP) 
		return obj->def->start_frame +
			(uint)floorf((t + obj->phase) * obj->def->fps) % obj->def->n_frames;

	assert(0 && "Animation not implemented");
	return 0;
}

void objects_render(float t, uint alpha) {
	assert(objects);
	assert(alpha < 256);

	Object* objs = DARRAY_DATA_PTR(*objects, Object);
	for(uint i = 0; i < objects->size; ++i) {
		Object* obj = &objs[i];

		uint frame = _get_frame(obj, t);

		// Multiplicative alpha chanel blend, integer ops only
		int final_alpha = alpha * (obj->color >> 24) / 256;
		Color final = final_alpha << 24 | (obj->color & 0xFFFFFF);

		gfx_draw_textured_rect(
			obj->def->texture, obj->layer,
			&frames[frame], &obj->pos, obj->rot,
			obj->size, final 
		);
	}
}

// --- Edit mode ---

extern FontHandle small_font;

static bool obj_sel = false;
static uint obj_sel_id = 0;
static Vector2 mdown_pos, sdown_pos, rdown_pos, obj_old_pos;
static Vector2 ldown_pos, tdown_pos, pdown_pos;
static float obj_old_scale, obj_old_rot, obj_old_phase;
static int obj_old_layer, obj_old_type;

void objects_edit(void) {
	const Color hot_color = COLOR_RGBA(128, 128, 128, 64);
	const Color sel_color = COLOR_RGBA(198, 128, 148, 128);

	Object* objs = DARRAY_DATA_PTR(*objects, Object);

	uint mx, my;
	mouse_pos(&mx, &my);
	Vector2 mpos = {(float)mx, (float)my};

	// Figure out 'hot' object
	bool hovering = false;
	uint hot_id = 0;
	for(uint i = 0; i < objects->size; ++i) {
		RectF r = frames[objs[i].def->start_frame];
		float half_width = rectf_width(&r) / 2.0f;
		float half_height = rectf_height(&r) / 2.0f;
		r = rectf(
			objs[i].pos.x - half_width, objs[i].pos.y - half_height,
			objs[i].pos.x + half_width, objs[i].pos.y + half_height
		);
		if(obj_sel && obj_sel_id == i) {
			gfx_draw_rect_rotscale(TEXT_LAYER, &r, 
				objs[i].rot, objs[i].size, sel_color);
		}
		if(rectf_contains_point_rotscale(&r, objs[i].rot, objs[i].size, &mpos)) {
			gfx_draw_rect_rotscale(TEXT_LAYER, &r, 
				objs[i].rot, objs[i].size, hot_color);

			if(mouse_down(MBTN_LEFT)) {
				obj_sel = true;
				obj_sel_id = i;
			}

			hovering = true;
			hot_id = i;
			break;
		}
	}
	if(!hovering && mouse_down(MBTN_LEFT)) {
		obj_sel = false;
	}

	if(obj_sel) {
		Object* obj = &objs[obj_sel_id];	

		Vector2 cursor = { 4.0f, 290.0f };
		char text[256];
		const float y_adv = 8.0f;

		sprintf(text, "[t]ype: %s", obj->def->name);
		font_draw(small_font, text, TEXT_LAYER, &cursor, COLOR_WHITE);
		cursor.y += y_adv;

		sprintf(text, "[l]ayer: %u", obj->layer);
		font_draw(small_font, text, TEXT_LAYER, &cursor, COLOR_WHITE);
		cursor.y += y_adv;

		sprintf(text, "[p]hase: %.2f", obj->phase);
		font_draw(small_font, text, TEXT_LAYER, &cursor, COLOR_WHITE);

		// Move
		if(hovering && obj_sel_id == hot_id) {
			if(char_down('m')) {
				mdown_pos = mpos;
				obj_old_pos = obj->pos; 
			}
			if(char_pressed('m')) {
				Vector2 delta = vec2_sub(mpos, mdown_pos);
				obj->pos = vec2_add(obj_old_pos, delta);
			}
		}

		// Scale
		if(char_down('s')) {
			sdown_pos = mpos;
			obj_old_scale = obj->size;
		}
		if(char_pressed('s')) {
			obj->size = obj_old_scale + (mpos.x - sdown_pos.x) / 50.0f;
		}

		// Rotate
		if(char_down('r')) {
			rdown_pos = mpos;
			obj_old_rot = obj->rot;
		}
		if(char_pressed('r')) {
			obj->rot = obj_old_rot + (mpos.x - rdown_pos.x) / 50.0f;
		}

		// Layer
		if(char_down('l')) {
			ldown_pos = mpos;
			obj_old_layer = obj->layer;
		}
		if(char_pressed('l')) {
			int d = (int)((mpos.x - ldown_pos.x) / 20.0f);
			obj->layer = MAX(0, MIN(15, (int)obj_old_layer + d));
		}

		// Phase
		if(char_down('p')) {
			pdown_pos = mpos;
			obj_old_phase = obj->phase;
		}
		if(char_pressed('p')) {
			float max_phase = (float)obj->def->n_frames / obj->def->fps;
			float d = (mpos.x - pdown_pos.x) / 80.0f;
			obj->phase = clamp(0.0f, max_phase, obj_old_phase + d);
		}

		// Type
		if(char_down('t')) {
			tdown_pos = mpos;
			obj_old_type = _get_def(obj->def->name); 
		}
		if(char_pressed('t')) {
			int d = (int)((mpos.x - tdown_pos.x) / 50.0f);
			int new_type = MAX(0, MIN(n_object_defs-1, (int)obj_old_type + d));
			obj->def = &object_defs[new_type]; 
		}

		// Remove object
		if(obj_sel && char_up('d')) {
			darray_remove_fast(objects, obj_sel_id);
			obj_sel = false;
		}
	}

	// Add object
	if(char_down('a')) {
		Object new = {NULL, {0.0f, 0.0f}, 1.0f, 0.0f, 0.0f, 15, COLOR_WHITE}; 
		if(obj_sel)
			new = objs[obj_sel_id];
		else
			new.def = objects->size ? objs[objects->size-1].def : &object_defs[0];
		new.pos = mpos;
		darray_append(objects, &new);
		return;
	}
	if(char_pressed('a')) {
		objs[objects->size-1].pos = mpos;	
	}

	// Help
	Vector2 cursor = vec2(80.0f, 306.0f);
	const char* help_string = "[a]dd [d]elete [m]ove [s]cale [r]otate";
	if(!obj_sel) {
		help_string = "[a]dd";
		cursor.x = 4.0f;
	}
	font_draw(small_font, help_string , TEXT_LAYER, &cursor, COLOR_WHITE);
}

