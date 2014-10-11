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

#define n_light_rays 64
#define n_floor_light_rays 4

static float* prim_ray_x = NULL;
static float* prim_ray_y = NULL;
static float* prim_ray_nx = NULL;
static float* prim_ray_ny = NULL;
static float* light_ray_x = NULL;
static float* light_ray_y = NULL;
static float* light_ray_nx = NULL;
static float* light_ray_ny = NULL;
static byte* light_id = NULL;

static float* f_light_ray_x = NULL;
static float* f_light_ray_y = NULL;
static float* f_light_ray_nx = NULL;
static float* f_light_ray_ny = NULL;
static byte* f_light_id = NULL;


void world_render(
	World* world, Vector2 p, Vector2 dir,
	float fov, uint y_res, uint x_res, Color* backbuffer
) {
	// Alloc primary & light ray component buffers
	if(unlikely(prim_ray_x == NULL)) {
		// Primary
		prim_ray_x = malloc(sizeof(float) * y_res * 4);
		prim_ray_y = prim_ray_x + y_res;
		prim_ray_nx = prim_ray_y + y_res;
		prim_ray_ny = prim_ray_nx + y_res;

		// Light
		light_ray_x = malloc(sizeof(float) * n_light_rays * 4);
		light_ray_y = light_ray_x + n_light_rays;
		light_ray_nx = light_ray_y + n_light_rays;
		light_ray_ny = light_ray_nx + n_light_rays;
		light_id = malloc(n_light_rays);

		// Floor light
		f_light_ray_x = malloc(sizeof(float) * n_floor_light_rays * x_res);
		f_light_ray_y = f_light_ray_x + n_floor_light_rays * x_res;
		f_light_ray_nx = f_light_ray_y + n_floor_light_rays * x_res;
		f_light_ray_ny = f_light_ray_nx + n_floor_light_rays * x_res;
		f_light_id = malloc(n_floor_light_rays * x_res);
	}

	// Cast all primary rays as a single packet
	world_cast_primary(
		world, p, dir, fov, y_res, 
		prim_ray_x, prim_ray_y, prim_ray_nx, prim_ray_ny
	);

	// Render wall columns now
	const float inv_n_light_rays = 1.0f / n_light_rays;
	const Light* lights = world->lights.data;
	const uint n_lights = world->lights.size;
	const float xr = (float)x_res;
	for(uint i = 0; i < y_res; ++i) {
		// Column world position & normal
		float column_x = prim_ray_x[i];
		float column_y = prim_ray_y[i];
		float column_nx = prim_ray_nx[i];
		float column_ny = prim_ray_ny[i];

		float dx = column_x - p.x;
		float dy = column_y - p.y;
		float sqr_d = dx*dx + dy*dy;
		float a = fov * (-0.5f + (float)i/y_res);
		float c = cosf(a);

		float h = sqrtf(xr*xr / (sqr_d * c * c));
		int ih = (int)h;		
		
		// Prep light rays
		for(uint j = 0; j < n_light_rays; ++j) {
			light_ray_x[j] = column_x;
			light_ray_y[j] = column_y;
			light_id[j] = j % n_lights;
			Light l = lights[light_id[j]];
			float a = rand_float_range(0.0f, PI * 2.0f);
			float x = l.x + cosf(a) * l.r;
			float y = l.y + sinf(a) * l.r;
			light_ray_nx[j] = x - column_x;
			light_ray_ny[j] = y - column_y;
		}

		// Cast light rays
		kdtree_trace_light(
			&world->walls, light_ray_x, light_ray_y,
			light_ray_nx, light_ray_ny, light_id, n_light_rays
		);

		// Render column
		int t0_x = ((int)x_res - ih) / 2;
		float inv_h = 1.0f / h;
		int start_x = MAX(0, t0_x);
		for(int x = 0; (x < ih) && (start_x + x < (int)x_res); ++x) {
			float z = lerp(world->floor, world->ceiling, (float)(x + start_x - t0_x) * inv_h);

			float r = 0.0f, g = 0.0f, b = 0.0f;

			// Sample direct lighting
			for(uint j = x%2; j < n_light_rays; j+=2) {
				uint id = light_id[j];
				if(id < n_lights) {
					Light l = lights[id];
					float lx = light_ray_nx[j];
					float ly = light_ray_ny[j];
					float lz = l.z - z;
					float len_sq = lx*lx + ly*ly + lz*lz;
					float inv_len = 1.0f / sqrtf(len_sq);
					//float inv_len = rsqrt(len_sq);
					lx *= inv_len;
					ly *= inv_len;

					float dot = clamp(0.0f, 1.0f, column_nx*lx + column_ny*ly);
					float att = 1.0f - clamp(0.0f, 1.0f, len_sq * l.inv_att * l.inv_att); 
					float f = inv_n_light_rays * dot * att * 2.0f;
					r += l.em_r * f;
					g += l.em_g * f;
					b += l.em_b * f;
				}
			}

			r = clamp(0.0f, 1.0f, r);
			g = clamp(0.0f, 1.0f, g);
			b = clamp(0.0f, 1.0f, b);

			byte fr = (byte)(r * 255.0f);
			byte fg = (byte)(g * 255.0f);
			byte fb = (byte)(b * 255.0f);

			int idx = i * x_res + x + start_x;
			Color old = backbuffer[idx];
			backbuffer[idx] = color_lerp(old, COLOR_RGBA(fr, fg, fb, 255), 0.5f);
		}

		/*
		// Render ground, not perspective correct for now
		if(start_x > 0) {
			// Determine world position of a bottom pixel
			float inv_c = 1.0f / c;
			float sx = p.x + dir.x * inv_c;
			float sy = p.y + dir.y * inv_c;

			// Fill up ray packet
			for(int i = 0; i < start_x; ++i) {
				float x = lerp(sx, column_x, (float)i / (float)start_x);
				float y = lerp(sy, column_y, (float)i / (float)start_x);

				for(int j = 0; j < n_floor_light_rays; ++j) {
					f_light_ray_x[i*n_floor_light_rays + j] = x;
					f_light_ray_y[i*n_floor_light_rays + j] = y;

					light_id[j] = j % n_lights;
					Light l = lights[light_id[j]];
					float a = rand_float_range(0.0f, PI * 2.0f);
					float lx = l.x + cosf(a) * l.r;
					float ly = l.y + sinf(a) * l.r;
					light_ray_nx[j] = lx - column_x;
					light_ray_ny[j] = ly - column_y;
				}
			}
		}
		*/
	}
}

