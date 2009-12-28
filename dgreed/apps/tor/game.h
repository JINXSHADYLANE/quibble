#ifndef GAME_H
#define GAME_H

#include "puzzles.h"

PuzzleDesc game_puzzle;
uint game_puzzle_num;
uint* game_state;
uint game_tiles_total;
Vector2 game_corner;

// Load new puzzle here
void game_init(void);
// Paint puzzle on screen
void game_render(void);
// Update game: input, new loaded puzzle...
void game_update(void);
// Close game.
void game_close(void);


#endif // GAME_H

