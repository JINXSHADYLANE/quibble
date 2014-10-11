#include "powerup_help.h"

#include <uidesc.h>
#include <vfont.h>

#include "common.h"
#include "obj_types.h"
#include "game.h"
#include "hud.h"

static uint item_count = 4;

static bool powerup_help_update(void) {
	game_update_empty();
	return true;
}

static bool powerup_help_render(float t) {
	if(t != 0.0f)
		game_update_empty();

	UIElement* element = uidesc_get("powerup_help");

	UIElement* text_elem2 = uidesc_get_child(element, "text2");
	UIElement* text_elem3 = uidesc_get_child(element, "text3");	

	UIElement* star = uidesc_get_child(element, "star");
	UIElement* item = uidesc_get_child(element, "item");	
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float state_alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * state_alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 

	static float xpos = 0.0f;
	static float inc = 600.0f;

	xpos = hud_scroll(xpos, inc, item_count, t);

	for(uint i = 0; i < item_count; ++i) {
		Vector2 off = vec2(xpos + i * inc, 0.0f);
		float d = normalize(fabsf(off.x), 0.0f, inc * (item_count-1));
		float scroll_alpha = 1.0f / exp(PI*d);
		byte a2 = lrintf(255.0f * scroll_alpha * state_alpha);
		Color col2 = COLOR_RGBA(255, 255, 255, a2);

		// Star
		spr_draw_cntr_h(star->spr, hud_layer, vec2_add(off, star->vec2), time_s()/10.0f, 1.0f, col2);

		// Item
		const char* item_img = powerup_params[i].unlocked_spr;
		spr_draw_cntr(item_img, hud_layer, vec2_add(off, item->vec2), 0.0f, 1.0f, col2);

		// Description text
		const char* text3 = powerup_params[i].description;
		vfont_select(FONT_NAME, 32.0f);
		Vector2 half_size3 = vec2_scale(vfont_size(text3), 0.5f);
		vfont_draw(text3, hud_layer+1, vec2_add(off, vec2_sub(text_elem3->vec2,half_size3)), col2);	

		// Item name
		const char* text2 = powerup_params[i].name;
		vfont_select(FONT_NAME, 48.0f);
		Vector2 half_size2 = vec2_scale(vfont_size(text2), 0.5f);
		vfont_draw(text2, hud_layer+1, vec2_add(off, vec2_sub(text_elem2->vec2,half_size2)), col2);	

	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	return true;
}

StateDesc powerup_help_state = {
	.update = powerup_help_update,
	.render = powerup_help_render
};

