#include "menus.h"

#include <system.h>
#include <font.h>
#include <gui.h>
#include <gfx_utils.h>

MenuState menu_state;
MenuState menu_transition;
float menu_transition_t;

#define MENU_BACKGROUND_LAYER 1
#define MENU_PANEL_LAYER  2
#define MENU_SHADOW_LAYER 3
#define MENU_TEXT_LAYER 12

#define BACKGROUND_IMG "greed_assets/back_chapter_1.png"
#define MENU_ATLAS_IMG "greed_assets/menu_atlas.png"

TexHandle background;
TexHandle menu_atlas;

static RectF background_source = {0.0f, 0.0f, 480.0f, 320.0f};
static RectF panel_source = {0.0f, 0.0f, 360.0f, 225.0f};
static RectF title_source = {362.0f, 0.0f, 362.0f + 70.0f, 256.0f};
static RectF separator_source = {0.0f, 246.0f, 322.0f, 246.0f + 8.0f};

static Color menu_text_color = COLOR_RGBA(214, 214, 215, 255);

extern FontHandle huge_font;

void menus_init(void) {
	background = tex_load(BACKGROUND_IMG);
	menu_atlas = tex_load(MENU_ATLAS_IMG);

	menu_state = menu_transition = MENU_MAIN;
}

void menus_close(void) {
	tex_free(background);
	tex_free(menu_atlas);
}

void menus_update(void) {
}

Vector2 _adjust_scale_pos(Vector2 p, Vector2 center, float scale) {
	Vector2 c_to_p = vec2_sub(p, center);
	c_to_p = vec2_scale(c_to_p, scale);
	return vec2_add(center, c_to_p);
}

void _render_main(float t) {
	const char* items[] = {
		"Play",
		"Settings",
		"Info"
	};

	float scale = 1.0f / lerp(1.5f, 0.5f, (t + 1.0f) / 2.0f);

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));
	Color text = color_lerp(menu_text_color, menu_text_color && 0xFFFFFF, 
		fabs(t));

	// Background
	RectF dest = rectf(0.0f, 0.0f, 0.0f, 0.0f);
	video_draw_rect(background, MENU_BACKGROUND_LAYER, &background_source,
		&dest, COLOR_WHITE);

	// Panel
	Vector2 center = vec2(240.0f, 168.0f);
	gfx_draw_textured_rect(menu_atlas, MENU_PANEL_LAYER, &panel_source,
		&center, 0.0f, scale, c);

	// Title
	Vector2 vdest = _adjust_scale_pos(vec2(240.0f, 46.0f), center, scale);
	gfx_draw_textured_rect(menu_atlas, MENU_TEXT_LAYER, &title_source,
		&vdest, -PI/2.0f, scale, c);
	
	// Text
	vdest = vec2(240.0f, 139.0f);
	for(uint i = 0; i < ARRAY_SIZE(items); ++i) {
		Vector2 adj_vdest = _adjust_scale_pos(vdest, center, scale);
		font_draw_ex(huge_font, items[i], MENU_TEXT_LAYER, 
			&adj_vdest, scale, text);

		if(i < ARRAY_SIZE(items)-1) {
			vdest.y += 19.0f;
			adj_vdest = _adjust_scale_pos(vdest, center, scale);
			gfx_draw_textured_rect(menu_atlas, MENU_SHADOW_LAYER,
				&separator_source, &adj_vdest, 0.0f, scale, c);	
		}

		vdest.y += 17.0f;	
	}
}

void menus_render(void) {
	if(menu_state == MENU_MAIN)
		_render_main(0.0f);
}

