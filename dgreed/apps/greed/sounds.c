#include "sounds.h"
#include <system.h>

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
	sounds_event_ex(type, ~0);
}

void sounds_event_ex(SoundEventType type, uint arg) {
	SoundHandle music;
	switch(type) {
		case MUSIC:
			music = (arg == 0) ? 
				music_c1 : music_c2;
			sound_stop(music);
			sound_play(music);
			return;
		case SHOT:
			sound_play(sound_shot);
			return;
		default:
			break;
	};

	//LOG_ERROR("Unknown SoundEventType");
}	

