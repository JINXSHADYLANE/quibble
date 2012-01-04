#include <malka/malka.h>

int dgreed_main(int argc, const char** argv) {
	log_init("nulis2.log", LOG_LEVEL_INFO);
	malka_init();
	malka_params(argc, argv);
	int res = malka_states_run("nulis2_scripts/main.lua");	
	malka_close();
	log_close();
	return res;
}

