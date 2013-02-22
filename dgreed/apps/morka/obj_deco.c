#include "obj_types.h"
#include "common.h"

static void obj_deco_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	Vector2 size = sprsheet_get_size_h(spr_handle);

	RectF dest = {
		pos.x, pos.y - size.y,
		pos.x + size.x, pos.y
	};

	// Render
	RenderComponent* render = self->render;
	render->world_dest = dest;
	render->layer = self->type == OBJ_DECO_TYPE ? bg_mushrooms_layer : ground_layer;
	render->color = COLOR_RGBA(255, 255, 255, 255);
	render->camera = self->type == OBJ_DECO_TYPE ? 1 : 0;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
}

GameObjectDesc obj_deco_desc = {
	.type = OBJ_DECO_TYPE,
	.size = sizeof(ObjDeco),
	.has_physics = false,
	.has_render = true,
	.has_update = false,
	.construct = obj_deco_construct,
	.destruct = NULL
};

GameObjectDesc obj_fg_deco_desc = {
	.type = OBJ_FG_DECO_TYPE,
	.size = sizeof(ObjDeco),
	.has_physics = false,
	.has_render = true,
	.has_update = false,
	.construct = obj_deco_construct,
	.destruct = NULL
};


