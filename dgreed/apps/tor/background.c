#include "background.h"
#include "common.h"
#include <system.h>

TexHandle atlas1;
RectF background1_src = {1.0f, 1.0f, 481.0f, 273.0f};
RectF background2_src = {0.0f, 275.0f, 960.0f, 275.0f + 544.0f};
float background1_width, background2_width;

// Tweakables
float background1_scroll_speed = 20.0f;

void background_init(void) {
	atlas1 = tex_load(ATLAS1);
	background1_width = background1_src.right - background1_src.left;
	background2_width = background2_src.right - background2_src.left;
}

void background_close(void) {
	tex_free(atlas1);
}

void background_update(void) {
}

void background_render(void) {
	float t = time_ms() / 1000.0f;

	RectF dest = rectf(fmodf(t * background1_scroll_speed, SCREEN_WIDTH), 0.0f,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	dest.right += dest.left;	
	video_draw_rect(atlas1, 0, &background1_src, &dest, COLOR_WHITE);
	dest.left -= SCREEN_WIDTH;
	dest.right -= SCREEN_WIDTH;
	video_draw_rect(atlas1, 0, &background1_src, &dest, COLOR_WHITE);
	
	dest = rectf(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	video_draw_rect(atlas1, 1, &background2_src, &dest, COLOR_WHITE);
}
