#include "common.h"
#include "obj_types.h"
#include "levels.h"
#include "minimap.h"
#include "game.h"
#include "hud.h"
#include "tutorials.h"
#include <mfx.h>
#include <memory.h>

#include <system.h>
#include <async.h>

#define TIME_S (malka_state_time("game") + malka_state_time("game_over"))

const static float rabbit_hitbox_width = 70.0f;
const static float rabbit_hitbox_height = 62.0f;

extern bool draw_ai_debug;
extern bool camera_follow;
extern ObjRabbit* rabbit;

void obj_rabbit_player_control(GameObject* self){
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;

	if(!d->input_disabled){
		d->virtual_key_up = key_up(KEY_A);
		d->virtual_key_down = key_down(KEY_A);
		d->virtual_key_pressed = key_pressed(KEY_A);
	}

	if(tutorials_are_enabled()){

		ObjRabbit* rabbit = (ObjRabbit*)self;
		ObjRabbitData* d = rabbit->data;
		PhysicsComponent* p = self->physics;
		Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);

		Vector2 start;
		Vector2 end;
		GameObject* obj;

		if(d->touching_ground){
			// raycast for shroom in front
			start = vec2(pos.x+p->vel.x * 0.6f,pos.y - 100.0f);
			end = vec2(start.x,pos.y);
			obj = objects_raycast(start,end);

			// debug render
			if(draw_ai_debug){
				RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
				RectF r = objects_world2screen(rec,0);
				Vector2 s = vec2(r.left, r.top);
				Vector2 e = vec2(r.right, r.bottom);
				video_draw_line(10,	&s, &e, COLOR_RGBA(255, 255, 255, 255));
			}

			// tutorial event on mushroom in front
			if(obj){
				if(obj->type == OBJ_MUSHROOM_TYPE){
					tutorial_event(MUSHROOM_IN_FRONT);
				}	
			}

		} else {

			// raycast below rabbit for shrooms
			start = vec2(pos.x + (p->vel.x * 0.23f) *(579.0f-pos.y)/579.0f,pos.y+rabbit_hitbox_height+30);
			//start = vec2(pos.x + 100.0f +(( p->vel.x) * (p->vel.x/6000.0f)) *(579.0f-pos.y)/579.0f,pos.y+rabbit_hitbox_height+20);
			end = vec2(start.x,768);

			if(draw_ai_debug){
				RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
				RectF r = objects_world2screen(rec,0);
				Vector2 s = vec2(r.left, r.top);
				Vector2 e = vec2(r.right, r.bottom);
				video_draw_line(10,	&s, &e, COLOR_RGBA(255, 255, 255, 255));
			}

			obj = objects_raycast(start,end);

			// tutorial event when mushroom below rabbit
			if(obj){
				if(obj->type == OBJ_MUSHROOM_TYPE){
					tutorial_event(MUSHROOM_BELOW);
				}
			}

		} 

	}
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
		start = vec2(pos.x+p->vel.x * 0.6f,pos.y - 100.0f);
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
				d->virtual_key_down = true;
				d->virtual_key_pressed = true;
			}	
		} 

		// raycast for gap ahead
		start = vec2(pos.x + 80.0f + (p->vel.x * 0.1f),768+100);
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
			if(obj->type == OBJ_FALL_TRIGGER_TYPE || obj->type == OBJ_TRAMPOLINE_TYPE){
				//printf("jump before gap \n");
				d->virtual_key_down = true;
				d->virtual_key_pressed = true;
			}	
		} 

		//raycast for cactus ahead
		start = vec2(pos.x + 80.0f + (p->vel.x * 0.3f),pos.y - 100.0f);
		end = vec2(start.x,pos.y);
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
			if(obj->type == OBJ_CACTUS_TYPE){
				d->virtual_key_down = true;
				d->virtual_key_pressed = true;
			}	
		} 

	} else if(!d->touching_ground){

		// raycast for safe landing
		start = vec2(pos.x + 100.0f + (p->vel.x * 0.4f) *(579.0f-pos.y)/579.0f,768.0f - 100.0f);
		end = vec2(start.x-100.0f,start.y);
		obj = objects_raycast(start,end);

		bool safe_to_land = true;
		if(obj){
			if(obj->type == OBJ_FALL_TRIGGER_TYPE || obj->type == OBJ_TRAMPOLINE_TYPE){
				safe_to_land = false;
			}	
		}

		//debug render
		if(draw_ai_debug){
			RectF rec = {.left = start.x,.top = start.y,.right = end.x,.bottom = end.y};
			RectF r = objects_world2screen(rec,0);
			Vector2 s = vec2(r.left, r.top);
			Vector2 e = vec2(r.right, r.bottom);
			video_draw_line(10,	&s, &e, safe_to_land ? COLOR_RGBA(0, 255, 0, 255) : COLOR_RGBA(255, 0, 0, 255));
		}		


		if(!d->is_diving){
			// release button when in the air
			if(d->virtual_key_pressed){
				d->virtual_key_pressed = false;
				d->virtual_key_up = true;
			}

			// raycast landing zone for gap while in air
			// + (579.0f-pos.y) * p->vel.x * 0.0025f
			//d->xjump * d->xjump *(time_s() - prev_time)
			//printf("pos: %f vel: %f\n",pos.y,p->vel.y);
			start = vec2(d->land,768.0f + 100.0f);
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
				if((obj->type == OBJ_FALL_TRIGGER_TYPE || obj->type == OBJ_TRAMPOLINE_TYPE) && !d->is_diving && p->vel.y > 0.0f) {
					d->virtual_key_down = true;
					d->virtual_key_pressed = true;
					//printf("%d dive before gap, vel.x: %.0f dy %.0f distance: %f \n",self, p->vel.x,(579.0f - pos.y),obj->physics->cd_obj->pos.x - pos.x);
				}
				if(obj->type == OBJ_FALL_TRIGGER_TYPE || obj->type == OBJ_TRAMPOLINE_TYPE) safe_to_land = false;	
			}

			// raycast below rabbit for shrooms
			start = vec2(pos.x + 100.0f + (p->vel.x * 0.3f) *(579.0f-pos.y)/579.0f,pos.y+rabbit_hitbox_height+30);
			//start = vec2(pos.x + 100.0f +(( p->vel.x) * (p->vel.x/6000.0f)) *(579.0f-pos.y)/579.0f,pos.y+rabbit_hitbox_height+20);
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
			if(obj && safe_to_land && d->combo_counter < d->ai_max_combo){
				if(obj->type == OBJ_MUSHROOM_TYPE){
					//printf("%d diving on shroom with vel.x: %.0f dy %.0f distance: %f \n",self, p->vel.x,(579.0f - pos.y),obj->physics->cd_obj->pos.x - pos.x);
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
		// rubber band
		if(d->rubber_band){

			for(int i = 0; i < minimap_get_count();i++){

				ObjRabbit* other = minimap_get_rabbit(i);
				float delta = other->header.physics->cd_obj->pos.x - p->cd_obj->pos.x;

				if(other != rabbit && !other->data->player_control && !other->data->is_dead
				// && other->header.physics->cd_obj->pos.x < minimap_player_x()
					){

					float min_dist = 800.0f;
					float force = other->data->speed*other->data->speed;
					force *= 10.0f;

					if(delta > 0.0f && delta < min_dist) objects_apply_force(self, vec2(-force/delta, 0.0f));
				} else if(other->data->player_control) {
					delta = delta*2.0f + d->speed;
					objects_apply_force(self, vec2(delta, 0.0f));
				}
			}

			if(p->vel.x < 0.0f) p->vel.x = d->speed;

			p->cd_obj->pos.y = 370.0f;
			d->touching_ground = true;

			if(minimap_player_x() - p->cd_obj->pos.x < 0.0) d->rubber_band = false;
		}

		if(camera_follow && !d->rubber_band) rabbit->control(self);

		if(d->virtual_key_down)
			d->last_keypress_t = ts;
		if(d->virtual_key_up)
			d->last_keyrelease_t = ts;

		Vector2 dir = {.x = 0.0f, .y = 0.0f};
		// Constantly move right
		dir.x += d->speed;


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
		r->anim_frame = anim_frame_ex(rabbit->anim, TIME_S);
		
		if(d->touching_ground) {
			d->force_dive = false;
			if(d->has_trampoline) d->has_trampoline = false;	
			d->is_diving = false;
			// Jump
			if(d->virtual_key_down || d->force_jump){
				d->force_jump = false;
				d->touching_ground = false;
				d->jump_off_mushroom = false;
				d->jump_time = ts;
				objects_apply_force(self, vec2(d->xjump*d->xjump, -d->yjump*d->yjump));
				anim_play_ex(rabbit->anim, "jump", TIME_S);
				d->land = pos.x + p->vel.x - rabbit_hitbox_width / 2.0f;
				d->combo_counter = 0;
			
				ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, pos, NULL);
				if(r->was_visible)
					mfx_trigger_follow("jump",&anchor->screen_pos,NULL);

				//printf("%d mushroom hit\n",self);
			}
		}
		else {
			if(p->vel.y > 0.0f && !d->falling_down){
				anim_play_ex(rabbit->anim, "down", TIME_S);
				d->falling_down = true;

			} else if (p->vel.y <= 0.0f) {
				d->falling_down = false;		
			}
			// Trampoline
			if(p->cd_obj->pos.y > 579.0f && p->vel.y > 0.0f && d->tokens >=10 && !d->has_trampoline && !rabbit->data->game_over){
				d->has_trampoline = true;
				d->tokens -= 10;

				// Trampoline sprite
				SprHandle sprt = sprsheet_get_handle("trampoline");
				Vector2 size = sprsheet_get_size_h(sprt);
				float width = size.x;
				float height = size.y;

				// find the gap to draw trampoline in
				Vector2 start = vec2(p->cd_obj->pos.x + rabbit_hitbox_width,HEIGHT + 100.0f);
				Vector2 end = vec2(start.x,HEIGHT - 100.0f);
				GameObject* obj = objects_raycast(start,end);
				Vector2 gap_pos = vec2(0.0f,0.0f);
				Vector2 txt_pos = vec2(0.0f,0.0f);
				bool found = false;
				if(obj){
					if(obj->type == OBJ_FALL_TRIGGER_TYPE) {
						PhysicsComponent* p = obj->physics;
						gap_pos = vec2(p->cd_obj->pos.x + (p->cd_obj->size.size.x - width) / 2.0f,HEIGHT + 15.0f);
						txt_pos = vec2(p->cd_obj->pos.x + (p->cd_obj->size.size.x) / 2.0f,HEIGHT + 15.0f - height);
						found = true;
					} else if(obj->type == OBJ_TRAMPOLINE_TYPE) {
						RenderComponent* render = obj->render;
						gap_pos = vec2(render->world_dest.left,HEIGHT + 15.0f);
						txt_pos = vec2(render->world_dest.left + width / 2.0f,HEIGHT + 15.0f - height);
						found = true;						
					}
				}

				if(!found){
					start = vec2(p->cd_obj->pos.x - 50.0f,HEIGHT + 100.0f);
					end = vec2(start.x,HEIGHT - 100.0f);
					obj = objects_raycast(start,end);
					if(obj){
						if(obj->type == OBJ_FALL_TRIGGER_TYPE) {
							PhysicsComponent* p = obj->physics;
							gap_pos = vec2(p->cd_obj->pos.x + (p->cd_obj->size.size.x - width) / 2.0f,HEIGHT + 15.0f);
							txt_pos = vec2(p->cd_obj->pos.x + (p->cd_obj->size.size.x) / 2.0f,HEIGHT + 15.0f - height);
							found = true;
						} else if(obj->type == OBJ_TRAMPOLINE_TYPE) {
							RenderComponent* render = obj->render;
							gap_pos = vec2(render->world_dest.left,HEIGHT + 15.0f);
							txt_pos = vec2(render->world_dest.left + width / 2.0f,HEIGHT + 15.0f - height);
							found = true;						
						}
					}
				}

				// Create Trampoline
				ObjTrampoline* trampoline = (ObjTrampoline*) objects_create(&obj_trampoline_desc, gap_pos, (void*)sprt);
				trampoline->owner = self;


				sprt = sprsheet_get_handle("token_tag");

				// Create floating text
				ObjFloater* floater = (ObjFloater*) objects_create(&obj_floater_desc, txt_pos, (void*)sprt);
				sprintf(floater->text,"-10");
				floater->a = 0.5f;
			}

			if(ts - d->mushroom_hit_time < 0.1f) {

				if(fabsf(d->mushroom_hit_time - d->last_keypress_t) < 0.1f)
					d->jump_off_mushroom = true;

				if(fabsf(d->mushroom_hit_time - d->last_keyrelease_t) < 0.1f)
					d->jump_off_mushroom = true;
					
			}
			else if(!d->touching_ground) {
				if(key_pressed(KEY_A) && (ts - d->jump_time) < 0.2f) {
				//	objects_apply_force(self, vec2(0.0f, -8000.0f));
				}
				else if( (!d->is_diving && d->virtual_key_down) || d->force_dive ) {
					// Dive	
					d->is_diving = true;
					objects_apply_force(self, vec2(0.0f, 20000.0f));
					anim_play_ex(rabbit->anim, "dive", TIME_S);
				}
				else if(d->is_diving && d->virtual_key_pressed) {
					objects_apply_force(self, vec2(0.0f, 25000.0f));
				}
				else if(d->is_diving && !d->virtual_key_pressed) {
					d->is_diving = false;
					anim_play_ex(rabbit->anim, "glide", TIME_S);
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
			p->vel.x *= 0.980f;
	//	else if(p->vel.x < 700.0f)
	//		p->vel.x *= 0.96f;
	//	else if(p->vel.x < 1000.0f)
	//		p->vel.x *= 0.95f;
		
		p->vel.y *= 0.995f;
	
		objects_apply_force(self, dir);

		if(d->combo_counter >= 3 && d->boost == 0){
			if(r->was_visible)
				mfx_trigger_ex("boost",vec2_add(screen_pos,vec2(20.0f,0.0f)),0.0f);
			p->vel.x *= 1.045;
			d->boost = 5;
		}
		if(d->boost > 0) d->boost--;


		if(!d->touching_ground) {
			// Apply gravity
			objects_apply_force(self, vec2(0.0f, 6000.0f));
		} else {
			d->combo_counter = 0;
			d->boost = 0;
			// Trigger water/land particle effects on ground
			if(d->last_frame != r->anim_frame && r->was_visible) {
				const char* effect = NULL;
				if(d->on_water){
					if(r->anim_frame == 1)
						effect = "water";
					if(r->anim_frame == 11)
						effect = "water_front";
				}
				else { 
					if(r->anim_frame == 1)
						effect = "run1";
					if(r->anim_frame == 11)
						effect = "run1_front";
				}

				if(effect)
					mfx_trigger_ex(effect, screen_pos, 0.0f);
			}
			d->last_frame = r->anim_frame;
		}
		d->on_water = false;
	if(p->cd_obj->pos.y < rabbit_hitbox_height){
		p->cd_obj->pos.y = rabbit_hitbox_height;
		p->vel.y = 0.0f;
		d->touching_ground = false;
	}
	if(p->cd_obj->pos.y > HEIGHT){
		//printf("rabbit %d dead %.0f %.0f \n",self,p->vel.x,p->vel.y);
		rabbit->data->is_dead = true;
		p->vel.x = 0.0f;
		p->vel.y = 0.0f;
		if(!rabbit->data->game_over) rabbit->data->rabbit_time = -1.0f;
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
	r->anim_frame = anim_frame_ex(rabbit->anim, TIME_S);
}
static void obj_rabbit_became_visible(GameObject* self) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;
	PhysicsComponent* p = self->physics;
	if(!d->player_control && d->rubber_band){
		d->rubber_band = false;
		Vector2 pos = p->cd_obj->pos;

		// is it safe to appear on screen?
		Vector2 start = vec2(pos.x + 100.0f + (p->vel.x * 0.4f) *(579.0f-pos.y)/579.0f,768.0f - 100.0f);
		Vector2 end = vec2(start.x-100.0f,start.y);
		GameObject* obj = objects_raycast(start,end);

		bool safe_to_land = true;
		if(obj){
			if(obj->type == OBJ_FALL_TRIGGER_TYPE || obj->type == OBJ_TRAMPOLINE_TYPE){
				safe_to_land = false;
			}	
		}
		p->vel.y = 0.0f;
		d->touching_ground = false;
		if(safe_to_land){
			p->vel.x = rabbit->header.physics->vel.x;
			p->cd_obj->pos.y = 579.0f;
		} else {
			p->cd_obj->pos.y = rand_float_range(550.0f,579.0f);
			objects_apply_force(self, vec2(d->xjump*d->xjump, -d->yjump*d->yjump));
		}
	} 
}
static void obj_rabbit_became_invisible(GameObject* self) {
	ObjRabbit* rabbit = (ObjRabbit*)self;
	ObjRabbitData* d = rabbit->data;
	PhysicsComponent* p = self->physics;
	float delta = minimap_player_x() - p->cd_obj->pos.x;
	if(!d->is_dead && !d->player_control && delta > 0.0f) d->rubber_band = true;
}

static void _rabbit_delayed_bounce(void* r) {
	ObjRabbit* rabbit = r;
	ObjRabbitData* d = rabbit->data;
	PhysicsComponent* p = rabbit->header.physics;
	if(d->jump_off_mushroom || d->is_diving) {
		if(d->player_control) tutorial_event(BOUNCE_PERFORMED);
		d->force_dive = false;
		d->is_diving = false;
		GameObject* self = r;
		objects_apply_force(self, d->bounce_force); 
		d->jump_off_mushroom = false;

		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_particle_anchor_desc, p->cd_obj->pos, NULL);
		mfx_trigger_follow("jump",&anchor->screen_pos,NULL);

		if(d->combo_counter+1 == 3){
			if(d->player_control) tutorial_event(COMBO_X3);	

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

			RenderComponent* render = rabbit->header.render;
			if(render->was_visible)
				mfx_trigger_ex("boost_explosion",screen_pos,0.0f);
		} 

		//printf("pos.x: %f v: %f %f \n",p->cd_obj->pos.y,p->vel.x,p->vel.y);
		d->land = p->cd_obj->pos.x + (405.0f-p->vel.y) + p->vel.x + (p->vel.x) / (2.0f + p->vel.x/1000.0f);
		d->combo_counter++;
		 
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
				anim_play_ex(rabbit->anim, "land", TIME_S);
				/*printf("p: %f actual: %f  d: %f (%f %)\n",d->jump+self->physics->vel.x,
														 self->physics->cd_obj->pos.x,
														 d->jump + self->physics->vel.x - self->physics->cd_obj->pos.x,
														 self->physics->cd_obj->pos.x/(d->jump+self->physics->vel.x));*/
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
			//ObjMushroom* mushroom = (ObjMushroom*)other;
			d->mushroom_hit_time = time_s();
			anim_play_ex(rabbit->anim, "bounce", TIME_S);
			vel.y = -vel.y;

			Vector2 f = {
				.x = MIN(vel.x*d->xjump, 110000.0f),
				.y = MAX(vel.y*d->yjump,-250000.0f)
			};
			d->bounce_force = f;


			// Slow down vertical movevment
			self->physics->vel.y *= 0.2f;

			// Delay actual bouncing 0.1s
			async_schedule(_rabbit_delayed_bounce, 100, self);
		}
	}

	// Collision with trampoline
	if(other->type == OBJ_TRAMPOLINE_TYPE) {
		ObjTrampoline* trampoline = (ObjTrampoline*)other;
		if(trampoline->owner == self) {
			self->physics->vel.y = 0.0f;
			objects_apply_force(self, vec2(d->xjump*d->xjump, -d->yjump*d->yjump));
		}
	}	
}

static void obj_rabbit_construct(GameObject* self, Vector2 pos, void* user_data) {
	int id = (int)user_data;

	ObjRabbit* rabbit = (ObjRabbit*)self;
	rabbit->anim = anim_new_ex("rabbit", TIME_S);
	
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

	physics->cd_obj->pos.y = 579.0f;

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
	render->layer = rabbit_layer;
	render->anim_frame = 0;
	render->update_pos = obj_rabbit_update_pos;
	render->became_visible = obj_rabbit_became_visible;
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

	d->touching_ground = true;
	d->jump_off_mushroom = false;
	d->is_diving = false;
	d->is_dead = false;
	d->on_water = false;
	d->bounce_force = vec2(0.0f, 0.0f);
	d->player_control = false;
	d->falling_down = false;
	d->rabbit_time = 0.0f;
	d->game_over = false;
	d->boost = 0;
	d->speed = 100.0f;
	d->rubber_band = false;
	d->tokens = 0;
	d->has_trampoline = false;
	d->force_jump = false;
	d->force_dive = false;
	d->input_disabled = false;

	d->last_frame = 0;

	if(id < 0){
		render->spr = sprsheet_get_handle("rabbit");
		rabbit->control = obj_rabbit_player_control;
		//rabbit->control = obj_rabbit_ai_control;
		d->minimap_color = COLOR_RGBA(150, 150, 150, 255);
		d->rabbit_name = "You";
		d->player_control = true;
		d->speed = 500.0f;
		d->xjump = 100.0f;
		d->yjump = 400.0f;
		d->ai_max_combo = 0;
	} else {
		render->spr = levels_current_desc()->ai_rabbit_spr[id];
		d->minimap_color = levels_current_desc()->ai_rabbit_colors[id];
		rabbit->control = obj_rabbit_ai_control;
		d->rabbit_name = levels_current_desc()->ai_rabbit_names[id];
		d->speed = levels_current_desc()->ai_rabbit_speeds[id];
		d->xjump = levels_current_desc()->ai_rabbit_xjumps[id];
		d->yjump = levels_current_desc()->ai_rabbit_yjumps[id];
		d->ai_max_combo = levels_current_desc()->ai_max_combo[id];
		render->layer = ai_rabbit_layer;
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

