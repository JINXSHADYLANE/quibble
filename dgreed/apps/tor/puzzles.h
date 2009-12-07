#ifndef PUZZLES_H
#define PUZZLES_H

#include <system.h>
#include <utils.h>

typedef struct {
	char* name;
	TexHandle image;
	Vector2 tile_size; // single tile size in pixels
	uint width, height; // puzzle size in tiles
	uint* solved; // width * height uints which represent solved puzzle
} PuzzleDesc;

extern uint puzzle_count;
extern PuzzleDesc* puzzle_descs;

// Must be initialized after video!
void puzzles_init(void);
void puzzles_close(void);

#endif

