#ifndef WORLD_H 
#define WORLD_H

#include <utils.h>
#include "kdtree.h"

#define max_lights 16

typedef struct {
	float x, y, z, r;
	float inv_att, em_r, em_g, em_b;
} Light;

typedef struct {
	KdTree walls;
	float floor;
	float ceiling;
	DArray lights;
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
