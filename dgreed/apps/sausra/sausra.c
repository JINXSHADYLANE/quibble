#include <malka/malka.h>

int dgreed_main(int argc, const char** argv) {
	log_init("sausra.log", LOG_LEVEL_INFO);
	int res = malka_run("sausra_scripts/main.lua");	
	log_close();
	return res;
}
