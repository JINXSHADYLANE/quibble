#include "game_over.h"
#include "game.h"
#include "obj_types.h"
#include "minimap.h"
#include "hud.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

extern float rabbit_current_distance;
extern uint longest_combo;
extern bool game_over;

static float display_distance = 0.0f;


static void game_over_init(void) {

}

static void game_over_close(void) {

}

static void game_over_preenter(void) {
	display_distance = rabbit_current_distance;
}

static void game_over_enter(void) {
}

static void game_over_leave(void) {
}

static bool game_over_update(void) {
	game_update_empty();
	minimap_update_places();	
	return true;
}

static bool game_over_render(float t) {
	// Game scene
	if(t != 0) {
		game_update_empty();
		minimap_update_places();	
	}
	if(t == 0){
		game_render(0);
	} 

	// Game Over overlay
	UIElement* element = uidesc_get("game_over");
	uint layer = hud_layer+1;

	UIElement* button_restart = uidesc_get_child(element, "button_restart");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 


	if(minimap_get_count() > 1){

		UIElement* text = uidesc_get_child(element, "text");
		UIElement* result_text = uidesc_get_child(element, "result_text");
		UIElement* result_time = uidesc_get_child(element, "result_time");

		// Text
		vfont_select(FONT_NAME, 48.0f); 
		const char* str = "The race is over.";
		static Vector2 half_size = {0.0f, 0.0f};
		if(half_size.x == 0.0f) {
			half_size = vec2_scale(vfont_size(str), 0.5f);
		}
		vfont_draw(str, layer, vec2_sub(text->vec2, half_size), col);

		for(int i = 0; i < minimap_get_count();i++){
			char result_str[32];
			char result_time_str[32];
			ObjRabbit* rabbit = minimap_get_place(i);
			Color c = COLOR_RGBA(255, 255, 255, a); 
			if(rabbit != NULL){
				if(rabbit->data->rabbit_time > 0.0f){
					sprintf(result_str, "%d. %s",i+1,rabbit->data->rabbit_name);
					sprintf(result_time_str, "%5.1fs",rabbit->data->rabbit_time);
				}
				else{
					sprintf(result_str, "%d. %s",i+1,rabbit->data->rabbit_name);
					sprintf(result_time_str, " Out");	
				}
				if(rabbit->data->player_control) c = COLOR_RGBA(237, 78, 0, a);

				vfont_draw(result_time_str, layer,vec2_add(result_time->vec2,vec2(0.0f,i*60.0f)), c);
			} else {
				sprintf(result_str, "%d. ---",i+1);
			}
			vfont_draw(result_str, layer,vec2_add(result_text->vec2,vec2(0.0f,i*60.0f)), c);
		}		
	} else {
		UIElement* complete = uidesc_get_child(element, "complete");

		// Text
		vfont_select(FONT_NAME, 48.0f); 
		const char* str = "Tutorial complete.";
		static Vector2 half_size = {0.0f, 0.0f};
		if(half_size.x == 0.0f) {
			half_size = vec2_scale(vfont_size(str), 0.5f);
		}
		vfont_draw(str, layer, vec2_sub(complete->vec2, half_size), col);
	}

	// Restart Button
	spr_draw_cntr_h(button_restart->spr, layer, button_restart->vec2, 0.0f, 1.0f, col);	
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_restart->vec2)) < 40.0f * 40.0f) {
			game_request_reset();
			malka_states_pop();
		}
	}

	// Quit Button
	spr_draw_cntr_h(button_quit->spr, layer, button_quit->vec2, 0.0f, 1.0f, col);	
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_quit->vec2)) < 40.0f * 40.0f) {
			malka_states_pop();
			malka_states_pop();
		}
	}		

	return true;
}

StateDesc game_over_state = {
	.init = game_over_init,
	.close = game_over_close,
	.enter = game_over_enter,
	.preenter = game_over_preenter,
	.leave = game_over_leave,
	.update = game_over_update,
	.render = game_over_render
};
