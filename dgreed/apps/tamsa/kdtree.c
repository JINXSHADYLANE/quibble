#include "kdtree.h"

// 0 - vertical, 1 - horizontal
static int _seg_dir(KdSeg seg) {
	assert(seg.ay == seg.by || seg.ax == seg.bx);
	return (seg.ay - seg.by) != 0;
}

static void __attribute__((unused)) _seg_plane(KdSeg seg, int* a, int* b, int* c) {
//	assert(seg.ay == seg.by || seg.ax == seg.bx);
	// todo
}

static int __attribute__((unused)) _eval_splitplane(KdSeg* segs, uint i, uint j, int8 a, int8 b, int8 c) {
	// Our heuristic:
	// abs(left - right)^2 + splits*8
	
	int left = 0, right = 0, splits = 0;
	for(uint p = i; p < j; ++p) {
		KdSeg seg = segs[p];

		int side_a = seg.ax * a + seg.ay * b + c;
		int side_b = seg.bx * a + seg.by * b + c;

		if(side_a < 0 && side_b < 0)
			left++;
		else if(side_a >= 0 && side_b >= 0)
			right++;
		else 
			splits++;
	}

	int d = left - right;
	return d * d + splits * 8;
}

void __attribute__((unused)) _split(KdSeg* segs, uint i, uint j, int dir, int min_x, int max_x, int min_y, int max_y) {
	// Find best splitplane
	for(uint p = i; p < j; ++p) {
		KdSeg pivot = segs[p];
		if(_seg_dir(pivot) == dir) {
			// ...
		}
	}
	
	// Mask pivot seg
	
	// Split
	
	// Recursively split children
}

void kdtree_init(KdTree* tree, KdSeg* segs, uint n_segs) {
	*tree = darray_create(sizeof(KdNode), 0);
}

void kdtree_free(KdTree* tree) {
	darray_free(tree);
}
