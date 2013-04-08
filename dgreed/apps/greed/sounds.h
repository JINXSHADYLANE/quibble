#ifndef SOUNDS_H
#define SOUNDS_H

#include <utils.h>

typedef enum {
	MUSIC = 0,
	SHOT,
	COLLISION_BULLET_WALL,
	COLLISION_BULLET_SHIP,
	COLLISION_SHIP_SHIP,
	COLLISION_SHIP_WALL,
	PLATFORM_TAKEN,
	PLATFORM_NEUTRALIZED,
	PLATFORM_BEEP,
	SHIP_ACC,
	SHIP_KILL,
	GUI_CLICK
} SoundEventType;	

void sounds_init();
void sounds_close();
void sounds_update();
void sounds_event(SoundEventType type);
void sounds_event_ex(SoundEventType type, uint arg);
void sounds_set_effect_volume(float volume);
void sounds_set_music_volume(float volume);

#endif

