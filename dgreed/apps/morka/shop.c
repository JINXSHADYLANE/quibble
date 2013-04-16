#include "shop.h"

#include "characters.h"
#include "common.h"
#include "game.h"
#include "hud.h"
#include "level_select.h"
#include "minimap.h"
#include "obj_types.h"

#include <keyval.h>
#include <uidesc.h>
#include <vfont.h>

extern uint player;

// Character select variables
static float xpos = 0.0f;
static float delta = 0.0f;
const float inc = 470.0f;

uint coins = 0;
static uint coins_original = 0;

bool powerups[POWERUP_COUNT];
static float powerup_appear[POWERUP_COUNT] = {0};

static void shop_init(void) {

}

static void shop_close(void) {

}

static void shop_preenter(void) {
	player = keyval_get_int("player_character",0);

	coins = keyval_get_int("coins",0);
	coins_original = coins;
	xpos = player * -inc;

	for(uint i = 0; i < POWERUP_COUNT;i++){
		powerups[i] = false;
		powerup_appear[i] = 0.0f;
	}
}

static void shop_enter(void) {
	game_pause();
}

static void shop_leave(void) {
	keyval_set_int("player_character",player);
}

static bool shop_update(void) {
	game_update_empty();
	return true;
}

static void _render_powerups_buy(float t){
	UIElement* parent = uidesc_get("shop");
	UIElement* element = uidesc_get_child(parent, "hud_powerups");

	const float duration = 0.5f;

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	

	byte a2 = lrintf(76.0f * alpha);
	Color col_30 = COLOR_RGBA(255, 255, 255, a2);		

	byte alpha_txt = lrintf(255.0f * alpha);

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
			Vector2 pos = vec2_add(powerup_place, vec2(x_offset, -size.y / 2.0f ) );
			spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col_30);

			if(powerups[i]){

				if(powerup_appear[i] == 0.0f) powerup_appear[i] = time_s() + duration;

				float td = normalize(ts,powerup_appear[i]-duration,powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				alpha_txt = lrintf(255.0f * (1-td) * alpha);

				y_offset = sin(PI*td/2.0f) * -size.y / 2.0f;

			} else {

				if(powerup_appear[i] > 0.0f)
					powerup_appear[i] = -(time_s() + duration);			

				float td = normalize(ts,-powerup_appear[i]-duration,-powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				alpha_txt = lrintf(255.0f * alpha);

				if(td == 1.0f)
					powerup_appear[i] = 0.0f;

				y_offset = -size.y + (sin(PI*td/2.0f) * size.y / 2.0f);

				if(touches_down() && !powerups[i] && coins >= powerup_params[i].cost) {
					Touch* t = touches_get();
					if(t){
						float r_sqr = 40.0f * 40.0f;
						if(vec2_length_sq(vec2_sub(t[0].hit_pos, pos)) < r_sqr) {
							powerups[i] = true;
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
				sprintf(str, "%uc",powerup_params[i].cost);
				Vector2	half_size = vec2_scale(vfont_size(str), 0.5f);
				txt_pos = vec2_sub(txt_pos, half_size);
				txt_pos.y -= size.y - 20.0f;
				vfont_draw(str, hud_layer, txt_pos, col_txt);
			}


		}
	}
}

static float find_closest_pos(float value, float increment){
	float result = 0.0f;
	float min = fabsf(increment * character_count);
	for(uint i = 0; i < character_count;i++){
		float t = fabsf(value - (i * increment) );
		if(t <= min){
			player = i;
			result = i * increment;
			min = t;
		}
	}
	return result;
}

bool _shop_character_owned(uint i){
	char key_name[32];
	sprintf(key_name, "ulck_c%u",i);

	return i == 0 || keyval_get_bool(key_name, false);
}

bool _shop_character_buy(uint i){
	if(coins >= default_characters[i].cost){
		char key_name[32];
		sprintf(key_name, "ulck_c%u",i);

		keyval_set_bool(key_name, true);
		coins -= default_characters[i].cost;
		coins_original -= default_characters[i].cost;
		keyval_set_int("coins",coins_original);
		return true;	
	}

	return false;
}

static bool shop_render(float t) {

	vfont_select(FONT_NAME, 48.0f);

	UIElement* element = uidesc_get("shop");
	UIElement* coin_icon = uidesc_get_child(element, "coin_icon");
	UIElement* coin_text = uidesc_get_child(element, "coin_text");	
	UIElement* character_icon = uidesc_get_child(element, "character_icon");
	UIElement* character_name = uidesc_get_child(element, "character_name");
	UIElement* bar_size = uidesc_get_child(element, "bar_size");	
	UIElement* speed_txt = uidesc_get_child(element, "speed_txt");
	UIElement* speed_bar = uidesc_get_child(element, "speed_bar");	
	UIElement* jump_txt = uidesc_get_child(element, "jump_txt");
	UIElement* jump_bar = uidesc_get_child(element, "jump_bar");	
	UIElement* about_txt = uidesc_get_child(element, "about_txt");
	UIElement* cost_txt = uidesc_get_child(element, "cost_txt");	
	UIElement* button_play = uidesc_get_child(element, "button_play");
	UIElement* button_buy = uidesc_get_child(element, "button_buy");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	Vector2 pos;

	float state_alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * state_alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, v_width, v_height), col); 

	static float x1 = 0.0f;
	static float x2 = 0.0f;

	static float current_x = 0.0f;
	static float prev_x = 0.0f;

	static bool hold = false;
	static bool release = false;

	static float start = 0.0f;

	static float anim_start = 0.0f;
	static float anim_end = 0.0f;

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
			if(xpos > (character_count-1) * -inc){
				//regular scroll speed within main bounds
			} else if (xpos < (character_count-1) * -inc && (xpos > (character_count-0.5) * -inc) ){
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
					delta = 120.0f;
				else
					delta = -120.0f;
			}
		}
		hold = false;
		xpos = lerp(xpos,find_closest_pos(xpos,-inc),0.1f);
		delta *= 0.8f;	
	}

	xpos -= delta;

	for(uint i = 0; i < character_count;i++){

		Vector2 offset = vec2(xpos + i * inc,0.0f);
		float d = normalize(fabsf(offset.x),0.0f,inc * (character_count-1));

		float scroll_alpha = 1.0f / exp(PI*d);
		byte a2 = lrintf(255.0f * scroll_alpha * state_alpha);
		Color col2 = COLOR_RGBA(255, 255, 255, a2);

		Color yel = COLOR_RGBA(255, 234, 0, a2);

		byte a20 = lrintf(255.0f * scroll_alpha * state_alpha * 0.2f);
		Color col20 = COLOR_RGBA(255, 255, 255, a20);

		// Character icon
		SprHandle icon = sprsheet_get_handle(default_characters[i].icon);
		Vector2 size = sprsheet_get_size_h(icon);
		Vector2 icon_pos = character_icon->vec2;
		icon_pos.x -= size.x / 2.0f;
		icon_pos = vec2_add(icon_pos,offset);
		spr_draw_cntr_h(icon, hud_layer,icon_pos, 0.0f, 1.0f, col2);

		// Character txt
		vfont_select(FONT_NAME, 38.0f);
		vfont_draw(default_characters[i].name, hud_layer, vec2_add(character_name->vec2,offset), col2);	

		SprHandle empty = sprsheet_get_handle("position_knob");

		vfont_select(FONT_NAME, 30.0f);
		const char* speed = "Speed";
		Vector2	half_size = vec2_scale(vfont_size(speed), 0.5f);
		pos = vec2_add(speed_txt->vec2,offset);
		pos = vec2_sub(pos,vec2(0.0f,half_size.y));		
		vfont_draw(speed, hud_layer, pos, col2);

		pos = vec2_add(speed_bar->vec2,offset);

		RectF rec1 = {
			.left = pos.x,
			.top = pos.y - bar_size->vec2.y/ 2.0f,
			.right = pos.x + bar_size->vec2.x,
			.bottom = pos.y + bar_size->vec2.y/ 2.0f			
		};

		spr_draw_h(empty,hud_layer,rec1,col20);

		float speed_n = normalize(default_characters[i].speed,0.0f,600.0f);
		rec1.right = pos.x + bar_size->vec2.x * speed_n,

		spr_draw_h(empty,hud_layer,rec1,col2);		

		const char* jump = "Jump";
		half_size = vec2_scale(vfont_size(jump), 0.5f);
		pos = vec2_add(jump_txt->vec2,offset);
		pos = vec2_sub(pos,vec2(0.0f,half_size.y));
		vfont_draw(jump, hud_layer, pos, col2);

		pos = vec2_add(jump_bar->vec2,offset);

		RectF rec2 = {
			.left = pos.x,
			.top = pos.y - bar_size->vec2.y/ 2.0f,
			.right = pos.x + bar_size->vec2.x,
			.bottom = pos.y + bar_size->vec2.y/ 2.0f			
		};

		spr_draw_h(empty,hud_layer,rec2,col20);		

		float jump_n = normalize(default_characters[i].yjump*default_characters[i].xjump,0.0f,50000.0f);
		rec2.right = pos.x + bar_size->vec2.x * jump_n,

		spr_draw_h(empty,hud_layer,rec2,col2);	

		vfont_draw(default_characters[i].description, hud_layer, vec2_add(about_txt->vec2,offset), yel);	

		float t = time_s();

		if(!_shop_character_owned(i) || (i == player && t < anim_end) ){

			byte alpha_txt = lrintf(255.0f * scroll_alpha * state_alpha);
			Color col_a = COLOR_RGBA(255, 255, 255, alpha_txt);
			Vector2 pos = vec2_add(cost_txt->vec2,offset);

			if( (i == player && t < anim_end) ){

				float td = 0.0f;
				if(anim_end > 0.0f){
					td = normalize(t,anim_start,anim_end);
					td = clamp(0.0f,1.0f,td);
				}

				float y_offset = sin(PI*td/2.0f) * 30.0f;

				alpha_txt = lrintf(255.0f  * scroll_alpha * state_alpha * (1.0f - td) );
				col_a = COLOR_RGBA(255, 255, 255, alpha_txt);		

				pos.y -= y_offset;

			}

			vfont_select(FONT_NAME, 48.0f);
			char cost[32];
			sprintf(cost, "%uc",default_characters[i].cost);
			vfont_draw(cost, hud_layer, pos, col_a);
		}

	}

	// Coin icon
	spr_draw_cntr_h(coin_icon->spr, hud_layer,coin_icon->vec2, 0.0f, 1.0f, col);
	// Coin txt
	vfont_select(FONT_NAME, 38.0f);
	char str[32];
	sprintf(str, "%u",coins);
	vfont_draw(str, hud_layer, coin_text->vec2, col);

	if(_shop_character_owned(player)){
		// Play button
		if(hud_button(button_play, col, t)) {
			game_request_reset();
			malka_states_push("game");
		}
	} else {
		// Buy button
		if(hud_button(button_buy, col, t)) {
			if(_shop_character_buy(player)){
				anim_start = time_s();
				anim_end = anim_start + 0.7f;
			}
		}
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	_render_powerups_buy(t);

	return true;
}

StateDesc shop_state = {
	.init = shop_init,
	.close = shop_close,
	.enter = shop_enter,
	.preenter = shop_preenter,
	.leave = shop_leave,
	.update = shop_update,
	.render = shop_render
};
