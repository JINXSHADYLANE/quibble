#include "common.h"
#include "obj_types.h"
#include "minimap.h"
#include "game.h"
#include "hud.h"
#include <mfx.h>
#include <memory.h>

#include <system.h>
#include <async.h>

const static float rabbit_hitbox_width = 70.0f;
const static float rabbit_hitbox_height = 62.0f;

extern bool draw_ai_debug;
extern bool camera_follow;

void obj_rabbit_player_control(GameObject* self){
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;

	d->virtual_key_up = key_up(KEY_A);
	d->virtual_key_down = key_down(KEY_A);
	d->virtual_key_pressed = key_pressed(KEY_A);
}

void obj_rabbit_ai_control(GameObject* self){
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;
	PhysicsComponent* p = self->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);

	Vector2 start;
	Vector2 end;
	GameObject* obj;

	// key_down and key_up only last one frame
	if(d->virtual_key_down) d->virtual_key_down = false;
	else if(d->virtual_key_up){
		d->virtual_key_up = false;
		d->virtual_key_down = false;
		d->virtual_key_pressed = false;
	}	 
	
	// release button on contact with ground or shroom
	if( (d->jump_off_mushroom || d->touching_ground) &&
		d->virtual_key_pressed && !d->virtual_key_down ){

		d->virtual_key_pressed = false;
		d->virtual_key_up = true;
	}

	if(d->touching_ground){

		// AI avoids walking on water
		if(d->on_water){
			d->virtual_key_down = true;
			d->virtual_key_pressed = true;
		}

		// raycast for shroom in front
		start = vec2(pos.x+p->vel.x * 0.8f,pos.y - 100.0f);
		end = vec2(start.x,pos.y);
		obj = objects_raycast(start,end);

		// debug render
		if(draw_ai_debug){
			RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
			RectF r = objects_world2screen(rec,0);
			Vector2 s = vec2(r.left, r.top);
			Vector2 e = vec2(r.right, r.bottom);
			video_draw_line(10,	&s, &e, COLOR_RGBA(0, 0, 255, 255));
		}

		// AI jumps before shrooms to bounce on them
		if(obj){
			if(obj->type == OBJ_MUSHROOM_TYPE){
				ObjMushroom* mushroom = (ObjMushroom*)obj;
				if(mushroom->damage == 0.0f){
					//printf("jumping before shroom with vel.x: %f\n", p->vel.x);
					d->virtual_key_down = true;
					d->virtual_key_pressed = true;
				}
			}	
		} 

		// raycast for ground
		start = vec2(pos.x + 80.0f,768+100);
		end = vec2(start.x,768-100);
		obj = objects_raycast(start,end);

		// debug render
		if(draw_ai_debug){
			RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
			RectF r = objects_world2screen(rec,0);
			Vector2 s = vec2(r.left, r.top);
			Vector2 e = vec2(r.right, r.bottom);
			video_draw_line(10,	&s, &e, COLOR_RGBA(255, 0, 0, 255));
		}

		// AI avoids gaps
		if(obj){
			if(obj->type == OBJ_FALL_TRIGGER_TYPE){
					d->virtual_key_down = true;
					d->virtual_key_pressed = true;
			}	
		} 

	} else if(!d->touching_ground && !d->is_diving){
		// release button when in the air
		if(d->virtual_key_pressed){
			d->virtual_key_pressed = false;
			d->virtual_key_up = true;
		}

		// raycast for safe landing
		start = vec2(pos.x + 80.0f,768.0f - 100.0f);
		if(p->vel.x < 2800.0f)
			start.x += (579.0f-pos.y) * p->vel.x / 1500.0f;
		else 
			start.x += (579.0f-pos.y) * p->vel.x / 1200.0f;
		end = vec2(start.x-80.0f,start.y);
		obj = objects_raycast(start,end);

		//debug render
		if(draw_ai_debug){
			RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
			RectF r = objects_world2screen(rec,0);
			Vector2 s = vec2(r.left, r.top);
			Vector2 e = vec2(r.right, r.bottom);
			video_draw_line(10,	&s, &e, COLOR_RGBA(0, 255, 0, 255));
		}

		bool safe_to_land = true;
		if(obj){
			if(obj->type == OBJ_FALL_TRIGGER_TYPE){
				safe_to_land = false;
			}	
		}

		// raycast for gap while in air
		start = vec2(pos.x + 80.0f + p->vel.x/2.6f ,768.0f + 100.0f);
		if(p->vel.x < 2800.0f)
			start.x += (579.0f-pos.y) * p->vel.x / 1550.0f;
		else 
			start.x += (579.0f-pos.y) * p->vel.x / 1200.0f;

		end = vec2(start.x,768-100);
		obj = objects_raycast(start,end);

		// debug render
		if(draw_ai_debug){
			RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
			RectF r = objects_world2screen(rec,0);
			Vector2 s = vec2(r.left, r.top);
			Vector2 e = vec2(r.right, r.bottom);
			video_draw_line(10,	&s, &e, COLOR_RGBA(255, 0, 0, 255));
		}

		// AI dives before gaps, so it can jump over them
		if(safe_to_land && obj){
			if(obj->type == OBJ_FALL_TRIGGER_TYPE && !d->is_diving && (start.x - pos.x) > (579.0f - pos.y)) {
				d->virtual_key_down = true;
				d->virtual_key_pressed = true;
				//printf("dive before gap, vel.x: %f \n", p->vel.x);
			}	
		}

		// raycast below rabbit
		start = vec2(pos.x + 80.0f,pos.y+rabbit_hitbox_height+40);
		if(p->vel.x < 2800.0f)
			start.x += (579.0f-pos.y) * p->vel.x / 1500.0f;
		else 
			start.x += (579.0f-pos.y) * p->vel.x / 1200.0f;

		end = vec2(start.x,768);

		if(draw_ai_debug){
			RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
			RectF r = objects_world2screen(rec,0);
			Vector2 s = vec2(r.left, r.top);
			Vector2 e = vec2(r.right, r.bottom);
			video_draw_line(10,	&s, &e, COLOR_RGBA(0, 0, 255, 255));
		}

		obj = objects_raycast(start,end);
		// dive on shrooms if possible
		if(obj){
			if(obj->type == OBJ_MUSHROOM_TYPE){
				ObjMushroom* mushroom = (ObjMushroom*)obj;
				if(mushroom->damage == 0.0f){
					//printf("diving on shroom with vel.x: %f\n", p->vel.x);
					d->virtual_key_down = true;
					d->virtual_key_pressed = true;
				}
			}
		} 
	}
}

