#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>
#include <sprsheet.h>
#include <particles.h>
#include <anim.h>
#include <uidesc.h>
#include <mfx.h>
#include <keyval.h>
#include <vfont.h>

#include <time.h>

#include "game.h"
#include "level_select.h"
#include "pause.h"
#include "game_over.h"

#include "mchains.h"
#include "devmode.h"
#include "common.h"

bool button_click = false;	// prevents buttons from being clicked multiple times during transition.

void dgreed_preinit(void) {
}

bool dgreed_init(int argc, const char** argv) {
#ifndef _WIN32
	params_init(argc, argv);
#else
	params_init(1, NULL);
#endif

	rand_init(time(NULL));

	const char* sprsheet = ASSETS_DIR "spritesheet.mml";
	uint v_width = 1024;
	uint v_height = 768;
    
    uint n_width, n_height;
    video_get_native_resolution(&n_width, &n_height);

	if(params_find("-retina") != ~0 || (n_width == 2048 && n_height == 1536)) {
		sprsheet = ASSETS_DIR "r_spritesheet.mml";
		n_width = 2048;
		n_height = 1536;
	}
	else {
		n_width = 1024;
		n_height = 768;
	}

	if(params_find("-s") != ~0) {
		n_width /= 2;
		n_height /= 2;
	}

	video_init_ex(n_width, n_height, v_width, v_height, "morka", false);
	sound_init();

	keyval_init("morka.db");

	const char* particles = "particles.mml";
	particles_init_ex(ASSETS_DIR, particles, particle_layer);
	mfx_init(ASSETS_DIR "effects.mml");
	
	anim_init(ASSETS_DIR "animations.mml");

	devmode_init();

	malka_init();
	malka_params(argc, argv);

	malka_states_register("game", &game_state);
	malka_states_register("level_select", &level_select_state);
	malka_states_register("pause", &pause_state);
	malka_states_register("game_over", &game_over_state);

	malka_states_set_transition_len(0.5f);

	malka_states_prerender_cb(game_render_level);

	malka_states_push("level_select");

	sprsheet_init(sprsheet);
	mchains_init(ASSETS_DIR "mchains.mml");
	uidesc_init(ASSETS_DIR "uidesc.mml");

	malka_states_init(SCRIPTS_DIR "main.lua");
	malka_states_start();
    
    return true;
}

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();

	uidesc_close();
	mchains_close();
	sprsheet_close();

	malka_close();

	devmode_close();

	anim_close();
	mfx_close();
	particles_close();

	keyval_close();

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
	devmode_render();
	particles_draw();
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

