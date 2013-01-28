#include "obj_types.h"

#include <system.h>

static void obj_fall_trigger_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		rabbit->touching_ground = false;
	}
}

static void obj_fall_trigger_update(GameObject* self, float ts, float dt){
	PhysicsComponent* physics = self->physics;
	RectF pos = {
		.right = physics->cd_obj->pos.x + physics->cd_obj->size.size.x
	};
	RectF result = objects_world2screen(pos,0);
	
	if(result.right < 0){
		objects_destroy(self);
	}
}

static void obj_fall_trigger_construct(GameObject* self, Vector2 pos, void* user_data) {
	uint width = (uint)user_data;
	// Physics

	PhysicsComponent* physics = self->physics;
	RectF collider = {
		pos.x + 70, pos.y - 127 - 62,
		pos.x + width + 120, pos.y
	};	
		
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->hit_callback = obj_fall_trigger_collide;

	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_fall_trigger_update;	
}

GameObjectDesc obj_fall_trigger_desc = {
	.type = OBJ_TRIGGER_TYPE,
	.size = sizeof(ObjFallTrigger),
	.has_physics = true,
	.has_render = false,
	.has_update = true,
	.construct = obj_fall_trigger_construct,
	.destruct = NULL
};

