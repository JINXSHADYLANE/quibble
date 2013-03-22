#include "obj_types.h"

#include <system.h>

static void obj_speed_trigger_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;

	// Physics
	PhysicsComponent* physics = self->physics;
	RectF collider = {
		pos.x, pos.y - 127 - 10,
		pos.x + width, pos.y - 127
	};	
		
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
}

GameObjectDesc obj_speed_trigger_desc = {
	.type = OBJ_SPEED_TRIGGER_TYPE,
	.size = sizeof(ObjSpeedTrigger),
	.has_physics = true,
	.has_render = false,
	.has_update = false,
	.construct = obj_speed_trigger_construct,
	.destruct = NULL
};

