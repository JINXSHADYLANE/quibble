#include "game.h"

#include "common.h"
#include "background.h"
#include <memory.h>

// Globals
PuzzleDesc game_puzzle;
uint game_puzzle_num;
uint* puzzle_state = NULL;
uint game_tiles_total;
Vector2 game_corner;
GameState game_state;

uint mouse_start_x, mouse_start_y;
uint mouse_end_x, mouse_end_y;
uint tile_clicked_row, tile_clicked_col;
int shift;

// Tweakables
uint alpha = 255;
uint alpha_min = 0;
uint alpha_max = 255;
float alpha_change = 0.0f;
float alpha_const = 0.25f;
bool animate = false;
bool animate_plus = false;
bool animate_minus = false;
bool game_rotate = false;
// 0 if x axis
// 1 if y axis.
bool axis = 0;

void game_init(void)
{
}

void game_reset(uint puzzle_num)
{
	assert(puzzle_num < puzzle_count);

	game_puzzle_num = puzzle_num;
	game_puzzle = puzzle_descs[game_puzzle_num];

	// Fill puzzle_state for game start
	game_tiles_total = game_puzzle.width * game_puzzle.height; 

	if(puzzle_state)
		MEM_FREE(puzzle_state);
	puzzle_state = MEM_ALLOC(game_tiles_total * sizeof(uint));
// ToDo: randomize puzzle start state
	puzzle_state = memcpy(puzzle_state, game_puzzle.solved,
					    game_tiles_total * sizeof(uint));

	game_corner.x = SCREEN_WIDTH/2 - (game_puzzle.tile_size.x * 
					game_puzzle.width) / 2;
	game_corner.y = SCREEN_HEIGHT/2 - (game_puzzle.tile_size.y *
					game_puzzle.height) / 2;


// ToDo: Is it needed?
	animate = false;
	animate_plus = false;
	animate_minus = false;
	game_rotate = false;
}

void game_render(void)
{
	background_render();

	// Alpha counting part

	if (animate_plus) alpha_change += alpha_const;
	else if (animate_minus) alpha_change -= alpha_const;
	else alpha_change = 1;

	alpha = color_lerp(alpha_min, alpha_max, alpha_change);

	if (alpha == 0 && animate_minus) game_rotate = true;
	if (alpha == 255) animate_plus = false;

	assert(alpha >= 0 && alpha < 256);

	// Table drawing part

	RectF src = rectf_null();
	RectF dest = rectf_null();

	for (uint i=0; i<game_tiles_total; i++)
	{
		// Take i-th tile from original picture...
		src = puzzle_get_tile_rect(game_puzzle_num, puzzle_state[i]);
	
		// ... And place where it belongs
		dest.left = (i % game_puzzle.width) * game_puzzle.tile_size.x +
			game_corner.x;
		dest.top = (i / game_puzzle.width) * game_puzzle.tile_size.y + 
			game_corner.y;
		dest.right = ((i % game_puzzle.width) + 1)  * 
			game_puzzle.tile_size.x + game_corner.x;
		dest.bottom = ((i / game_puzzle.width) + 1) * 
			game_puzzle.tile_size.y + game_corner.y;
		
		assert(dest.left >= 0 && dest.left < SCREEN_WIDTH);
		assert(dest.right >= 0 && dest.right < SCREEN_WIDTH);
		assert(dest.top >= 0 && dest.top < SCREEN_HEIGHT);
		assert(dest.bottom >= 0 && dest.bottom < SCREEN_HEIGHT);
		
		animate = false;
		if (animate_plus || animate_minus)
			animate = shift;
		
		if (animate) 
			video_draw_rect(game_puzzle.image, 2, &src, &dest, COLOR_RGBA(255, 255, 255, alpha));
		else video_draw_rect(game_puzzle.image, 2, &src, &dest, COLOR_WHITE);
	}
}

void game_update(void)
{
	background_update();
	
	if (mouse_down(MBTN_LEFT))
	{
		mouse_pos(&mouse_start_x, &mouse_start_y);
		game_mouse_tile(mouse_start_x, mouse_start_y, &tile_clicked_row,
						&tile_clicked_col);
		if (tile_clicked_row < 0 || tile_clicked_row >= game_puzzle.height)
			tile_clicked_row = -1;
		if (tile_clicked_col < 0 || tile_clicked_col >= game_puzzle.width)
			tile_clicked_col = -1;
		
	}
	else if (mouse_up(MBTN_LEFT))
	{
		uint tile_rel_row, tile_rel_col;
	
		mouse_pos(&mouse_end_x, &mouse_end_y);
		game_mouse_tile(mouse_end_x, mouse_end_y, &tile_rel_row, &tile_rel_col);

		game_delta_tile(tile_clicked_row, tile_clicked_col, tile_rel_row,
						tile_rel_col, &shift, &axis);
	
		animate_minus = shift;
	}

	if (game_rotate) 
	{
		animate_minus = false;
		game_rotate_board(shift, axis);
		animate_plus = true;
		game_rotate = false;
	}
}

void game_close(void)
{
	if(puzzle_state)
		MEM_FREE(puzzle_state);
	puzzle_state = NULL;	
}

void game_mouse_tile(uint mpos_x, uint mpos_y, uint* row, uint* col)
{
	*row = (mpos_y - game_corner.y) / game_puzzle.tile_size.y;
	*col = (mpos_x - game_corner.x) / game_puzzle.tile_size.x;
}

void game_delta_tile(uint clicked_row, uint clicked_col, uint rel_row, uint
	rel_col, int* shift, bool* axis)
{
	int dy = rel_row - clicked_row;
	int dx = rel_col - clicked_col;
	
	if (dx)
	{
		*axis = 0;
		*shift = dx;
	}
	else
	{
		*axis = 1;
		*shift = dy;
	}
	if (dx && dy) *shift = 0;

}

void game_rotate_board(int shift, bool axis)
{
	if (tile_clicked_row == -1 || tile_clicked_col == -1) return;

	uint row_first = game_puzzle.width * tile_clicked_row;
	uint row_last = game_puzzle.width * (tile_clicked_row+1) - 1;
	uint col_first = tile_clicked_col;
	uint col_last = (game_puzzle.height - 1) * game_puzzle.width + tile_clicked_col;
	
	assert(row_first < game_tiles_total);
	assert(row_last < game_tiles_total);
	assert(col_first < game_tiles_total);
	assert(col_last < game_tiles_total);

	// Rotates right
	if (shift > 0 && axis == 0)
	{
		uint temp = puzzle_state[row_last]; 
		for (uint i=row_last; i>row_first; i--)
			puzzle_state[i] = puzzle_state[i-1];
		puzzle_state[row_first] = temp;
		return;
	}
	// Rotates left
	if (shift < 0 && axis == 0)
	{
		uint temp = puzzle_state[row_first];
		for (uint i=row_first; i<row_last; i++)
			puzzle_state[i] = puzzle_state[i+1];
		puzzle_state[row_last] = temp;
		return;
	}

	// Rotates down
	if (shift > 0 && axis == 1)
	{
		uint temp = puzzle_state[col_last];
		for (uint i=col_last; i>col_first; i-=game_puzzle.width)
			puzzle_state[i] = puzzle_state[i - game_puzzle.width];
		puzzle_state[col_first] = temp;
		return;
	}
	// Rotates up
	if (shift < 0 && axis == 1)
	{
		uint temp = puzzle_state[col_first];
		for (uint i=col_first; i<col_last; i+=game_puzzle.width)
			puzzle_state[i] = puzzle_state[i + game_puzzle.width];
		puzzle_state[col_last] = temp;
		return;
	}
	
	return;
}

