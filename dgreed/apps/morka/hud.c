#include "hud.h"
#include "common.h"
#include "game.h"
#include "minimap.h"
#include "levels.h"

#include <vfont.h>
#include <malka/ml_states.h>

extern void game_reset(void);
extern void game_pause(void);
extern void game_unpause(void);
extern bool game_is_paused(void);

extern uint longest_combo;
extern ObjRabbit* rabbit;

static uint last_combo = 0;
static uint current_combo = 0;
static float combo_flip_t = 0.0f;

void _hud_render_ui(UIElement* element, uint layer) {
	// Render
	if(element->members & UI_EL_SPR) {
		RectF dest = rectf_null();
		if(element->members & UI_EL_RECT) {
			dest = element->rect;
		}
		else if(element->members & UI_EL_VEC2) {
			dest.left = element->vec2.x;
			dest.top = element->vec2.y;
		}

		spr_draw_h(element->spr, layer, dest, COLOR_WHITE);
	}

	// Iterate over children
	UIElement* child;
	list_for_each_entry(child, &element->child_list, list) {
		_hud_render_ui(child, layer-1);
	}
}

static void _hud_render_combo(UIElement* element, uint layer, uint mult, float t) {
	vfont_select(FONT_NAME, 38.0f);

	// Combine two smootherstep functions into a new one,
	// where x stays constant for a while near t = 0.5
	float x;
	if(t < 0.5f)
		x = smootherstep(-1.0f, 0.0f, t * 2.0f);
	else
		x = smootherstep(0.0f, 1.0f, (t - 0.5f) * 2.0f);

	// Classic sine there-and-back-again for alpha
	float a = MAX(0.0f, sinf((t*1.4f - 0.2f) * PI));

	char text[64];
	sprintf(text, "Combo x%u", mult);
	Vector2 half_size = vec2_scale(vfont_size(text), 0.5f);

	Vector2 pos = vec2_sub(element->vec2, half_size);
	pos.x -= x * 200.0f;
	vfont_draw(text, layer, pos, COLOR_FTRANSP(a));
}

void hud_init(void) {
	vfont_init_ex(1024, 1024);
}

void hud_close(void) {
	vfont_close();
}

void hud_trigger_combo(uint multiplier) {
	last_combo = current_combo;
	current_combo = multiplier;
	longest_combo = MAX(longest_combo, current_combo);
	combo_flip_t = time_s();
}

void hud_render(float t) {
	static bool animation_reset = false;

	UIElement* token_icon = uidesc_get("token_icon");
	spr_draw_cntr_h(token_icon->spr, hud_layer,token_icon->vec2, 0.0f, 1.0f, COLOR_WHITE);

	static float t0 = 0.0f;
	static float t1 = 0.0f;

	// Resque icon appearance animation
	if(rabbit->data->tokens >= 10){
		if(!animation_reset){
			const float animation_length = 0.4f; // 1.0 - 1s
			t0 = time_s();
			t1 = t0 + animation_length;
			animation_reset = true;
		}

		float ct = time_s();
		float t = 0.0f;
		float s = 1.0f;

		if(ct > t0 && ct < t1){
			t = (ct - t0) / (t1 - t0);
			s = sin(t*3.0f)+1.0f;
		}
		UIElement* resque_icon = uidesc_get("resque_icon");
		spr_draw_cntr_h(resque_icon->spr, hud_layer,resque_icon->vec2, ct, s, COLOR_WHITE);
	} else {
		animation_reset = false;
	}

	UIElement* token_text = uidesc_get("token_text");
	vfont_select(FONT_NAME, 38.0f);
	char str[32];
	sprintf(str, "%d",rabbit->data->tokens);
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, hud_layer, token_text->vec2, COLOR_WHITE);

	UIElement* pause = uidesc_get("hud_pause");
	_hud_render_ui(pause, hud_layer);
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, pause->vec2)) < 40.0f * 40.0f) {
			game_pause();
			malka_states_push("pause");
		}
	}

	UIElement* combo_text = uidesc_get("combo_text");
	float ts = time_s();
	float ct = (ts - combo_flip_t) / 0.4f;
	if(ct < 1.0f) {
		if(current_combo)
			_hud_render_combo(combo_text, hud_layer+1, current_combo, ct * 0.5f);
		if(last_combo)
			_hud_render_combo(combo_text, hud_layer+1, last_combo, 0.5f + ct * 0.5f);
	}
	else {
		if(current_combo)
			_hud_render_combo(combo_text, hud_layer+1, current_combo, 0.5f);
	}

	// Minimap
	if(minimap_get_count() > 1) minimap_draw();
	minimap_update_places();
}

void hud_render_game_over_out(float t) {	
	UIElement* element = uidesc_get("game_over_out");
	uint layer = hud_layer+1;	

	UIElement* complete = uidesc_get_child(element, "text");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "You are out of the race.";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(complete->vec2, half_size), col);

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


}

void hud_render_game_over_tut(float t) {
	UIElement* element = uidesc_get("game_over_tut");
	uint layer = hud_layer+1;	

	UIElement* complete = uidesc_get_child(element, "text");
	UIElement* button_next = uidesc_get_child(element, "next");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Tutorial complete.";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(complete->vec2, half_size), col);

//	if(!levels_is_final()){
		// Next Button
		spr_draw_cntr_h(button_next->spr, layer, button_next->vec2, 0.0f, 1.0f, col);	
		if(touches_down() && t == 0.0f) {
			Touch* t = touches_get();
			if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_next->vec2)) < 40.0f * 40.0f) {
				levels_set_next();
				game_request_reset();
				malka_states_pop();
			}
		}
//	}

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


}

void hud_render_game_over_scores(float t) {
	UIElement* element = uidesc_get("game_over_scores");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* result_text = uidesc_get_child(element, "result_text");
	UIElement* result_time = uidesc_get_child(element, "result_time");

	UIElement* button_next = uidesc_get_child(element, "next");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "The race is over.";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(text->vec2, half_size), col);

	// Timetable
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

	//if(!levels_is_final()){
		// Next Button
		spr_draw_cntr_h(button_next->spr, layer, button_next->vec2, 0.0f, 1.0f, col);	
		if(touches_down() && t == 0.0f) {
			Touch* t = touches_get();
			if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_next->vec2)) < 40.0f * 40.0f) {
				levels_set_next();
				game_request_reset();
				malka_states_pop();
			}
		}
	//}

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


}
