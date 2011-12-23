#include <system.h>
#include <font.h>
#include <gui.h>
#include <memory.h>
#include "gui_style.h"
#include "game.h"
#include "ai.h"
#include "sounds.h"

bool highres = false;

GuiDesc style;
#ifdef TRACK_MEMORY
MemoryStats stats;
#endif

bool dgreed_init(void) {
	log_init("greed.log", LOG_LEVEL_INFO);
	rand_init(47891);
	
	if(highres)
		video_init_ex(960, 640,	480, 320, "Greed", false);
	else
		video_init(480, 320, "Greed");

	// Load empty texture first, to make sure it has '0' handle
	TexHandle empty = tex_load("greed_assets/empty.png");
	assert(empty == 0);
	
	sounds_init();
	
	style = greed_gui_style(highres);
	gui_init(&style);
	
	game_init();
	
	return true;
}

bool dgreed_update(void) {
	game_update();
	sound_update();
	
	if(key_up(KEY_QUIT))
		return false;
	
	return true;
}

bool dgreed_render(void) {
	game_render();
	video_present();
	
	return true;
}

void dgreed_close(void) {
	game_close();
	gui_close();
	greed_gui_free();
	sounds_close();
	tex_free(0);
	video_close();
	log_close();
}


#ifndef TARGET_IOS
int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);

	highres = true;
	if(params_find("-s") != ~0)
		highres = false;
	
	dgreed_init();
	while(system_update()) {
		if(!dgreed_update())
			break;
		if(!dgreed_render())
			break;
	}	
	dgreed_close();

	return 0;
}	
#endif
