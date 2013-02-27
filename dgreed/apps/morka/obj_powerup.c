#include "obj_types.h"
#include "common.h"
#include <system.h>
#include <mfx.h>

static void obj_powerup_rocket_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;		
		PhysicsComponent* p = self->physics;
		
		// Powerup effect
		objects_apply_force(other, vec2(d->xjump*d->xjump*30.0f, -d->yjump)); // PLACEHOLDER

		// Particles
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, p->cd_obj->pos, NULL);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		// Dissapearing animation
		SprHandle spr = self->render->spr;
		Vector2 size = sprsheet_get_size_h(spr);
		Vector2 pos = vec2_add(p->cd_obj->pos,vec2(size.x / 2.0f,size.y));

		ObjFloater* floater = (ObjFloater*) objects_create(&obj_floater_desc,pos, (void*)spr);
		sprintf(floater->text,"");
		floater->duration = 0.15f;

		// Destroy powerup
		objects_destroy(self);

	}
}

static void obj_powerup_coin_collide(GameObject* self, GameObject* other) {
	if(other->type == OBJ_RABBIT_TYPE) {
		ObjRabbit* rabbit = (ObjRabbit*)other;
		ObjRabbitData* d = rabbit->data;
		PhysicsComponent* p = self->physics;		

		// Powerup effect
		d->tokens += 1;

		// Particles
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);
		mfx_trigger_follow("coin_pick",&anchor->screen_pos,NULL);

		// Dissapearing animation
		SprHandle spr = self->render->spr;
		Vector2 size = sprsheet_get_size_h(spr);
		Vector2 pos = vec2_add(p->cd_obj->pos,vec2(size.x / 2.0f,size.y));

		ObjFloater* floater = (ObjFloater*) objects_create(&obj_floater_desc, pos, (void*)spr);
		sprintf(floater->text,"");
		floater->duration = 0.15f;

		// Destroy powerup
		objects_destroy(self);

	}
}

static void obj_powerup_construct(GameObject* self, Vector2 pos, void* user_data) {
	PowerupType* powerup = (PowerupType*)user_data;

	// Render
	RenderComponent* render = self->render;
	render->spr = sprsheet_get_handle(powerup->spr);
	Vector2 size = sprsheet_get_size_h(render->spr);
	float width = size.x;
	float height = size.y;
	render->world_dest = rectf(
			pos.x - width/2.0f, pos.y - height/2.0f,
			pos.x + width/2.0f, pos.y + height/2.0f
	);
	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;

	// Physics
	PhysicsComponent* physics = self->physics;
	RectF collider = {
		pos.x - width/2.0f, pos.y - height/2.0f,
		pos.x + width/2.0f, pos.y + height/2.0f
	};
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, 1, NULL);
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

PowerupType coin_powerup = {
	.spr = "token",
	.hit_callback = obj_powerup_coin_collide
};

PowerupType rocket_powerup = {
	.spr = "resque_tag", // PLACEHOLDER
	.hit_callback = obj_powerup_rocket_collide
};