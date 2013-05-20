#include "item_unlock.h"

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

static float fade_start;
static float fade_end;

static char text1[64];
static char text2[64];
static char text3[64];
static SprHandle image = (SprHandle)MAX_UINT64;

static float s = 1.0f;
static float ds = 1.0f;

static float s2 = 1.0f;
static float ds2 = 1.0f;

static uint state_num = 1;

void item_unlock_set(ObjItemUnlockParams * params){
	if(params->spr)
		image = sprsheet_get_handle(params->spr);
	if(params->text1)
			strncpy(text1, params->text1, 64);
	if(params->text2)
			strncpy(text2, params->text2, 64);
	if(params->text3)
			strncpy(text3, params->text3, 64);
	state_num = params->state_num + 1;								 
}

static void item_unlock_init(void) {
}

static void item_unlock_close(void) {

}

static void item_unlock_preenter(void) {
	s = 1.0f;
	ds = 1.0f;
	s2 = 1.0f;
	ds2 = 0.4f;		
	fade_start = time_s() + 0.5f;
	fade_end = fade_start + 0.6f;	
}

static void item_unlock_enter(void) {
	game_pause();
}

static void item_unlock_leave(void) {

}

static bool item_unlock_update(void) {
	game_update_empty();
	return true;
}

static bool item_unlock_render(float t) {
	if(t != 0.0f) game_update_empty();
	
	UIElement* element = uidesc_get("item_unlock");

	UIElement* text_elem1 = uidesc_get_child(element, "text1");
	UIElement* text_elem2 = uidesc_get_child(element, "text2");
	UIElement* text_elem3 = uidesc_get_child(element, "text3");	

	UIElement* star = uidesc_get_child(element, "star");
	UIElement* item = uidesc_get_child(element, "item");	
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float state_alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * state_alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	byte a2 = lrintf(255.0f * state_alpha * 0.5f);
	Color star_col = COLOR_RGBA(255, 255, 255, a2);

	float dt = time_delta() / 1000.0f;

	// Item image animation
	float f = 30.0f * (1.0f - s);
	ds += f * dt;
	s += (ds * 10.0f) * dt;
	ds *= 0.9f;

	// Star animation
	float f2 = 25.0f * (1.0f - s2);
	ds2 += f2 * dt;
	s2 += (ds2 * 8.0f) * dt;
	ds2 *= 0.9f;	

	float ts = time_s();
	byte a3 = 0;

	if(ts >= fade_start && ts < fade_end ){
		float x = normalize(ts,fade_start,fade_end);
		x = clamp(0.0f,1.0f,x);

		x = sinf(x * PI/2.0f);
		a3 = lrintf(255.0f * state_alpha * x);
	} else if( ts > fade_start && ts >= fade_end){
		a3 = lrintf(255.0f * state_alpha);
	}
	Color txt_col = COLOR_RGBA(255, 255, 255, a3);


	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, v_width, v_height), col); 

	// Spinning star
	spr_draw_cntr_h(star->spr, hud_layer, star->vec2, 0.0f, s2, star_col);

	if(image != empty_spr)
		spr_draw_cntr_h(image, hud_layer, item->vec2, 0.0f, s, col);


	vfont_select(FONT_NAME, 32.0f);

	// Unlocked:
	Vector2 half_size1 = vec2_scale(vfont_size(text1), 0.5f);
	vfont_draw(text1, hud_layer+1, vec2_sub(text_elem1->vec2,half_size1), txt_col);

	// Description text
	Vector2 half_size3 = vec2_scale(vfont_size(text3), 0.5f);
	vfont_draw(text3, hud_layer+1, vec2_sub(text_elem3->vec2,half_size3), txt_col);	

	vfont_select(FONT_NAME, 48.0f);
	// Item title
	Vector2 half_size2 = vec2_scale(vfont_size(text2), 0.5f);
	vfont_draw(text2, hud_layer+1, vec2_sub(text_elem2->vec2,half_size2), txt_col);	


	// Quit button
	if(hud_button(button_quit, col, t)) {
		if(!hud_unlock_check(state_num))		
			malka_states_pop_multi(state_num);
	}

	return true;
}

StateDesc item_unlock_state = {
	.init = item_unlock_init,
	.close = item_unlock_close,
	.enter = item_unlock_enter,
	.preenter = item_unlock_preenter,
	.leave = item_unlock_leave,
	.update = item_unlock_update,
	.render = item_unlock_render
};
