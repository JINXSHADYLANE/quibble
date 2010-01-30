#include <system.h>
#include <utils.h>
#include "common.h"
#include "layouts.h"
#include "game.h"

int main(int argc, const char** argv) {
	log_init("keymingler.log", LOG_LEVEL_INFO);
	rand_init(52890);
	layouts_init();
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	game_init();

	while(system_update()) {
		game_update();
		game_render();
		video_present();
	}

	game_close();
	video_close();
	layouts_close();
	log_close();
}

