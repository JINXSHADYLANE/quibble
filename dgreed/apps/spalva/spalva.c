#include <malka/malka.h>
#include <malka/ml_states.h>

void dgreed_preinit(void) {
}

void dgreed_init(int argc, const char** argv) {
	malka_init();
	malka_params(argc, argv);

	malka_states_init("spalva_scripts/main.lua");
	malka_states_start();
}

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();

	malka_close();
}

bool dgreed_update(void) {
	return true;
}

bool dgreed_render(void) { 
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

