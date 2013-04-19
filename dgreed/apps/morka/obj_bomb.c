#include "obj_types.h"
#include "common.h"
#include "minimap.h"
#include <system.h>
#include <math.h>
#include <mfx.h>

static void obj_bomb_became_invisible(GameObject* self) {
	// empty
}

static void obj_bomb_detonate(GameObject* self){
	PhysicsComponent* p = self->physics;
	RenderComponent* sr = self->render;
	// Explosion particles
	if(sr->was_visible){
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);
		mfx_trigger_follow("bomb_explode",&anchor->screen_pos,NULL);
	}

	float r = 200.0f;

	Vector2 range = vec2(r,r);

	// Apply explosion force to rabbits based on distance
	for(int i = 0; i < minimap_get_count();i++){
		ObjRabbit* rabbit = minimap_get_rabbit(i);
		Vector2 delta = vec2_sub(rabbit->header.physics->cd_obj->pos, p->cd_obj->pos);

		if(fabsf(delta.x) <= range.x && fabsf(delta.y) <= range.y && !rabbit->data->jump_out){
			if(rabbit->data->has_powerup[SHIELD]){
				rabbit->data->has_powerup[SHIELD] = false;

				RenderComponent* rr = rabbit->header.render;
				if(rr->was_visible){
					ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, self->physics->cd_obj->pos, NULL);	
					mfx_trigger_follow("bubble_explode",&anchor->screen_pos,NULL);	
				}	

			} else {	
				if(delta.x < 0.0f) range.x *= -1.0f;
				if(delta.y < 0.0f) range.y *= -1.0f;

				delta = vec2_sub(range,delta);
				delta = vec2_scale(delta,1000.0f);

				GameObject* other = (GameObject*) rabbit;
				objects_apply_force(other, vec2(delta.x, delta.y));
			}
		}

	}

	objects_destroy(self);	
}

static void obj_bomb_collide(GameObject* self, GameObject* other) {
	ObjBomb* bomb = (ObjBomb*)self;

	if(other->type == OBJ_GROUND_TYPE){
		Vector2 vel = self->physics->vel;
		if(vel.y > 0.0f){
			self->physics->vel.x = 0.0f;
			self->physics->vel.y = -1000.0f;
		}
	} else if(other->type == OBJ_RABBIT_TYPE && other != bomb->owner) {
		obj_bomb_detonate(self);
	}

}

static void obj_bomb_update_pos(GameObject* self) {
	// Update render data
	RenderComponent* r = self->render;
	PhysicsComponent* p = self->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos = vec2_add(pos, vec2(19.0f,18.5f));
	r->world_dest = rectf_centered(
		pos, rectf_width(&r->world_dest), rectf_height(&r->world_dest)
	);
}

static void obj_bomb_update(GameObject* self, float ts, float dt) {
	PhysicsComponent* p = self->physics;

	// gravity
	objects_apply_force(self,  vec2(0.0f, 6000.0f) );	

	float t = clamp(0.0f, 1.0f, (p->vel.x - 220.0f) / 1000.0f);
	float damp = smoothstep(1.0f, 0.98f, t);
	p->vel = vec2(p->vel.x * damp, p->vel.y * 0.995f);

}

static void obj_bomb_construct(GameObject* self, Vector2 pos, void* user_data) {
	ObjBomb* bomb = (ObjBomb*)self;	
	SprHandle spr_handle = sprsheet_get_handle("bomb_obj");

	bomb->owner = (GameObject*)user_data;
	PhysicsComponent* p = bomb->owner->physics;

	Vector2 size = sprsheet_get_size_h(spr_handle);
	float width = size.x;
	float height = size.y;

	RectF collider = {
		pos.x, 		pos.y - height,
		pos.x  + width, pos.y
	};

	// Physics
	PhysicsComponent* physics = self->physics;
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &collider, OBJ_BOMB_TYPE, NULL);
	float mass = 3.0f;
	physics->inv_mass = 1.0f / mass;
	physics->vel = p->vel;
	physics->hit_callback = obj_bomb_collide;

	objects_apply_force(self, vec2(0.0f, -80000.0f) );	

	// Render
	RenderComponent* render = self->render;
	render->world_dest = rectf(
			pos.x, pos.y - height,
			pos.x + width, pos.y
	);

	render->layer = foreground_layer;
	render->anim_frame = MAX_UINT16;
	render->spr = spr_handle;
	render->update_pos = obj_bomb_update_pos;
	render->became_invisible = obj_bomb_became_invisible;

	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_bomb_update;	
}

GameObjectDesc obj_bomb_desc = {
	.type = OBJ_BOMB_TYPE,
	.size = sizeof(ObjBomb),
	.has_physics = true,
	.has_render = true,
	.has_update = true,
	.construct = obj_bomb_construct,
	.destruct = NULL
};