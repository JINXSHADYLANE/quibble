#include "hud.h"
#include "common.h"
#include "game.h"
#include "item_unlock.h"
#include "shop.h"
#include "minimap.h"
#include "levels.h"

#include <mfx.h>
#include <vfont.h>
#include <malka/ml_states.h>

extern void game_pause(void);

extern ObjRabbit* player;
extern bool tutorial_level;

extern float game_over_anim_start;
extern float game_over_anim_end;
extern uint coins_earned;

extern uint coins;

static uint last_combo = 0;
static uint current_combo = 0;
static float combo_flip_t = 0.0f;

static float powerup_appear[POWERUP_COUNT] = {0};

bool hud_click = false;

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
	float x_offset = -count * (size.x + 27.0f) + size.x / 2.0f;

	for(int i = 0; i < POWERUP_COUNT;i++){

		if(levels_current_desc()->powerup_num[i] > 0){

			float ts = time_s();
			float y_offset = 0.0f;
			SprHandle spr = sprsheet_get_handle(powerup_params[i].btn);
			Vector2 size = sprsheet_get_size_h(spr);
			
			x_offset += size.x + 27.0f;
			Vector2 pos = vec2_add(powerup_place, vec2(x_offset, -size.y / 2.0f) );
			spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col_30);

			if(player->data->has_powerup[i]){

				if(powerup_appear[i] == 0.0f) powerup_appear[i] = time_s() + duration;

				float td = normalize(ts,powerup_appear[i]-duration,powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				y_offset = sin(PI*td/2.0f) * -size.y / 2.0f;

			} else {

				if(powerup_appear[i] > 0.0f)
					powerup_appear[i] = -(time_s() + duration);			

				float td = normalize(ts,-powerup_appear[i]-duration,-powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				if(td == 1.0f)
					powerup_appear[i] = 0.0f;

				y_offset = -size.y / 2.0f + (sin(PI*td/2.0f) * size.y);

			}

			if(powerup_appear[i] != 0.0f){

				pos = vec2_add(powerup_place, vec2(x_offset, y_offset) );
				spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col);

				if(!powerup_params[i].passive && player->data->has_powerup[i]){

					if(camera_follow && player->data->respawn == 0.0f){

						if(hud_button_ex(empty_spr,pos,40.0f,col,t)){
							GameObject* r = (GameObject*) player;
							(powerup_params[i].powerup_callback) (r);							
						}

					} else {
						spr_draw_cntr_h(block, hud_layer, vec2_add(pos,vec2(0.0f,5.0f)), 0.0f, 1.0f, col);
					}
				}


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

	static uint old = 0;
	uint new = hash_murmur(&mult,sizeof(mult),0);

	static Vector2 half_size = {0.0f, 0.0f};

	if(old != new) {
		old = new;
		half_size = vec2_scale(vfont_size(text), 0.5f);
	}

	Vector2 pos = vec2_sub(element->vec2, half_size);
	pos.x -= x * 200.0f;
	vfont_draw_number(mult, text, layer, pos, COLOR_FTRANSP(a));
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

	if(!tutorial_level && player && player->header.type){

		UIElement* coin_icon = uidesc_get("coin_icon");
		spr_draw_cntr_h(coin_icon->spr, hud_layer,coin_icon->vec2, 0.0f, 1.0f, col);

		UIElement* coin_text = uidesc_get("coin_text");
		vfont_select(FONT_NAME, 38.0f);

		static uint coins = 0;
		static uint prev = 0;
		static float anim = 0.0f;
		static float duration = 0.3f;

		prev = coins;
		coins = player->data->tokens;


		if(prev != coins && coins != 0)
			anim = time_s() + duration;

		float scale = 1.0f;
		if(anim - time_s() > 0)
			scale += anim - time_s();

		vfont_draw_number_ex(coins, NULL, hud_layer, coin_text->vec2, col, scale);


		if(!player->data->game_over) _hud_render_powerups(t);

	}

	//Pause button
	UIElement* pause = uidesc_get("hud_pause");
	if(hud_button(pause, col, t)) {
		game_pause();
		malka_states_push("pause");
	}
	const char* combo_text;
	float combo_text_size;
	uint min_combo;

	if(tutorial_level){
		combo_text = "x";
		combo_text_size = 120.0f;
		min_combo = 1;
	} else {
		combo_text = "x Combo";
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
	//if(levels_current_desc()->ai_rabbit_num > 0) minimap_draw_distance_from(t,player);	
}

bool hud_button(UIElement* element, Color col, float ts) {
	return hud_button_ex(element->spr,element->vec2,40.0f,col,ts);
}

bool hud_button_ex(SprHandle spr, Vector2 pos, float r, Color col, float ts){
	if(spr != empty_spr)
		spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col);	
	Touch* t = touches_get();
	if(touches_down() && t && ts == 0.0f) {		
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, pos)) < r * r){
			hud_click = true;
			return true;
		}
	}
	return false;	
}

