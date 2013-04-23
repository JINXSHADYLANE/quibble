#include "world.h"
#include "kdtree.h"

void world_cast_primary(
	World* world, Vector2 p, Vector2 dir,
	float fov, uint y_res, float* out_x, float* out_y,
	float* out_nx, float* out_ny
) {
	assert(world && y_res);

	dir = vec2_scale(dir, 1000.0f);

	Vector2 frustum_left = vec2_add(
		p, vec2_rotate(dir, -fov * 0.5f)
	);

	Vector2 frustum_right = vec2_add(
		p, vec2_rotate(dir, fov * 0.5f)
	);

	byte* surfaces = alloca(y_res);

	// Fill up ray components buffers
	for(uint i = 0; i < y_res; ++i) {
		float t = (float)i / (float)(y_res-1);
		Vector2 ray_end = vec2_lerp(
			frustum_left,
			frustum_right,
			t
		);

		out_x[i] = p.x;
		out_y[i] = p.y;

		out_nx[i] = ray_end.x - p.x;
		out_ny[i] = ray_end.y - p.y;
	}

	// Cast rays as a single packet
	kdtree_trace_surface(
		&world->walls, out_x, out_y,
		out_nx, out_ny, surfaces, y_res
	);
}

