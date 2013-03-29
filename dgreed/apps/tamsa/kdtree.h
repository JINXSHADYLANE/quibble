#ifndef KDTREE_H
#define KDTREE_H

#include <utils.h>
#include <darray.h>

typedef struct {
	int8 ax, ay, bx, by;
	byte surface;
} KdSeg;

// 8 bytes
typedef struct {
	int8 o, a, b;
	byte surface;
	uint16 left, right;
} KdNode;

typedef DArray KdTree;

void kdtree_init(KdTree* tree, KdSeg* segs, uint n_segs);
void kdtree_free(KdTree* tree);

// Traces a packet of n rays and fills up hit surface info for each ray.
// Destroys the data, puts hit positions in ox, oy and normals in dx, dy.
void kdtree_trace_surface(
	float* ox, float* oy, float* dx, float* dy, 
	byte* surfaces, uint n
);

// Traces a packet of n shadow rays, returns 0 - 1 ratio
// of how many hit the target at (ex, ey)
float kdtree_trace_shadow(
	float* ox, float* oy, float ex, float ey, uint n
);


#endif
