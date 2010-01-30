#include <system.h>
#include <font.h>
#include "common.h"
#include "layouts.h"

int main(int argc, const char** argv) {
	log_init("keymingler.log", LOG_LEVEL_INFO);
	layouts_init();
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);

	FontHandle f = font_load("keymingler_assets/appopaint.bft");

	while(system_update()) {
		Vector2 tl = vec2(10.0f, 10.0f);
		font_draw(f, "test", 0, &tl, COLOR_WHITE);
		video_present();
	}

	font_free(f);

	video_close();
	layouts_close();
	log_close();
}

