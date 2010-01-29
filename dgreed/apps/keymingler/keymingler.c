#include <system.h>
#include "common.h"
#include "layouts.h"

int main(int argc, const char** argv) {
	log_init("keymingler.log", LOG_LEVEL_INFO);
	layouts_init();
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);

	while(system_update()) {
		video_present();
	}

	video_close();
	layouts_close();
	log_close();
}