static void obj_rabbit_update(GameObject* self, float ts, float dt) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;
	if(!d->is_dead){
		PhysicsComponent* p = self->physics;
	
		if(camera_follow) rabbit->control(self);

		if(d->virtual_key_down)
			d->last_keypress_t = ts;
		if(d->virtual_key_up)
			d->last_keyrelease_t = ts;

		Vector2 dir = {.x = 0.0f, .y = 0.0f};
		// Constantly move right
		dir.x += 500.0f;


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
		
		// rubber band
		/*if(!d->player_control){
			float rx = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
			//printf("dif: %f \n",minimap_player_x() - rx);
			if(minimap_player_x() - rx > 0.0f){
				//float delta = minimap_player_x() - rx;
				//objects_apply_force(self, vec2(delta*5000.0f, 0.0f));
			}
		}*/


		if(d->touching_ground) {	
			d->is_diving = false;
			// Jump
			if(d->virtual_key_down){
				d->touching_ground = false;
				d->jump_off_mushroom = false;
				d->jump_time = ts;
				objects_apply_force(self, vec2(45000.0f, -160000.0f));
				anim_play(rabbit->anim, "jump");
				d->combo_counter = 0;
			
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, pos, NULL);
				mfx_trigger_follow("jump",&anchor->screen_pos,NULL);
			}
		}
		else {
			if(p->vel.y > 0.0f && !d->falling_down){
				anim_play(rabbit->anim, "down");
				d->falling_down = true;	
			} else if (p->vel.y <= 0.0f) {
				d->falling_down = false;		
			}


			if(ts - d->mushroom_hit_time < 0.1f) {
				if(fabsf(d->mushroom_hit_time - d->last_keypress_t) < 0.1f)
					d->jump_off_mushroom = true;

				if(fabsf(d->mushroom_hit_time - d->last_keyrelease_t) < 0.1f)
					d->jump_off_mushroom = true;
				
				if(!d->particle_spawn && d->jump_off_mushroom){
					d->particle_spawn = true;
					ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, pos, NULL);
					mfx_trigger_follow("jump",&anchor->screen_pos,NULL);
				}
			
			}
			else if(!d->touching_ground) {
				d->particle_spawn = false;
				if(key_pressed(KEY_A) && (ts - d->jump_time) < 0.2f) {
				//	objects_apply_force(self, vec2(0.0f, -8000.0f));
				}
				else if(!d->is_diving && d->virtual_key_down) {
					// Dive 	
					d->is_diving = true;
					objects_apply_force(self, vec2(0.0f, 20000.0f));
					anim_play(rabbit->anim, "dive");
				}
				else if(d->is_diving && d->virtual_key_pressed) {
					objects_apply_force(self, vec2(0.0f, 25000.0f));
				}
				else if(d->is_diving && !d->virtual_key_pressed) {
					d->is_diving = false;
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


		if(d->combo_counter >= 3 && d->boost == 0){
			mfx_trigger_ex("boost",vec2_add(screen_pos,vec2(20.0f,0.0f)),0.0f);
			p->vel.x *= 1.025;
			d->boost = 5;
		}
		if(d->boost > 0) d->boost--;


		if(!d->touching_ground) {
			// Apply gravity
			objects_apply_force(self, vec2(0.0f, 5000.0f));
		} else {
			d->combo_counter = 0;
			d->boost = 0;
			// Trigger water/land particle effects on ground
			if(d->on_water){
				if(r->anim_frame == 1) mfx_trigger_ex("water",screen_pos,0.0f);
				if(r->anim_frame == 11) mfx_trigger_ex("water_front",screen_pos,0.0f);			
			} else { 
				if(r->anim_frame == 1) mfx_trigger_ex("run1",screen_pos,0.0f);
				if(r->anim_frame == 11) mfx_trigger_ex("run1_front",screen_pos,0.0f);
			}
		}
		d->on_water = false;

	if(p->cd_obj->pos.y < rabbit_hitbox_height){
		p->cd_obj->pos.y = rabbit_hitbox_height;
		p->vel.y = 0.0f;
	}



	if(!d->game_over) d->rabbit_time += time_delta() / 1000.0f;
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
	PhysicsComponent* p = self->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	ObjRabbit* rabbit = (ObjRabbit*)self;
	if(pos.y > HEIGHT){
		rabbit->data->is_dead = true;
		p->vel.x = 0.0f;
		p->vel.y = 0.0f;
		if(!rabbit->data->game_over) rabbit->data->rabbit_time = -1.0f;
	}
}

static void _rabbit_delayed_bounce(void* r) {
	ObjRabbit* rabbit = r;
	ObjRabbitData* d = rabbit->data;
	if(d->jump_off_mushroom || d->is_diving) {
		d->is_diving = false;
		GameObject* self = r;
		objects_apply_force(self, d->bounce_force); 
		d->jump_off_mushroom = false;
		if(d->combo_counter++ > 1)
			if(d->player_control) hud_trigger_combo(d->combo_counter);
	}
	else
		d->combo_counter = 0;

	d->bounce_force = vec2(0.0f, 0.0f);
}

static void obj_rabbit_collide(GameObject* self, GameObject* other) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;
	// Collision with ground
	if(other->type == OBJ_GROUND_TYPE) {
		CDObj* cd_rabbit = self->physics->cd_obj;
		CDObj* cd_ground = other->physics->cd_obj;
		float rabbit_bottom = cd_rabbit->pos.y + cd_rabbit->size.size.y;
		float ground_top = cd_ground->pos.y;
		float penetration = (rabbit_bottom + cd_rabbit->offset.y) - ground_top;
		if(penetration > 0.0f && cd_rabbit->pos.y < cd_ground->pos.y) {
			self->physics->vel.y = 0.0f;
			if(!d->touching_ground) {
				if(d->player_control) hud_trigger_combo(0);
				anim_play(rabbit->anim, "land");
			}
			d->touching_ground = true;
			cd_rabbit->offset = vec2_add(
				cd_rabbit->offset, 
				vec2(0.0f, -penetration)
			);
		}
	}

	// Collision with mushroom
	Vector2 vel = self->physics->vel;
	if(other->type == OBJ_MUSHROOM_TYPE && !d->touching_ground && vel.y > 500.0f) {
		if(d->bounce_force.y == 0.0f) {
			ObjMushroom* mushroom = (ObjMushroom*)other;
			d->mushroom_hit_time = time_s();
			anim_play(rabbit->anim, "bounce");
			vel.y = -vel.y;

			if(mushroom->damage == 0.0f) {
				Vector2 f = {
					.x = MIN(vel.x*200.0f, 220000.0f),
					.y = MAX(vel.y*400.0f,-250000.0f)
				};
				d->bounce_force = f;
			}
			else {
				Vector2 f = {
					.x = MAX(-vel.x*200.0f, -80000.0f),
					.y = MAX(vel.y*300.0f,-180000.0f)
				};
				d->bounce_force = f;
				d->combo_counter = 0;
			}

			// Slow down vertical movevment
			self->physics->vel.y *= 0.2f;

			// Delay actual bouncing 0.1s
			async_schedule(_rabbit_delayed_bounce, 100, self);
		}
	}
}

