#include "obj_types.h"
#include "hud.h"
#include <mfx.h>

#include <system.h>
#include <async.h>

static uint combo_counter = 0;
static float last_keypress_t = 0.0f;
static float last_keyrelease_t = 0.0f;
static float jump_time = 0.0f;
static float mushroom_hit_time = 0.0f;

const static float rabbit_hitbox_width = 70.0f;
const static float rabbit_hitbox_height = 62.0f;

static void obj_rabbit_update(GameObject* self, float ts, float dt) {
	static bool jump_particles = false;
	ObjRabbit* rabbit = (ObjRabbit*)self;
	PhysicsComponent* p = self->physics;
	if(key_down(KEY_A))
		last_keypress_t = ts;
	if(key_up(KEY_A))
		last_keyrelease_t = ts;

	Vector2 dir = {.x = 0.0f, .y = 0.0f};

	// Constantly move right
	dir.x += 550.0f;
	
	// Position for particles
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);	// for follower particles
	pos.y += rabbit_hitbox_height - 20;
	
	RectF rec = {
		.left = pos.x, 
		.top = pos.y,
		.right = 0,
		.bottom = 0
	};
	RectF result = objects_world2screen(rec,0);
	Vector2 screen_pos = vec2(result.left,result.top);			// for standard particles

	RenderComponent* r = self->render;
	r->anim_frame = anim_frame(rabbit->anim);
		
	// Jump
	if(rabbit->touching_ground && key_down(KEY_A)) {
		rabbit->touching_ground = false;
		rabbit->jump_off_mushroom = false;
		jump_time = ts;
		objects_apply_force(self, vec2(50000.0f, -160000.0f));
		anim_play(rabbit->anim, "jump");
		combo_counter = 0;
		
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, pos, NULL);
		mfx_trigger_follow("jump",&anchor->screen_pos,NULL);
		
	}
	else {
		if(ts - mushroom_hit_time < 0.1f) {
			if(fabsf(mushroom_hit_time - last_keypress_t) < 0.1f)
				rabbit->jump_off_mushroom = true;

			if(fabsf(mushroom_hit_time - last_keyrelease_t) < 0.1f)
				rabbit->jump_off_mushroom = true;
				
			if(!jump_particles && rabbit->jump_off_mushroom){
				jump_particles = true;
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, pos, NULL);
				mfx_trigger_follow("jump",&anchor->screen_pos,NULL);
			}
			
		}
		else if(!rabbit->touching_ground) {
			jump_particles = false;
			if(key_pressed(KEY_A) && (ts - jump_time) < 0.2f) {
			//	objects_apply_force(self, vec2(0.0f, -8000.0f));
			}
			else if(!rabbit->is_diving && key_down(KEY_A)) {
				// Dive 	
				rabbit->is_diving = true;
				objects_apply_force(self, vec2(0.0f, 20000.0f));
				anim_play(rabbit->anim, "dive");
			}
			else if(rabbit->is_diving && key_pressed(KEY_A)) {
				objects_apply_force(self, vec2(0.0f, 25000.0f));
			}
			else if(rabbit->is_diving && !key_pressed(KEY_A)) {
				rabbit->is_diving = false;
				anim_play(rabbit->anim, "glide");
			}
		}
	}	
	
	// Damping
	if(p->vel.x < 300.0f) {
	}	
	else if(p->vel.x < 600.0f)
		p->vel.x *= 0.995f;
	else if(p->vel.x < 1000.0f)
		p->vel.x *= 0.99f;
	else
		p->vel.x *= 0.985f;
//	else if(p->vel.x < 700.0f)
//		p->vel.x *= 0.96f;
//	else if(p->vel.x < 1000.0f)
//		p->vel.x *= 0.95f;
		
	p->vel.y *= 0.995f;
	
	objects_apply_force(self, dir);

	if(!rabbit->touching_ground) {
		// Apply gravity
		objects_apply_force(self, vec2(0.0f, 5000.0f));
	} else {
		// Trigger water/land particle effects on ground
		if(rabbit->on_water){
			if(r->anim_frame == 1) mfx_trigger_ex("water",screen_pos,0.0f);
			if(r->anim_frame == 11) mfx_trigger_ex("water_front",screen_pos,0.0f);
			rabbit->on_water = false;			
		} else { 
			if(r->anim_frame == 1)mfx_trigger_ex("run1",screen_pos,0.0f);
			if(r->anim_frame == 11) mfx_trigger_ex("run1_front",screen_pos,0.0f);
			
		}
	}
}

static void obj_rabbit_update_pos(GameObject* self) {
	// Update render data
	RenderComponent* r = self->render;
	PhysicsComponent* p = self->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos = vec2_add(pos, vec2(rabbit_hitbox_width / 2.0f, rabbit_hitbox_height / 2.0f));
	float half_w = rectf_width(&r->world_dest) / 2.0f;
	float half_h = rectf_height(&r->world_dest) / 2.0f;
	r->world_dest = rectf(
			pos.x - half_w, pos.y - half_h, pos.x + half_w, pos.y + half_h
	);

	ObjRabbit* rabbit = (ObjRabbit*)self;
	r->anim_frame = anim_frame(rabbit->anim);
}

