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

// Traces a single ray, returns true if it hit something and shortens dx,dy
// accordingly. Tracing ray packets with functions below is much more efficient!
bool kdtree_trace_single(
	KdTree* tree, float ox, float oy, float* dx, float* dy
);

// Traces a packet of n rays and fills up hit surface info for each ray.
// Destroys the data, puts hit positions in ox, oy and normals in dx, dy.
void kdtree_trace_surface(
	KdTree* tree, float* ox, float* oy, float* dx, float* dy, 
	byte* surfaces, uint n
);

// Traces packet of n light rays, doesn't preserve order,
// sets lightid of occluded rays to 0xFF
void kdtree_trace_light(
	KdTree* tree, float* ox, float* oy, float* dx, float* dy,
	byte* lightid, uint n
);

#endif
