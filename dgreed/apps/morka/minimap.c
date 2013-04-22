#include "minimap.h"
#include "hud.h"
#include "common.h"
#include "game.h"

#include <vfont.h>

static uint level_distance = 1;
static SprHandle distance_pointer;
static SprHandle distance_id;
static SprHandle handle;
static SprHandle finish;
DArray minimap_pointers;

ObjRabbit* results[4];
float distance_pos[4];

static float player_x;

void minimap_init(void){
	minimap_pointers = darray_create(sizeof(ObjRabbit*), 0);
	distance_pointer = sprsheet_get_handle("distance_pointer");
	distance_id = sprsheet_get_handle("distance_id");
	handle = sprsheet_get_handle("position_knob");
	finish = sprsheet_get_handle("finish_tile");
}

void minimap_close(void){
	darray_free(&minimap_pointers);
}

void minimap_track(ObjRabbit* rabbit){
	darray_append(&minimap_pointers, &rabbit);
	results[minimap_pointers.size-1] = rabbit;
}

void minimap_draw_finish_line(void){
	if(level_distance > 0){
		// Draw a finish line when its visible to player rabbit
		float d = (float)(level_distance + 2.0f) * (v_width/3.0f);
		if(d - player_x < v_width){
			for(int i = -110; i < (int)v_height;i+=66){
				RectF dest = {
					.left = d,
					.top = (float) i 
				};
				RectF result = objects_world2screen(dest,0);
				result.right = 0.0f;
				result.bottom = 0.0f;
				spr_draw_h(finish, finish_line_layer,result,COLOR_WHITE);
			}
		}
	} 	
}

void minimap_draw(float t){
	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	

	UIElement* position_line = uidesc_get("position_line");

	spr_draw_cntr_h(position_line->spr, hud_layer, position_line->vec2, 0.0f, 1.0f, col);

	Vector2 size = sprsheet_get_size_h(position_line->spr);
		
	float w = size.x;

	Vector2 pos = position_line->vec2;
	pos = vec2_add(pos,vec2(-size.x/2.0f,0.0f));	

	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		if(rabbit && rabbit->header.type){
				
			float rd = rabbit->header.render->world_dest.left / (v_width / 3.0f) - 2.0f;
			if(rabbit->data->player_control) 
				player_x = rabbit->header.physics->cd_obj->pos.x;

			rd = (float)rd*w/level_distance;
			rd = clamp(7.0f,w - 7.0f,rd );

			Vector2 dest = vec2(pos.x + rd,pos.y);

			byte r,g,b,a2;
			COLOR_DECONSTRUCT(rabbit->data->minimap_color,r,g,b,a2);
			Color c = COLOR_RGBA(r,g,b,a);

			spr_draw_cntr_h(handle, hud_layer, dest, 0.0f, 1.0f, c);
		}
	}

	minimap_update_places();
}

static float _pix_to_meters(float pix) {
	return pix * (3.0f / v_width) - 2.0f;
}

