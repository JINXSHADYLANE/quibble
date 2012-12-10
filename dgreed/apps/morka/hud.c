#include "hud.h"

#include <uidesc.h>
#include <vfont.h>

#define hud_layer 7

extern float rabbit_remaining_time;
extern float rabbit_distance;
extern float rabbit_current_distance;

extern bool game_over;
float game_over_alpha = 0.0f;

typedef struct {
	uint multiplier;
	float t;
} Combo;

#define max_combos 4
static Combo combos[4];
static uint n_combos = 0;

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
	for(uint i = 0; i < 4; ++i) {
		digits[3-i] = number % 10;
		number /= 10;
	}

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

static void _hud_render_combo(UIElement* element, uint layer, uint mult, float t) {
	vfont_select("Baskerville-Bold", 48.0f);

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

	//Vector2 pos = vec2_sub(element->vec2, half_size);
	Vector2 pos = vec2_sub(vec2(512.0f, 80.0f), half_size);
	pos.x -= x * 200.0f;
	vfont_draw(text, layer, pos, COLOR_FTRANSP(a));
}

void hud_init(void) {
	vfont_init();
}

void hud_close(void) {
	vfont_close();
}

void hud_trigger_combo(uint multiplier) {
	assert(n_combos < max_combos);
	uint i = n_combos++;
	combos[i].multiplier = multiplier;
	combos[i].t = time_s();
}

void hud_render(void) {
	_hud_render_ui(uidesc_get("hud_pause"), hud_layer);

	UIElement* clock = uidesc_get("hud_clock");
	_hud_render_ui(clock, hud_layer);
	_hud_render_distance(clock, hud_layer+1, lrintf(rabbit_current_distance));

	UIElement* hud_clock_needle = uidesc_get("hud_clock_needle");

	float angle = (rabbit_remaining_time / 60.0f) * 2.0f * PI;
	_hud_render_clock_needle(hud_clock_needle, hud_layer, angle);

	UIElement* combo_text = uidesc_get("combo_text");
	float ts = time_s();
	for(uint i = 0; i < n_combos; ++i) {
		float t = (ts - combos[i].t) / 0.8f;
		if(t > 1.0f) 
			combos[i--] = combos[--n_combos];
		else
			_hud_render_combo(combo_text, hud_layer+1, combos[i].multiplier, t);
	}

	game_over_alpha += game_over ? 0.03f : -0.05f;
	game_over_alpha = clamp(0.0f, 1.0f, game_over_alpha);

	if(game_over_alpha > 0.0f) {
		_hud_render_game_over(uidesc_get("game_over"), hud_layer+1, game_over_alpha);
	}
}

