#include "background.h"
#include <system.h>

#define BACKGROUND_LAYER 5
#define BACKGROUND_IMG "greed_assets/anim_bg.png"
#define TRIANGLE_IMG "greed_assets/triangle.png"

static TexHandle background;
static TexHandle triangle;

static RectF background_src = {0.0f, 0.0f, 480.0f, 320.0f};

void background_init(void) {
	background = tex_load(BACKGROUND_IMG);
	triangle = tex_load(TRIANGLE_IMG);
}

void background_close(void) {
	tex_free(triangle);
	tex_free(background);
}

void background_update(void) {
}

void background_render(Color c) {
	const RectF dest = {0.0f, 0.0f, 0.0f, 0.0f};
	video_draw_rect(background, BACKGROUND_LAYER, &background_src,
		&dest, c);
}


