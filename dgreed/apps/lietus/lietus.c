#include <system.h>
#include "game.h"

int dgreed_main(int argc, const char** argv) {
	log_init("lietus.log", LOG_LEVEL_INFO);
	video_init(480, 320, "lietus");

	game_init();

	while(system_update()) {
		game_update();
		game_render();
		video_present();
	}

	game_close();

	video_close();
	log_close();

	return 0;
}

