#include "kdtree.h"

float eps = 0.00001f;

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
	return (seg.ay - seg.by) == 0;
}

static void _seg_plane(KdSeg seg, int* a, int* b, int* c) {
	int la = _sgn(seg.ay - seg.by);
	int lb = _sgn(seg.ax - seg.bx);

	*a = la;
	*b = lb;
	*c = -(la * seg.ax + lb * seg.ay);

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
	assert(_sgn(side_a) + _sgn(side_b) == 0 && side_a != 0 && side_b != 0);
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

void _split(KdTree* tree, uint node, KdSeg* segs, DArray* out, uint i, uint j, int dir) {
	assert(i <= j);
	int bk = -1, ba = 0, bb = 0, bc = 0;
	int bf = MAX_INT32;
	int n_left = 0;
	int n_right = 0;

	if(j - i < 2) {
		// Stop recursion
		KdSeg seg = segs[i];
		if(dir == _seg_dir(seg)) {
			bk = 0; bf = 0;
			goto split;
		}
		else {
			// Insert dummy node
			KdNode dummy = {
				.o = dir ? seg.ay : seg.ax,
				.a = dir ? seg.ax : seg.ay,
				.b = dir ? seg.ax : seg.ay,
				.surface = 0,
				.left = node + 1,
				.right = -1
			};

			darray_reserve(tree, node+1);
			KdNode* nodes = tree->data;
			tree->size = MAX(tree->size, node+1);
			nodes[node] = dummy;

			_split(tree, node + 1, segs, out, i, j, dir ^ 1);
			return;
		}
	}

	// Find best splitplane
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

split:
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
			out_segs[n_left + c_right++] = right;
		}
	}

	assert(c_left == n_left);
	assert(c_right == n_right);

	// Construct kdtree node
	KdNode new = {
		.o = dir ? pivot.ay : pivot.ax,
		.a = dir ? pivot.ax : pivot.ay,
		.b = dir ? pivot.bx : pivot.by,
		.surface = pivot.surface,
		.left = n_left ? node + 1 : -1,
		.right = -1			
	};

	// Recursively split children
	
	DArray child_out;
	
	if(n_left || n_right)
		child_out = darray_create(sizeof(KdSeg), 0);

	darray_reserve(tree, node+1);
	KdNode* nodes = tree->data;
	tree->size = MAX(tree->size, node+1);

	// Left
	if(n_left) {
		_split(tree, node + 1, out_segs, &child_out, 0, c_left, dir ^ 1);
	}

	// Right
	if(n_right) {
		new.right = tree->size;
		child_out.size = 0;
		_split(tree, new.right, out_segs, &child_out, c_left, c_left + c_right, dir ^ 1);
	}

	nodes[node] = new;

	if(n_left || n_right)
		darray_free(&child_out);
}

void kdtree_init(KdTree* tree, KdSeg* segs, uint n_segs) {
	*tree = darray_create(sizeof(KdNode), 0);

	DArray out = darray_create(sizeof(KdSeg), 0);

	_split(tree, 0, segs, &out, 0, n_segs, 0);

	darray_free(&out);
}

void kdtree_free(KdTree* tree) {
	darray_free(tree);
}

static void _node_plane(KdNode node, int dir, float* a, float* b, float* c) {
	float la = (float)((dir^1) * (node.a - node.b));
	float lb = (float)(dir * (node.a - node.b));

	float x = (float)((dir^1) * node.o + dir * node.a);
	float y = (float)((dir^1) * node.a + dir * node.o);

	*a = la;
	*b = lb;
	*c = - (la * x + lb * y);
}

#define _swap(i, mn) { \
	float tox=ox[i], toy=oy[i], tdx=dx[i], tdy=dy[i]; \
	uint16 tid=id[i]; \
	byte ts=surfaces[i]; \
	ox[i] = ox[mn]; oy[i] = oy[mn]; \
	dx[i] = dx[mn]; dy[i] = dy[mn]; \
	id[i] = id[mn]; \
	surfaces[i] = surfaces[mn]; \
	ox[mn] = tox; oy[mn] = toy; dx[mn] = tdx; dy[mn] = tdy; \
	id[mn] = tid; \
	surfaces[mn] = ts; \
}

static int _second_pass(int start, int end, float a, float b, float c,
	float* ox, float* oy, float* dx, float* dy, byte* surfaces, uint16* id, float eps
) {
	int skipped = 0;
	for(int i = start; i < end; ++i) {
		float side_e = (ox[i] + dx[i]) * a + (oy[i] + dy[i]) * b + c;
		if(side_e > eps) {
			_swap(i, end-1);
			end--; i--;
		}
		else {
			skipped++;
		}
	}
	return skipped;
}

