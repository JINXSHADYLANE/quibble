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

// Get mouse position in tile coordinates
void game_mouse_tile(uint mpos_x, uint mpos_y, uint* row, uint* col);
// Return tile shift and axis
void game_delta_tile(uint clicked_row, uint clicked_col, uint rel_row, 
	uint rel_col, int* shift, bool* axis);
// Change puzzle state (do tiles rotation)
void game_rotate_board(int shift, bool axis);

#endif // GAME_H

