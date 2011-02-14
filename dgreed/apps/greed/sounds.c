#include "sounds.h"
#include "system.h"

SoundHandle sound_shot;
SoundHandle music_c1;
SoundHandle music_c2;

void sounds_init() {
	sound_init();
	
	music_c1 = sound_load_stream("greed_assets/BGC1.ogg");
	music_c2 = sound_load_stream("greed_assets/BGC2.ogg");
	sound_shot = sound_load_sample("greed_assets/F_slide.wav");
}	

void sounds_close() {
	sound_free(sound_shot);
	sound_free(music_c2);
	sound_free(music_c1);

	sound_close();
}

void sounds_event(SoundEventType type) {
	switch(type) {
		case MUSIC:
			sound_stop(music_c1);
			sound_play(music_c1);
			return;
		case SHOT:
			sound_play(sound_shot);
			return;
		default:
			break;
	};

	//LOG_ERROR("Unknown SoundEventType");
}	

