#include <system.h>

int dgreed_main(int argc, const char** argv) {
	log_init("empty.log", LOG_LEVEL_INFO);
	video_init(480, 272, "Empty");

	while(system_update()) {
		video_present();
	}

	video_close();
	log_close();

	return 0;
}

