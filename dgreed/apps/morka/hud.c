#include "hud.h"
#include "common.h"
#include "game.h"
#include "minimap.h"
#include "levels.h"

#include <vfont.h>
#include <malka/ml_states.h>

extern void game_pause(void);

extern ObjRabbit* rabbit;
extern bool tutorial_level;

extern uint coins;

static uint last_combo = 0;
static uint current_combo = 0;
static float combo_flip_t = 0.0f;

static float powerup_appear[POWERUP_COUNT] = {0};

void hud_reset(void){
	last_combo = 0;
	current_combo = 0;
	combo_flip_t = 0.0f;
	for(uint i = 0; i < POWERUP_COUNT;i++)
		powerup_appear[i] = 0.0f;
}

static void _hud_render_powerups(float t){
	UIElement* element = uidesc_get("hud_powerups");

	const float duration = 0.5f;

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	

	byte a2 = lrintf(76.0f * alpha);
	Color col_30 = COLOR_RGBA(255, 255, 255, a2);		

	Vector2 powerup_place = element->vec2;

	SprHandle spr = sprsheet_get_handle(powerup_params[0].btn);
	SprHandle block = sprsheet_get_handle("blocked");

	Vector2 size = sprsheet_get_size_h(spr);

	int count = levels_get_powerup_count() +1;
	float x_offset = -count * (size.x + 27.0f);

	for(int i = 0; i < POWERUP_COUNT;i++){

		if(levels_current_desc()->powerup_num[i] > 0){

			float ts = time_s();
			float y_offset = 0.0f;
			SprHandle spr = sprsheet_get_handle(powerup_params[i].btn);
			Vector2 size = sprsheet_get_size_h(spr);
			
			x_offset += size.x + 27.0f;
			Vector2 pos = vec2_add(powerup_place, vec2(x_offset, -size.y) );
			spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col_30);

			if(rabbit->data->has_powerup[i]){

				if(powerup_appear[i] == 0.0f) powerup_appear[i] = time_s() + duration;

				float td = normalize(ts,powerup_appear[i]-duration,powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				y_offset = sin(PI*td/2.0f) * -size.y;

			} else {

				if(powerup_appear[i] > 0.0f)
					powerup_appear[i] = -(time_s() + duration);			

				float td = normalize(ts,-powerup_appear[i]-duration,-powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				if(td == 1.0f)
					powerup_appear[i] = 0.0f;

				y_offset = -size.y + (sin(PI*td/2.0f) * size.y);

			}

			if(powerup_appear[i] != 0.0f){

				pos = vec2_add(powerup_place, vec2(x_offset, y_offset) );
				spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col);

				if(!powerup_params[i].passive && rabbit->data->has_powerup[i]){

					if(camera_follow && rabbit->data->respawn == 0.0f){

						if(touches_down()) {
							Touch* t = touches_get();
							if(t){
								float r_sqr = 40.0f * 40.0f;
								if(vec2_length_sq(vec2_sub(t[0].hit_pos, pos)) < r_sqr) {
									GameObject* r = (GameObject*) rabbit;

									(powerup_params[i].powerup_callback) (r);
								}
							}
						}

					} else {
						spr_draw_cntr_h(block, hud_layer, vec2_add(pos,vec2(0.0f,5.0f)), 0.0f, 1.0f, col);
					}
				}


			}


		}
	}
}

