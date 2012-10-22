#include "obj_types.h"

#include <system.h>

static void obj_mushroom_became_invisible(GameObject* self) {
	objects_destroy(self);
}

static void obj_mushroom_construct(GameObject* self, Vector2 pos, void* user_data) {
	const char* spr_name = user_data;
	SprHandle spr_handle = sprsheet_get_handle(spr_name);

	TexHandle tex;
	RectF src;
	sprsheet_get_h(spr_handle, &tex, &src);
	float width = rectf_width(&src);
	float height = rectf_height(&src);
	
	// Take upper half of image rect as a collider geometry
	RectF collider = {
		pos.x - width / 2.0f, pos.y - height / 2.0f,
		pos.x + height / 2.0f, pos.y
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;

	// Render
	RenderComponent* render = self->render;
	render->world_pos = pos;
	render->extent_min = pos.x - width / 2.0f;
	render->extent_max = pos.x + width / 2.0f;
	render->scale = 1.0f;
	render->layer = 2;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->became_invisible = obj_mushroom_became_invisible;
}

GameObjectDesc obj_mushroom_desc = {
	.type = OBJ_MUSHROOM_TYPE,
	.size = sizeof(ObjMushroom),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_mushroom_construct,
	.destruct = NULL
};