bool hud_button_rect(SprHandle spr, Vector2 pos, Vector2 size, Color col, float ts){
	if(spr != empty_spr)
		spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col);	
	Touch* t = touches_get();
	if(touches_down() && t && ts == 0.0f) {		

		size = vec2_scale(size,0.5f);

		RectF rec = {
			.left = pos.x - size.x,
			.top = pos.y - size.y,
			.right = pos.x + size.x,
			.bottom = pos.y + size.y
		};

		if(rectf_contains_point(&rec,&t[0].hit_pos)){
			hud_click = true;
			return true;
		}
	}	
	return false;
}

void hud_render_game_over_out(float t) {	
	UIElement* element = uidesc_get("game_over_out");

	UIElement* complete = uidesc_get_child(element, "text");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "You are out of the race.";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, hud_layer, vec2_sub(complete->vec2, half_size), col);

	// Restart button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		malka_states_pop_multi(2);
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();	
		malka_states_pop_multi(3);
	}
}

bool hud_unlock_check(uint state_num){
	int id = levels_get_next_unlocked_powerup();		
	if(id != -1){
		ObjItemUnlockParams p = {
			.spr = powerup_params[id].unlocked_spr,
			.text1 = "Unlocked:",
			.text2 = powerup_params[id].name,
			.text3 = powerup_params[id].description,
			.state_num = state_num
		};
		levels_set_unlocked_powerup_seen(id);
		item_unlock_set(&p);
		malka_states_push("item_unlock");
		return true;
	}
	return false;
}

void hud_render_game_over_tut(float t) {
	UIElement* element = uidesc_get("game_over_tut");

	UIElement* level_text = uidesc_get_child(element, "level_text");
	UIElement* complete = uidesc_get_child(element, "text");
	UIElement* button_next = uidesc_get_child(element, "next");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	// Level text
	vfont_select(FONT_NAME, 38.0f);
	LevelDesc* lvl = levels_current_desc();
	assert(lvl);
	vfont_draw(lvl->name, hud_layer+1, level_text->vec2, col);

	spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Tutorial complete.";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, hud_layer, vec2_sub(complete->vec2, half_size), col);

	// Next button
	if(hud_button(button_next, col, t)) {
		levels_set_next();
		game_request_reset();	
		if(!hud_unlock_check(2))	
			malka_states_pop_multi(2);
	}

	// Restart button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		if(!hud_unlock_check(2))		
			malka_states_pop_multi(2);
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();
		if(!hud_unlock_check(3))				
			malka_states_pop_multi(3);
	}
}

