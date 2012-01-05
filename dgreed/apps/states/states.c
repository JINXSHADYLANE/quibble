#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>

#define ASSETS_PRE "states_assets/"

// Winter state

TexHandle img;

void winter_init(void) {
	img = tex_load(ASSETS_PRE "yerka1.png");
	printf("init winter\n");
}

void winter_close(void) {
	tex_free(img);
	printf("close winter\n");
}

void winter_enter(void) {
	printf("enter winter\n");
}

void winter_leave(void) {
	printf("leave winter\n");
}

bool winter_update(void) {
	if(mouse_down(MBTN_LEFT))
		malka_states_replace("spring");
	return true;
}

bool winter_render(float t) {
	float sgn = t / fabsf(t+0.0001f);
	float tt = smoothstep(0.0f, 1.0f, fabsf(t)) * sgn;

	RectF dest = {31.0f + tt * 512.0f, 55.0f, 0.0f, 0.0f};
	video_draw_rect(img, 0, NULL, &dest, COLOR_WHITE);

	return true;
}

StateDesc winter_state = {
	.init = winter_init,
	.close = winter_close,
	.enter = winter_enter,
	.leave = winter_leave,
	.update = winter_update,
	.render = winter_render
};

int dgreed_main(int argc, const char** argv) {
	log_init("states.log", LOG_LEVEL_INFO);
	malka_init();
	malka_params(argc, argv);
	malka_states_register("winter", &winter_state);
	int res = malka_states_run("states_scripts/main.lua");	
	malka_close();
	log_close();
	return res;
}

