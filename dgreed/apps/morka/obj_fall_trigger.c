#include "obj_types.h"

#include <system.h>

static void obj_fall_trigger_construct(GameObject* self, Vector2 pos, void* user_data) {
	uint width = (uint)user_data;
	// Physics

	PhysicsComponent* physics = self->physics;
	RectF collider = {
		pos.x, pos.y - 127 - 62,
		pos.x + width, pos.y
	};	
		
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);

	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_fall_trigger_update;	
}

GameObjectDesc obj_fall_trigger_desc = {
	.type = OBJ_FALL_TRIGGER_TYPE,
	.size = sizeof(ObjFallTrigger),
	.has_physics = true,
	.has_render = false,
	.has_update = false,
	.construct = obj_fall_trigger_construct,
	.destruct = NULL
};

