#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <mfx.h>

extern ObjRabbit* player;
static SprHandle golden_rabbit = (SprHandle)MAX_UINT32;

static void obj_powerup_became_invisible(GameObject* self) {
	// empty
}

// Trampoline powerup

static ObjFloaterParams trampoline_floater_params = {
	.spr = "trampoline",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_trampoline_effect(GameObject* other){
	ObjRabbit* rabbit = (ObjRabbit*)other;
	ObjRabbitData* d = rabbit->data;
	d->has_powerup[TRAMPOLINE] = true;
}

static void obj_powerup_trampoline_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		if(!d->has_powerup[TRAMPOLINE]){
			// Powerup effect
			obj_powerup_trampoline_effect(other);

			RenderComponent* sr = self->render;
			if(sr->was_visible){
				// Particles
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
					&obj_particle_anchor_desc, 
					rectf_center(&other->render->world_dest), 
					NULL
				);
				mfx_trigger_follow("powerup_pick",&anchor->screen_pos,NULL);

				// Dissapearing animation
				Vector2 pos = rectf_center(&self->render->world_dest);
				objects_create(
					&obj_floater_desc, pos, 
					(void*)&trampoline_floater_params
				);
			}
			// Destroy powerup
			objects_destroy(self);
		}
	}
}

// Bomb powerup

static ObjFloaterParams bomb_floater_params = {
	.spr = "bomb",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_bomb_effect(GameObject* other){
	ObjRabbit* rabbit = (ObjRabbit*)other;
	ObjRabbitData* d = rabbit->data;
	PhysicsComponent* p = other->physics;	

	objects_create(&obj_bomb_desc, p->cd_obj->pos, (void*)other);

	d->has_powerup[BOMB] = false;

	if(rabbit->header.render->was_visible)
		mfx_trigger("throw");
}

static void obj_powerup_bomb_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		if(!d->has_powerup[BOMB]){
			// Powerup effect
			d->has_powerup[BOMB] = true;

			RenderComponent* sr = self->render;
			if(sr->was_visible){
				// Particles
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
					&obj_particle_anchor_desc, 
					rectf_center(&other->render->world_dest), 
					NULL
				);
				mfx_trigger_follow("powerup_pick",&anchor->screen_pos,NULL);

				// Dissapearing animation
				Vector2 pos = rectf_center(&self->render->world_dest);
				objects_create(
					&obj_floater_desc, pos, 
					(void*)&bomb_floater_params
				);
			}
			// Destroy powerup
			objects_destroy(self);
		}
	}
}

// Rocket powerup

static ObjFloaterParams rocket_floater_params = {
	.spr = "rocket",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_rocket_effect(GameObject* other){
	ObjRabbit* rabbit = (ObjRabbit*)other;
	ObjRabbitData* d = rabbit->data;
	d->rocket_start = true;
	d->has_powerup[ROCKET] = false;
}

static void obj_powerup_rocket_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		if(!d->has_powerup[ROCKET]){
			// Powerup effect
			d->has_powerup[ROCKET] = true;

			RenderComponent* sr = self->render;
			if(sr->was_visible){
				// Particles
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
					&obj_particle_anchor_desc, 
					rectf_center(&other->render->world_dest), 
					NULL
				);
				mfx_trigger_follow("powerup_pick",&anchor->screen_pos,NULL);

				// Dissapearing animation
				Vector2 pos = rectf_center(&self->render->world_dest);
				objects_create(
					&obj_floater_desc, pos, 
					(void*)&rocket_floater_params
				);
			}

			// Destroy powerup
			objects_destroy(self);
		}
	}
}

// Shield powerup

static ObjFloaterParams shield_floater_params = {
	.spr = "shield",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_shield_effect(GameObject* other){
	ObjRabbit* rabbit = (ObjRabbit*)other;
	ObjRabbitData* d = rabbit->data;

	d->has_powerup[SHIELD] = true;
}

static void obj_powerup_shield_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		if(!d->has_powerup[SHIELD]){	
			// Powerup effect
			obj_powerup_shield_effect(other);

			RenderComponent* sr = self->render;
			if(sr->was_visible){
				// Particles
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
					&obj_particle_anchor_desc, 
					rectf_center(&other->render->world_dest), 
					NULL
				);
				mfx_trigger_follow("powerup_pick",&anchor->screen_pos,NULL);

				// Dissapearing animation
				Vector2 pos = rectf_center(&self->render->world_dest);
				objects_create(
					&obj_floater_desc, pos, 
					(void*)&shield_floater_params
				);
			}
			// Destroy powerup
			objects_destroy(self);
		}
	}
}

// Coin powerup