static void _hud_render_powerups_buy(float t){
	UIElement* parent = uidesc_get("shop");
	UIElement* element = uidesc_get_child(parent, "hud_powerups");

	const float duration = 0.5f;

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	

	byte a2 = lrintf(76.0f * alpha);
	Color col_30 = COLOR_RGBA(255, 255, 255, a2);		

	byte alpha_txt = 255;

	Vector2 powerup_place = element->vec2;

	SprHandle spr = sprsheet_get_handle(powerup_params[0].btn);
	Vector2 size = sprsheet_get_size_h(spr);

	int count = levels_get_powerup_count() +1;
	float x_offset = (-count * (size.x + 27.0f)) /2.0f;

	for(int i = 0; i < POWERUP_COUNT;i++){

		if(levels_current_desc()->powerup_num[i] > 0){

			float ts = time_s();
			float y_offset = 0.0f;
			SprHandle spr = sprsheet_get_handle(powerup_params[i].btn);
			Vector2 size = sprsheet_get_size_h(spr);
			
			x_offset += size.x + 27.0f;
			Vector2 pos = vec2_add(powerup_place, vec2(x_offset, -size.y) );
			spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col_30);

			if(rabbit->data->has_powerup[i]){

				if(powerup_appear[i] == 0.0f) powerup_appear[i] = time_s() + duration;

				float td = normalize(ts,powerup_appear[i]-duration,powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				alpha_txt = lrintf(255.0f * (1-td));

				y_offset = sin(PI*td/2.0f) * -size.y;

			} else {

				if(powerup_appear[i] > 0.0f)
					powerup_appear[i] = -(time_s() + duration);			

				float td = normalize(ts,-powerup_appear[i]-duration,-powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				alpha_txt = 255;

				if(td == 1.0f)
					powerup_appear[i] = 0.0f;

				y_offset = -size.y + (sin(PI*td/2.0f) * size.y);


				if(touches_down() && !rabbit->data->has_powerup[i] && coins >= powerup_params[i].cost) {
					Touch* t = touches_get();
					if(t){
						float r_sqr = 40.0f * 40.0f;
						if(vec2_length_sq(vec2_sub(t[0].hit_pos, pos)) < r_sqr) {
							rabbit->data->has_powerup[i] = true;
							coins -= powerup_params[i].cost;	
						}
					}
				}


			}

			Vector2 txt_pos = pos;
			Color col_txt = COLOR_RGBA(255, 255, 255, alpha_txt);

			if(powerup_appear[i] != 0.0f){
				Vector2 pos2 = vec2_add(powerup_place, vec2(x_offset, y_offset) );
				spr_draw_cntr_h(spr, hud_layer, pos2, 0.0f, 1.0f, col);

				txt_pos.y += y_offset;
			}
			if(alpha_txt){
				// Txt
				vfont_select(FONT_NAME, 38.0f);
				char str[32];
				sprintf(str, "%dc",powerup_params[i].cost);
				Vector2	half_size = vec2_scale(vfont_size(str), 0.5f);
				txt_pos = vec2_sub(txt_pos, half_size);
				txt_pos.y -= size.y - 20.0f;
				vfont_draw(str, hud_layer, txt_pos, col_txt);
			}


		}
	}
}

void _hud_render_ui(UIElement* element, uint layer, Color col) {
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

		spr_draw_h(element->spr, layer, dest, col);
	}

	// Iterate over children
	UIElement* child;
	list_for_each_entry(child, &element->child_list, list) {
		_hud_render_ui(child, layer-1,col);
	}
}

static void _hud_render_combo_internal(
	UIElement* element, uint layer, uint mult, 
	float t, float text_size, const char* text) {

	vfont_select(FONT_NAME, text_size);

	// Combine two smootherstep functions into a new one,
	// where x stays constant for a while near t = 0.5
	float x;
	
	if(t < 0.5f)
		x = smootherstep(-1.0f, 0.0f, t * 2.0f);
	else
		x = smootherstep(0.0f, 1.0f, (t - 0.5f) * 2.0f);

	// Classic sine there-and-back-again for alpha
	float a = MAX(0.0f, sinf((t*1.4f - 0.2f) * PI));
	char final_text[64];
	sprintf(final_text, text, mult);

	static uint old = 0;
	uint new = hash_murmur(&mult,sizeof(mult),0);

	static Vector2 half_size = {0.0f, 0.0f};

	if(old != new) {
		old = new;
		half_size = vec2_scale(vfont_size(final_text), 0.5f);
	}

	Vector2 pos = vec2_sub(element->vec2, half_size);
	pos.x -= x * 200.0f;
	vfont_draw(final_text, layer, pos, COLOR_FTRANSP(a));
}

void hud_init(void) {
	vfont_init_ex(1024, 512);
}

void hud_close(void) {
	vfont_close();
}

void hud_trigger_combo(uint multiplier) {
	last_combo = current_combo;
	current_combo = multiplier;
	combo_flip_t = time_s();
}

