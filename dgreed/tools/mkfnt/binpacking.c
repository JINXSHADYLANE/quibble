#include "binpacking.h"

#include <memory.h>

static uint _largest_rect(uint n, uint* widths, uint* heights) {
	uint largest_i = 0, largest_area = 0;
	for(uint i = 0; i < n; ++i) {
		if(largest_area < widths[i] * heights[i]) {
			largest_area = widths[i] * heights[i];
			largest_i = i;
		}
	}
	return largest_i;
}

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

	// Find largest rect, move it to front
	//uint largest_i = _largest_rect(n, widths, heights);
	//_rect_swap(0, largest_i, widths, heights, pos, idx);

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

	// Areas of vertical split are rw * h and (w - rw) * h,
	// horizontal split - rh * w and (h - rh) * w;
	// with some rearranging choice of split direction boils down to this:
	bool split_vertical = abs((2*rw - w) * h) < abs((2*rh - h) * w);

	uint front_i = 1;
	uint b_x_off, b_y_off, b_w, b_h;
	if(split_vertical) {
		uint a_y_off = y_off + rh;
		uint a_w = rw;
		uint a_h = h - rh;

		b_x_off = x_off + rw;
		b_y_off = y_off;
		b_w = w - rw;
		b_h = h;

		// Iterate over remaining rects, consecutively put the ones that fit
		// into bin A
		uint y_cursor = 0;
		for(uint i = 1; i < n && y_cursor < a_h; ++i) {
			rw = widths[i];
			rh = heights[i];

			if(rw <= a_w && y_cursor + rh <= a_h) {
				// It fits, bring to front
				_rect_swap(front_i, i, widths, heights, pos, idx);
				pos[front_i].x = x_off;
				pos[front_i].y = a_y_off + y_cursor;
				y_cursor += rh;
				front_i++;
			}
		}
 
	}
	else {
		uint a_x_off = x_off + rw;
		uint a_w = w - rw;
		uint a_h = rh;

		b_x_off = x_off;
		b_y_off = y_off + rh;
		b_w = w;
		b_h = h - rh;

		// Mostly the same code as for vertical split
		uint x_cursor = 0;
		for(uint i = 1; i < n && x_cursor < a_w; ++i) {
			rw = widths[i];
			rh = heights[i];

			if(rh <= a_h && x_cursor + rw <= a_w) {
				_rect_swap(front_i, i, widths, heights, pos, idx);
				pos[front_i].x = a_x_off + x_cursor;
				pos[front_i].y = y_off;
				x_cursor += rw;
				front_i++;
			}
		}
	}

	// Put the rest into bin B
	return _pack_recursive(b_x_off, b_y_off, b_w, b_h, n - front_i, 
			widths + front_i, heights + front_i, pos + front_i, idx + front_i);
}

void _insertion_sort(uint n, uint* widths, uint* heights, Pos* pos, uint* idx) {
	for(int i = 1; i < n; ++i) {
		int j = i;
		while(j-1 >= 0 && widths[j] * heights[j] < widths[j-1] * heights[j-1]) {
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

