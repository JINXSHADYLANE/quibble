#include "obj_types.h"

static void obj_deco_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	TexHandle tex;
	RectF src;
	sprsheet_get_h(spr_handle, &tex, &src);
	float width = rectf_width(&src);
	float height = rectf_height(&src);

	RectF dest = {
		pos.x - width / 2.0f, pos.y - height / 2.0f,
		pos.x + width / 2.0f, pos.y + height / 2.0f
	};

	// Render
	RenderComponent* render = self->render;
	render->world_dest = dest;
	render->layer = 1;
	render->color = COLOR_RGBA(255, 255, 255, 255);
	render->camera = 1;
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

