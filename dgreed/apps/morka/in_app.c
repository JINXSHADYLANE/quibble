#include "in_app.h"

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

#ifdef TARGET_IOS
	#include "iap.h"
#else
 
#endif

extern uint coins;
extern uint coins_original;

static void in_app_init(void) {

}

static void in_app_close(void) {

}

static void in_app_preenter(void) {

}

static void in_app_enter(void) {
	game_pause();
}

static void in_app_leave(void) {
	keyval_set_int("coins",coins_original);
}

static bool in_app_update(void) {
	game_update_empty();
	return true;
}

static bool in_app_render(float t) {
	if(t != 0.0f) game_update_empty();
	
	UIElement* element = uidesc_get("in_app");
	UIElement* coin_icon = uidesc_get_child(element, "coin_icon");
	UIElement* coin_text = uidesc_get_child(element, "coin_text");

	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float state_alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * state_alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, v_width, v_height), col); 

	// Coin icon
	spr_draw_cntr_h(coin_icon->spr, hud_layer+1,coin_icon->vec2, 0.0f, 1.0f, col);
	// Coin txt
	vfont_select(FONT_NAME, 38.0f);
	char str[32];
	sprintf(str, "%u",coins);
	vfont_draw(str, hud_layer+1, coin_text->vec2, col);

	vfont_select(FONT_NAME, 48.0f);

	// Buttons
	for(int i = 1; i <= 3; i++){
		char element_name[16];
		sprintf(element_name, "buy_%d", i);
		UIElement* elem = uidesc_get_child(element, element_name);

		char txt[32];
		uint value;
		switch(i){
			case 1:
				sprintf(txt, "Buy 100c for 0.99$");
				value = 100;
			break;

			case 2:
				sprintf(txt, "Buy 300c for 1.99$");
				value = 300;
			break;

			default:
				sprintf(txt, "Buy 1000c for 2.99$");
				value = 1000;
			break;
		}
	
		Vector2 half_size = vec2_scale(vfont_size(txt), 0.5f);
		vfont_draw(txt, hud_layer, vec2_sub(elem->vec2, half_size), col);
		Vector2 dim = vec2_scale(half_size, 2.0f);

		if(hud_button_rect(empty_spr,elem->vec2,dim,col,t)){
			coins += value;
			coins_original += value;
		}
	
	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	return true;
}

StateDesc in_app_state = {
	.init = in_app_init,
	.close = in_app_close,
	.enter = in_app_enter,
	.preenter = in_app_preenter,
	.leave = in_app_leave,
	.update = in_app_update,
	.render = in_app_render
};