void hud_render(float t) {
	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	

	if(!tutorial_level){

		UIElement* coin_icon = uidesc_get("coin_icon");
		spr_draw_cntr_h(coin_icon->spr, hud_layer,coin_icon->vec2, 0.0f, 1.0f, col);

		UIElement* coin_text = uidesc_get("coin_text");
		vfont_select(FONT_NAME, 38.0f);
		char str[32];
		sprintf(str, "%d",rabbit->data->tokens);
		static Vector2 half_size = {0.0f, 0.0f};
		if(half_size.x == 0.0f) {
			half_size = vec2_scale(vfont_size(str), 0.5f);
		}
		vfont_draw(str, hud_layer, coin_text->vec2, col);


		if(!rabbit->data->game_over) _hud_render_powerups(t);

		UIElement* place_text = uidesc_get("place_text");
		vfont_select(FONT_NAME, 48.0f);
		char place[32];
		sprintf(place, "%d/%d",minimap_get_place_of_rabbit(rabbit),minimap_get_count());
		static Vector2 half_size2 = {0.0f, 0.0f};
		if(half_size2.x == 0.0f) {
			half_size2 = vec2_scale(vfont_size(str), 0.5f);
		}
		vfont_draw(place, hud_layer, place_text->vec2, col);
	

	}

	UIElement* pause = uidesc_get("hud_pause");
	_hud_render_ui(pause, hud_layer,col);
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(t){
			float r_sqr = 40.0f * 40.0f;
			if(vec2_length_sq(vec2_sub(t[0].hit_pos, pause->vec2)) < r_sqr) {
				game_pause();
				malka_states_push("pause");
			}
		}
	}

	const char* combo_text;
	float combo_text_size;
	uint min_combo;

	if(tutorial_level){
		combo_text = "%ux";
		combo_text_size = 120.0f;
		min_combo = 1;
	} else {
		combo_text = "%ux Combo";
		combo_text_size = 38.0f;
		min_combo = 3;
	}

#define _render_combo(x, t) \
	_hud_render_combo_internal(combo_el, hud_layer, x, t, combo_text_size, combo_text)

	// Combo rendering
	UIElement* combo_el = uidesc_get("combo_text");
	float ts = time_s();
	float ct = (ts - combo_flip_t) / 0.4f;
	if(ct < 1.0f) {

		if(current_combo >= min_combo)
			_render_combo(current_combo, ct * 0.5f);
		if(last_combo >= min_combo)
			_render_combo(last_combo, 0.5f + ct * 0.5f);
	}
	else {
		if(current_combo >= min_combo)
			_render_combo(current_combo, 0.5f);
	}	
	// Minimap
	if(levels_current_desc()->distance > 0) minimap_draw(t);	
}

bool hud_button(UIElement* element, Color col, float ts) {
	spr_draw_cntr_h(element->spr, hud_layer+1, element->vec2, 0.0f, 1.0f, col);	
	Touch* t = touches_get();
	if(touches_down() && t && ts == 0.0f) {
		float r_sqr = 40.0f * 40.0f;		
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, element->vec2)) < r_sqr)
			return true;
	}
	return false;
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

	// Restart button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		malka_states_pop();
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();	
		malka_states_pop_multi(3);
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

	// Next button
	if(hud_button(button_next, col, t)) {
		levels_set_next();
		game_request_reset();
		malka_states_pop_multi(2);
	}

	// Restart button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		malka_states_pop();
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();	
		malka_states_pop_multi(3);
	}
}

void hud_render_game_over_win(float t) {
	UIElement* element = uidesc_get("game_over_win");
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
		Color c = COLOR_RGBA(255, 255, 255, a);
		char result_str[32];
		char result_time_str[32];
		ObjRabbit* rabbit = minimap_get_rabbit_in_place(i);

		if(rabbit->data->rabbit_time > 0.0f){
			sprintf(result_str, "%d. %s",i+1,rabbit->data->rabbit_name);
			sprintf(result_time_str, "%5.1fs",rabbit->data->rabbit_time);
		}
		else{
			sprintf(result_str, "%d. %s",i+1,rabbit->data->rabbit_name);
			sprintf(result_time_str, " Out");	
		}
		if(rabbit->data->player_control) 
			c = COLOR_RGBA(237, 78, 0, a);
		
		vfont_draw(result_str, layer,vec2_add(result_text->vec2,vec2(0.0f,i*60.0f)), c);
		vfont_draw(result_time_str, layer,vec2_add(result_time->vec2,vec2(0.0f,i*60.0f)), c);
	}	

	// Next button
	if(hud_button(button_next, col, t)) {
		levels_set_next();
		game_request_reset();
		malka_states_pop_multi(2);
	}

	// Restart Button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		malka_states_pop();
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();		
		malka_states_pop_multi(3);
	}
}

