#include <system.h>
#include <utils.h>
#include "common.h"
#include "layouts.h"
#include "game.h"

SoundHandle music;
float play_time;

int main(int argc, const char** argv) {
	log_init("keymingler.log", LOG_LEVEL_INFO);
	rand_init(52890);
	layouts_init();
	video_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	game_init();
	sound_init();
	music = sound_load_sample(MUSIC_FILE);
	play_time = -1000.0f;

	while(system_update()) {
		game_update();
		game_render();
		video_present();
		sound_update();
		
		if (time_ms() / 1000.0f - play_time > MUSIC_LENGTH) {
			sound_play(music);
			play_time = time_ms() / 1000.0f;
		}	
	}
	
	sound_free(music);
	sound_close();
	game_close();
	video_close();
	layouts_close();
	log_close();
}

