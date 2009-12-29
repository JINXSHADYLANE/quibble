#include "game.h"
#include "common.h"
#include <memory.h>

PuzzleDesc game_puzzle;
uint game_puzzle_num;
uint* puzzle_state = NULL;
uint game_tiles_total;
Vector2 game_corner;
GameState game_state;

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
		video_draw_rect(game_puzzle.image, 1, &src, &dest, COLOR_WHITE);
	}
}

void game_update(void)
{
	
	// algorithm to change current puzzle.

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

