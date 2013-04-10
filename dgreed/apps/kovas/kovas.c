#include <malka/malka.h>
#include <malka/ml_states.h>

#include "bind.h"

int dgreed_main(int argc, const char** argv) {
	malka_init();
	malka_params(argc, argv);
	malka_register(bind_open_kovas);
	int res = malka_states_run("kovas_scripts/main.lua");	
	malka_close();
	bind_close_kovas();
	return res;
}