void hud_render_game_over_main(float t){
	UIElement* element = uidesc_get("game_over_main");

	UIElement* level_text = uidesc_get_child(element, "level_text");
	UIElement* text = uidesc_get_child(element, "text");
	UIElement* place_icon = uidesc_get_child(element, "place_icon");
	UIElement* platform = uidesc_get_child(element, "platform");
	UIElement* particles1 = uidesc_get_child(element, "particles1");
	UIElement* particles2 = uidesc_get_child(element, "particles2");	
	
	UIElement* text2 = uidesc_get_child(element, "text2");
	UIElement* coin_text = uidesc_get_child(element, "coin_text");
	UIElement* coin_icon = uidesc_get_child(element, "coin_icon");

	SprHandle place_spr;

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);
	
	// Level text
	vfont_select(FONT_NAME, 38.0f);
	LevelDesc* lvl = levels_current_desc();
	assert(lvl);
	vfont_draw(lvl->name, hud_layer+1, level_text->vec2, col);

	spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 

	uint place = minimap_get_place_of_rabbit(player);
	char place_txt[32];

	float off = 0;

	switch(place){
		case 1:
			place_spr = sprsheet_get_handle("first_place");
			sprintf(place_txt, "You're first!");
		break;
		case 2:
			place_spr = sprsheet_get_handle("second_place");
			sprintf(place_txt, "You're second!");
		break;
		case 3:
			place_spr = sprsheet_get_handle("third_place");
			sprintf(place_txt, "You're third!");
		break;
		default:
			place_spr = sprsheet_get_handle("last_place");
			sprintf(place_txt, "You're fourth");
		break;							
	}

	spr_draw_cntr_h(platform->spr, hud_layer,platform->vec2, 0.0f, 1.0f, col);

	float ts = time_s();
	static bool particles_spawned = false;
	static float coin_start = 0.0f;
	float coin_anim = 0.0f;	

	if(ts < game_over_anim_start){
		off = v_height/1.8f;

		particles_spawned = false;
		coin_start = 0.0f;

	} else if(ts >= game_over_anim_start && ts < game_over_anim_end ){
		float td = normalize(ts,game_over_anim_start,game_over_anim_end);
		td = clamp(0.0f,1.0f,td);

		off = sin(PI/2.0f*td + PI/2.0f) * v_height/1.8f;

		particles_spawned = false;
		coin_start = 0.0f;	

	} else {

		if(!particles_spawned){
			mfx_trigger_ex("dusts2", particles2->vec2, 0.0f);
			mfx_trigger_ex("dusts2", particles1->vec2, 180.0f);
			particles_spawned = true;
			coin_start = time_s() + 0.5f;
		}

		if(coin_start > 0.0f && time_s() > coin_start && coins_earned < player->data->tokens ){

			static float coin_time = 0.0f;

			if(time_s() > coin_time){

				float delta = normalize((float)coins_earned,0.0f,(float)player->data->tokens);

				delta = 1.01f - powf( cos(delta * PI/2.0f),0.2f ); 
				coin_time = time_s() + delta;
				coin_anim = time_s() + 0.22f;
				coins_earned++;
			}

			if(touches_down()){
				coins_earned = player->data->tokens;
			}			

		}

	}	

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	static uint prev_place = 0;
	static Vector2 half_size = {0.0f, 0.0f};
	if(prev_place != place) {
		prev_place = place;
		half_size = vec2_scale(vfont_size(place_txt), 0.5f);
	}
	Vector2 txt_pos = vec2_sub(text->vec2, vec2(half_size.x,0));
	txt_pos.y -= off;
	vfont_draw(place_txt, hud_layer,txt_pos , col);

	Vector2 place_pos = place_icon->vec2;
	place_pos.y -= off + sprsheet_get_size_h(place_spr).y / 2.0f;
	if(place > 3) place_pos.y -= 7;
	spr_draw_cntr_h(place_spr, hud_layer,place_pos, 0.0f, 1.0f, col);

	if(coin_start > 0.0f){
		// Text
		vfont_select(FONT_NAME, 38.0f); 
		const char* str = "You earned:";
		static Vector2 half_size2 = {0.0f, 0.0f};
		if(half_size2.x == 0.0f) {
			half_size2 = vec2_scale(vfont_size(str), 0.5f);
		}

		float scale1 = -(coin_start - 0.4f - time_s()) * 3.0f;
		if(scale1 < 0.0f) 
			scale1 += 1.0f;
		else
			scale1 = 1.0f;

		vfont_draw_ex(str, hud_layer, vec2_sub(text2->vec2, half_size2), col,scale1);

		// Coin icon
		Vector2 size = sprsheet_get_size_h(coin_icon->spr);
		Vector2 pos = vec2_add(coin_icon->vec2,vec2(size.x/2.0f,0.0f));
		spr_draw_cntr_h(coin_icon->spr, hud_layer,pos, 0.0f, scale1, col); 
		
		// Coin txt
		vfont_select(FONT_NAME, 48.0f);
		static Vector2 fsize = {0};
		Vector2 fsize2 = vfont_number_size(coins_earned);
		if(fsize2.x > fsize.x) fsize = fsize2;

		Vector2 str_pos = vec2_sub(coin_text->vec2, vec2(fsize.x,fsize.y/2.0f));

		float scale2 = scale1;
		if(coin_anim > 0.0f) scale2 += coin_anim - time_s();

		vfont_draw_number_ex(coins_earned, NULL, hud_layer, str_pos, col, scale2);
	}

}

void hud_render_game_over_win(float t) {

	hud_render_game_over_main(t);

	UIElement* element = uidesc_get("game_over_win");
	UIElement* button_next = uidesc_get_child(element, "next");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");	

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	// Next button
	if(hud_button(button_next, col, t)) {
		if(levels_set_next()) {
			game_request_reset();
			if(!hud_unlock_check(2))	
				malka_states_pop_multi(2);
		}
		else {
			malka_states_pop_multi(3);
		}
	}

	// Restart Button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		if(!hud_unlock_check(2))	
			malka_states_pop_multi(2);
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		hud_reset();
		if(!hud_unlock_check(3))				
			malka_states_pop_multi(3);
	}
}

