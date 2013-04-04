#include <system.h>

#include "common.h"

static Color backbuffer[scr_size] = {0};
static TexHandle backbuffer_tex;

void init(void) {
	backbuffer_tex = tex_create(
		next_pow2(scr_width), 
		next_pow2(scr_height)
	);
}

bool render(void) {
	// Fill backbuffer with noise
	for(uint y = 0; y < scr_height; ++y) {
		for(uint x = 0; x < scr_width; ++x) {
			backbuffer[y * scr_width + x] = (rand_uint() + rand_uint()) / 2;
		}
	}

	// Transfer backbuffer to gpu
	tex_blit(backbuffer_tex, backbuffer, 0, 0, scr_width, scr_height);

	// Draw backbuffer
	RectF dest = rectf_null();
	video_draw_rect(backbuffer_tex, 1, NULL, &dest, COLOR_WHITE);

	return !key_up(KEY_QUIT) && !char_up('q');
}

void close(void) {
	tex_free(backbuffer_tex);
}

int dgreed_main(int argc, const char** argv) {
	video_init_exr(
		scr_width * scr_scale, scr_height * scr_scale,
		scr_width, scr_height, "tamsa", false
	);

	init();

	while(system_update()) {
		if(!render())
			break;
		video_present();
	}

	close();

	video_close();
	return 0;
}

