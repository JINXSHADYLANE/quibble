#include "splash.h"

#include "common.h"
#include "game.h"

#include <sprsheet.h>

static SprHandle splash_img = 0;
static SprHandle splash_img2 = 0;

static void splash_init(void) {
	if(v_width < 1024.0f) {
		splash_img = sprsheet_get_handle("splash_iphone");
	}
	else if(v_width == 1136.0f) {
		splash_img = sprsheet_get_handle("splash_iphone5_1");
		splash_img2 = sprsheet_get_handle("splash_iphone5_2");
	}
	else {
		printf("ipad\n");
		splash_img = sprsheet_get_handle("splash_ipad");
	}
}

static void splash_postleave(void) {
	sprsheet_unload_h(splash_img);
}

static void splash_leave(void) {
	malka_states_prerender_cb(game_render_level);
}

static bool splash_update(void) {
	if(key_down(KEY_A))
		malka_states_replace("level_select");

	return true;
}

static bool splash_render(float t) {
	float a = 1.0f - fabsf(t);
	Color c = COLOR_FTRANSP(a);

	spr_draw_h(splash_img, 14, rectf_null(), c);
	if(splash_img2) {
		Vector2 pos = {1048.0f, 320.0f};
		spr_draw_cntr_h(splash_img2, 14, pos, -PI/2.0f, 1.0f, c);
	}

	float s = time_s();
	Color c2 = COLOR_FTRANSP(a * (sinf(s * 2.0f * PI) * 0.2f + 0.8f));

	Vector2 p = {v_width / 2.0f, v_height / 2.0f};
	p.y = lerp(p.y, v_height, 0.12f);
	spr_draw_cntr("tap_to_start", 15, p, 0.0f, 1.0f, c2);

	return true;
}

StateDesc splash_state = {
	.init = splash_init,
	.leave = splash_leave,
	.postleave = splash_postleave,
	.update = splash_update,
	.render = splash_render
};

