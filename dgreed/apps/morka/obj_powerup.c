#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <mfx.h>

// Bomb powerup

static ObjFloaterParams bomb_floater_params = {
	.spr = "bomb",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_bomb_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		//ObjRabbit* rabbit = (ObjRabbit*)other;
		//ObjRabbitData* d = rabbit->data;		
		
		// Powerup effect
		//objects_apply_force(other, vec2(d->xjump*d->xjump*30.0f, -d->yjump)); // PLACEHOLDER

		// Particles
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
			&obj_particle_anchor_desc, 
			rectf_center(&other->render->world_dest), 
			NULL
		);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		// Dissapearing animation
		Vector2 pos = rectf_center(&self->render->world_dest);
		objects_create(
			&obj_floater_desc, pos, 
			(void*)&bomb_floater_params
		);

		// Destroy powerup
		objects_destroy(self);
	}
}

PowerupParams bomb_powerup = {
	.spr = "bomb",
	.btn = "btn_bomb",
	.count = 0,
	.hit_callback = obj_powerup_bomb_collide
};

// Rocket powerup

static ObjFloaterParams rocket_floater_params = {
	.spr = "rocket",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_rocket_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		//ObjRabbit* rabbit = (ObjRabbit*)other;
		//ObjRabbitData* d = rabbit->data;		
		
		// Powerup effect
		//objects_apply_force(other, vec2(d->xjump*d->xjump*30.0f, -d->yjump)); // PLACEHOLDER

		// Particles
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
			&obj_particle_anchor_desc, 
			rectf_center(&other->render->world_dest), 
			NULL
		);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		// Dissapearing animation
		Vector2 pos = rectf_center(&self->render->world_dest);
		objects_create(
			&obj_floater_desc, pos, 
			(void*)&rocket_floater_params
		);

		// Destroy powerup
		objects_destroy(self);
	}
}

PowerupParams rocket_powerup = {
	.spr = "rocket",
	.btn = "btn_rocket",
	.count = 0,
	.hit_callback = obj_powerup_rocket_collide
};

// Shield powerup

static ObjFloaterParams shield_floater_params = {
	.spr = "shield",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_shield_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		//ObjRabbit* rabbit = (ObjRabbit*)other;
		//ObjRabbitData* d = rabbit->data;		
		
		// Powerup effect
		//objects_apply_force(other, vec2(d->xjump*d->xjump*30.0f, -d->yjump)); // PLACEHOLDER

		// Particles
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(
			&obj_particle_anchor_desc, 
			rectf_center(&other->render->world_dest), 
			NULL
		);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		// Dissapearing animation
		Vector2 pos = rectf_center(&self->render->world_dest);
		objects_create(
			&obj_floater_desc, pos, 
			(void*)&shield_floater_params
		);

		// Destroy powerup
		objects_destroy(self);
	}
}

PowerupParams shield_powerup = {
	.spr = "shield",
	.btn = "btn_shield",
	.count = 0,
	.hit_callback = obj_powerup_shield_collide
};

// Coin powerup

static ObjFloaterParams coin_floater_params = {
	.spr = "token",
	.text = NULL,
	.duration = 0.15f
};

static void obj_powerup_coin_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		// Powerup effect
		d->tokens += 1;

		// Particles
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		// Dissapearing animation
		Vector2 pos = rectf_center(&self->render->world_dest);
		objects_create(
			&obj_floater_desc, pos, 
			(void*)&coin_floater_params
		);

		// Destroy powerup
		objects_destroy(self);
	}
}

PowerupParams coin_powerup = {
	.spr = "token",
	.btn = NULL,
	.count = 0,
	.hit_callback = obj_powerup_coin_collide
};

// Generic ObjPowerup code

static void obj_powerup_construct(GameObject* self, Vector2 pos, void* user_data) {
	PowerupParams* powerup = (PowerupParams*)user_data;

	// Render
	RenderComponent* render = self->render;
	render->spr = sprsheet_get_handle(powerup->spr);
	Vector2 size = sprsheet_get_size_h(render->spr);
	float width = size.x;
	float height = size.y;
	render->world_dest = rectf_centered(pos, width, height);
	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &render->world_dest, 1, NULL);
	physics->inv_mass = 0.0f;
	physics->hit_callback = powerup->hit_callback;
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

