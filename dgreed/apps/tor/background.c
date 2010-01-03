#include "background.h"
#include "common.h"
#include <system.h>

TexHandle atlas1;
TexHandle atlas2;
RectF background1_src = {1.0f, 1.0f, 481.0f, 273.0f};
RectF background2_src = {0.0f, 275.0f, 960.0f, 275.0f + 544.0f};
RectF background3_src = {0.0f, 0.0f, 960.0f, 544.0f};
float background1_width, background2_width;
uint state; // 0 - menu, 1 - game
float transition_factor, transition_start; // transition factor

// Tweakables
float background1_scroll_speed = 20.0f;
float transition_length = 1.0f;

void background_init(void) {
	atlas1 = tex_load(ATLAS1);
	atlas2 = tex_load(ATLAS2);
	background1_width = background1_src.right - background1_src.left;
	background2_width = background2_src.right - background2_src.left;
	state = 0;
	transition_factor = 0.0f;
}

void background_close(void) {
	tex_free(atlas1);
	tex_free(atlas2);
}

void background_update(void) {
	float t = time_ms() / 1000.0f;


	float dt = t - transition_start;
	if(state == 0 && transition_factor > 0.0f)
		transition_factor = 1.0f - (dt / transition_length);
	if(state == 1 && transition_factor < 1.0f)
		transition_factor = dt / transition_length;
	
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
	
	Color c1 = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, clamp(0.0f, 1.0f, transition_factor));
	Color c2 = color_lerp(COLOR_TRANSPARENT, COLOR_WHITE, clamp(0.0f, 1.0f, transition_factor));

	dest = rectf(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
	if(transition_factor < 1.0f)
		video_draw_rect(atlas1, 1, &background2_src, &dest, c1);
	if(transition_factor > 0.0f)	
		video_draw_rect(atlas2, 1, &background3_src, &dest, c2);
}

void background_switch(void) {
	state = (state == 0) ? 1 : 0;
	transition_start = time_ms() / 1000.0f;
}

