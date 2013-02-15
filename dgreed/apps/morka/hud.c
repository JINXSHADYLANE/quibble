#include "hud.h"
#include "minimap.h"

#include <vfont.h>
#include <malka/ml_states.h>

#include "common.h"

extern void game_reset(void);
extern void game_pause(void);
extern void game_unpause(void);
extern bool game_is_paused(void);

static uint last_combo = 0;
static uint current_combo = 0;

static float combo_flip_t = 0.0f;

extern uint longest_combo;

extern ObjRabbit* rabbit;

void _hud_render_ui(UIElement* element, uint layer) {
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

static void _hud_render_combo(UIElement* element, uint layer, uint mult, float t) {
	vfont_select(FONT_NAME, 38.0f);

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

void hud_render(float t) {
	UIElement* token_icon = uidesc_get("token_icon");
	spr_draw_cntr_h(token_icon->spr, hud_layer,token_icon->vec2, 0.0f, 1.0f, COLOR_WHITE);

	UIElement* token_text = uidesc_get("token_text");
	vfont_select(FONT_NAME, 38.0f);
	char str[32];
	sprintf(str, "%d",rabbit->data->tokens);
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, hud_layer, token_text->vec2, COLOR_WHITE);

	UIElement* pause = uidesc_get("hud_pause");
	_hud_render_ui(pause, hud_layer);
	if(touches_down() && t == 0.0f) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, pause->vec2)) < 40.0f * 40.0f) {
			game_pause();
			malka_states_push("pause");
		}
	}

	UIElement* combo_text = uidesc_get("combo_text");
	float ts = time_s();
	float ct = (ts - combo_flip_t) / 0.4f;
	if(ct < 1.0f) {
		if(current_combo)
			_hud_render_combo(combo_text, hud_layer+1, current_combo, ct * 0.5f);
		if(last_combo)
			_hud_render_combo(combo_text, hud_layer+1, last_combo, 0.5f + ct * 0.5f);
	}
	else {
		if(current_combo)
			_hud_render_combo(combo_text, hud_layer+1, current_combo, 0.5f);
	}

	// Minimap
	minimap_draw();

}

