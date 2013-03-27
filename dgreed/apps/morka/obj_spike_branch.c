#include "obj_types.h"
#include "common.h"
#include <mfx.h>
#include <system.h>

static void obj_spike_branch_became_invisible(GameObject* self) {
	// empty
}

static void obj_spike_branch_collide(GameObject* self, GameObject* other) {
	ObjSpikeBranch* spike = (ObjSpikeBranch*)self;
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;
		PhysicsComponent* rabbit_physics = other->physics;
		
		if(rabbit_physics->vel.y > 0.0f && !d->spike_hit) {
			spike->dh -= 5.0f;
			d->touching_ground = false;
			ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);

				if(!d->has_powerup[SHIELD]){
					Vector2 f = {
						.x = 0.0f,
						.y =  0.0f
					};

					if(other->physics->vel.y > 0.0f){	
						f.y = -100000.0f;
						other->physics->vel.y = 0.0f;
					}

					objects_apply_force(other, f);
				} else {
					d->has_powerup[SHIELD] = false;
					mfx_trigger_follow("bubble_explode",&anchor->screen_pos,NULL);	
				}
				
				mfx_trigger_follow("cactus_reaction",&anchor->screen_pos,NULL);
				//cactus->damage = 0.0f;

				rabbit_physics->vel.x = 0.0f;

				d->spike_hit = true;

		}

	}
}

static void obj_spike_branch_update_pos(GameObject* self) {
	ObjSpikeBranch* spike = (ObjSpikeBranch*)self;
	RenderComponent* r = self->render;

	float f = 100.0f * (spike->oh - spike->h);
	spike->dh += f * (time_delta()/1000.0f);
	spike->h += (spike->dh * 10.0f) * (time_delta()/1000.0f);
	spike->dh *= 0.9f;

	r->world_dest.top = r->world_dest.bottom - spike->h;
}

static void obj_spike_branch_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjBranch* spike = (ObjBranch*)self;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	spike->oh = height;
	spike->h = height;
	spike->dh = 0.0f;

	// Take 10 pixel slice at the top as a collider geometry
	RectF collider = {
		pos.x, 		pos.y - height + 20.0f,
		pos.x  + width, pos.y - height + 20.0f + 10.0f
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = obj_spike_branch_collide;

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x, pos.y - height,
			pos.x + width, pos.y
	);
	render->layer = branch_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_spike_branch_update_pos;
	render->became_invisible = obj_spike_branch_became_invisible;
}

GameObjectDesc obj_spike_branch_desc = {
	.type = OBJ_SPIKE_BRANCH_TYPE,
	.size = sizeof(ObjSpikeBranch),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_spike_branch_construct,
	.destruct = NULL
};

