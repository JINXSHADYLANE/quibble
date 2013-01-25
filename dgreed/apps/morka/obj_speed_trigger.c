#include "obj_types.h"

#include <system.h>

static void obj_speed_trigger_collide(GameObject* self, GameObject* other) {
	ObjSpeedTrigger* t = (ObjSpeedTrigger*)self;
	if(other->type == OBJ_RABBIT_TYPE) {
		PhysicsComponent* p = other->physics;
		objects_apply_force(other, vec2(-p->vel.x * t->drag_coef, 0.0f));
	}
}

static void obj_speed_trigger_update(GameObject* self){
	PhysicsComponent* physics = self->physics;
	RectF pos = {
		.right = physics->cd_obj->pos.x + physics->cd_obj->size.size.x
	};
	RectF result = objects_world2screen(pos,0);
	
	if(result.right < 0){
		objects_destroy(self);
	}
}

static void obj_speed_trigger_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	//float height = size.y;

	// Physics

	PhysicsComponent* physics = self->physics;
	RectF collider = {
		pos.x, pos.y - 127 - 10,
		pos.x + width, pos.y - 127
	};	
		
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->hit_callback = obj_speed_trigger_collide;
	
	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_speed_trigger_update;	
}

GameObjectDesc obj_speed_trigger_desc = {
	.type = OBJ_TRIGGER_TYPE,
	.size = sizeof(ObjSpeedTrigger),
	.has_physics = true,
	.has_render = false,
	.has_update = true,
	.construct = obj_speed_trigger_construct,
	.destruct = NULL
};

