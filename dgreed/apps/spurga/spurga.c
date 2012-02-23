#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>
#include <sprsheet.h>
#include <mfx.h>
#include <keyval.h>

#define ASSETS_PRE "spurga_assets/"
#define SCRIPTS_PRE "spurga_scripts/"

void dgreed_preinit(void) {
	video_clear_color(COLOR_RGBA(255, 255, 255, 255));
}

void dgreed_init(int argc, const char** argv) {
#ifndef _WIN32
	params_init(argc, argv);
#else
	params_init(1, NULL);
#endif

	rand_init(433);

	uint width = 320, height = 480;
	uint v_width = 320, v_height = 480;
	const char* sprsheet = ASSETS_PRE "sprsheet_320p.mml";

	uint n_width, n_height;
	video_get_native_resolution(&n_width, &n_height);

	if(params_find("-ipad") != ~0 || (n_width == 1024 && n_height == 768)) {
		// Ipad
		v_width = width = 1024;
		v_height = height = 768;
		sprsheet = ASSETS_PRE "sprsheet_768p.mml";
		printf("ipad\n");
	}
	else if(params_find("-retina") != ~0 || (n_width == 960 && n_height == 640)) {
		// Retina
		width = 640;
		height = 960;
		sprsheet = ASSETS_PRE "sprsheet_640p.mml";
	}

	video_init_ex(width, height, v_width, v_height, "spurga", false);
	sound_init();

	keyval_init("spurga.db");
	sprsheet_init(sprsheet);
	mfx_init(ASSETS_PRE "effects.mml");

	malka_init_ex(true);
	malka_params(argc, argv);

	malka_states_init(SCRIPTS_PRE "main.lua");
	malka_states_start();
}

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();
	malka_close();

	mfx_close();
	sprsheet_close();
	keyval_close();

	sound_close();
	video_close();
}

bool dgreed_update(void) {
	if(char_up('q'))
		malka_states_pop();

	mfx_update();
	sound_update();

	malka_gc(1);

	return true;
}

bool dgreed_render(void) {
	return malka_states_step();
}

#ifndef TARGET_IOS
int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);

	dgreed_preinit();
	dgreed_init(argc, argv);
	while(true) {
		if(!dgreed_update())
			break;
		if(!dgreed_render())
			break;
	}
	dgreed_close();

	return 0;
}
#endif

