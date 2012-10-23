#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>
#include <sprsheet.h>
#include <anim.h>
#include <mfx.h>
#include <keyval.h>

#include "game.h"

extern bool draw_gfx_debug;

#define ASSETS_DIR "morka_assets/"
#define SCRIPTS_DIR "morka_scripts/"

void dgreed_preinit(void) {
}

bool dgreed_init(int argc, const char** argv) {
#ifndef _WIN32
	params_init(argc, argv);
#else
	params_init(1, NULL);
#endif

	rand_init(432);

	const char* sprsheet = ASSETS_DIR "iPad_spritesheet.mml";
	uint width = 1024;
	uint height = 768;
	uint v_width = 1024;
	uint v_height = 768;
    
    uint n_width, n_height;
    video_get_native_resolution(&n_width, &n_height);

	video_init_ex(width, height, v_width, v_height, "morka", false);
	sound_init();

	keyval_init("morka.db");

	anim_init(ASSETS_DIR "animations.mml");
//	mfx_init(ASSETS_PRE "effects.mml");

	malka_init();
	malka_params(argc, argv);
	malka_states_register("game", &game_state);
	malka_states_push("game");

	sprsheet_init(sprsheet);

	malka_states_init(SCRIPTS_DIR "main.lua");
	malka_states_start();
    
    return true;
}

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();

	sprsheet_close();

	malka_close();

//	mfx_close();
	anim_close();

	keyval_close();

	sound_close();
	video_close();
}

bool dgreed_update(void) {
	if(char_up('q'))
		malka_states_pop();

//	mfx_update();
	sound_update();

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

