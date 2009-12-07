#include <system.h>
#include "common.h"
#include "puzzles.h"

int main(int argc, const char** argv) {
	log_init("tor.log", LOG_LEVEL_INFO);
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	puzzles_init();

	while(system_update()) {
		video_present();
	}

	puzzles_close();
	video_close();
}

