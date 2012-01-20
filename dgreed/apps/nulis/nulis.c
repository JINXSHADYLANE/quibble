#include <malka/malka.h>
#include "bind.h"

int dgreed_main(int argc, const char** argv) {
	malka_init();
	malka_params(argc, argv);
	malka_register(bind_open_nulis);
	int res = malka_states_run("nulis_scripts/main.lua");	
	malka_close();
	return res;
}

