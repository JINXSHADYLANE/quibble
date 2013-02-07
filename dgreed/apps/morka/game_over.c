#include "game_over.h"
#include "game.h"
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
	return true;
}

static bool game_over_render(float t) {
	// Game scene
	//if(t != 0.f)game_update();
	game_render(0);

	// Game Over overlay
	UIElement* element = uidesc_get("game_over");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* dist_text = uidesc_get_child(element, "distance_text");
	UIElement* combo_text = uidesc_get_child(element, "combo_text");
	UIElement* button_restart = uidesc_get_child(element, "button_restart");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

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

	// Distance text
	char distance_str[32];
	sprintf(distance_str, "You ran %d meters", (int)lrintf(display_distance));
	Vector2 half_dist_size = vec2_scale(vfont_size(distance_str), 0.5f);
	vfont_draw(distance_str, layer, vec2_sub(dist_text->vec2, half_dist_size), col);

	// Combo text
	char combo_str[32];
	sprintf(combo_str, "Longest combo - %u", longest_combo);
	Vector2 half_combo_size = vec2_scale(vfont_size(combo_str), 0.5f);
	vfont_draw(combo_str, layer, vec2_sub(combo_text->vec2, half_combo_size), col);
	
	// Restart Button
	spr_draw_cntr_h(button_restart->spr, layer, button_restart->vec2, 0.0f, 1.0f, col);	
	if(touches_down()) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_restart->vec2)) < 40.0f * 40.0f) {
			game_request_reset();
			malka_states_pop();
		}
	}

	// Quit Button
	spr_draw_cntr_h(button_quit->spr, layer, button_quit->vec2, 0.0f, 1.0f, col);	
	if(touches_down()) {
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
