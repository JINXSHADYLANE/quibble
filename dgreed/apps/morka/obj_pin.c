#include "obj_types.h"
#include "common.h"

static void obj_pin_update_pos(GameObject* self) {
	ObjPin* pin = (ObjPin*)self;
	RenderComponent* r = self->render;

	r->world_dest.left += pin->speed;
	r->world_dest.right += pin->speed;
}

static void obj_pin_construct(GameObject* self, Vector2 pos, void* user_data) {
	float speed = *(float*)user_data;

	ObjPin* pin = (ObjPin*)self;
	pin->speed = speed;

	SprHandle spr_handle = sprsheet_get_handle("pin");

	TexHandle tex;
	RectF src;
	sprsheet_get_h(spr_handle, &tex, &src);
	float width = rectf_width(&src);
	float height = rectf_height(&src);


	RectF dest = {
		pos.x - width / 2.0f, 0.0f,
		pos.x + width / 2.0f, height
	};

	// Render
	RenderComponent* render = self->render;
	render->world_dest = dest;
	render->layer = rabbit_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_pin_update_pos;
	render->became_invisible = NULL;
	render->color = COLOR_RGBA(64, 64, 192, 255);
	if(speed > 17.0f)
		render->color = COLOR_RGBA(64, 192, 64, 255);
	if(speed > 19.0f)
		render->color = COLOR_RGBA(192, 64, 64, 255);

}

GameObjectDesc obj_pin_desc = {
	.type = OBJ_PIN,
	.size = sizeof(ObjPin),
	.has_physics = false,
	.has_render = true,
	.has_update = false,
	.construct = obj_pin_construct,
	.destruct = NULL 
};
