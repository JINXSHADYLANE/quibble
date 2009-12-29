#ifndef GAME_H
#define GAME_H

#include "puzzles.h"

typedef enum
{
	MENU_STATE,
	PUZZLE_STATE
} GameState;

extern GameState game_state;

// Load new puzzle here
void game_reset(uint puzzle_num);

void game_init(void);

// Paint puzzle on screen
void game_render(void);
// Update game: input, new loaded puzzle...
void game_update(void);
// Close game.
void game_close(void);

#endif // GAME_H

