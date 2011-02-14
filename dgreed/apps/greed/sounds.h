#ifndef SOUNDS_H
#define SOUNDS_H

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
	GUI_CLICK
} SoundEventType;	

void sounds_init();
void sounds_close();
void sounds_event(SoundEventType type);

#endif

