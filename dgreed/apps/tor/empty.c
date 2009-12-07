#include <system.h>

int main(int argc, const char** argv) {
	log_init("tor.log", LOG_LEVEL_INFO);
	video_init(480, 272);

	while(system_update()) {
		video_present();
	}

	video_close();
}

