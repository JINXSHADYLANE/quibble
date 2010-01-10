#ifndef GAME_H
#define GAME_H

#include "puzzles.h"

typedef enum
{
	MENU_STATE,
	PUZZLE_STATE
} GameState;

extern GameState game_state;

// Game initialization
void game_init(void);
// Load new puzzle here
void game_reset(uint puzzle_num);
// Paint puzzle on screen
void game_render(void);
// Update game: input, new loaded puzzle...
void game_update(void);
// Close game.
void game_close(void);

// Get mouse position in tiles number
uint game_mouse_tile(uint mpos_x, uint mpos_y);
// Returns number showing delta direction from mouse start point
int game_delta_tile(uint mpos0, uint mpos1, int tile_size);
// Change puzzle state (do tiles rotation)
void game_rotate_board(int dtile_x, int dtile_y, uint tile_clicked);

#endif // GAME_H