void minimap_draw_distance_from(float t, ObjRabbit* rabbit){
	if(camera_follow && rabbit && rabbit->header.type) {
		float alpha = 1.0f-fabsf(t);
		byte a = lrintf(255.0f * alpha);
		Color col = COLOR_RGBA(255, 255, 255, a);

		float d1 = _pix_to_meters(rabbit->header.render->world_dest.left);

		for(int i = 0; i < minimap_pointers.size;i++){
			ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
			ObjRabbit* other = *p_rabbit;
			if(other && other->header.type && other != rabbit && !other->header.render->was_visible){
				float d2 = _pix_to_meters(other->header.render->world_dest.left);

				float delta = d2 - d1;

				Vector2 dest = {0};
				Vector2 txt_pos = {0};

				byte r,g,b,a2;
				COLOR_DECONSTRUCT(other->data->minimap_color, r, g, b, a2);
				Color c = COLOR_RGBA(r, g, b, a);

				vfont_select(FONT_NAME, 28.0f);
				char str[32];
				sprintf(str, "%ldm", lrintf(fabsf(delta)));
				Vector2	half_size = vec2_scale(vfont_size(str), 0.5f);

				float dest_x = 20.0f;
				float txt_x = 75.0f;
				float angle = 0.0f;

				if(delta >= 0.0f) {
					dest_x = v_width - 20.0f;
					txt_x = v_width - 75.0f;
					angle = PI;
				}

				float y = other->header.physics->cd_obj->pos.y;
				distance_pos[i] = lerp(distance_pos[i], y, 0.01f);
				dest = vec2(dest_x,distance_pos[i]);
				txt_pos = vec2(txt_x,dest.y);
				spr_draw_cntr_h(distance_pointer, hud_layer, dest, angle, 1.0f, col);

				spr_draw_cntr_h(distance_id, hud_layer, dest, 0.0f, 1.0f, c);

				txt_pos = vec2_sub(txt_pos, half_size);
				vfont_draw(str, hud_layer, txt_pos, col);
			}
		}
	}

	minimap_update_places();

}

static int rabbit_compar(const void* a, const void* b) {
	const ObjRabbit** pa = (const ObjRabbit**) a;
	const ObjRabbit** pb = (const ObjRabbit**) b;

	const ObjRabbit* ra = *pa;
	const ObjRabbit* rb = *pb;

	float fa;
	float fb;

	if( (ra->data->game_over || rb->data->game_over)  &&
		ra->data->rabbit_time > 0 && rb->data->rabbit_time > 0
		){
		// if either of rabbits has finished the race, but not fallen out,
		// compare times (smaller is better)
		fa = ra->data->rabbit_time;
		fb = rb->data->rabbit_time;
	} else {
		// if neither of rabbits has finished the race or fallen out,
		// compare distances (longer is better)
		fb = ra->header.render->world_dest.left;
		fa = rb->header.render->world_dest.left;
	}

	return fa - fb;
}

void minimap_update_places(void){
	for(uint i = 0; i < minimap_pointers.size;i++ ){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;

		if(rabbit && rabbit->header.type && !rabbit->data->game_over){
			if(rabbit->data->is_dead)
				rabbit->data->game_over = true;
			else {
				float rd = rabbit->header.render->world_dest.left / (v_width / 3.0f) - 2.0f;
				if(rd > level_distance)
					rabbit->data->game_over = true;
			}
		} 				
		
	}
	if(results[0] && results[0]->header.type )
		sort_insertion(results, minimap_pointers.size, sizeof(ObjRabbit*),rabbit_compar);
}

void minimap_reset(uint distance){
	minimap_pointers.size = 0;
	level_distance = distance;
	for(int i = 0; i < 4;i++){
		results[i] = NULL;
		distance_pos[i] = v_height / 2.0f;
	}	
}

float minimap_max_x(void){
	float first = 0;
	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		if(rabbit && rabbit->header.type){
			PhysicsComponent* p = rabbit->header.physics;
			Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
			if(pos.x > first) first = pos.x;
		}

	}
	return first + v_width;
}

float minimap_min_x(void){
	float last = objects_camera[0].left;
	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		if(rabbit && rabbit->header.type
		&& !rabbit->data->is_dead && !rabbit->data->game_over){
			PhysicsComponent* p = rabbit->header.physics;
			Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
			if(pos.x < last) last = pos.x;
		}

	}
	return last;
}

uint minimap_get_count(void){
	return minimap_pointers.size;
}
ObjRabbit* minimap_get_rabbit_in_place(uint i){
	return results[i];
}

ObjRabbit* minimap_get_rabbit(uint i){
	ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
	ObjRabbit* rabbit = *p_rabbit;
	return rabbit;
}

uint minimap_get_place_of_rabbit(ObjRabbit* rabbit){
	for(uint i = 0; i < minimap_pointers.size;i++ )
		if(results[i] == rabbit) return i+1;

	return minimap_pointers.size;
}
