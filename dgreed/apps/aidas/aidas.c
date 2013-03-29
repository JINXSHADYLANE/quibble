#include <malka/malka.h>
#include <malka/ml_states.h>

int dgreed_main(int argc, const char** argv) {
	malka_init();
	malka_params(argc, argv);
	int res = malka_states_run("aidas_scripts/main.lua");	
	malka_close();
	return res;
}

