#include "sounds.h"
#include <system.h>

#define UNDEFINED ~0U
#define SOUNDDEF_PADDING {NULL, UNDEFINED}
#define SOUNDDEF(filename) {filename, UNDEFINED}  

typedef struct {
	const char* filename;
	SoundHandle handle;
} SoundDef;	

// These must precisely match SoundEventType enum to benefit 
// from ultra-fast O(1) lookups
SoundDef snd[] = {
	SOUNDDEF_PADDING,
	SOUNDDEF("SHOT"),
	SOUNDDEF("BULLET_WALL"),
	SOUNDDEF("SHIP_HIT"),
	SOUNDDEF("SHIP_HIT"),
	SOUNDDEF("SHIP_SHIP"),
	SOUNDDEF("SHIP_WALL"),
	SOUNDDEF("BASE_TAKEN"),
	SOUNDDEF("BASE_ZERO"),
	SOUNDDEF("BASE_BEEP"),
	SOUNDDEF("SHIP_ACC"),
	SOUNDDEF("SHIP_KILL"),
	SOUNDDEF("GUI_CLICK")
};


typedef enum {
	MUS_C1 = 0, 
	MUS_C2,
	MUS_C3,
	MUS_C4,
	MUS_C5,
	MUS_ERROR,
	MUS_MENU
} MusicType;

// Like snd, this must match MusicType enum above
SoundDef music[] = {
	SOUNDDEF("C1"),
	SOUNDDEF("C2"),
	SOUNDDEF("C3"),
	SOUNDDEF("C4"),
	SOUNDDEF("C5"),
	SOUNDDEF("ERROR"),
	SOUNDDEF("MENU")
};	

SoundHandle current_music = UNDEFINED;
float music_volume = 1.0f;
float effect_volume = 1.0f;

static void _load_samples(void) {
	char filename[256];
	for(uint i = 0; i < ARRAY_SIZE(snd); ++i) {
		if(snd[i].filename) {
			sprintf(filename, "greed_assets/%s.wav", snd[i].filename);
			snd[i].handle = sound_load_sample(filename);
		}	
	}
}

static void _unload_samples(void) {
	for(uint i = 0; i < ARRAY_SIZE(snd); ++i) {
		if(snd[i].handle != UNDEFINED)
			sound_free(snd[i].handle);
	}
}

static void _unload_music(void) {
	if(current_music != UNDEFINED)
		sound_free(current_music);
}

static void _load_music(uint id) {
	char filename[256];
	_unload_music();
	if(music[id].filename) {
		sprintf(filename, "greed_assets/%s.ogg", music[id].filename);
		current_music = sound_load_stream(filename);
	}
}

void sounds_init() {
	sound_init();
	_load_samples();

	_load_music(MUS_MENU);
	sound_play(current_music);
}	

void sounds_close() {
	_unload_music();
	_unload_samples();
	sound_close();
}

void sounds_event(SoundEventType type) {
	sounds_event_ex(type, UNDEFINED);
}

void sounds_event_ex(SoundEventType type, uint arg) {
	switch(type) {
		case MUSIC:
			sound_stop(current_music);

			if(arg >= 1 && arg <= 5)
				_load_music(MUS_C1 + arg-1);
			else 
				_load_music(MUS_MENU);

			sound_set_volume(current_music, music_volume);
			sound_play(current_music);
			return;
		case PLATFORM_BEEP:
			//sound_play(snd[PLATFORM_BEEP].handle);
			return;
		case SHIP_ACC:
			// If sound ended (could be 3 sounds: init, continuous, end)
			// game.c, line 346
			//sound_play(snd[SHIP_ACC].handle);
			return;
		case GUI_CLICK:
			//sound_play(snd[GUI_CLICK].handle);
			return;
		default:
			if(snd[type].handle != UNDEFINED)
				sound_play(snd[type].handle);
			break;
	};

	//LOG_ERROR("Unknown SoundEventType");
}	

void sounds_set_effect_volume(float volume) {
	if(volume != effect_volume) {
		for(uint i = 0; i < ARRAY_SIZE(snd); ++i) {
			if(snd[i].handle != UNDEFINED)
				sound_set_volume(snd[i].handle, volume);
		}
		effect_volume = volume;
	}
}

void sounds_set_music_volume(float volume) {
	if(volume != music_volume) {
		sound_set_volume(current_music, volume);
		music_volume = volume;
	}
}

