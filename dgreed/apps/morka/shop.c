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

uint coins = 1;

bool powerups[POWERUP_COUNT];
static float powerup_appear[POWERUP_COUNT] = {0};

static void shop_init(void) {

}

static void shop_close(void) {

}

static void shop_preenter(void) {
	player = keyval_get_int("player_character",0);

	coins = keyval_get_int("coins",0);
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
			Vector2 pos = vec2_add(powerup_place, vec2(x_offset, -size.y) );
			spr_draw_cntr_h(spr, hud_layer, pos, 0.0f, 1.0f, col_30);

			if(powerups[i]){

				if(powerup_appear[i] == 0.0f) powerup_appear[i] = time_s() + duration;

				float td = normalize(ts,powerup_appear[i]-duration,powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				alpha_txt = lrintf(255.0f * (1-td) * alpha);

				y_offset = sin(PI*td/2.0f) * -size.y;

			} else {

				if(powerup_appear[i] > 0.0f)
					powerup_appear[i] = -(time_s() + duration);			

				float td = normalize(ts,-powerup_appear[i]-duration,-powerup_appear[i]);
				td = clamp(0.0f,1.0f,td);

				alpha_txt = lrintf(255.0f * alpha);

				if(td == 1.0f)
					powerup_appear[i] = 0.0f;

				y_offset = -size.y + (sin(PI*td/2.0f) * size.y);

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
				sprintf(str, "%dc",powerup_params[i].cost);
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

static bool shop_render(float t) {

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

	static float current_x = 0.0f;
	static float prev_x = 0.0f;
	static bool hold = false;
	static bool release = false;

	static float start = 0.0f;

	Touch* touch = touches_get();
	if(touch && touch->hit_pos.y < 467.0f){

		if(!hold){
			start = time_s();
			current_x = touch->hit_pos.x;
			prev_x = touch->hit_pos.x;
			hold = true;
 		} else {
 			prev_x = current_x;
			current_x = touch->pos.x;
		}
		delta = clamp(-100.0f,100.0f,prev_x - current_x);	
		//printf("%f\n",delta );

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
			release = true;
		//	printf("delta time: %.2f <= 0.15\n",time_s() - start);
			if(fabsf(delta) > 5.0f && time_s() - start <= 0.15f){
				if(delta > 0.0f) 
					delta = 100.0f;
				else
					delta = -100.0f;

		//		printf("flick\n");
			}
		}
		hold = false;
		xpos = lerp(xpos,find_closest_pos(xpos,-inc),0.1f);
		delta *= 0.8f;	
	}

	xpos -= delta;

	for(uint i = 0; i < character_count;i++){

		Vector2 offset = vec2(xpos + i * inc,0.0f);

		// Character icon
		SprHandle icon = sprsheet_get_handle(default_characters[i].icon);
		Vector2 size = sprsheet_get_size_h(icon);
		Vector2 icon_pos = character_icon->vec2;
		icon_pos.x -= size.x / 2.0f;
		spr_draw_cntr_h(icon, hud_layer,vec2_add(icon_pos,offset), 0.0f, 1.0f, col);

		// Character txt
		vfont_select(FONT_NAME, 38.0f);

		vfont_draw(default_characters[i].name, hud_layer, vec2_add(character_name->vec2,offset), col);	

		// Stats
		const char* speed = "Speed";
		vfont_draw(speed, hud_layer, vec2_add(speed_txt->vec2,offset), col);	
		const char* jump = "Jump";
		vfont_draw(jump, hud_layer, vec2_add(jump_txt->vec2,offset), col);			
	}

	// Play button
	if(hud_button(button_play, col, t)) {
		game_request_reset();
		malka_states_push("game");
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
