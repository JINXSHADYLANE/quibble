#include "ai.h"
#include "game.h"
#include "common.h"
#include <gfx_utils.h>

extern bool draw_ai_debug;

static RectF in_front_and_below(GameObject* obj){
	PhysicsComponent* p = obj->physics;

	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos.x += 80.0f + (p->vel.x * 0.2f);

	RectF rec = {
		.left = pos.x,
		.top = pos.y ,
		.right = pos.x,
		.bottom = v_height
	};

	return rec;
}

static RectF in_front_and_below2(GameObject* obj){
	PhysicsComponent* p = obj->physics;

	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos.x += 80.0f + (p->vel.x * 0.2f);

	RectF rec = {
		.left = pos.x + 70.0f,
		.top = pos.y ,
		.right = pos.x + 70.0f,
		.bottom = v_height
	};

	return rec;
}

static RectF below(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	RectF rec = {
		.left = d->dive.x + 70.0f,
		.top = d->dive.y,
		.right = d->dive.x + 70.0f,
		.bottom = d->dive.y + 62.0f
	};

	return rec;
}

static RectF in_front_of(GameObject* obj){
	PhysicsComponent* p = obj->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos.x += p->vel.x * 0.6f;

	RectF rec = {
		.left = pos.x,
		.top = pos.y - 100.0f,
		.right = pos.x + 80.0f,
		.bottom = pos.y + 10.0f
	};

	return rec;
}

static RectF in_front_of_avoid(GameObject* obj){
	PhysicsComponent* p = obj->physics;
	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos.x += 80.0f + (p->vel.x * 0.3f);

	RectF rec = {
		.left = pos.x,
		.top = pos.y - 100.0f,
		.right = pos.x + 80.0f,
		.bottom = pos.y + 10.0f
	};

	return rec;
}

static RectF in_landing_location(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	RectF rec = {
		.left = d->land.x,
		.top = d->land.y,
		.right = d->land.x + 70.0f,
		.bottom = d->land.y + 62.0f
	};

	return rec;
}

static bool there_is(uint obj_type, RectF rec){

	obj_type = obj_type & ~collision_flag;

	// Debug rendering
	if(draw_ai_debug){
		RectF r = objects_world2screen(rec,0);
		gfx_draw_rect(10, &r, COLOR_RGBA(0, 0, 255, 255));
	}

	if(rec.left == rec.right){
		GameObject* result = NULL;
		result = objects_raycast(vec2(rec.left,rec.top),vec2(rec.right,rec.bottom));

		if(result && (result->type & obj_type) ) return true;

	} else {

		GameObject* result[3] = {0};
		objects_aabb_query(&rec,&result[0],3);
		for(uint i = 0; i < 3; i++)	{
			if(result[i]){

				uint type = result[i]->type & ~collision_flag;
				if(type & obj_type)
					return true;

			}
		}
	}	

	return false;
}

static bool release_keys(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	bool release = false;

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
		release = true;
	}

	// release button when in the air
	if(!d->touching_ground && !d->is_diving){
		if(d->virtual_key_pressed){
			d->virtual_key_pressed = false;
			d->virtual_key_up = true;
			release = true;
		}	
	}

	return release;

}

static void use_powerups(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	if(d->has_powerup[SHIELD]) (powerup_params[SHIELD].powerup_callback) (obj);
	if(d->has_powerup[BOMB]) (powerup_params[BOMB].powerup_callback) (obj);	
	if(d->has_powerup[ROCKET]) (powerup_params[ROCKET].powerup_callback) (obj);
}

void ai_control_autumn(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	bool release = release_keys(obj);
	
	bool input = false;

	if(camera_follow && d->respawn == 0.0f)
		use_powerups(obj);

	if(!release){
		if(d->touching_ground){

			if(d->on_water && !d->has_powerup[SHIELD]){
				input = true;
				//printf("	water\n");
			}

			if( there_is( OBJ_MUSHROOM_TYPE, in_front_of(obj) ) ){
				//printf("	mushroom in front\n");
				input = true;
			}	

			if( there_is( OBJ_CACTUS_TYPE, in_front_of_avoid(obj) ) ){
				//printf("	cactus in front\n");
				input = true;
			}

			if( there_is( OBJ_FALL_TRIGGER_TYPE, in_front_and_below(obj) ) ){
				//printf("	gap in front\n");
				input = true;
			}

		} else {

			if( there_is( OBJ_FALL_TRIGGER_TYPE, in_landing_location(obj) ) ){ 
				input = true;
			}

			if( there_is( OBJ_MUSHROOM_TYPE, below(obj) ) && d->combo_counter < d->ai_max_combo ){ 
				input = true;
				//printf("	mushroom below\n" );
			}

			if( there_is( OBJ_FALL_TRIGGER_TYPE, below(obj) ) ){ 
				input = false;
				//printf("	unsafe to land, blocking input.\n");
			}
		}
	}

	// if ai decided to take action, press virtual keys
	if(input){	
		if(!d->virtual_key_down && !d->virtual_key_pressed) d->virtual_key_down = true;
		d->virtual_key_pressed = true;
	}

}

void ai_control_winter(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	bool release = release_keys(obj);
	
	bool input = false;
	use_powerups(obj);
	if(!release){
		if(d->touching_ground){

			if(d->on_water && !d->has_powerup[SHIELD]){
				input = true;
				//printf("	water\n");
			}

			if( there_is( OBJ_BRANCH_TYPE | OBJ_SPRING_BRANCH_TYPE, in_front_of(obj) ) ){
				//printf("	branch/mushroom in front\n");
				input = true;
			}	

			if( there_is( OBJ_FALL_TRIGGER_TYPE, in_front_and_below(obj)) &&
				there_is( OBJ_FALL_TRIGGER_TYPE, in_front_and_below2(obj))
			  ){
				//printf("	gap in front\n");
				input = true;
			}

		} else {

			if( there_is( OBJ_FALL_TRIGGER_TYPE, in_landing_location(obj) ) ){ 
				input = true;
			}

			if( there_is( OBJ_SPRING_BRANCH_TYPE, below(obj) ) && d->combo_counter < d->ai_max_combo ){ 
				input = true;
				//printf("	spring branch below\n" );
			}

			if( there_is( OBJ_FALL_TRIGGER_TYPE, below(obj) ) ){ 
				input = false;
				//printf("	unsafe to land, blocking input.\n");
			}
		}
	}
	// if ai decided to take action, press virtual keys
	if(input){	
		if(!d->virtual_key_down && !d->virtual_key_pressed) d->virtual_key_down = true;
		d->virtual_key_pressed = true;
	}

}

void ai_control_spring(GameObject* obj){
	// placeholder
}

void ai_control_summer(GameObject* obj){
	// placeholder
}