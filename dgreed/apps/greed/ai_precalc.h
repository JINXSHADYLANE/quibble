#ifndef AI_PRECALC_H
#define AI_PRECALC_H

#include "utils.h"
#include "darray.h"

typedef struct {
	uint n_nodes;
	float* adjacency;
	float* distance;
	float* radius;
} NavMesh;	

NavMesh ai_precalc_navmesh(DArray geometry);

#endif

