#include "kdtree.h"

static int _sgn(int x) {
	int m = x >> 31;
	uint n = (uint) -x;
	int p = (int)(n >> 31);
	int result = m | p;
	return result;
}

// 0 - vertical, 1 - horizontal
static int _seg_dir(KdSeg seg) {
	assert(seg.ay == seg.by || seg.ax == seg.bx);
	return (seg.ay - seg.by) != 0;
}

static void _seg_plane(KdSeg seg, int* a, int* b, int* c) {
	int la = _sgn(seg.ay - seg.by);
	int lb = _sgn(seg.ax - seg.bx);

	*a = la;
	*b = lb;
	*c = la * seg.ax + lb * seg.ay;

	assert(la * seg.bx + lb * seg.by + *c == 0);
}

static int _eval_splitplane(KdSeg* segs, uint i, uint j, 
	int a, int b, int c, int* n_left, int* n_right) {

	// Our heuristic:
	// (left - right)^2 + splits*8
	
	int left = 0, right = 0, splits = 0;
	for(uint p = i; p < j; ++p) {
		KdSeg seg = segs[p];

		int side_a = _sgn(seg.ax * a + seg.ay * b + c);
		int side_b = _sgn(seg.bx * a + seg.by * b + c);

		int sum = side_a + side_b;

		if(sum < 0)
			left++;
		else if(sum > 0 || (side_a | side_b) == 0)
			right++;
		else 
			splits++;
	}

	*n_left = left + splits;
	*n_right = right + splits;

	int d = left - right;
	return d * d + splits * 8;
}

void _split_seg(KdSeg seg, int a, int b, int c, KdSeg* left, KdSeg* right) {
	int a2, b2, c2;
	_seg_plane(seg, &a2, &b2, &c2);

	// Now solve system of linear equations using Cramer's rule:
	// a*x + b*y  = -c
	// a2*x + b2*y = -c2
	
	int d = a * b2 - b * a2;
	int x = (-c * b2 - b * (-c2)) / d;
	int y = (a * (-c2) - (-c) * a2) / d;

	// Split seg into two new

	KdSeg seg_a = {
		.ax = seg.ax,
		.ay = seg.ay,
		.bx = x,
		.by = y,
		.surface = seg.surface
	};

	KdSeg seg_b = {
		.ax = x,
		.ay = y,
		.bx = seg.bx,
		.by = seg.by,
		.surface = seg.surface
	};

	// Determine which new seg goes to which side

	int side_a = a * seg.ax + b * seg.ay + c;

#ifdef _DEBUG
	int side_b = a * seg.bx + b * seg.by + c;
	assert(side_a + side_b == 0 && side_a != 0 && side_b != 0);
#endif

	if(side_a < 0) {
		*left = seg_a;
		*right = seg_b;
	}
	else {
		*left = seg_b;
		*right = seg_a;
	}
}

void __attribute__((unused)) _split(KdSeg* segs, DArray* out, uint i, uint j, int dir) {
	assert(i <= j);

	if(j - i < 2) {
		// Stop recursion
	}

	// Find best splitplane
	int bk = -1, ba = 0, bb = 0, bc = 0;
	int bf = MAX_INT32;
	int n_left = 0;
	int n_right = 0;
	for(uint k = i; k < j; ++k) {
		KdSeg pivot = segs[k];
		if(_seg_dir(pivot) == dir) {
			int a, b, c;
			_seg_plane(pivot, &a, &b, &c);

			segs[k] = segs[i];

			int nl, nr;
			int fitness = _eval_splitplane(
				segs, i+1, j, a, b, c, &nl, &nr
			);

			if(fitness < bf) {
				bk = k; ba = a; bb = b; bc = c;
				bf = fitness; n_left = nl; n_right = nr;
			}

			segs[k] = pivot;
		}
	}

	// TODO:
	// Do something if no splitplane found - insert a dummy one maybe

	assert(bf < MAX_INT32);
	
	// Mask pivot seg
	KdSeg pivot = segs[bk];
	segs[bk] = segs[i];
	segs[i] = pivot;
	
	// Split
	darray_reserve(out, n_left + n_right);
	KdSeg* out_segs = out->data;
	int c_left = 0;
	int c_right = 0;
	for(uint k = i + 1; k < j; ++k) {
		KdSeg seg = segs[k];

		int side_a = _sgn(ba * seg.ax + bb * seg.ay + bc);
		int side_b = _sgn(ba * seg.bx + bb * seg.by + bc);
		int sum = side_a + side_b;

		if(sum < 0) {
			out_segs[c_left++] = seg;
		}
		else if(sum > 0 || (side_a | side_b) == 0) {
			out_segs[n_left + c_right++] = seg;
		}
		else {
			KdSeg left, right;
			_split_seg(seg, ba, bb, bc, &left, &right);
			out_segs[c_left++] = left;
			out_segs[n_left + c_right++] = seg;
		}
	}

	assert(c_left == n_left);
	assert(c_right == n_right);
	
	// Recursively split children
	
}

void kdtree_init(KdTree* tree, KdSeg* segs, uint n_segs) {
	*tree = darray_create(sizeof(KdNode), 0);
}

void kdtree_free(KdTree* tree) {
	darray_free(tree);
}

