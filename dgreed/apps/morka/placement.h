#ifndef PLACEMENT_H
#define PLACEMENT_H

#include "obj_types.h"
#include <darray.h>

typedef struct{
	const char* tiles;	
	const char* unwanted;
} PlacementRuleReadable;

typedef struct{
	uint start_tile, n_tiles;
	uint start_unwanted, n_unwanted;
} PlacementRule;

typedef struct{
	Vector2 range;
	uint start_unwanted, n_unwanted;
} PlacementInterval;

void placement_init(void);
void placement_close(void);
void placement_reset(void);

void placement_interval(Vector2 pos, SprHandle spr);
bool placement_allowed(Vector2 pos, SprHandle spr);

#endif
