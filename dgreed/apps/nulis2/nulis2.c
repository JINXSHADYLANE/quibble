#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>
#include <sprsheet.h>
#include <particles.h>
#include <mfx.h>

#include "common.h"
#include "bind.h"

ScreenSize scr_size;

void dgreed_init(int argc, const char** argv) {
#ifndef _WIN32
	params_init(argc, argv);
#else
	params_init(1, NULL);
#endif

	rand_init(432);

	const char* sprsheet = ASSETS_PRE "sprsheet_768p.mml";
	uint width = 1024;
	uint height = 768;
	uint v_width = 1024;
	uint v_height = 768;
    
    uint n_width, n_height;
    video_get_native_resolution(&n_width, &n_height);

	// Select screen size
	scr_size = SCR_IPAD;
	if(params_find("-iphone") != ~0 || (n_width < 960 || n_height < 640)) {
		sprsheet = ASSETS_PRE "sprsheet_320p.mml";
		scr_size = SCR_IPHONE;
		v_width = width = 480;
		v_height = height = 320;
	}
	else if(params_find("-retina") != ~0 || (n_width < 1024 || n_height < 768)) {
		sprsheet = ASSETS_PRE "sprsheet_640p.mml";
		scr_size = SCR_IPHONE;
		v_width = 480;
		v_height = 320;
		width = v_width * 2;
		height = v_height * 2;
	}

	video_init_ex(width, height, v_width, v_height, "nulis", false);
	sound_init();

	particles_init(ASSETS_PRE, 4);
	mfx_init(ASSETS_PRE "effects.mml");

	malka_init();
	malka_params(argc, argv);
	malka_register(bind_open_nulis2);

	sprsheet_init(sprsheet);

	malka_states_init(SCRIPTS_PRE "main.lua");
	malka_states_start();
}

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();

	sprsheet_close();

	malka_close();

	mfx_close();
	particles_close();

	sound_close();
	video_close();
}

bool dgreed_update(void) {
	if(char_up('q'))
		malka_states_pop();

	mfx_update();
	sound_update();

	return true;
}

bool dgreed_render(void) {
	particles_draw();
	return malka_states_step();
}

#ifndef TARGET_IOS
int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);

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

