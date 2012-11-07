#include "obj_types.h"

#include <system.h>

static void obj_ground_construct(GameObject* self, Vector2 pos, void* user_data) {
	const float ground_height = 170.0f;
	const float slice_width = 256.0f;

	uint frame_number = (uint)user_data;

	// Physics
	PhysicsComponent* physics = self->physics;
	RectF dest = {
		pos.x - slice_width / 2.0f, pos.y + ground_height / 2.0f - 120.0f,
		pos.x + slice_width / 2.0f, pos.y + ground_height / 2.0f
	};
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &dest, 1, NULL);

	// Render
	RenderComponent* render = self->render;
	render->world_pos = pos;
	render->extent_min = dest.left;
	render->extent_max = dest.right;
	render->scale = 1.0f;
	render->layer = 4;
	render->anim_frame = frame_number;
	render->spr = sprsheet_get_handle("grass");
}

GameObjectDesc obj_ground_desc = {
	.type = OBJ_GROUND_TYPE,
	.size = sizeof(ObjGround),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_ground_construct,
	.destruct = NULL
};

