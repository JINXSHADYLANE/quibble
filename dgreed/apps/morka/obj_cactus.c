#include "obj_types.h"
#include "common.h"
#include <system.h>

static void obj_cactus_collide(GameObject* self, GameObject* other) {
	ObjCactus* cactus = (ObjCactus*)self;
	if(other->type == OBJ_RABBIT_TYPE) {
		if(cactus->damage == 1.0f){

			PhysicsComponent* p = other->physics;
			cactus->m = 20.0f;
			cactus->d = 1.0f;

			p->vel.x = 0.0f;
			if(p->vel.y > 0.0f) p->vel.y = -p->vel.y;

			cactus->damage = 0.5f;
		}
	}
}

static void obj_cactus_update_pos(GameObject* self) {
	ObjCactus* cactus = (ObjCactus*)self;
	RenderComponent* r = self->render;

	if(cactus->d < cactus->m && cactus->damage > 0.1f) cactus->d *= 1.7f;
	if(cactus->d > cactus->m){
		if(cactus->damage > 0.1f){
			cactus->m = -20.0f;
			cactus->damage = 0.0f;
		} else {
			cactus->d -= 200.0f * (time_delta()/1000.0f);
		}
	} 

	r->world_dest.top = cactus->original.top - cactus->d;
	r->world_dest.left = cactus->original.left - cactus->d;
	r->world_dest.right= cactus->original.right + cactus->d;
}

static void obj_cactus_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjCactus* cactus = (ObjCactus*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	cactus->d = 0.0f;
	cactus->m = 0.0f;

	cactus->damage = 1.0f;

	RectF collider = {
		pos.x  + 30.0f, 		pos.y - height + 20.0f,
		pos.x  + width - 30.0f, pos.y
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = obj_cactus_collide;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x, pos.y - height,
			pos.x + width, pos.y
	);

	cactus->original = render->world_dest;

	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_cactus_update_pos;
}

GameObjectDesc obj_cactus_desc = {
	.type = OBJ_CACTUS_TYPE,
	.size = sizeof(ObjCactus),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_cactus_construct,
	.destruct = NULL
};

