#include <system.h>

int dgreed_main(int argc, const char** argv) {
	video_init(480, 272, "Empty");

	while(system_update()) {
		video_present();
	}

	video_close();
	return 0;
}