void hud_render_game_over_lose(float t) {
	UIElement* element = uidesc_get("game_over_lose");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* result_text = uidesc_get_child(element, "result_text");
	UIElement* result_time = uidesc_get_child(element, "result_time");

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
		Color c = COLOR_RGBA(255, 255, 255, a);
		char result_str[32];
		char result_time_str[32];
		ObjRabbit* rabbit = minimap_get_rabbit_in_place(i);

		if(rabbit->data->rabbit_time > 0.0f){
			sprintf(result_str, "%d. %s",i+1,rabbit->data->rabbit_name);
			sprintf(result_time_str, "%5.1fs",rabbit->data->rabbit_time);
		}
		else{
			sprintf(result_str, "%d. %s",i+1,rabbit->data->rabbit_name);
			sprintf(result_time_str, " Out");	
		}
		if(rabbit->data->player_control) 
			c = COLOR_RGBA(237, 78, 0, a);
		
		vfont_draw(result_str, layer,vec2_add(result_text->vec2,vec2(0.0f,i*60.0f)), c);
		vfont_draw(result_time_str, layer,vec2_add(result_time->vec2,vec2(0.0f,i*60.0f)), c);
	}	

	// Restart Button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		malka_states_pop();
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();		
		malka_states_pop_multi(3);
	}
}

void hud_render_regular_pause(float t){
		// Pause overlay
		UIElement* element = uidesc_get("pause");
		uint layer = hud_layer+1;

		UIElement* text = uidesc_get_child(element, "text");
		UIElement* button_play = uidesc_get_child(element, "button_play");
		UIElement* button_restart = uidesc_get_child(element, "button_restart");
		UIElement* button_quit = uidesc_get_child(element, "button_quit");

		float alpha = 1.0f-fabsf(t);
		byte a = lrintf(255.0f * alpha);
		Color col = COLOR_RGBA(255, 255, 255, a);

		spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 
		// Text
		vfont_select(FONT_NAME, 48.0f); 
		const char* str = "Paused";
		static Vector2 half_size = {0.0f, 0.0f};
		if(half_size.x == 0.0f) {
			half_size = vec2_scale(vfont_size(str), 0.5f);
		}
		vfont_draw(str, layer, vec2_sub(text->vec2, half_size), col);

		// Play (continue) button
		if(hud_button(button_play, col, t)) {
			malka_states_pop();		
		}

		// Restart button
		if(hud_button(button_restart, col, t)) {
			game_request_reset();
			malka_states_pop();	
		}

		// Quit button
		if(hud_button(button_quit, col, t)) {
			hud_reset();
			malka_states_pop_multi(3);
		}		
}

void hud_render_shop(float t){
	vfont_select(FONT_NAME, 48.0f);

	UIElement* element = uidesc_get("shop");

	UIElement* coin_icon = uidesc_get_child(element, "coin_icon");
	UIElement* coin_text = uidesc_get_child(element, "coin_text");	
	UIElement* character_icon = uidesc_get_child(element, "character_icon");
	UIElement* character_name = uidesc_get_child(element, "character_name");
	UIElement* speed_txt = uidesc_get_child(element, "speed_txt");
	UIElement* jump_txt = uidesc_get_child(element, "jump_txt");
	UIElement* button_play = uidesc_get_child(element, "button_play");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Coin icon
	spr_draw_cntr_h(coin_icon->spr, hud_layer,coin_icon->vec2, 0.0f, 1.0f, col);
	// Coin txt
	vfont_select(FONT_NAME, 38.0f);
	char str[32];
	sprintf(str, "%d",coins);
	vfont_draw(str, hud_layer, coin_text->vec2, col);

	// Character icon
	SprHandle icon = sprsheet_get_handle("rabbit_icon");
	Vector2 size = sprsheet_get_size_h(icon);
	Vector2 icon_pos = character_icon->vec2;
	icon_pos.x -= size.x / 2.0f;
	spr_draw_cntr_h(icon, hud_layer,icon_pos, 0.0f, 1.0f, col);

	// Character txt
	vfont_select(FONT_NAME, 38.0f);

	const char* name = "Rabbit";
	vfont_draw(name, hud_layer, character_name->vec2, col);	

	// Stats
	const char* speed = "Speed";
	vfont_draw(speed, hud_layer, speed_txt->vec2, col);	
	const char* jump = "Jump";
	vfont_draw(jump, hud_layer, jump_txt->vec2, col);	

	// Play button
	if(hud_button(button_play, col, t)) {
		malka_states_push("game");
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	_hud_render_powerups_buy(t);
}