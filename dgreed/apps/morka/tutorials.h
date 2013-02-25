#ifndef TUTORIALS_H
#define TUTORIALS_H

#include <utils.h>
#include "obj_types.h"

typedef enum{
	//NONE = 0,				// NULL event (never triggered)
	AUTO,					// Happens automatically (always triggered)
	MUSHROOM_IN_FRONT,		// Mushroom available in front of rabbit
	MUSHROOM_BELOW,			// Mushroom below rabbit, possible to dive
	BOUNCE_PERFORMED,		// Succesfull dive+bounce on a mushroom performed.
	COMBO_X3,				// Combo x3 is performed
	COINS_10,				// 10 Coins are collected

	FINAL_STEP				// Indicates last step (never triggered)	
} EventType;

typedef struct {
	float delay;
	const char* text;
	const char* img;
	const char* state;
	Vector2 finger_pos;
	bool pause_game;
	bool force_input;
	bool disable_input;
	bool show_hint_press;

	EventType show_on;
	EventType dismiss_on;
} TutorialStep;

typedef struct {
	const char* level_name;
	TutorialStep* steps;
} TutorialScenario;

void tutorial_event(EventType e);
void tutorials_init(void);
void tutorials_close(void);
void tutorials_set_level(const char* level);
void tutorials_reset(ObjRabbit* r);
bool tutorials_render(float t);

void tutorials_enable(void);
void tutorials_disable(void);
bool tutorials_are_enabled(void);
bool tutorials_show_paused(void);

void tutorials_hint_press(bool p);


#endif