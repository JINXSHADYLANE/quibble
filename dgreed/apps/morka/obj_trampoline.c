#include "obj_types.h"
#include "common.h"
#include <system.h>

static void obj_trampoline_collide(GameObject* self, GameObject* other) {
	ObjTrampoline* trampoline = (ObjTrampoline*)self;
	if(other->type == OBJ_RABBIT_TYPE && other == trampoline->owner) {
			trampoline->dh -= 50.0f;
			//trampoline->db = 5.0f;
	}
}

static void obj_trampoline_update_pos(GameObject* self) {
	ObjTrampoline* trampoline = (ObjTrampoline*)self;
	RenderComponent* r = self->render;

	float f = 30.0f * (trampoline->oh - trampoline->h);
	trampoline->dh += f * (time_delta()/1000.0f);
	trampoline->h += (trampoline->dh * 10.0f) * (time_delta()/1000.0f);
	trampoline->dh *= 0.9f;

	r->world_dest.top = r->world_dest.bottom - trampoline->h;
	r->world_dest.bottom += trampoline->db;

	PhysicsComponent* p = self->physics;

	if(p->cd_obj->pos.y > 783.0f && trampoline->db == 0.0f){
	 	p->cd_obj->pos.y -= 10.0f;
	 	r->world_dest.bottom -= 10.0f;
	}

	if(trampoline->owner->physics->cd_obj->pos.x > r->world_dest.right +50.0f) trampoline->db = 5.0f;
}

static void obj_trampoline_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjTrampoline* trampoline = (ObjTrampoline*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	trampoline->oh = height;
	trampoline->h = height;
	trampoline->dh = 0.0f;
	trampoline->db = 0.0f;

	RectF collider = {
		pos.x - 50.0f, 			pos.y,
		pos.x  + width + 50.0f, pos.y - height
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = obj_trampoline_collide;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x, pos.y - height,
			pos.x + width, pos.y
	);
	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_trampoline_update_pos;

	// for rising up animation
	physics->cd_obj->pos.y += height/2.0f - 15.0f;
	render->world_dest.bottom += height/2.0f - 15.0f;
}

GameObjectDesc obj_trampoline_desc = {
	.type = OBJ_TRAMPOLINE_TYPE,
	.size = sizeof(ObjTrampoline),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_trampoline_construct,
	.destruct = NULL
};

