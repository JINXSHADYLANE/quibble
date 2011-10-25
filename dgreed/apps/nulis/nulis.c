#include <malka/malka.h>
#include "bind.h"

int dgreed_main(int argc, const char** argv) {
	log_init("nulis.log", LOG_LEVEL_INFO);
	malka_init();
	malka_params(argc, argv);
	malka_register(bind_open_nulis);
	int res = malka_states_run("nulis_scripts/main.lua");	
	malka_close();
	log_close();
	return res;
}