static void _trace(KdNode* nodes,
	int node_id, int dir, float* ox, float* oy, float* dx, float* dy,
	byte* surfaces, uint start, uint end, uint16* id
) {
	KdNode node = nodes[node_id];

	float a, b, c;
	_node_plane(node, dir, &a, &b, &c);

	// Sort the data into four buckets 
	// 1. Rays that are on the left 
	// 2. Rays that start on the left and intersect node
	// 3. Rays that start on the right and intersect node
	// 4. Rays that are on the right
	
	// First sort pass, move types 3/4 to the back
	int l = 0, li = 0, r = 0, ri = 0;
	int n = end;
	for(int i = start; i < n; ++i) {
		float side_s = ox[i] * a + oy[i] * b + c;
		float side_e = (ox[i] + dx[i]) * a + (oy[i] + dy[i]) * b + c;

		if(side_s < eps && side_e < eps) {
			l++;
		}
		else if(side_s > -eps && side_e > -eps) {
			r++;
			_swap(i, n-1);
			n--; i--;
		}
		else {
			if(side_s < eps) {
				li++;
			}
			else {
				_swap(i, n-1);
				n--; i--;
				ri++;
			}
		}
	}

	// Do second sorting pass for left and right sides
	if(l && li)
		_second_pass(start, start + l + li, a, b, c, ox, oy, dx, dy, surfaces, id, eps);
	if(r && ri)
		_second_pass(start + l + li, end, a, b, c, ox, oy, dx, dy, surfaces, id, -eps);

	// Intersect rays of kinds 2 and 3 with node plane
	for(uint i = start + l; i < start + l + li + ri; ++i) {
		float lox = ox[i], loy = oy[i];
		float ldx = dx[i], ldy = dy[i];

		// Ray plane equation
		float a2 = -ldy;
		float b2 = ldx;
		float c2 = -(a2 * lox + b2 * loy);

		// Solve with Cramer's
		float d = a * b2 - b * a2;
		float inv_d = 1.0f / d;
		float x = (-c * b2 - b * (-c2)) * inv_d;
		float y = (a * (-c2) - (-c) * a2) * inv_d;
		
		float fa = node.a;
		float fb = node.b;

		// Check if intersection hits wall seg
		float p = dir ? x : y;
		float hits = (fa-fb)*(fa-fb) - (p-fa)*(p-fa) - (fb-p)*(fb-p); 
		if(hits >= -eps) {
			/*
			// Check if intersection is on the ray seg
			float t_ndx_dx = (x - lox) * ldx;
			float t_ndy_dy = (y - loy) * ldy;

			if(t_ndx_dx >= -eps && t_ndy_dy >= -eps) {
				if(t_ndx_dx < ldx * ldx + eps && t_ndy_dy < ldy * ldy + eps) {
					dx[i] = x - lox;
					dy[i] = y - loy;
					surfaces[i] = node.surface;
				}
			}
			*/

			// Screw it, kdtree guarantees ordering so don't check seg
			dx[i] = x - lox;
			dy[i] = y - loy;
			surfaces[i] = node.surface;
		}
		else {

		}
	}

	// Trace left
	int nr = 0;
	if(node.left != MAX_UINT16 && l + li + ri > 0) {
		_trace(nodes, node.left, dir^1, ox, oy, dx, dy, surfaces, start, start + l + li + ri, id);
	
		// Move all rays that might intersect with right to the back
		int nl = _second_pass(start, start + l + li, a, b, c, ox, oy, dx, dy, surfaces, id, eps);
		nr = l + li - nl;
	}

	// Trace right, including left rays that are still intersecting node.
	// This will get tail-call optimized, hopefully
	if(node.right != MAX_UINT16 && r + ri + nr > 0) {
		_trace(nodes, node.right, dir^1, ox, oy, dx, dy, surfaces, start + l + li - nr, end, id);
	}
}

void kdtree_trace_surface(
	KdTree* tree, float* ox, float* oy, float* dx, float* dy,
	byte* surfaces, uint n
) {
	uint16* id = alloca(n * sizeof(uint16));
	for(uint i = 0; i < n; ++i) {
		id[i] = i;
	}

	KdNode* nodes = tree->data;

	_trace(nodes, 0, 0, ox, oy, dx, dy, surfaces, 0, n, id);

	// Debug print rays for preview in processing
	/*
	static bool printed = false;
	if(!printed) {
		printed = true;
		for(uint i = 0; i < n; ++i) {
			printf("line(%f, %f, %f, %f);\n",
				ox[i]*100.0f, 10.0f + oy[i]*100.0f, (ox[i]+dx[i])*100.0f, 10.0f+(oy[i]+dy[i])*100.0f
			);
		}
	}	
	*/

	// Unshuffle rays
	for(int i = 0; i < n; ++i) {
		int dest = id[i];
		if(dest != i) {
			_swap(i, dest)
			i--;
		}
	}
}

