#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>
#include <sprsheet.h>
#include <particles.h>
#include <mfx.h>

#include "common.h"
#include "game.h"
#include "bind.h"

void dgreed_init(int argc, const char** argv) {
	log_init("nulis2.log", LOG_LEVEL_INFO);
	rand_init(432);

	video_init(1024, 768, "Nulis");

	particles_init(ASSETS_PRE, 4);
	mfx_init(ASSETS_PRE "effects.mml");

	malka_init();
	malka_params(argc, argv);
	malka_register(bind_open_nulis2);

	sprsheet_init(ASSETS_PRE "sprsheet_768p.mml");

	malka_states_register("game", &game_state);
	malka_states_push("game");
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

	video_close();
	log_close();
}

bool dgreed_update(void) {
	if(char_up('q'))
		malka_states_pop();

	mfx_update();
	particles_update(time_s());

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