static void obj_rabbit_construct(GameObject* self, Vector2 pos, void* user_data) {
	SprHandle spr_handle = (SprHandle)user_data;

	ObjRabbit* rabbit = (ObjRabbit*)self;
	rabbit->anim = anim_new("rabbit");
	
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
	render->spr = spr_handle;
	render->update_pos = obj_rabbit_update_pos;
	render->became_invisible = obj_rabbit_became_invisible;
	
	// Init update
	UpdateComponent* update = self->update;
	update->update = obj_rabbit_update;

	// additional rabbit data
	rabbit->data = MEM_ALLOC(sizeof(ObjRabbitData));
	ObjRabbitData*d = rabbit->data;

	d->combo_counter = 0;
	d->last_keypress_t = 0.0f;
	d->last_keyrelease_t = 0.0f;
	d->jump_time = 0.0f;
	d->mushroom_hit_time = 0.0f;

	d->virtual_key_up = false;
	d->virtual_key_down = false;
	d->virtual_key_pressed = false;

	d->touching_ground = false;
	d->jump_off_mushroom = false;
	d->is_diving = false;
	d->is_dead = false;
	d->on_water = false;
	d->bounce_force = vec2(0.0f, 0.0f);
	d->player_control = false;
	d->particle_spawn = false;
	d->falling_down = false;
	d->rabbit_time = 0.0f;
	d->game_over = false;
	d->boost = 0;

	if(spr_handle == sprsheet_get_handle("rabbit")){
		rabbit->control = obj_rabbit_player_control;
		d->player_control = true;
		d->rabbit_name = "You";
	} else {
		rabbit->control = obj_rabbit_ai_control;
		d->rabbit_name = "Name";
	}
}

static void obj_rabbit_destruct(GameObject* self) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	MEM_FREE(rabbit->data);
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

