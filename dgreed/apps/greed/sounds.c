#include "sounds.h"
#include "system.h"

SoundHandle sound_shot;

void sounds_init() {
	sound_init();

	sound_shot = sound_load_sample("greed_assets/F_slide.wav");
}	

void sounds_close() {
	sound_free(sound_shot);

	sound_close();
}

void sounds_event(SoundEventType type) {
	switch(type) {
		case SHOT:
			sound_play(sound_shot);
			return;
		default:
			break;
	};

	//LOG_ERROR("Unknown SoundEventType");
}	

