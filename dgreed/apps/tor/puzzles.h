#ifndef PUZZLES_H
#define PUZZLES_H

#include <utils.h>

typedef struct {
	const char* name;
	TexHandle image;
	Vector2 tile_size; // single tile size in pixels
	uint width, height; // puzzle size in tiles
	uint* solved; // width * height uints which represent solved puzzle
} PuzzleDesc;

extern uint puzzle_count;
extern PuzzleDesc* puzzle_descs;

void puzzles_init(void);
void puzzles_close(void);

#endif

