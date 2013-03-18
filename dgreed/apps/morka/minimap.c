#include "minimap.h"
#include "hud.h"
#include "common.h"
#include "game.h"

static uint level_distance = 1;
static SprHandle handle;
static SprHandle finish;
DArray minimap_pointers;

ObjRabbit* results[4];
static int dead_rabbits = 0;
static int place = 0;

static float player_x;

void minimap_init(void){
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

void minimap_draw_finish_line(void){
	if(level_distance > 0){
		// Draw a finish line when its visible to player rabbit
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
	} 	
}

void minimap_draw(float t){
	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	

	UIElement* position_line = uidesc_get("position_line");
	_hud_render_ui(position_line, hud_layer,col);

	Vector2 pos = position_line->vec2;

	Vector2 size = sprsheet_get_size_h(position_line->spr);
		
	float w = size.x - 14.0f;
	float h = size.y;

	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		if(rabbit && rabbit->header.type){
				
			float rd = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
			if(rabbit->data->player_control) 
				player_x = rabbit->header.physics->cd_obj->pos.x;

			rd = (float)rd*w/level_distance;
			if(rd > w)	rd = w;

			RectF dest = {
				.left = pos.x + rd + 4.0f, 
				.top = pos.y+h/2.0f - 4.0f,
				.right = 0,
				.bottom = 0
			};
			byte r,g,b,a2;
			COLOR_DECONSTRUCT(rabbit->data->minimap_color,r,g,b,a2);
			Color c = COLOR_RGBA(r,g,b,a);

			spr_draw_h(handle, hud_layer,dest,c);
		}
	}

	minimap_update_places();
}

void minimap_update_places(void){
	if(level_distance > 0){
		for(int i = 0; i < minimap_pointers.size;i++){
			ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
			ObjRabbit* rabbit = *p_rabbit;
			if(rabbit && rabbit->header.type && !rabbit->data->game_over){
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
	dead_rabbits = 0;
	place = 0;
	for(int i = 0; i < 4;i++)
		results[i] = NULL;
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
	return first + 1024.0f;
}

float minimap_min_x(void){
	float last = minimap_max_x();
	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		if(rabbit && rabbit->header.type && !rabbit->data->is_dead){
			PhysicsComponent* p = rabbit->header.physics;
			Vector2 pos = vec2_add(p->cd_obj->pos, p->cd_obj->offset);
			if(pos.x < last) last = pos.x;
		}

	}
	return last;
}

float minimap_player_x(void){
	return player_x;
}

uint minimap_get_count(void){
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