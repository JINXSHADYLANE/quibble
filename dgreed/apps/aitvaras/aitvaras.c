#include <system.h>
#include <malka/malka.h>
#include <malka/ml_states.h>

#include "ml_aitvaras.h"

#define SCRIPTS_PRE "aitvaras_scripts/"

void dgreed_preinit(void) {
	video_clear_color(COLOR_RGBA(255, 255, 255, 255));
}

bool dgreed_init(int argc, const char** argv) {
	sound_init();

	malka_init();
	malka_params(argc, argv);
	malka_register(malka_open_aitvaras);

	malka_states_init(SCRIPTS_PRE "main.lua");
	malka_states_start();

	return true;
}

extern void _http_check_close(void);

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();

	malka_close();

	sound_close();

	_http_check_close();
}

bool dgreed_update(void) {
	if(char_up('q'))
		malka_states_pop();

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

