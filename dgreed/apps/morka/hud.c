#include "hud.h"

#include <uidesc.h>
#include <vfont.h>

#define hud_layer 7

extern float rabbit_remaining_time;
extern float rabbit_distance;
extern float rabbit_current_distance;

extern bool game_over;
float game_over_alpha = 0.0f;

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

static void _hud_render_clock_needle(UIElement* element, uint layer, float angle) {
	spr_draw_cntr_h(element->spr, layer, element->vec2, angle, 1.0f, COLOR_WHITE);
}

extern void game_reset(void);

static void _hud_render_game_over(UIElement* element, uint layer, float alpha) {
	UIElement* text = uidesc_get_child(element, "text");
	UIElement* dist_text = uidesc_get_child(element, "distance_text");
	UIElement* button = uidesc_get_child(element, "button");

	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	// Text
	vfont_select("Baskerville-Bold", 48.0f); 
	const char* str = "The time is over.";
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

	// Button
	spr_draw_cntr_h(button->spr, layer, button->vec2, 0.0f, 1.0f, col);	

	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button->vec2)) < 40.0f * 40.0f) {
			game_reset();
		}
	}
}

static void _hud_render_distance(UIElement* element, uint layer, uint number) {
	uint digits[4];
	digits[3] = number % 10;
	digits[2] = (number / 10) % 10;
	digits[1] = (number / 100) % 10;
	digits[0] = (number / 1000);

	vfont_select("Baskerville-Bold", 18.0f);

	UIElement* digit1 = uidesc_get_child(element, "digit1");
	UIElement* digit_offset = uidesc_get_child(element, "digit_offset");
	Vector2 cursor = digit1->vec2;
	char str[] = {'\0', '\0'};
	for(uint i = 0; i < 4; ++i) {
		str[0] = '0' + digits[i];
		vfont_draw(str, layer, cursor, COLOR_BLACK);
		cursor = vec2_add(cursor, digit_offset->vec2);
		cursor.x = floorf(cursor.x);
	}
}

void hud_init(void) {
	vfont_init();
}

void hud_close(void) {
	vfont_close();
}

void hud_render(void) {
	_hud_render_ui(uidesc_get("hud_pause"), hud_layer);

	UIElement* clock = uidesc_get("hud_clock");
	_hud_render_ui(clock, hud_layer);
	_hud_render_distance(clock, hud_layer+1, lrintf(rabbit_current_distance));

	UIElement* hud_clock_needle = uidesc_get("hud_clock_needle");

	float angle = (rabbit_remaining_time / 60.0f) * 2.0f * M_PI;
	_hud_render_clock_needle(hud_clock_needle, hud_layer, angle);

	game_over_alpha += game_over ? 0.03f : -0.05f;
	game_over_alpha = clamp(0.0f, 1.0f, game_over_alpha);

	if(game_over_alpha > 0.0f) {
		_hud_render_game_over(uidesc_get("game_over"), hud_layer+1, game_over_alpha);
	}
}

