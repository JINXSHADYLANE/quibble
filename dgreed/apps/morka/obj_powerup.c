#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <mfx.h>

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

		// Powerup effect
		obj_powerup_trampoline_effect(other);

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

		// Destroy powerup
		objects_destroy(self);
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
}

static void obj_powerup_bomb_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;

		// Powerup effect
		d->has_powerup[BOMB] = true;

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

		// Destroy powerup
		objects_destroy(self);
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

		// Powerup effect
		d->has_powerup[ROCKET] = true;

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

		// Destroy powerup
		objects_destroy(self);
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
		// Powerup effect
		obj_powerup_shield_effect(other);

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

		// Destroy powerup
		objects_destroy(self);
	}
}

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
	render->became_invisible = obj_powerup_became_invisible;

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &render->world_dest, OBJ_POWERUP_TYPE, NULL);
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

PowerupParams powerup_params[] = {
	{
		"trampoline",
		"btn_trampoline",
		true,
		obj_powerup_trampoline_collide,
		obj_powerup_trampoline_effect,
		15
	},

	{
		"shield",
		"btn_shield",
		true,
		obj_powerup_shield_collide,
		obj_powerup_shield_effect,
		20
	},

	{
		"bomb",
		"btn_bomb",
		false,
		obj_powerup_bomb_collide,
		obj_powerup_bomb_effect,
		25
	},
	
	{
		"rocket",
		"btn_rocket",
		false,
		obj_powerup_rocket_collide,
		obj_powerup_rocket_effect,
		30
	},

};

// Token/coin powerup stored seperately because it has no activation button
PowerupParams coin_powerup = {
	.spr = "token",
	.btn = NULL,
	.hit_callback = obj_powerup_coin_collide,
	.powerup_callback = NULL,
	.cost = 0
};