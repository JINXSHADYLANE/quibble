#include "game.h"
#include "common.h"
#include "background.h"
#include <memory.h>

PuzzleDesc game_puzzle;
uint game_puzzle_num;
uint* puzzle_state = NULL;
uint game_tiles_total;
Vector2 game_corner;
GameState game_state;

uint mouse_start_x, mouse_start_y;
uint mouse_end_x, mouse_end_y;
int dtile_x, dtile_y;
uint tile_clicked;

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

	// Initial puzzle corner coordinates
	game_corner.x = SCREEN_WIDTH/2 - (game_puzzle.tile_size.x * 
					game_puzzle.width) / 2;
	game_corner.y = SCREEN_HEIGHT/2 - (game_puzzle.tile_size.y *
					game_puzzle.height) / 2;

}

void game_render(void)
{
	background_render();

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
		
		// Draw that piece
		video_draw_rect(game_puzzle.image, 2, &src, &dest, COLOR_WHITE);
	}
}

void game_update(void)
{
	background_update();
	
	// algorithm to change current puzzle.
	
	// Initialize values?

	if (mouse_down(MBTN_RIGHT))
	{
		mouse_pos(&mouse_start_x, &mouse_start_y);
		// get tile number
		tile_clicked = game_mouse_tile(mouse_start_x, mouse_start_y);
	}
	else if (mouse_up(MBTN_RIGHT))
	{
		mouse_pos(&mouse_end_x, &mouse_end_y);
		dtile_x = game_delta_tile(mouse_start_x, mouse_end_x, game_puzzle.tile_size.x);
		dtile_y = game_delta_tile(mouse_start_y, mouse_end_y, game_puzzle.tile_size.y);
		
		// ToDo: Animate rotation
		game_rotate_board(dtile_x, dtile_y, tile_clicked);
	}

// If puzzle loaded and mouse just pressed:
	//	 Take mouse coordinates
	//	 check for mouse position (square number)
	// If mouse just released:
	//	 Count_delta_coordinates()
	//	 rotate_puzzle(row, col, direction)

}

void game_close(void)
{
	if(puzzle_state)
		MEM_FREE(puzzle_state);
	puzzle_state = NULL;	
}

uint game_mouse_tile(uint mpos_x, uint mpos_y)
{
	mpos_x = mpos_x - game_corner.x;
	mpos_y = mpos_y - game_corner.y;
	
	// Check if mouse position is out of board.
	if (mpos_x >= (game_puzzle.tile_size.x * game_puzzle.width))
		return -1;
	if (mpos_y >= (game_puzzle.tile_size.y * game_puzzle.height))
		return -1;
	if (mpos_x < 0 || mpos_y < 0)
		return -1;

	uint row = mpos_y / game_puzzle.tile_size.y;
	uint col = mpos_x / game_puzzle.tile_size.x;

	return (row * game_puzzle.width + col);
}

int game_delta_tile(uint mpos0, uint mpos1, int tile_size)
{
	if (mpos1 > mpos0)
	{
		if (mpos1 - mpos0 >= tile_size) return 1;
		else return 0;
	}
	else if (mpos1 < mpos0)
	{
		if (mpos0 - mpos1 >= tile_size) return -1;
		else return 0;
	}
	return 0;
}

void game_rotate_board(int dtile_x, int dtile_y, uint tile_clicked)
{
	if (tile_clicked == -1) return;

	uint row = tile_clicked / game_puzzle.width;
	uint col = tile_clicked % game_puzzle.width;
	uint row_first = game_puzzle.width * row;
	uint row_last = game_puzzle.width * (row+1) - 1;
	uint col_first = col;
	uint col_last = (game_puzzle.height - 1) * game_puzzle.width + col;

	assert(row_first < game_tiles_total);
	assert(row_last < game_tiles_total);
	assert(col_first < game_tiles_total);
	assert(col_last < game_tiles_total);

	// Rotates right
	if (dtile_x == 1)
	{
		uint temp = puzzle_state[row_last]; 
		for (uint i=row_last; i>row_first; i--)
			puzzle_state[i] = puzzle_state[i-1];
		puzzle_state[row_first] = temp;
		return;
	}
	// Rotates left
	if (dtile_x == -1)
	{
		uint temp = puzzle_state[row_first];
		for (uint i=row_first; i<row_last; i++)
			puzzle_state[i] = puzzle_state[i+1];
		puzzle_state[row_last] = temp;
		return;
	}
	// Rotates down
	if (dtile_y == 1)
	{
		uint temp = puzzle_state[col_last];
		for (uint i=col_last; i>col_first; i-=game_puzzle.width)
			puzzle_state[i] = puzzle_state[i - game_puzzle.width];
		puzzle_state[col_first] = temp;
		return;
	}
	// Rotates up
	if (dtile_y == -1)
	{
		uint temp = puzzle_state[col_first];
		for (uint i=col_first; i<col_last; i+=game_puzzle.width)
			puzzle_state[i] = puzzle_state[i + game_puzzle.width];
		puzzle_state[col_last] = temp;
		return;
	}
	
	return;
}

