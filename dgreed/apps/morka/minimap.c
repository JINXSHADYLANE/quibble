#include "minimap.h"
#include "hud.h"
#include "common.h"

static uint level_distance = 0;
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
	darray_append(&minimap_pointers, rabbit);
}

void minimap_draw(){
	UIElement* position_line = uidesc_get("position_line");
	_hud_render_ui(position_line, hud_layer);

	//Vector2 pos = position_line->vec2;

	for(int i = 0; i < minimap_pointers.size;i++){
		ObjRabbit* rabbit = darray_get(&minimap_pointers, i);
		printf("rabbit pointer: %p \n", (void*)rabbit);
		
		/*if(!rabbit->data->is_dead){
			printf("placing rabbit pin\n");
			float rabbit_pos = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
			Color tint = COLOR_RGBA(255, 0, 0, 255);
			RectF dest = {
				.left = pos.x, 
				.top = pos.x+8,
				.right = pos.y,
				.bottom = pos.x+8
			};
			spr_draw_h(handle, hud_layer+1,dest,tint);
		}*/

	}
}

void minimap_reset(uint distance){
	minimap_pointers.size = 0;
	level_distance = distance;
}
