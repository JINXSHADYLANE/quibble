#include <malka/malka.h>

int dgreed_main(int argc, const char** argv) {
	malka_params(argc, argv);
	int res = malka_run("gurbas_scripts/main.lua");	
	return res;
}

