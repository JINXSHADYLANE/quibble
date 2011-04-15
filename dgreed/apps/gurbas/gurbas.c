#include <malka/malka.h>

int dgreed_main(int argc, const char** argv) {
	log_init("gurbas.log", LOG_LEVEL_INFO);
	int res = malka_run("gurbas_scripts/main.lua");	
	log_close();
	return res;
}

