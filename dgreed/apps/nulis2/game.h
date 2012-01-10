#ifndef GAME_H
#define GAME_H

#include <malka/ml_states.h>

// Main game state

extern StateDesc game_state;

void game_init(void);
void game_close(void);
void game_enter(void);
void game_leave(void);
bool game_update(void);
bool game_render(float t);

#endif
