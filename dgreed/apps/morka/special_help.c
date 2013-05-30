#include "special_help.h"

#include <uidesc.h>
#include <vfont.h>

#include "common.h"
#include "game.h"
#include "hud.h"

static bool special_help_update(void) {
	game_update_empty();
	return true;
}

static bool special_help_render(float t) {
	UIElement* element = uidesc_get("special_help");

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float state_alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * state_alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 

	// Text
	const char* msg = "Win 8 first places to unlock";
	vfont_select(FONT_NAME, 32.0f);
	Vector2 half_size = vec2_scale(vfont_size(msg), 0.5f);
	vfont_draw(msg, hud_layer+1, vec2_sub(text->vec2, half_size), col);

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	return true;
}

StateDesc special_help_state = {
	.update = special_help_update,
	.render = special_help_render
};

