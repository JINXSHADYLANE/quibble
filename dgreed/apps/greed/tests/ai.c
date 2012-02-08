#include "../../apps/greed/ai_precalc.h"

TEST_(wall_distance) {
	DArray geometry = darray_create(sizeof(Segment), 7);
	Segment segs[] = {
		{{2.0f, 3.0f},{1.0f, 1.0f}},
		{{3.0f, 1.0f},{2.0f, 3.0f}},
		{{4.0f, 2.0f},{3.0f, 1.0f}},
		{{4.0f, 3.0f},{4.0f, 2.0f}},
		{{3.0f, 5.0f},{4.0f, 3.0f}},
		{{1.0f, 5.0f},{3.0f, 5.0f}},
		{{1.0f, 1.0f},{1.0f, 5.0f}}
	};
	darray_append_multi(&geometry, segs, ARRAY_SIZE(segs));
	
	ASSERT_(feql(ai_wall_distance(vec2(0.0f, 0.0f), geometry), sqrtf(2.0f)));
	ASSERT_(feql(ai_wall_distance(vec2(1.0f, 0.0f), geometry), 1.0f)); 
	ASSERT_(feql(ai_wall_distance(vec2(0.0f, 1.0f), geometry), 1.0f));
	ASSERT_(feql(ai_wall_distance(vec2(2.0f, 7.0f), geometry), 2.0f));
	ASSERT_(feql(ai_wall_distance(vec2(7.0f, 2.0f), geometry), 3.0f));
	ASSERT_(feql(ai_wall_distance(vec2(2.0f, 1.0f), geometry), 0.894427f));

	ASSERT_(ai_wall_distance(vec2(3.0f, 3.0f), geometry) == 0.0f);
	ASSERT_(ai_wall_distance(vec2(2.0f, 4.0f), geometry) == 0.0f);
	ASSERT_(ai_wall_distance(vec2(1.1f, 2.0f), geometry) == 0.0f);

	darray_free(&geometry);
}