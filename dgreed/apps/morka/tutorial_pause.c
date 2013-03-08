#include "tutorial_pause.h"
#include "game.h"
#include "common.h"
#include "tutorials.h"
#include "minimap.h"
#include "hud.h"
#include <uidesc.h>
#include <vfont.h>

extern bool button_click;

static void tutorial_pause_init(void) {
}

static void tutorial_pause_close(void) {
}

static void tutorial_pause_enter(void) {
}

static void tutorial_pause_leave(void) {
}

static bool tutorial_pause_update(void) {
	minimap_update_places();
	return true;
}

static bool tutorial_pause_render(float t) {
	// Game scene
	if(t != 0) {
		minimap_update_places();
		game_update();
	}
	if(t == 0) game_render(0);
	tutorials_render(t);

	// TODO: refactor repeating code to function in hud
	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);	
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

	return true;
}

StateDesc tutorial_pause_state = {
	.init = tutorial_pause_init,
	.close = tutorial_pause_close,
	.enter = tutorial_pause_enter,
	.leave = tutorial_pause_leave,
	.update = tutorial_pause_update,
	.render = tutorial_pause_render
};
