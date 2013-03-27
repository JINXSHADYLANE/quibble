#ifndef GAME_OVER_H
#define GAME_OVER_H

#include <malka/ml_states.h>

extern StateDesc game_over_state;

typedef enum{
	TUTORIAL_SCREEN = 0,
	OUT_SCREEN,
	WIN_SCREEN,
	LOSE_SCREEN
} ScreenType;

void game_over_set_screen(ScreenType s);

#endif
