#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <math.h>
#include <mfx.h>

static void obj_cactus_became_invisible(GameObject* self) {
	// empty
}

static void obj_cactus_collide(GameObject* self, GameObject* other) {
	ObjCactus* cactus = (ObjCactus*)self;
	if(other->type == OBJ_RABBIT_TYPE) {

		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		if(cactus->damage == 1.0f && d->rocket_time == 0.0f ){



			const float animation_length = 1.0f; // seconds

			cactus->t0 = time_s();
			cactus->t1 = cactus->t0 + animation_length;

			ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);

			if(!d->shield_up){
				Vector2 f = {
					.x = -100000.0f,
					.y =  0.0f
				};

				if(other->physics->vel.y > 0.0f){	
					f.y = -100000.0f;
					other->physics->vel.y = 0.0f;
				}

				objects_apply_force(other, f);
			} else {
				d->shield_up = false;
				mfx_trigger_follow("bubble_explode",&anchor->screen_pos,NULL);	
			}
			
			mfx_trigger_follow("cactus_reaction",&anchor->screen_pos,NULL);
			cactus->damage = 0.0f;

		}
	}
}

static void obj_cactus_update_pos(GameObject* self) {
	ObjCactus* cactus = (ObjCactus*)self;
	RenderComponent* r = self->render;

	float ct = time_s();
	float t = 0.0f;

	if(ct > cactus->t0 && ct < cactus->t1){

		t = (ct - cactus->t0) / (cactus->t1 - cactus->t0);

		cactus->d = sinf(sqrtf(t) * 2.0f * PI) * (1.0f - t) - t/2.0f;

		cactus->d *= 50.0f;	// offset
	}

	r->world_dest.top = cactus->original.top - cactus->d;
	r->world_dest.left = cactus->original.left - cactus->d;
	r->world_dest.right= cactus->original.right + cactus->d;

	if(cactus->damage == 0.0f){
		cactus->original.top += 1.0f;
		r->world_dest.bottom += 1.0f;
	} 

	if(r->world_dest.top > 641.0f)
		objects_destroy(self);

}

static void obj_cactus_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjCactus* cactus = (ObjCactus*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	cactus->damage = 1.0f;
	cactus->t0 = 0.0f;
	cactus->t1 = 1.0f;
	cactus->d = 0.0f;

	RectF collider = {
		pos.x  + 40.0f, 		pos.y - height + 30.0f,
		pos.x  + width - 40.0f, pos.y
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
	render->became_invisible = obj_cactus_became_invisible;
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