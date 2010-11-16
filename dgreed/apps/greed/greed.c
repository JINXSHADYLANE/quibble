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
	video_close();
	
#ifdef TRACK_MEMORY
	mem_stats(&stats);
	LOG_INFO("Memory usage stats:");
	LOG_INFO(" Total allocations: %u", stats.n_allocations);
	LOG_INFO(" Peak dynamic memory usage: %uB", stats.peak_bytes_allocated);
	if(stats.bytes_allocated) {
		LOG_INFO(" Bytes still allocted: %u", stats.bytes_allocated);
		LOG_INFO(" Dumping allocations info to memory.txt");
		mem_dump("memory.txt");
	}	
#endif
	
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
		dgreed_update();
		dgreed_render();
	}	
	dgreed_close();

	return 0;
}	
#endif
