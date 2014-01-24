#include <malka/malka.h>
#include <malka/ml_states.h>
#include <vfont.h>

extern bool draw_gfx_debug;

int dgreed_main(int argc, const char** argv) {
	draw_gfx_debug = false;
	malka_init();
//	vfont_init_ex(256, 256);
//	vfont_resolution_factor(4.0f);
	malka_params(argc, argv);
	int res = malka_states_run("pyktis_scripts/main.lua");	
	malka_close();
	return res;
}

