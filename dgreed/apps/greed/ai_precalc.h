#ifndef AI_PRECALC_H
#define AI_PRECALC_H

#include "utils.h"
#include "darray.h"

typedef struct {
	uint n_nodes;
	Vector2* navpoints; // Navpoints positions
	float* adjacency; // Graph adjacency matrix
	float* distance; // Shortest path between each pair of nodes
	float* radius; // Distance to wall
} NavMesh;	

typedef struct {
	uint v1, v2;
} Edge;	

NavMesh ai_precalc_navmesh(DArray geometry);
void ai_free_navmesh(NavMesh mesh);
void ai_save_navmesh(FileHandle file, const NavMesh* navmesh);
NavMesh ai_load_navmesh(FileHandle file);

#endif