static void obj_rabbit_became_invisible(GameObject* self) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	rabbit->is_dead = true;
}

static void _rabbit_delayed_bounce(void* r) {
	ObjRabbit* rabbit = r;

	if(rabbit->jump_off_mushroom || rabbit->is_diving) {
		rabbit->is_diving = false;
		GameObject* self = r;
		objects_apply_force(self, rabbit->bounce_force); 
		rabbit->jump_off_mushroom = false;
		if(combo_counter++ > 1)
			hud_trigger_combo(combo_counter);
	}
	else
		combo_counter = 0;

	rabbit->bounce_force = vec2(0.0f, 0.0f);
}

static void obj_rabbit_collide(GameObject* self, GameObject* other) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	// Collision with ground
	if(other->type == OBJ_GROUND_TYPE) {
		CDObj* cd_rabbit = self->physics->cd_obj;
		CDObj* cd_ground = other->physics->cd_obj;
		float rabbit_bottom = cd_rabbit->pos.y + cd_rabbit->size.size.y;
		float ground_top = cd_ground->pos.y;
		float penetration = (rabbit_bottom + cd_rabbit->offset.y) - ground_top;
		if(penetration > 0.0f && cd_rabbit->pos.y < cd_ground->pos.y) {
			self->physics->vel.y = 0.0f;
			if(!rabbit->touching_ground) {
				hud_trigger_combo(0);
				anim_play(rabbit->anim, "land");
			}
			rabbit->touching_ground = true;
			cd_rabbit->offset = vec2_add(
				cd_rabbit->offset, 
				vec2(0.0f, -penetration)
			);
		}
	}

	// Collision with mushroom
	Vector2 vel = self->physics->vel;
	if(other->type == OBJ_MUSHROOM_TYPE && !rabbit->touching_ground && vel.y > 500.0f) {
		if(rabbit->bounce_force.y == 0.0f) {
			ObjMushroom* mushroom = (ObjMushroom*)other;
			mushroom_hit_time = time_s();
			anim_play(rabbit->anim, "bounce");
			vel.y = -vel.y;

			if(mushroom->damage == 0.0f) {
				Vector2 f = {
					.x = MIN(vel.x*200.0f, 220000.0f),
					.y = MAX(vel.y*400.0f,-250000.0f)
				};
				rabbit->bounce_force = f;
			}
			else {
				Vector2 f = {
					.x = MAX(-vel.x*200.0f, -80000.0f),
					.y = MAX(vel.y*300.0f,-180000.0f)
				};
				rabbit->bounce_force = f;
				combo_counter = 0;
			}

			// Slow down vertical movevment
			self->physics->vel.y *= 0.2f;

			// Delay actual bouncing 0.1s
			async_schedule(_rabbit_delayed_bounce, 100, self);
		}
	}
}

static void obj_rabbit_construct(GameObject* self, Vector2 pos, void* user_data) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	rabbit->touching_ground = false;
	rabbit->jump_off_mushroom = false;
	rabbit->is_diving = false;
	rabbit->is_dead = false;
	rabbit->anim = anim_new("rabbit");
	rabbit->bounce_force = vec2(0.0f, 0.0f);

	// Init physics
	PhysicsComponent* physics = self->physics;
	float hw = rabbit_hitbox_width / 2.0f;
	float hh = rabbit_hitbox_height / 2.0f;
	RectF rect = {
		pos.x - hw, pos.y - hh,
		pos.x + hw, pos.y + hh 
	};
	physics->cd_obj = coldet_new_aabb(objects_cdworld, &rect, 1, NULL);
	float mass = 3.0f;
	physics->inv_mass = 1.0f / mass;
	physics->vel = vec2(0.0f, 0.0f);
	physics->hit_callback = obj_rabbit_collide;

	// Init render
	TexHandle h;
	RectF src;
	sprsheet_get_anim("rabbit", 0, &h, &src);
	RenderComponent* render = self->render;
	float half_w = rectf_width(&src) / 2.0f;
	float half_h = rectf_height(&src) / 2.0f;
	render->world_dest = rectf(
		pos.x - half_w, pos.y - half_h,
		pos.x + half_w, pos.y + half_h 
	);
	render->angle = 0.0f;
	render->layer = 4;
	render->anim_frame = 0;
	render->spr = sprsheet_get_handle("rabbit");
	render->update_pos = obj_rabbit_update_pos;
	render->became_invisible = obj_rabbit_became_invisible;
	
	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_rabbit_update;
}

static void obj_rabbit_destruct(GameObject* self) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	anim_del(rabbit->anim);
}

GameObjectDesc obj_rabbit_desc = {
	.type = OBJ_RABBIT_TYPE,
	.size = sizeof(ObjRabbit),
	.has_physics = true,
	.has_render = true,
	.has_update = true,
	.construct = obj_rabbit_construct,
	.destruct = obj_rabbit_destruct
};

