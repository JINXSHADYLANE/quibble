#include <system.h>

#include "common.h"
#include "kdtree.h"

static Color backbuffer[scr_size] = {0};
static TexHandle backbuffer_tex;

void tm_init(void) {
	backbuffer_tex = tex_create(
		next_pow2(scr_width), 
		next_pow2(scr_height)
	);


	KdSeg segs[] = {
		{0, 0, 6, 0, 7},
		{6, 0, 6, 5, 7},
		{6, 5, 0, 5, 7},
		{0, 5, 0, 0, 7},
		{2, 2, 2, 3, 13},
		{2, 3, 4, 3, 13},
		{4, 3, 4, 2, 13},
		{4, 2, 2, 2, 13}
	};

	KdTree tree;
	kdtree_init(&tree, segs, 8);

	// ...

	kdtree_free(&tree);
}

bool tm_render(void) {
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

void tm_close(void) {
	tex_free(backbuffer_tex);
}

int dgreed_main(int argc, const char** argv) {
	video_init_exr(
		scr_width * scr_scale, scr_height * scr_scale,
		scr_width, scr_height, "tamsa", false
	);

	tm_init();

	while(system_update()) {
		if(!tm_render())
			break;
		video_present();
	}

	tm_close();

	video_close();
	return 0;
}

