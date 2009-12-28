#include "game.h"
#include "common.h"

void game_init(void)
{
	// Get current puzzle as local game variable

// ToDo: take not the first puzzle
	game_puzzle_num = 0;
	game_puzzle = *puzzle_descs;

	// Fill game_state for game start
	game_tiles_total = game_puzzle.width * game_puzzle.height; 

	for (uint i=0; i<game_tiles_total; i++)
// ToDo: make sure it's alright
		*(game_state + i) = i;
	assert(game_state == NULL);

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
		src = puzzle_get_tile_rect(game_puzzle_num, i);
		
		// ... And place where it belongs
		dest.left = (i % game_puzzle.width) * game_puzzle.tile_size.x +
					 game_corner.x;
		dest.top = (i / game_puzzle.width) * game_puzzle.tile_size.y + 
				    game_corner.y;
		dest.right = ((i % game_puzzle.width) + 1)  * 
					  game_puzzle.tile_size.x + game_corner.x;
		dest.bottom = ((i / game_puzzle.width) + 1) * 
					  game_puzzle.tile_size.y + game_corner.y;

		assert(dest.left < 0 || dest.left > SCREEN_WIDTH);
		assert(dest.right < 0 || dest.right > SCREEN_WIDTH);
		assert(dest.top < 0 || dest.top > SCREEN_HEIGHT);
		assert(dest.bottom < 0 || dest.bottom > SCREEN_HEIGHT);
		
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
}

