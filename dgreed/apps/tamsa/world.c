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

static float* prim_ray_x = NULL;
static float* prim_ray_y = NULL;
static float* prim_ray_nx = NULL;
static float* prim_ray_ny = NULL;

void world_render(
	World* world, Vector2 p, Vector2 dir,
	float fov, uint y_res, uint x_res, Color* backbuffer
) {
	// Alloc primary ray component buffers
	if(unlikely(prim_ray_x == NULL)) {
		prim_ray_x = malloc(sizeof(float) * y_res * 4);
		prim_ray_y = prim_ray_x + y_res;
		prim_ray_nx = prim_ray_y + y_res;
		prim_ray_ny = prim_ray_nx + y_res;
	}

	// Cast all primary rays as a single packet
	world_cast_primary(
		world, p, dir, fov, y_res, 
		prim_ray_x, prim_ray_y, prim_ray_nx, prim_ray_ny
	);

	// Render wall columns now
	const float xr = (float)x_res;
	const float* nx = prim_ray_nx;
	const float* ny = prim_ray_ny;
	for(uint i = 0; i < y_res; ++i) {
		float sqr_d = nx[i]*nx[i] + ny[i]*ny[i];
		float a = fov * (-0.5f + (float)i/y_res);
		float c = cosf(a);

		float h = sqrtf(xr*xr / (sqr_d * c * c));
		int ih = (int)h;

		// Render column
		int start_x = MAX(0, ((int)x_res - ih) / 2);
		for(uint x = 0; (x < ih) && (start_x + x < x_res); ++x) {
			backbuffer[i * x_res + x + start_x] = COLOR_WHITE;
		}
	}
}

