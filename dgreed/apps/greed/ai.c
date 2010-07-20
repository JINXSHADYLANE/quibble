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
		for(uint j = i+1; j < n; ++j) {
			Vector2 v2 = nav_mesh->navpoints[j];
			if(nav_mesh->adjacency[IDX_2D(i, j, n)] > 0.0f) { 
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
		//gfx_draw_point(DEBUG_DRAW_LAYER, &nav_mesh->navpoints[i], COLOR_WHITE);
	}
}

