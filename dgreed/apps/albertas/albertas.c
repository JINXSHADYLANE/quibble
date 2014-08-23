#include <malka/malka.h>
#include <malka/ml_states.h>
#include <vfont.h>

extern bool draw_gfx_debug;

int dgreed_main(int argc, const char** argv) {
	draw_gfx_debug = false;
	malka_init();
	//vfont_init_ex(512, 512);
	malka_params(argc, argv);
	int res = malka_states_run("albertas_scripts/main.lua");	
	malka_close();
	return res;
}

