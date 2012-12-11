#ifndef GAME_H
#define GAME_H

#include <malka/ml_states.h>

extern StateDesc game_state;

bool game_is_paused(void);
void game_pause(void);
void game_unpause(void);
void game_reset(void);

#endif
