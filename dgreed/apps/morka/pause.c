#include "pause.h"
#include "game.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

extern bool button_click;

static void pause_init(void) {
}

static void pause_close(void) {
}

static void pause_enter(void) {
}

static void pause_leave(void) {
}

static bool pause_update(void) {
	return true;
}

static bool pause_render(float t) {
	// Game scene
	if(t == 0) game_render(0);

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
	spr_draw_cntr_h(button_play->spr, layer, button_play->vec2, 0.0f, 1.0f, col);
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_play->vec2)) < 40.0f * 40.0f) {
			malka_states_pop();
		}
	}

	// Restart button
	spr_draw_cntr_h(button_restart->spr, layer, button_restart->vec2, 0.0f, 1.0f, col);
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_restart->vec2)) < 40.0f * 40.0f) {
			game_request_reset();
			malka_states_pop();
		}
	}

	// Quit button
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

StateDesc pause_state = {
	.init = pause_init,
	.close = pause_close,
	.enter = pause_enter,
	.leave = pause_leave,
	.update = pause_update,
	.render = pause_render
};
