#ifndef GAME_H
#define GAME_H

#include <malka/ml_states.h>

extern StateDesc game_state;

extern bool camera_follow;

void game_pause(void);
void game_unpause(void);
void game_request_reset(void);
bool game_update(void);

void game_end(void);
bool game_update_empty(void);

void game_render_level(void);

#endif
