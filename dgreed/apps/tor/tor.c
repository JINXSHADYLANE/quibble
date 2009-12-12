#include <system.h>
#include "common.h"
#include "puzzles.h"
#include "background.h"
#include "menu.h"
#include "saves.h"

int main(int argc, const char** argv) {
	log_init("tor.log", LOG_LEVEL_INFO);
	saves_init();
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	puzzles_init();
	background_init();
	menu_init();

	while(system_update()) {
		menu_update();
		menu_render();
		video_present();
	}

	menu_close();
	background_close();
	puzzles_close();
	video_close();
	saves_close();
	log_close();
}

