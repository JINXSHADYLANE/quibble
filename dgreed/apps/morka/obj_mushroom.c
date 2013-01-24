#include "obj_types.h"

#include <system.h>

static void obj_mushroom_collide(GameObject* self, GameObject* other) {
	ObjMushroom* mushroom = (ObjMushroom*)self;
	if(other->type == OBJ_RABBIT_TYPE && mushroom->damage == 0.0f) {
		PhysicsComponent* rabbit_physics = other->physics;
		if(rabbit_physics->vel.y > 0.0f) {
			mushroom->dh -= 10.0f;
		}
	}
}

static void obj_mushroom_update_pos(GameObject* self) {
	ObjMushroom* mushroom = (ObjMushroom*)self;
	RenderComponent* r = self->render;

	float f = 30.0f * (mushroom->oh - mushroom->h);
	mushroom->dh += f * (time_delta()/1000.0f);
	mushroom->h += (mushroom->dh * 10.0f) * (time_delta()/1000.0f);
	mushroom->dh *= 0.9f;

	r->world_dest.top = r->world_dest.bottom - mushroom->h;
}

static void obj_mushroom_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjMushroom* mushroom = (ObjMushroom*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	mushroom->oh = height;
	mushroom->h = height;
	mushroom->dh = 0.0f;
	//mushroom->damage = 0.0f;
	
	// Take 10 pixel slice at the top as a collider geometry
	RectF collider = {
		pos.x - width / 2.0f + 20.0f, pos.y - height / 2.0f,
		pos.x + width / 2.0f - 20.0f, pos.y - height / 2.0f + 10.0f
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = obj_mushroom_collide;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x - width / 2.0f, pos.y - height / 2.0f,
			pos.x + width / 2.0f, pos.y + height / 2.0f
	);
	render->layer = 2;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_mushroom_update_pos;
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

