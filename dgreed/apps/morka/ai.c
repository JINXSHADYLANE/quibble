#include "ai.h"
#include "common.h"

extern bool draw_ai_debug;

static RectF in_front_and_below(GameObject* obj){
	PhysicsComponent* p = obj->physics;

	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos.x += 80.0f + (p->vel.x * 0.1f);

	RectF rec = {
		.left = pos.x,
		.top = 580.0f,
		.right = pos.x,
		.bottom = 620.0f
	};

	return rec;
}

static RectF below(GameObject* obj){
	PhysicsComponent* p = obj->physics;

	Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
	pos.x += 80.0f + (p->vel.x * 0.3f) *(579.0f-pos.y)/579.0f;	
	pos.y += 92.0f;

	RectF rec = {
		.left = pos.x,
		.top = pos.y,
		.right = pos.x,
		.bottom = HEIGHT
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
		.bottom = pos.y
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
		.bottom = pos.y
	};

	return rec;
}

static RectF in_landing_location(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	RectF rec = {
		.left = d->land-50.0f,
		.top = HEIGHT - 50.0f,
		.right = d->land+50.0f,
		.bottom = HEIGHT-10.0f
	};

	return rec;
}

static bool there_is(int obj_type, RectF rec){
	//GameObject* result[2] = {0};
	GameObject* result = NULL;

	if(rec.left == rec.right){
		result = objects_raycast(vec2(rec.left,rec.top),vec2(rec.right,rec.bottom));
	} else {
		objects_aabb_query(&rec,&result,1);
	}

	if(draw_ai_debug){
		RectF r = objects_world2screen(rec,0);

		Vector2 s = vec2(r.left, r.top);
		Vector2 e = vec2(r.left, r.bottom);
		video_draw_line(10,	&s, &e, COLOR_RGBA(0, 0, 255, 255));

		s = vec2(r.right, r.top);
		e = vec2(r.right, r.bottom);
		video_draw_line(10,	&s, &e, COLOR_RGBA(0, 0, 255, 255));		

		s = vec2(r.left, r.top);
		e = vec2(r.right, r.top);
		video_draw_line(10,	&s, &e, COLOR_RGBA(0, 0, 255, 255));

		s = vec2(r.left, r.bottom);
		e = vec2(r.right, r.bottom);
		video_draw_line(10,	&s, &e, COLOR_RGBA(0, 0, 255, 255));															
	}

	if(result && result->type == obj_type) return true;

	return false;
}

static void release_keys(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

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

	// release button when in the air
	if(!d->touching_ground && !d->is_diving){
		if(d->virtual_key_pressed){
			d->virtual_key_pressed = false;
			d->virtual_key_up = true;
		}	
	}

}

void ai_control(GameObject* obj){
	ObjRabbit* rabbit = (ObjRabbit*)obj;
	ObjRabbitData* d = rabbit->data;

	release_keys(obj);
	
	bool input = false;

	if(d->touching_ground){

		if(d->on_water)
			input = true;

		if( there_is( OBJ_MUSHROOM_TYPE, in_front_of(obj) ) )
			input = true;

		if( there_is( OBJ_CACTUS_TYPE, in_front_of_avoid(obj) ) )
			input = true;

		if( there_is( OBJ_FALL_TRIGGER_TYPE, in_front_and_below(obj) ) )
			input = true;

	} else {

		if( ! there_is( OBJ_GROUND_TYPE, in_landing_location(obj) ) ) 
			input = true;

		if( there_is( OBJ_MUSHROOM_TYPE, below(obj) ) && d->combo_counter < d->ai_max_combo ) 
			input = true;

		if( there_is( OBJ_FALL_TRIGGER_TYPE, below(obj) ) ) 
			input = false;
	}

	// if ai decided to take action, press virtual keys
	if(input){
		d->virtual_key_pressed = true;	
		if(!d->virtual_key_down) d->virtual_key_down = true;
	}

}