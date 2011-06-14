#ifndef AI_PRECALC_H
#define AI_PRECALC_H

#include "utils.h"
#include "darray.h"

typedef struct {
	uint n_nodes;
	Vector2* navpoints; // Navpoints positions
	
	uint* neighbour_count;	// Amount of neighbours for each node
	uint* neighbours_start; // Where neighbour list starts
	uint* neighbours; // Neighbour lists
	
	float* distance; // Shortest path between each pair of nodes
	float* radius; // Distance to wall

	// Spatial hash grid for quick nearest-navpoint query
	uint* nn_grid_count;
	uint* nn_grid_start;
	uint* nn_grid;

	// Point-to-point visibility raycast query bitmap
	uint* vis_bitmap;
} NavMesh;	

typedef struct {
	uint v1, v2;
} Edge;	

void ai_precalc_bounds(float width, float height);
NavMesh ai_precalc_navmesh(DArray geometry, DArray platforms,
	float width, float height);
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

// Returns nearest-navpoint grid cell id
uint ai_nn_grid_cell(Vector2 p);

// Returns squared distance between two points
// in torus-shaped space
float ai_distance_sq(Vector2 p1, Vector2 p2);

// Returns nearest navpoint
uint ai_nearest_navpoint(NavMesh* navmesh, Vector2 pos);

// Returns next node in a path towards dest
uint ai_find_next_path_node(NavMesh* navmesh, uint current, uint dest);

// Returns rough distance between two points, 
// when traveling along navmesh edges
float ai_navmesh_distance(NavMesh* navmesh, Vector2 p1, Vector2 p2);

// Returns true if there is straight line of sight between two points;
// false negatives are possible, but not false positives
bool ai_vis_query(NavMesh* navmesh, Vector2 p1, Vector2 p2);

// Renders visibility bitmap
void ai_debug_vis_draw(NavMesh* navmesh, uint layer);

#endif

