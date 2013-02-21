#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <mfx.h>

static void obj_floater_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjFloater* floater = (ObjFloater*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x - width/2.0f, pos.y - height/2.0f,
			pos.x + width/2.0f, pos.y + height/2.0f
	);
	render->layer = particle_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
}

GameObjectDesc obj_floater_desc = {
	.type = OBJ_FLOATER_TYPE,
	.size = sizeof(ObjFloater),
	.has_physics = false,
	.has_render = true,
	.has_update = false,
	.construct = obj_floater_construct,
	.destruct = NULL
};

