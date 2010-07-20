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

void ai_precalc_bounds(float width, float height);
NavMesh ai_precalc_navmesh(DArray geometry, float width, float height);
void ai_free_navmesh(NavMesh mesh);
void ai_save_navmesh(FileHandle file, const NavMesh* navmesh);
NavMesh ai_load_navmesh(FileHandle file);

// Returns shortest path segment in wrap-around space
Segment ai_shortest_path(Vector2 p1, Vector2 p2);
// Splits segment into two if it crosses world bound and returns true.
// Returns false if segment is within bounds.
bool ai_split_path(Segment path, Segment* out_p1, Segment* out_p2);

// Visible for testing:
float ai_wall_distance(Vector2 p, DArray geometry);

#endif

