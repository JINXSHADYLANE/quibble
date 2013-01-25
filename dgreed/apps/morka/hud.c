#include "hud.h"

#include <uidesc.h>
#include <vfont.h>

#define hud_layer 7

#include "common.h"

#ifndef __APPLE__
	#define FONT_NAME ASSETS_DIR "Chango-Regular.ttf"
#else
	#define FONT_NAME "Baskerville-Bold"
#endif

extern float rabbit_remaining_time;
extern float rabbit_distance;
extern float rabbit_current_distance;

extern void game_reset(void);
extern void game_pause(void);
extern void game_unpause(void);
extern bool game_is_paused(void);

extern bool game_over;
float game_over_alpha = 0.0f;

static uint last_combo = 0;
static uint current_combo = 0;
static uint longest_combo = 0;
static float combo_flip_t = 0.0f;

uint clocks_collected = 0;

static void _hud_render_ui(UIElement* element, uint layer) {
	// Render
	if(element->members & UI_EL_SPR) {
		RectF dest = rectf_null();
		if(element->members & UI_EL_RECT) {
			dest = element->rect;
		}
		else if(element->members & UI_EL_VEC2) {
			dest.left = element->vec2.x;
			dest.top = element->vec2.y;
		}

		spr_draw_h(element->spr, layer, dest, COLOR_WHITE);
	}

	// Iterate over children
	UIElement* child;
	list_for_each_entry(child, &element->child_list, list) {
		_hud_render_ui(child, layer-1);
	}
}

static void _hud_render_game_over(UIElement* element, uint layer, float alpha) {
	UIElement* text = uidesc_get_child(element, "text");
	UIElement* dist_text = uidesc_get_child(element, "distance_text");
	UIElement* combo_text = uidesc_get_child(element, "combo_text");
	UIElement* button = uidesc_get_child(element, "button");

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
	sprintf(distance_str, "You ran %d meters", (int)lrintf(rabbit_distance));
	Vector2 half_dist_size = vec2_scale(vfont_size(distance_str), 0.5f);
	vfont_draw(distance_str, layer, vec2_sub(dist_text->vec2, half_dist_size), col);

	// Combo text
	char combo_str[32];
	sprintf(combo_str, "Longest combo - %u", longest_combo);
	Vector2 half_combo_size = vec2_scale(vfont_size(combo_str), 0.5f);
	vfont_draw(combo_str, layer, vec2_sub(combo_text->vec2, half_combo_size), col);
	
	// Button
	spr_draw_cntr_h(button->spr, layer, button->vec2, 0.0f, 1.0f, col);	

	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button->vec2)) < 40.0f * 40.0f) {
			game_reset();
		}
	}
}

static void _hud_render_pause(UIElement* element, uint layer) {
	UIElement* text = uidesc_get_child(element, "text");
	UIElement* button = uidesc_get_child(element, "button");

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), COLOR_WHITE); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Paused";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(text->vec2, half_size), COLOR_WHITE);

	spr_draw_cntr_h(button->spr, layer, button->vec2, 0.0f, 1.0f, COLOR_WHITE);

	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button->vec2)) < 40.0f * 40.0f) {
			game_unpause();
			time_scale(1.0f);
		}
	}
}

static void _hud_render_combo(UIElement* element, uint layer, uint mult, float t) {
	vfont_select(FONT_NAME, 48.0f);


	// Combine two smootherstep functions into a new one,
	// where x stays constant for a while near t = 0.5
	float x;
	if(t < 0.5f)
		x = smootherstep(-1.0f, 0.0f, t * 2.0f);
	else
		x = smootherstep(0.0f, 1.0f, (t - 0.5f) * 2.0f);

	// Classic sine there-and-back-again for alpha
	float a = MAX(0.0f, sinf((t*1.4f - 0.2f) * PI));

	char text[64];
	sprintf(text, "Combo x%u", mult);
	Vector2 half_size = vec2_scale(vfont_size(text), 0.5f);

	Vector2 pos = vec2_sub(element->vec2, half_size);
	pos.x -= x * 200.0f;
	vfont_draw(text, layer, pos, COLOR_FTRANSP(a));
}

void hud_init(void) {
	vfont_init_ex(1024, 1024);
}

void hud_close(void) {
	vfont_close();
}

void hud_trigger_combo(uint multiplier) {
	last_combo = current_combo;
	current_combo = multiplier;
	longest_combo = MAX(longest_combo, current_combo);
	combo_flip_t = time_s();
}

void hud_render(void) {
	UIElement* pause = uidesc_get("hud_pause");
	_hud_render_ui(pause, hud_layer);
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, pause->vec2)) < 40.0f * 40.0f) {
			time_scale(0.0f);
			game_pause();
		}
	}

	game_over_alpha += game_over ? 0.03f : -0.05f;
	game_over_alpha = clamp(0.0f, 1.0f, game_over_alpha);

	static bool showed_game_over_last_frame = false;
	if(game_over_alpha > 0.0f) {
		showed_game_over_last_frame = true;
		_hud_render_game_over(uidesc_get("game_over"), hud_layer+1, game_over_alpha);
	}
	else {
		if(showed_game_over_last_frame) {
			showed_game_over_last_frame = false;
			longest_combo = 0;
		}
		if(game_is_paused()) {
			UIElement* pause_screen = uidesc_get("pause");
			_hud_render_pause(pause_screen, hud_layer+1);
		}
	}

	UIElement* combo_text = uidesc_get("combo_text");
	float ts = time_s();
	float t = (ts - combo_flip_t) / 0.4f;
	if(t < 1.0f) {
		if(current_combo)
			_hud_render_combo(combo_text, hud_layer+1, current_combo, t * 0.5f);
		if(last_combo)
			_hud_render_combo(combo_text, hud_layer+1, last_combo, 0.5f + t * 0.5f);
	}
	else {
		if(current_combo)
			_hud_render_combo(combo_text, hud_layer+1, current_combo, 0.5f);
	}
}

