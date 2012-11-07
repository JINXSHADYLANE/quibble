#include "hud.h"
#include <uidesc.h>

#define hud_layer 7

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

void hud_init(void) {

}

void hud_close(void) {

}

void hud_render(void) {
	_hud_render_ui(uidesc_get("hud_pause"), hud_layer);
	_hud_render_ui(uidesc_get("hud_clock"), hud_layer);

	UIElement* hud_clock_needle = uidesc_get("hud_clock_needle");
	_hud_render_clock_needle(hud_clock_needle, hud_layer, time_s()/6.0f);
}

