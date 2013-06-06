#ifndef WORLD_H 
#define WORLD_H

#include <utils.h>
#include "kdtree.h"

typedef struct {
	KdTree walls;
	float floor;
	float ceiling;
} World;

void world_cast_primary(
	World* world, Vector2 p, Vector2 dir, 
	float fov, uint y_res, float* out_x, float* out_y,
	float* out_nx, float* out_ny
);

void world_render(
	World* world, Vector2 p, Vector2 dir,
	float fov, uint y_res, uint x_res, Color* backbuffer
);

#endif
