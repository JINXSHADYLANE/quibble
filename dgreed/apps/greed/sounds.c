#include "sounds.h"
#include <system.h>

SoundHandle snd_shot;
SoundHandle snd_bullet_wall;
SoundHandle snd_hit;
SoundHandle snd_ship_bump;
SoundHandle snd_wall_bump;
SoundHandle snd_base_taken;
SoundHandle snd_base_zero;
SoundHandle snd_base_beep;
SoundHandle snd_ship_acc;
SoundHandle snd_ship_kill;
SoundHandle snd_gui;

SoundHandle music_c1;
SoundHandle music_c2;
SoundHandle music_c3;
SoundHandle music_c4;
SoundHandle music_c5;
SoundHandle music_error;
SoundHandle music_menu;
SoundHandle music;

void sounds_init() {
	sound_init();

	snd_shot = sound_load_sample("greed_assets/SHOT.wav");
	snd_bullet_wall = sound_load_sample("greed_assets/BULLET_WALL.wav");
	snd_hit = sound_load_sample("greed_assets/SHIP_HIT.wav");
	snd_wall_bump = sound_load_sample("greed_assets/SHIP_WALL.wav");
	snd_ship_bump = sound_load_sample("greed_assets/SHIP_SHIP.wav");
	snd_base_taken = sound_load_sample("greed_assets/BASE_TAKEN.wav");
	snd_base_zero = sound_load_sample("greed_assets/BASE_ZERO.wav");
	snd_base_beep = sound_load_sample("greed_assets/BASE_BEEP.wav");
	snd_ship_acc = sound_load_sample("greed_assets/SHIP_ACC.wav");
	snd_ship_kill = sound_load_sample("greed_assets/SHIP_KILL.wav");
	snd_gui = sound_load_sample("greed_assets/GUI_CLICK.wav");
	
	music_c1 = sound_load_stream("greed_assets/C1.ogg");
	music_c2 = sound_load_stream("greed_assets/C2.ogg");
	music_c3 = sound_load_stream("greed_assets/C3.ogg");
	music_c4 = sound_load_stream("greed_assets/C4.ogg");
	music_c5 = sound_load_stream("greed_assets/C5.ogg");
	music_error = sound_load_stream("greed_assets/ERROR.ogg");
	music_menu = sound_load_stream("greed_assets/MENU.ogg");

	music = music_menu;
	sound_play(music);
}	

void sounds_close() {
	sound_free(music_menu);
	sound_free(music_error);
	sound_free(music_c5);
	sound_free(music_c4);
	sound_free(music_c3);
	sound_free(music_c2);
	sound_free(music_c1);

	sound_free(snd_gui);
	sound_free(snd_ship_kill);
	sound_free(snd_ship_acc);
	sound_free(snd_base_beep);
	sound_free(snd_base_zero);
	sound_free(snd_base_taken);
	sound_free(snd_ship_bump);
	sound_free(snd_wall_bump);
	sound_free(snd_hit);
	sound_free(snd_bullet_wall);
	sound_free(snd_shot);

	sound_close();
}

void sounds_event(SoundEventType type) {
	sounds_event_ex(type, ~0);
}

void sounds_event_ex(SoundEventType type, uint arg) {
	switch(type) {
		case MUSIC:
			sound_stop(music);
			
			if (arg == 1) music = music_c1;
			else if (arg == 2) music = music_c2;
			else if (arg == 3) music = music_c3;
			else if (arg == 4) music = music_c4;
			else if (arg == 5) music = music_c5;
			else music = music_menu;

			sound_play(music);
			return;
		case SHOT:
			sound_play(snd_shot);
			return;
		case COLLISION_BULLET_WALL:
			sound_play(snd_bullet_wall);
			return;
		case COLLISION_BULLET_SHIP:
			sound_play(snd_hit);
			return;
		case COLLISION_SHIP_SHIP:
			sound_play(snd_ship_bump);
			return;
		case COLLISION_SHIP_WALL:
			sound_play(snd_wall_bump);
			return;
		case PLATFORM_TAKEN:
			sound_play(snd_base_taken);
			return;
		case PLATFORM_NEUTRALIZED:
			sound_play(snd_base_zero);
			return;
		case PLATFORM_BEEP:
			//sound_play(snd_base_beep);
			return;
		case SHIP_ACC:
			// If sound ended (could be 3 sounds: init, continuous, end)
			// game.c, line 346
			//sound_play(snd_ship_acc);
			return;
		case SHIP_KILL:
			sound_play(snd_ship_kill);
			return;
		case GUI_CLICK:
			//sound_play(snd_gui);
			return;
		default:
			break;
	};

	//LOG_ERROR("Unknown SoundEventType");
}	

