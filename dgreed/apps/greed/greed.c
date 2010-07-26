#include <system.h>
#include <font.h>
#include <gui.h>
#include <memory.h>
#include "gui_style.h"
#include "game.h"
#include "ai.h"
#include "sounds.h"

#define ARENA_FILE "greed_assets/arena.txt"

const bool highres = true;

int dgreed_main(int argc, const char** argv) {
	log_init("greed.log", LOG_LEVEL_INFO);
	rand_init(47891);

	if(highres)
		video_init_ex(960, 640,	480, 320, "Greed", false);
	else
		video_init(480, 320, "Greed");

	sounds_init();

	GuiDesc style = greed_gui_style(highres);
	gui_init(&style);

	game_init();
	char* arena_file = txtfile_read(ARENA_FILE);
	game_reset(arena_file, 2);
	ai_init_agent(1);
	MEM_FREE(arena_file);

	while(system_update()) {
		game_update();
		game_render();

		video_present();
		sound_update();

		if (key_up(KEY_QUIT))
			break;
	}	

	game_close();
	gui_close();
	greed_gui_free();
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

