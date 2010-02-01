#include <system.h>
#include "common.h"
#include "puzzles.h"
#include "background.h"
#include "menu.h"
#include "saves.h"
#include "game.h"

int dgreed_main(int argc, const char** argv) {
	log_init("tor.log", LOG_LEVEL_INFO);
	saves_init();
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT, "Tor");
	puzzles_init();
	background_init();
	menu_init();
	game_init();

	game_state = MENU_STATE;

	while(system_update()) {
		switch(game_state) {
			case MENU_STATE:
				menu_update();
				menu_render();
				break;
			case PUZZLE_STATE:
				game_render();
				game_update();
				break;
			default:
				LOG_ERROR("Unknown state!");
		}		
		video_present();
	}

	game_close();
	menu_close();
	background_close();
	puzzles_close();
	video_close();
	saves_close();
	log_close();
}

