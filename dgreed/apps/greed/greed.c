#include "system.h"
#include "font.h"
#include "gui.h"
#include "game.h"
#include "memory.h"
#include "sounds.h"

int dgreed_main(int argc, const char** argv) {
	log_init("greed.log", LOG_LEVEL_INFO);
	rand_init(47891);
	//video_init(480, 320, "Greed");
	video_init_ex(960, 640,	480, 320, "Greed", false);
	sounds_init();
	gui_init();

	game_init();
	game_reset("greed_assets/c2_arena3.mml", 2);

	while(system_update()) {
		game_update();
		game_render();

		video_present();
		sound_update();
	}	

	game_close();
	gui_close();
	sounds_close();
	video_close();

	#ifdef TRACK_MEMORY
	MemoryStats stats;
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

	return 0;
}	

