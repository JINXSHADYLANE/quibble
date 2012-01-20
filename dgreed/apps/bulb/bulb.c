#include <malka/malka.h>

#include "bind.h"

int dgreed_main(int argc, const char** argv) {
	malka_init();
	malka_params(argc, argv);
	malka_register(bind_open_bulb);
	int res = malka_run_ex("bulb_scripts/main.lua");	
	malka_close();
	bind_close_bulb();
	return res;
}

