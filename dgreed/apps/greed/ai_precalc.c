#include "ai_precalc.h"

float _wall_distance(Vector2 p, DArray geometry) {
	Segment* segments = DARRAY_DATA_PTR(geometry, Segment);
	float distance = 10000.0f;
	for(uint i = 0; i < geometry.size; ++i) {
		float d = segment_point_dist(segments[i], p);
		if(abs(d) < abs(distance))
			distance = d;
	}
	return distance > 0.0f ? distance : 0.0f;
}





