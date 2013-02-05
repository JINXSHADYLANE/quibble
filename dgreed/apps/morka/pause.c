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
	printf("entering pause\n");
}

static void pause_leave(void) {
	printf("leaving pause\n");
}

static bool pause_update(void) {
	return true;
}

static bool pause_render(float t) {
	// Game scene
	//if(t > 0.0f) game_render(t);

	// Pause overlay
	UIElement* element = uidesc_get("pause");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* button_play = uidesc_get_child(element, "button_play");
	UIElement* button_restart = uidesc_get_child(element, "button_restart");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), COLOR_WHITE); 
	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Paused";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(text->vec2, half_size), COLOR_WHITE);

	// Play (continue) button
	spr_draw_cntr_h(button_play->spr, layer, button_play->vec2, 0.0f, 1.0f, COLOR_WHITE);
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_play->vec2)) < 40.0f * 40.0f) {
			game_unpause();
			time_scale(1.0f);
			malka_states_pop();
		}
	}

	// Restart button
	spr_draw_cntr_h(button_restart->spr, layer, button_restart->vec2, 0.0f, 1.0f, COLOR_WHITE);
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_restart->vec2)) < 40.0f * 40.0f) {
			game_reset();
			game_unpause();
			time_scale(1.0f);
			malka_states_pop();
		}
	}

	// Quit button
	spr_draw_cntr_h(button_quit->spr, layer, button_quit->vec2, 0.0f, 1.0f, COLOR_WHITE);
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_quit->vec2)) < 40.0f * 40.0f) {
			game_unpause();
			time_scale(1.0f);
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
