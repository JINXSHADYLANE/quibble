#include <system.h>
#include <utils.h>
#include <font.h>
#include <time.h>
#include "common.h"
#include "layouts.h"
#include "game.h"
#include "sounds.h"
#include "memory.h"

SoundHandle music;
FontHandle font;
float play_time;

#define LOADING_TEXT "PATIENCE"

int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);
	rand_init(time(NULL));
	layouts_init();
	layouts_set("dvorak");

	bool fullscreen = true;
	if(params_find("-windowed") != ~0)
		fullscreen = false;

	video_init_ex(SCREEN_WIDTH, SCREEN_HEIGHT, 
		SCREEN_WIDTH, SCREEN_HEIGHT, "KeyMingler", fullscreen);
	font = font_load(FONT_FILE);	
	float text_width = font_width(font, LOADING_TEXT);
	float text_height = font_height(font);
	Vector2 pos = vec2((SCREEN_WIDTH - text_width) / 2.0f,
		(SCREEN_HEIGHT - text_height) / 2.0f);
	font_draw(font, LOADING_TEXT, 0, &pos, COLOR_WHITE);	
	video_present();
	system_update();

	game_init();
	sounds_init();
	music = sound_load_stream(MUSIC_FILE);
	sound_play(music);

	while(system_update()) {
		game_update();
		game_render();
		video_present();
		sound_update();

		if(key_up(KEY_QUIT))
			break;
	}
	
	font_free(font);
	sound_free(music);
	sounds_close();
	game_close();
	video_close();
	layouts_close();

	return 0;
}

