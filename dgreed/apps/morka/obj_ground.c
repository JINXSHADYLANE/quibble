#include "obj_types.h"

#include <system.h>

static void obj_ground_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjGround* ground = (ObjGround*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	// Physics
	PhysicsComponent* physics = self->physics;
	RectF dest = {
		pos.x, pos.y - height + 60,
		pos.x + width, pos.y
	};
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &dest, 1, NULL);

	// Render
	RenderComponent* render = self->render;
	dest.top = pos.y - height;

	render->world_dest = dest;
	render->layer = 4;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
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

