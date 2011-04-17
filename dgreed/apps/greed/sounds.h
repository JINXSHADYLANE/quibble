#ifndef SOUNDS_H
#define SOUNDS_H

#include <utils.h>

typedef enum {
	MUSIC,
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
void sounds_event(SoundEventType type);
void sounds_event_ex(SoundEventType type, uint arg);

#endif

