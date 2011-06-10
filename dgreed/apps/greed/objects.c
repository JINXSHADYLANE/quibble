#include "objects.h"
#include "gfx_utils.h"
#include "memory.h"

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
		for(NodeIdx frame = mml_get_first_child(&defs, frame);
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
	}
	darray_free(&frames_array);
}

void object_reset(DArray* objs) {
	assert(objs);
	assert(sizeof(Object) == objs->item_size);

	objects = objs;
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
		uint def_id = 0;
		while(
			def_id < n_object_defs &&
			strcmp(def_name, object_defs[def_id++].name) != 0
		);
		assert(def_id <= n_object_defs);
		new.def = &object_defs[def_id-1];

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

void objects_render(float t) {
	assert(objects);

	Object* objs = DARRAY_DATA_PTR(*objects, Object);
	for(uint i = 0; i < objects->size; ++i) {
		Object* obj = &objs[i];

		uint frame = _get_frame(obj, t);

		gfx_draw_textured_rect(
			obj->def->texture, obj->layer,
			&frames[frame], &obj->pos, obj->rot,
			obj->size, obj->color
		);
	}
}

