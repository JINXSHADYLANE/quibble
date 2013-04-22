#include "obj_types.h"
#include "common.h"
#include "minimap.h"
#include <system.h>

static void obj_eraser_collide(GameObject* self, GameObject* other) {
	if(other->type != OBJ_ERASER_TYPE && other->type != OBJ_RABBIT_TYPE){
		objects_destroy(other);
	}	
}

static void obj_eraser_update(GameObject* self, float ts, float dt){
	PhysicsComponent* p = self->physics;
	float pos = minimap_min_x();
	p->cd_obj->offset = vec2(pos - p->cd_obj->pos.x - 512.0f * objects_camera_z[0], 0.0f);
}

static void obj_eraser_construct(GameObject* self, Vector2 pos, void* user_data) {

	// Physics
	PhysicsComponent* physics = self->physics;
	RectF collider = {
		pos.x - 50.0f, pos.y,
		pos.x, pos.y + 400.0f
	};	
		
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, OBJ_ERASER_TYPE, NULL);
	physics->hit_callback = obj_eraser_collide;

	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_eraser_update;	
}

GameObjectDesc obj_eraser_desc = {
	.type = OBJ_ERASER_TYPE,
	.size = sizeof(ObjEraser),
	.has_physics = true,
	.has_render = false,
	.has_update = true,
	.construct = obj_eraser_construct,
	.destruct = NULL
};

