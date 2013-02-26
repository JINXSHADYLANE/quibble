#include "obj_types.h"
#include "common.h"

#include <system.h>
#include <mfx.h>
#include <vfont.h>

void obj_floater_pre_render(GameObject* self){
	ObjFloater* floater = (ObjFloater*)self;
	RenderComponent* render = self->render;

	vfont_select(FONT_NAME, 48.0f); 

	if(render->world_dest.bottom == 0.0f) {
		// animation init
		floater->t0 = time_s();
		floater->t1 = floater->t0 + floater->a;

		// Centering text and image on pos
		Vector2 pos = vec2(render->world_dest.left,render->world_dest.top);

		Vector2 txt_size = vfont_size(floater->text);
		Vector2 img_size = sprsheet_get_size_h(render->spr);

		Vector2 half = vec2_scale(vec2_add(txt_size,img_size), 0.5f);

		render->world_dest = rectf(
				pos.x + half.x - img_size.x,
				pos.y - half.y - img_size.y / 2.0f,
				pos.x + half.x,
				pos.y - half.y + img_size.y / 2.0f
		);

		floater->txt_pos = vec2(pos.x - half.x, pos.y - half.y - txt_size.y / 2.0f);
	}

	// animation
	float ct = time_s();
	float delta = 0.0f;

	if(floater->t1 != 0.0f && ct > floater->t1 ) {
		objects_destroy(self);
		floater->t = 1.0f;
	} else {	

		if(ct > floater->t0 && ct < floater->t1){
			floater->t = (ct - floater->t0) / (floater->t1 - floater->t0);
			delta = (1.0f + (floater->t*floater->t)) * -3.0f;

			floater->txt_pos.y += delta;
			render->world_dest.top += delta;
			render->world_dest.bottom += delta;

		}

		float alpha = 1.0f-fabsf(floater->t);
		byte a = lrintf(255.0f * alpha);
		Color col = COLOR_RGBA(255, 255, 255, a);

		RectF txt_rec = rectf(
			floater->txt_pos.x,
			floater->txt_pos.y,
			0,
			0
		);

		RectF result = objects_world2screen(txt_rec,0);

		render->color = col;

		if(strcmp(floater->text, "") != 0) vfont_draw(floater->text, render->layer,vec2(result.left,result.top), col);

	}
}

static void obj_floater_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjFloater* floater = (ObjFloater*) self;
	floater->t = 0.0f;
	floater->t0 = 0.0f;
	floater->t1 = 0.0f;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x, pos.y,
			0.0f,0.0f
	);
	render->layer = particle_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
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

