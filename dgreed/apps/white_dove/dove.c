#include <malka/malka.h>

int dgreed_main(int argc, const char** argv)
{
	log_init("white_dove.log", LOG_LEVEL_INFO);
	int res = malka_run("white_dove_scripts/game.lua");
	log_close();
	return res;
}