void hud_render_game_over_lose(float t) {

	hud_render_game_over_main(t);

	UIElement* element = uidesc_get("game_over_lose");
	UIElement* button_restart = uidesc_get_child(element, "restart");
	UIElement* button_quit = uidesc_get_child(element, "quit");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	// Restart Button
	if(hud_button(button_restart, col, t)) {
		game_request_reset();
		malka_states_pop_multi(2);
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

		UIElement* text = uidesc_get_child(element, "text");
		UIElement* button_play = uidesc_get_child(element, "button_play");
		UIElement* button_restart = uidesc_get_child(element, "button_restart");
		UIElement* button_quit = uidesc_get_child(element, "button_quit");

		float alpha = 1.0f-fabsf(t);
		byte a = lrintf(255.0f * alpha);
		Color col = COLOR_RGBA(255, 255, 255, a);

		spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 
		// Text
		vfont_select(FONT_NAME, 48.0f); 
		const char* str = "Paused";
		static Vector2 half_size = {0.0f, 0.0f};
		if(half_size.x == 0.0f) {
			half_size = vec2_scale(vfont_size(str), 0.5f);
		}
		vfont_draw(str, hud_layer, vec2_sub(text->vec2, half_size), col);

		// Play (continue) button
		if(hud_button(button_play, col, t)) {
			malka_states_pop();		
		}

		// Restart button
		if(hud_button(button_restart, col, t)) {
			game_request_reset();
			malka_states_pop_multi(2);	
		}

		// Quit button
		if(hud_button(button_quit, col, t)) {
			hud_reset();
			malka_states_pop_multi(3);
		}		
}

uint hud_scroll_get_selection(float value, float increment, uint elements){
	uint result = 0;
	float min = fabsf(increment * elements);
	for(uint i = 0; i < elements;i++){
		float t = fabsf(value - (i * increment) );
		if(t <= min){
			result = i;
			min = t;
		}
	}
	return result;
}

float hud_scroll(float xpos, float inc, uint elements, float t){
	static float delta = 0.0f;
	static float x1 = 0.0f;
	static float x2 = 0.0f;
	static float current_x = 0.0f;
	static float prev_x = 0.0f;
	static bool hold = false;
	static bool release = false;
	static float start = 0.0f;

	Touch* touch = touches_get();
	if(touch && touch->hit_pos.y < v_height*0.61f && t == 0.0f){

		x1 = touch->hit_pos.x;
		x2 = touch->pos.x;

		if(!hold){
			start = time_s();
			current_x = touch->hit_pos.x;
			prev_x = touch->pos.x;
			hold = true;
 		} else {
 			prev_x = current_x;
			current_x = touch->pos.x;
		}
		delta = clamp(-100.0f,100.0f,prev_x - current_x);	

		if(delta < 0.0f){
			if(xpos < 0){
				//regular scroll speed within main bounds
			} else if (xpos > 0.0f && xpos < inc/2.0f){
				// Damping when trying to scroll outside bounds
				delta *= 0.15f;
			} else {
				// Disable scroll past certain point
				delta = 0.0f;
			}
		} else {
			if(xpos > (elements-1) * -inc){
				//regular scroll speed within main bounds
			} else if (xpos < (elements-1) * -inc && (xpos > (elements-0.5) * -inc) ){
				// Damping when trying to scroll outside bounds
				delta *= 0.15f;
			} else {
				// Disable scroll past certain point
				delta = 0.0f;
			}
		}

		release = false;

	} else {
		if(!release){
			//printf("delta: %f\n",fabsf(x1 - x2) );			
			//printf("time: %f\n",time_s() - start);			
			release = true;
			if(fabsf(x1 - x2) > 1.0f && time_s() - start <= 0.2f){
				if(x1 - x2 > 0.0f) 
					delta = fabsf(inc / 4.0f);
				else
					delta = -fabsf(inc / 4.0f);
			}
		}
		hold = false;
		uint selection = hud_scroll_get_selection(xpos,-inc,elements);
		xpos = lerp(xpos,selection * -inc,0.1f);
		delta *= 0.8f;	
	}

	xpos -= delta;
	return xpos;
}
