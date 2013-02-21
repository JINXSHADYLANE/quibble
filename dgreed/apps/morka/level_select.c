#include "level_select.h"
#include "game.h"
#include "levels.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

static void level_select_init(void) {

}

static void level_select_close(void) {

}

static void level_select_enter(void) {
	game_pause();
}

static void level_select_leave(void) {

}

static bool level_select_update(void) {
	game_update_empty();
	return true;
}

static bool level_select_render(float t) {
	// Game scene
	if(t != 0) game_update_empty();
	if(t == 0) game_render(0);

	UIElement* element = uidesc_get("level_select");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Pick a level:";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(text->vec2, half_size), col);

	// Five buttons for levels
	for(int i = 1; i <= 5; i++){
		char level_name[16];
		sprintf(level_name, "level%d", i);
		UIElement* level = uidesc_get_child(element, level_name);
		spr_draw_cntr_h(level->spr, layer, level->vec2, 0.0f, 1.0f, col);

		UIElement* offset = uidesc_get_child(level, "offset");

		if(touches_down() && t == 0.0f) {
			Touch* t = touches_get();
			if(vec2_length_sq(vec2_sub(t[0].hit_pos,
			vec2_add(level->vec2,offset->vec2))) < 70.0f * 70.0f) {

				levels_reset(level_name);
				game_request_reset();
				malka_states_push("game");
			}
		}	
	}
	return true;
}

StateDesc level_select_state = {
	.init = level_select_init,
	.close = level_select_close,
	.enter = level_select_enter,
	.leave = level_select_leave,
	.update = level_select_update,
	.render = level_select_render
};
