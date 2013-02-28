#include "obj_types.h"
#include "common.h"

#include <system.h>
#include <mfx.h>
#include <vfont.h>

static const float floating_speed = 3.0f;

void obj_floater_pre_render(GameObject* self){
	ObjFloater* floater = (ObjFloater*)self;
	RenderComponent* render = self->render;

	// animation
	float t0 = floater->t0;
	float t1 = t0 + floater->duration;
	float ct = time_s();

	if(unlikely(ct >= t1)) {
		objects_destroy(self);
	}
	else {	
		// No need to check range or clamp, because of invariants: 
		// ct < t1 (prev. branch)
		// ct >= t0 (time_s() is monotonic and t0 gets earlier value)
		float t = (ct - t0) / (t1 - t0);

		float alpha = 1.0f - t;
		byte a = lrintf(255.0f * alpha);
		Color col = COLOR_RGBA(255, 255, 255, a);

		floater->txt_pos.y -= floating_speed;
		render->world_dest.top -= floating_speed;
		render->world_dest.bottom -= floating_speed;
		render->color = col;

		if(strcmp(floater->text, "") != 0) {
			vfont_select(FONT_NAME, 48.0f); 
			vfont_draw(
				floater->text, 
				render->layer,
				objects_world2screen_vec2(floater->txt_pos, 0),
				col
			);
		}
	}
}

static void obj_floater_construct(GameObject* self, Vector2 pos, void* user_data) {
	ObjFloaterParams* params = (ObjFloaterParams*)user_data;
	ObjFloater* floater = (ObjFloater*)self;
	SprHandle spr = sprsheet_get_handle(params->spr);

	floater->t0 = time_s(); 
	floater->duration = params->duration;
	if(params->text)
		strncpy(floater->text, params->text, sizeof(floater->text));
	else
		floater->text[0] = '\0';

	vfont_select(FONT_NAME, 48.0f); 
	Vector2 txt_size = vfont_size(floater->text);
	Vector2 img_size = sprsheet_get_size_h(spr);

	// Render
	RenderComponent* render = self->render;

	// Sprite will be initially centered at pos
	render->world_dest = rectf_centered(pos, img_size.x, img_size.y);

	// Draw text to the right of sprite, centered in y axis
	floater->txt_pos = vec2(
		render->world_dest.right, 
		pos.y - txt_size.y / 2.0f
	);

	render->layer = particle_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr;
	render->pre_render = obj_floater_pre_render;
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

