#include "minimap.h"
#include "hud.h"
#include "common.h"
#include "game.h"

static uint level_distance = 1;
static SprHandle handle;
static SprHandle finish;
DArray minimap_pointers;

static bool ready = false;

ObjRabbit* results[4];
static int dead_rabbits = 0;
static int place = 0;

static float player_x;

void minimap_init(){
	minimap_pointers = darray_create(sizeof(ObjRabbit*), 0);
	handle = sprsheet_get_handle("position_knob");
	finish = sprsheet_get_handle("finish_tile");
}

void minimap_close(void){
	darray_free(&minimap_pointers);
}

void minimap_track(ObjRabbit* rabbit){
	darray_append(&minimap_pointers, &rabbit);
}

void minimap_draw(){
	if(level_distance > 0){
		UIElement* position_line = uidesc_get("position_line");
		_hud_render_ui(position_line, hud_layer);

		Vector2 pos = position_line->vec2;

		Vector2 size = sprsheet_get_size_h(position_line->spr);
		
		float w = size.x - 14.0f;
		float h = size.y;

		for(int i = 0; i < minimap_pointers.size;i++){
			ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
			ObjRabbit* rabbit = *p_rabbit;
			
			float rd = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
			if(rabbit->data->player_control) player_x = rabbit->header.physics->cd_obj->pos.x;
			rd = (float)rd*w/level_distance;
			if(rd > w)	rd = w;

			RectF dest = {
				.left = pos.x + rd + 4.0f, 
				.top = pos.y+h/2.0f - 4.0f,
				.right = 0,
				.bottom = 0
			};
			spr_draw_h(handle, hud_layer,dest,rabbit->data->minimap_color);


		}
	}
}

void minimap_update_places(){
	ready = true;
	if(level_distance > 0){
		// draws finish line when its visible to player rabbit
		float d = (float)(level_distance + 2.0f) * (1024.0f/3.0f);
		if(d - player_x < 1024.0f){
			for(int i = 0; i < 768;i+=66){
				RectF dest = {
					.left = d, 
				};
				RectF result = objects_world2screen(dest,0);
				result.top = (float) i;
				result.right = 0.0f;
				result.bottom = 0.0f;
				spr_draw_h(finish, finish_line_layer,result,COLOR_WHITE);
			}
		} 


		for(int i = 0; i < minimap_pointers.size;i++){
			ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
			ObjRabbit* rabbit = *p_rabbit;
			if(!rabbit->data->game_over){
				if(rabbit->data->is_dead){
					rabbit->data->game_over = true;
					results[minimap_pointers.size-1-dead_rabbits++] = rabbit;
				} else {
					float rd = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
					if(rd > level_distance){
						rabbit->data->game_over = true;
						results[place++] = rabbit;
					}
				}
			} 
		}
	}

}

void minimap_reset(uint distance){
	minimap_pointers.size = 0;
	level_distance = distance;
	ready = false;
	dead_rabbits = 0;
	place = 0;
	for(int i = 0; i < 4;i++)
		results[i] = NULL;
}

float minimap_max_x(){
	float first = 0;
	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		if(ready){
			PhysicsComponent* p = rabbit->header.physics;
			Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
			if(pos.x > first) first = pos.x;
		}

	}
	return first + 1024.0f;
}

float minimap_player_x(){
	return player_x;
}

uint minimap_get_count(){
	return minimap_pointers.size;
}
ObjRabbit* minimap_get_place(uint i){
	return results[i];
}

ObjRabbit* minimap_get_rabbit(uint i){
	ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
	ObjRabbit* rabbit = *p_rabbit;
	return rabbit;
}