static ObjFloaterParams coin_floater_params = {
	.spr = "token",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_coin_destroy(GameObject* self, GameObject* other){
		ObjPowerup* coin = (ObjPowerup*)self;	
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		// Powerup effect
		if(!d->game_over)
			d->tokens += 1;

		RenderComponent* sr = self->render;
		if(sr->was_visible){
			// Particles
			ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);
			mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

			if(coin->prev_r == 0.0f){
				// Dissapearing animation
				Vector2 pos = rectf_center(&self->render->world_dest);
				objects_create(
					&obj_floater_desc, pos, 
					(void*)&coin_floater_params
				);
			}

		}
		// Destroy powerup
		objects_destroy(self);
}

static void obj_powerup_coin_update_pos(GameObject* self){

	if(player->header.render->spr == golden_rabbit){

		ObjPowerup* coin = (ObjPowerup*)self;	

		RenderComponent* render = self->render;
		PhysicsComponent* p = self->physics;
		PhysicsComponent* rp = player->header.physics;

		Vector2 size = p->cd_obj->size.size;

		Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
		pos = vec2_add(pos, vec2_scale(size,0.5f));

		Vector2 pos2 = vec2_add(rp->cd_obj->pos, rp->cd_obj->offset);
		pos2.x += rp->cd_obj->size.size.x;
		pos2.y += rp->cd_obj->size.size.y / 2.0f;

		Vector2 delta = vec2_sub(pos2,pos);

		float radius = 256.0f;

		float r = vec2_length(delta);

		if(r < radius){

			float f = 100000.0f / sqrtf(r);
			Vector2 force = vec2_scale(vec2_normalize(delta),f); 

			if(r > coin->prev_r && coin->prev_r > 0.0f){
				GameObject* other = (GameObject*)player;
				obj_powerup_coin_destroy(self,other);
			}

			coin->prev_r = r;

			objects_apply_force(self,force);

			float alpha = 1.0f - clamp(0.0f, 1.0f, (radius - r) / 256.0f);
			byte a = lrintf(255.0f * alpha);
			render->color = COLOR_RGBA(255, 255, 255, a);	
		}

		p->vel = vec2_scale(p->vel,0.9f);

		render->world_dest = rectf_centered(
			pos, size.x, size.y
		);

	}

}

static void obj_powerup_coin_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		obj_powerup_coin_destroy(self,other);
	}
}

// Generic ObjPowerup code

static void obj_powerup_construct(GameObject* self, Vector2 pos, void* user_data) {
	PowerupParams* powerup = (PowerupParams*)user_data;

	if(golden_rabbit == (SprHandle)MAX_UINT32)
		golden_rabbit = sprsheet_get_handle("golden_rabbit");

	// Render
	RenderComponent* render = self->render;
	render->spr = sprsheet_get_handle(powerup->spr);
	Vector2 size = sprsheet_get_size_h(render->spr);
	float width = size.x;
	float height = size.y;
	render->world_dest = rectf_centered(pos, width, height);
	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;
	render->became_invisible = obj_powerup_became_invisible;
	render->update_pos = powerup->update_pos;

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &render->world_dest, OBJ_POWERUP_TYPE, NULL);
	float mass = 1.0f;
	physics->inv_mass = 1.0f / mass;
	physics->hit_callback = powerup->hit_callback;

	ObjPowerup* pwr = (ObjPowerup*)self;

	pwr->prev_r = 0.0f;
}

GameObjectDesc obj_powerup_desc = {
	.type = OBJ_POWERUP_TYPE,
	.size = sizeof(ObjPowerup),
	.has_physics = true,
	.has_render = true,
	.has_update = false,
	.construct = obj_powerup_construct,
	.destruct = NULL
};

PowerupParams powerup_params[] = {
	{
		"Trampoline",
		"Covers a pit",
		"trampoline",
		"btn_trampoline",
		"trampoline_unlock",
		true,
		obj_powerup_trampoline_collide,
		obj_powerup_trampoline_effect,
		NULL,
		15
	},

	{
		"Shield",
		"Protects from spikes and bombs",
		"shield",
		"btn_shield",
		"shield_unlock",
		true,
		obj_powerup_shield_collide,
		obj_powerup_shield_effect,
		NULL,
		20
	},

	{
		"Bomb",
		"Blows other racers back",
		"bomb",
		"btn_bomb",
		"bomb_unlock",
		false,
		obj_powerup_bomb_collide,
		obj_powerup_bomb_effect,
		NULL,
		25
	},
	
	{
		"Rocket",
		"Takes you for a wild ride",
		"rocket",
		"btn_rocket",
		"rocket_unlock",
		false,
		obj_powerup_rocket_collide,
		obj_powerup_rocket_effect,
		NULL,
		30
	},

};

// Token/coin powerup stored seperately because it has no activation button
PowerupParams coin_powerup = {
	.name = NULL,
	.description = NULL,
	.spr = "token",
	.btn = NULL,
	.unlocked_spr = NULL,
	.hit_callback = obj_powerup_coin_collide,
	.powerup_callback = NULL,
	.update_pos = obj_powerup_coin_update_pos,
	.cost = 0
};
