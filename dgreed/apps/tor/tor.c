#include <system.h>
#include "common.h"
#include "puzzles.h"
#include "background.h"
#include "menu.h"

int main(int argc, const char** argv) {
	log_init("tor.log", LOG_LEVEL_INFO);
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	background_init();
	menu_init();
	puzzles_init();

	while(system_update()) {
		menu_update();
		menu_render();
		video_present();
	}

	puzzles_close();
	menu_close();
	background_close();
	video_close();
}

