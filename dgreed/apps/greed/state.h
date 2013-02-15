#ifndef STATE_H
#define STATE_H

#include <utils.h>

typedef struct {
	float sound_volume;
	float music_volume;

	uint control_type;
	Vector2 joystick_pos;
	Vector2 acc_btn_pos;
	Vector2 shoot_btn_pos;
	Vector2 left_btn_pos;
	Vector2 right_btn_pos;
} PersistentState;

extern PersistentState pstate;

// Loads existing state file, or default state
void state_init(void);
// Frees up internal resources
void state_close(void);

// Saves current state 
void state_commit(void);

#endif
