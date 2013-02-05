#include "minimap.h"
#include "hud.h"
#include "common.h"

static uint level_distance = 1;
static SprHandle handle;
DArray minimap_pointers;

void minimap_init(){
	minimap_pointers = darray_create(sizeof(ObjRabbit*), 0);
	handle = sprsheet_get_handle("position_knob");
}

void minimap_close(void){
	darray_free(&minimap_pointers);
}

void minimap_track(ObjRabbit* rabbit){
	darray_append(&minimap_pointers, &rabbit);
}

void minimap_draw(){
	UIElement* position_line = uidesc_get("position_line");
	_hud_render_ui(position_line, hud_layer);

	Vector2 pos = position_line->vec2;

	Vector2 size = sprsheet_get_size_h(position_line->spr);
	
	float w = size.x - 14.0f;
	float h = size.y;

	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit** p_rabbit = darray_get(&minimap_pointers, i);
		ObjRabbit* rabbit = *p_rabbit;
		
		if(!rabbit->data->is_dead){
			float rd = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
			rd = (float)rd*w/level_distance;
			if(rd > w) rd = w;
			Color tint = COLOR_RGBA(0, 0, 0, 255);
			RectF dest = {
				.left = pos.x + rd + 4.0f, 
				.top = pos.y+h/2.0f - 4.0f,
				.right = 0,
				.bottom = 0
			};
			spr_draw_h(handle, hud_layer+1,dest,tint);
		}

	}
}

void minimap_reset(uint distance){
	minimap_pointers.size = 0;
	level_distance = distance;
}
