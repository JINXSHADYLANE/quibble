#include "ai.h"
#include "gfx_utils.h"

#define DEBUG_DRAW_LAYER 12

void ai_reset(float width, float height) {
	ai_precalc_bounds(width, height);
}

void ai_debug_draw() {
	NavMesh* nav_mesh = &current_arena_desc.nav_mesh;
	uint n = nav_mesh->n_nodes;

	for(uint i = 0; i < n; ++i) {
		Vector2 v1 = nav_mesh->navpoints[i];
		uint start = nav_mesh->neighbours_start[i];
		uint count = nav_mesh->neighbour_count[i];
		for(uint j = start; j < start + count; ++j) {
			uint i2 = nav_mesh->neighbours[j];
			if(i2 < i)
				continue;
			Vector2 v2 = nav_mesh->navpoints[i2];
			Segment s = ai_shortest_path(v1, v2);
			Segment s1, s2;
			if(ai_split_path(s, &s1, &s2)) {
				video_draw_line(DEBUG_DRAW_LAYER + i%2, 
					&s1.p1, &s1.p2, COLOR_WHITE);
				video_draw_line(DEBUG_DRAW_LAYER + i%2, 
					&s2.p1, &s2.p2, COLOR_WHITE);
			}
			else {
				video_draw_line(DEBUG_DRAW_LAYER + i%2,
					&s1.p1, &s1.p2, COLOR_WHITE);
			}		
		}
	}
}

