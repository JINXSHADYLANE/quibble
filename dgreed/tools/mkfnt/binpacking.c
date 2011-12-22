#include "binpacking.h"

#include <memory.h>

static void _rect_swap(uint i, uint j, uint* widths, uint* heights, Pos* pos, uint* idx) {
	uint tw = widths[i];
	widths[i] = widths[j];
	widths[j] = tw;

	uint th = heights[i];
	heights[i] = heights[j];
	heights[j] = th;

	Pos tp = pos[i];
	pos[i] = pos[j];
	pos[j] = tp;

	uint ti = idx[i];
	idx[i] = idx[j];
	idx[j] = ti;
}

static bool _pack_recursive(uint x_off, uint y_off, uint w, uint h, uint n, 
		uint* widths, uint* heights, Pos* pos, uint* idx) {
	// We've succeded if no more rects to pack
	if(n == 0)
		return true;

	// Size of the biggest rect
	uint rw = widths[0];
	uint rh = heights[0];

	// First rect does not fit, packing failed
	if(w < rw || h < rh)
		return false;

	// Put first rect into the corner and split remaining area into two rects 
	// A & B, so that their areas are as similar as possible
	
	pos[0].x = x_off;
	pos[0].y = y_off;
	
	uint front_i = 1;

	uint a_x_off = x_off + rw;
	uint a_w = w - rw;
	uint a_h = rh;

	uint b_x_off = x_off;
	uint b_y_off = y_off + rh;
	uint b_w = w;
	uint b_h = h - rh;

	// Iterate over remaining rects, consecutively put the ones that fit
	// into bin A
	uint x_cursor = 0;
	for(uint i = 1; i < n && x_cursor < a_w; ++i) {
		rw = widths[i];
		rh = heights[i];

		if(rh <= a_h && x_cursor + rw <= a_w) {
			// It fits, bring to front
			_rect_swap(front_i, i, widths, heights, pos, idx);
			pos[front_i].x = a_x_off + x_cursor;
			pos[front_i].y = y_off;
			x_cursor += rw;
			front_i++;
		}
	}

	// Put the rest into bin B
	return _pack_recursive(b_x_off, b_y_off, b_w, b_h, n - front_i, 
			widths + front_i, heights + front_i, pos + front_i, idx + front_i);
}

void _insertion_sort(uint n, uint* widths, uint* heights, Pos* pos, uint* idx) {
	for(int i = 1; i < n; ++i) {
		int j = i;
		// Sort by area
		while(j-1 >= 0 && widths[j] * heights[j] < widths[j-1] * heights[j-1]) {
		// Sort by height
		//while(j-1 >= 0 && heights[j] < heights[j-1]) {
			_rect_swap(j, j-1, widths, heights, pos, idx);
		}
	}
}

Pos* bpack(uint w, uint h, uint n, uint* widths, uint* heights) {
	if(w && h && n && widths && heights) {
		Pos* pos = MEM_ALLOC(sizeof(Pos) * n);

		uint* idx = MEM_ALLOC(sizeof(Pos) * n);
		for(uint i = 0; i < n; ++i)
			idx[i] = i;

		_insertion_sort(n, widths, heights, pos, idx);

		bool res = _pack_recursive(0, 0, w, h, n, widths, heights, pos, idx); 

		if(!res) {
			MEM_FREE(pos);
		}
		else {
			// Unshuffle
			for(uint i = 0; i < n; ++i) {
				if(idx[i] == i)
					continue;
				// Find i-th rect
				for(uint j = i+1; j < n; ++j) {
					if(idx[j] == i) {
						_rect_swap(i, j, widths, heights, pos, idx);
					}
				}
			}
		}

		MEM_FREE(idx);

		return res ? pos : NULL;
	}
	else
		return NULL;
}

