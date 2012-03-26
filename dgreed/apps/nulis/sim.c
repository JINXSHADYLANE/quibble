#include "sim.h"
#include "effects.h"
#include "common.h"
#include <darray.h>
#include <system.h>
#include <gfx_utils.h>

TexHandle atlas;
TexHandle backgr;
const RectF ball_rects[] = {
	{0.0f, 0.0f, 32.0f, 32.0f},
	{0.0f, 32.0f, 32.0f, 64.0f},
	{32.0f, 0.0f, 85.0f, 32.0f},
	{32.0f, 32.0f, 85.0f, 64.0f},
	{0.0f, 64.0f, 52.0f, 64.0f + 52.0f},
	{52.0f, 64.0f, 52.0f + 52.0f, 64.0f + 52.0f},
	{156.0f - 52.0f, 116.0f - 52.0f, 156.0f, 116.0f},
	{208.0f - 52.0f, 116.0f - 52.0f, 208.0f, 116.0f},
	{118.0f, 0.0f, 118.0f + 32.0f, 32.0f},
	{118.0f, 32.0f, 118.0f + 32.0f, 32.0f + 32.0f},
};
const RectF ball_core_rect = {85.0f, 0.0f, 85.0f + 32.0f, 32.0f};
const uint balls_layer = 3;

const RectF halo_rects[] = {
	{768.0f, 768.0f, 980.0f, 980.0f},
	{2.0f, 770.0f, 254.0f, 768.0f + 254.0f}
};

const float halo_radius = 106.0f;

typedef enum {
	BT_BLACK = 0,
	BT_WHITE,
	BT_GRAV_BLACK,
	BT_GRAV_WHITE,
	BT_TIME_BLACK,
	BT_TIME_WHITE,
	BT_TYPE_COUNT
} BallType;

typedef struct {
	Vector2 p, v;
	float r, a, w, birth_t, core;
	BallType type;
	uint mass;
	Color color;
	bool remove;
} Ball;

const float ball_radius = 16.0f;
const float linear_damp = 0.995f;
const float angular_damp = 0.99f;
const float ghost_lifetime = 1.8f;
const float spawn_rot = 0.01f;
const float collide_force = 0.05f;
const float born_force = 0.01f;

const float time_factor = 4.0f;
const float time_max_dist = 150.0f;
const float gravity_force = 8.0f;
const float gravity_max_dist = 150.0f;

// State

DArray balls_array;
DArray ghosts_array;

float sim_spawn_interval;
float sim_last_spawn;
uint sim_start_spawn_at;

static const Vector2 wrap_offsets[] = {
	{SCREEN_WIDTH, 0.0f},
	{-SCREEN_WIDTH, 0.0f},
	{0.0f, SCREEN_HEIGHT},
	{0.0f, -SCREEN_HEIGHT}
};

static float _distance(Vector2 start, Vector2 end, Vector2* out_path) {
	Vector2 path = vec2_sub(end, start);
	float min_d = vec2_length_sq(path);
	for(uint i = 0; i < ARRAY_SIZE(wrap_offsets); ++i) {
		Vector2 npath = vec2_sub(
			vec2_add(end, wrap_offsets[i]), start
		);
		float nd = vec2_length_sq(npath);
		if(nd < min_d) {
			min_d = nd;
			path = npath;
		}
	}
	if(out_path)
		*out_path = path;
	return min_d;
}

static void _ball_centers(const Ball* a, Vector2* c1, Vector2* c2) {
	assert(a && c1 && c2);

	if(a->mass == 2) {
		Vector2 d = vec2_rotate(vec2(a->r*0.75f/2.0f, 0.0f), a->a);
		*c1 = vec2_add(a->p, d);
		*c2 = vec2_sub(a->p, d);
		return;
	}

	*c1 = *c2 = a->p;
}

static bool _balls_collide(const Ball* a, const Ball* b) {
	assert(a && b);
	assert(a->mass && b->mass);

	float r = ball_radius * 2.0f;
	if(a->mass + b->mass == 2) {
		return _distance(a->p, b->p, NULL) < r*r;
	}

	Vector2 a_centers[2];
	Vector2 b_centers[2];

	_ball_centers(a, &a_centers[0], &a_centers[1]);
	_ball_centers(b, &b_centers[0], &b_centers[1]);

	for(uint i = 0; i < 2; ++i) {
		for(uint j = 0; j < 2; ++j) {
			if(_distance(a_centers[i], b_centers[j], NULL) < r*r)
				return true;
		}
	}

	return false;
}

void sim_init(void) {
	balls_array = darray_create(sizeof(Ball), 0);
	ghosts_array = darray_create(sizeof(Ball), 0);

	atlas = tex_load(ASSET_PRE "atlas.png");
	backgr = tex_load(ASSET_PRE "background.png");
}

void sim_close(void) {
	darray_free(&balls_array);
	darray_free(&ghosts_array);
	tex_free(atlas);
	tex_free(backgr);
}

void sim_reset(float spawn_interval, uint start_spawning_at) {
	balls_array.size = 0;
	ghosts_array.size = 0;

	sim_spawn_interval = spawn_interval;
	sim_start_spawn_at = start_spawning_at;
	sim_last_spawn = time_s();
}

static void _spawn_ex(Vector2 p, Vector2 v, uint type, float t, float a, 
		float w, uint mass, DArray* dest) {
	// Wrap coords
	p.x = fmodf(SCREEN_WIDTH + p.x, SCREEN_WIDTH);
	p.y = fmodf(SCREEN_HEIGHT + p.y, SCREEN_HEIGHT);

	// Color
	ColorHSV hsv = {0.0f, 0.8f, 0.9f, 1.0f};
	hsv.h = rand_float();
	Color c = hsv_to_rgb(hsv); 

	Ball new = {
		p, v, ball_radius, a, w, t, 0.0f, type, mass, c, false
	};
	if(mass != 1)
		new.r *= 2.0f;

	// Check for collisions
	if(dest == &balls_array) {
		Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
		for(uint i = 0; i < balls_array.size; ++i) {
			if(balls[i].type > BT_WHITE)
				continue;
			if(!balls[i].remove && _balls_collide(&new, &balls[i]))
				return;
		}
	}

	darray_append(dest, &new);
}

static void _spawn(Vector2 p, Vector2 v, uint type, float t, float a,
		float w, uint mass) {
	_spawn_ex(p, v, type, t, a, w, mass, &balls_array);
}

static void _spawn_ghost(Vector2 p, Vector2 v, uint type, float t, float a, 
		float w, uint mass) {
	_spawn_ex(p, v, type, t, a, w, mass, &ghosts_array);
}

void sim_spawn(Vector2 p, Vector2 v, uint type, float t) {
	_spawn(p, v, type, t, 0.0f, 0.0f, 1);
}

void sim_spawn_random(void) {
	Vector2 p;
	p.x = rand_float_range(0.0f, SCREEN_WIDTH);
	p.y = rand_float_range(0.0f, SCREEN_HEIGHT);

	Vector2 v = vec2(0.0f, rand_float_range(0.0f, 0.01f));
	v = vec2_rotate(v, rand_float_range(0.0f, PI * 2.0f));

	uint type = rand_int(0, 2);

	_spawn(p, v, type, time_s(), 0.0f, 0.0f, 1);
	effects_spawn(p);
}

static float _time_scale(Vector2 p) {
	float time_scale = 1.0f;

	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		Ball* b = &balls[i];
		float factor = 1.0f;
		if(b->p.x == p.x && b->p.y == p.y)
			continue;
		if(b->type == BT_TIME_WHITE) {
			float d = sqrtf(_distance(p, b->p, NULL)) / time_max_dist;
			d = clamp(0.0f, 1.0f, d);
			factor = lerp(powf(1.0f / time_factor, (float)b->mass), 1.0f, d);
		}
		if(b->type == BT_TIME_BLACK) {
			float d = sqrtf(_distance(p, b->p, NULL)) / time_max_dist;
			d = clamp(0.0f, 1.0f, d);
			factor = lerp(powf(time_factor, (float)b->mass), 1.0f, d);
		}
		time_scale *= factor;
	}

	return time_scale;
}

static void _apply_gravity(Ball* a, float dt) {
	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		Ball* b = &balls[i];
		if(a == b || (b->type != BT_GRAV_WHITE && b->type != BT_GRAV_BLACK))
			continue;
		
		Vector2 path;
		float sqr_d = _distance(a->p, b->p, &path);
		if(sqr_d > gravity_max_dist * gravity_max_dist)
			continue;

		float d = sqrtf(sqr_d);
		if(d <= a->r + b->r)
			continue;

		float decay = 
			MIN(fabs(d - gravity_max_dist), fabs(d + gravity_max_dist));
		decay = powf(decay / gravity_max_dist, 0.25f);

		float f = (float)a->mass * (float)b->mass / sqr_d;
		if(b->type == BT_GRAV_WHITE)
			f *= -1.0f;

		path = vec2_normalize(path);
		a->v = vec2_add(a->v, vec2_scale(path, dt * f * decay / (float)a->mass));
		b->v = vec2_add(b->v, vec2_scale(path, -dt * f * decay / (float)b->mass));
	}
}

void sim_force_field(Vector2 p, float r, float strength) {
	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		Ball* b = &balls[i];

		Vector2 path;
		float d = _distance(p, b->p, &path);
		if(d < r*r) {
			float ts = _time_scale(b->p);
			float f = 1.0f - clamp(0.0f, 1.0f, sqrtf(d) / r);
			path = vec2_normalize(path);
			b->v = vec2_add(b->v, vec2_scale(path, ts * f * strength / (float)b->mass));
			b->core += time_delta() / 100.0f * ts;
			if(b->core > 2.0f)
				b->core = 2.0f;
		}
	}
	effects_force_field(p, r, strength > 0.0f);
}

static void _update_ball(Ball* b, float dt) {
	float ts = _time_scale(b->p);
	//assert(ts == 1.0f);

	//_apply_gravity(b, dt, ts);

	b->core -= dt / 200.0f;
	if(b->core < 0.0f)
		b->core = 0.0f;

	b->p = vec2_add(b->p, vec2_scale(b->v, dt * ts));
	b->a += b->w * dt * ts;

	b->p.x = fmodf(b->p.x + SCREEN_WIDTH, SCREEN_WIDTH);
	b->p.y = fmodf(b->p.y + SCREEN_HEIGHT, SCREEN_HEIGHT);

	b->v = vec2_scale(b->v, linear_damp);
	b->w *= angular_damp;
}

static bool _is_closing(Ball* a, Ball* b) {
	float d1 = _distance(a->p, b->p, NULL);
	Vector2 nap = vec2_add(a->p, a->v);
	Vector2 nbp = vec2_add(b->p, b->v);
	float d2 = _distance(nap, nbp, NULL);

	return d2 <= d1;
}

void sim_update(void) {
	float t = time_s();
	float dt = time_delta();

	// Spawn random balls
	if(balls_array.size && balls_array.size < sim_start_spawn_at) {
		if(t - sim_last_spawn > sim_spawn_interval) {
			sim_last_spawn = t;
			sim_spawn_random();
		}
	}

	// Update ghosts
	Ball* ghosts = DARRAY_DATA_PTR(ghosts_array, Ball);
	for(uint i = 0; i < ghosts_array.size; ++i) {
		Ball* b = &ghosts[i];
		if(t - b->birth_t > ghost_lifetime) {
			ghosts[i] = ghosts[--ghosts_array.size];
			--i;
		}
		else {
			_update_ball(b, dt);
		}
	}

	// Update gravity
	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		Ball* b = &balls[i];
		if(b->remove) {
			balls[i] = balls[--balls_array.size];
			--i;
		}
		else {
			_apply_gravity(b, dt);
		}
	}

	// Update balls
	for(uint i = 0; i < balls_array.size; ++i) {
		Ball* b = &balls[i];
		assert(!b->remove);
		_update_ball(b, dt);
	}

	// Resolve collisions
	uint n = balls_array.size;
	for(uint i = 0; i < n; ++i) {
		Ball* a = &balls[i];
		for(uint j = i+1; j < n; ++j) {
			Ball* b = &balls[j];

			if(a->remove || b->remove)
				continue;

			if(_balls_collide(a, b)) {
				Vector2 path;
				_distance(a->p, b->p, &path);
				Vector2 midpoint = vec2_add(a->p, vec2_scale(path, 0.5f));
				float dir = atan2f(path.y, path.x);

				// Handle fancy ball types
				if(a->type > BT_WHITE || b->type > BT_WHITE) {

					Ball* normal = NULL;
					if(a->type <= BT_WHITE)
						normal = a;
					if(b->type <= BT_WHITE)
						normal = b;
					Ball* fancy = NULL;
					if(a->type > BT_WHITE)
						fancy = a;
					if(b->type > BT_WHITE)
						fancy = b;
					
					// Gravity + pair
					if(normal && normal->mass == 2 && 
						(fancy->type == BT_GRAV_WHITE || fancy->type == BT_GRAV_BLACK)) {
						normal->remove = true;
						fancy->v = vec2_add(fancy->v, normal->v);
						if( (normal->type == BT_WHITE && fancy->type == BT_GRAV_WHITE) ||
						 	(normal->type == BT_BLACK && fancy->type == BT_GRAV_BLACK)) {
							fancy->mass += 1;
							fancy->r *= sqrtf(2.0f);
						}
						else {
							fancy->mass -= 1;
							fancy->r /= sqrtf(2.0f);

							if(fancy->mass == 0) {
								fancy->type -= 2;
								fancy->mass = 1;
								fancy->r = 16.0f;
							}
						}
						continue;
					}

					// Time + pair
					if(normal && normal->mass == 2 && 
						(fancy->type == BT_TIME_WHITE || fancy->type == BT_TIME_BLACK)) {
						normal->remove = true;
						fancy->v = vec2_add(fancy->v, normal->v);
						if( (normal->type == BT_WHITE && fancy->type == BT_TIME_WHITE) ||
						 	(normal->type == BT_BLACK && fancy->type == BT_TIME_BLACK)) {
							fancy->mass += 1;
							fancy->r *= sqrtf(2.0f);
						}
						else {
							fancy->mass -= 1;
							fancy->r /= sqrtf(2.0f);

							if(fancy->mass == 0) {
								fancy->type -= 2;
								fancy->mass = 1;
								fancy->r = 16.0f;
							}
						}
						continue;
					}

					if(_is_closing(a, b)) {
						path = vec2_normalize(path);
						//effects_collide_ab(midpoint, dir);
						a->v = vec2_add(a->v, vec2_scale(path, -collide_force));
						b->v = vec2_add(b->v, vec2_scale(path, collide_force));
					}
					continue;
				}

				if(a->type == b->type) {
					// Same type
					a->remove = true;
					b->remove = true;
					float rot = rand_float_range(-spawn_rot, spawn_rot);
					if(a->mass + b->mass == 2) {
						effects_collide_aa(midpoint);
						Vector2 v = vec2_add(a->v, b->v);
						_spawn(midpoint, v, a->type, -1.0f, dir, rot, 2);
					}
					else {
						effects_collide_aaa(midpoint);
						Vector2 v = vec2_add(
							vec2_scale(a->v, (float)a->mass),
							vec2_scale(b->v, (float)b->mass)
						);
						_spawn_ghost(midpoint, vec2_scale(v, 1.0f/3.0f), a->type, t, 
							dir, rot, 3);

						if(a->mass + b->mass == 4) {
							_spawn(midpoint, vec2_scale(v, -1.0f/2.0f), a->type, -1.0f,
								0.0f, 0.0f, 1);
							balls = DARRAY_DATA_PTR(balls_array, Ball);
						}
					}
				}
				else {
					// Different type
					if(a->mass + b->mass == 2) {
						path = vec2_normalize(path);
						effects_collide_ab(midpoint, dir);
						a->v = vec2_add(a->v, vec2_scale(path, -collide_force));
						b->v = vec2_add(b->v, vec2_scale(path, collide_force));
					}
					else {
						effects_collide_aab(midpoint);
						float ang = rand_float_range(0, PI * 2.0f);
						a->remove = true;
						b->remove = true;

						uint n = 4;
						float rf = 1.5f;
						if(a->mass + b->mass == 4) {
							n = 6;
							rf = 2.5f;
						}

						for(uint k = 0; k < n; ++k) {
							ang += 2.0f * PI / (float)n; 
							Vector2 p = vec2(cosf(ang), sinf(ang));
							Vector2 v = vec2_add(a->v, b->v);
							v = vec2_add(v, vec2_scale(p, born_force));
							p = vec2_add(midpoint, vec2_scale(p, rf * ball_radius));
							_spawn(p, v, k%2, -1.0f, 0.0f, 0.0f, 1);
							balls = DARRAY_DATA_PTR(balls_array, Ball);
						}
					}
				}
			}
		}
	}
}

static float _ball_spawn_func(float t) {
	return t * (sinf(t*4.0f*PI)*(1.0f-t)+1);
}

static void _render_halo(Vector2 p, Ball* b, float scale, Color col) {
	RectF src = halo_rects[0];
	
	if(b->type == BT_TIME_WHITE || b->type == BT_TIME_BLACK)
		src = halo_rects[1]; 

	float t = time_s() * 4.0f;
	const int n_rings = 4;

	for(uint i = 0; i < n_rings; ++i) {
		float s = sinf(t + i*(PI*2.0f / n_rings))/2.0f + 0.5f;
		float c = cosf(t + i*(PI*2.0f / n_rings));

		if(c > 0.0f) {
			float r;
			if(b->type == BT_TIME_WHITE || b->type == BT_GRAV_WHITE)
				r = lerp(0.3f, 1.0f, s) * scale;
			else
				r = lerp(1.0f, 0.3f, s) * scale;

			Color transp = col & 0xFFFFFF;
			Color cc = color_lerp(transp, col, c);
			gfx_draw_textured_rect(backgr, balls_layer, &src, &p, b->a, r, cc); 
		}
	}
}

static void _render_ball_internal(Vector2 p, Ball* b, uint rect_i, 
		Color col, float scale) {
	gfx_draw_textured_rect(atlas, balls_layer, &ball_rects[rect_i], &p,
			b->a, scale, col);

	float t = clamp(0.0f, 1.0f, b->core);
	if(t > 0.0f && b->type <= BT_WHITE) {
		Color col = b->color;
		col &= ((byte)lrintf(t * 255.0f)) << 24 | 0xFFFFFF;
		if(b->mass == 1) {
			gfx_draw_textured_rect(atlas, balls_layer+1, &ball_core_rect, &p,
				b->a, scale, col);
		}
		else {
			Vector2 ca, cb;
			_ball_centers(b, &ca, &cb);
			gfx_draw_textured_rect(atlas, balls_layer+1, &ball_core_rect, &ca,
				b->a, scale, col);
			gfx_draw_textured_rect(atlas, balls_layer+1, &ball_core_rect, &cb,
				b->a, scale, col);
		}
	}
}

static void _render_ball(Ball* b, bool ghost, float t) {
	uint rect_i;
	if(b->type <= BT_WHITE)
		rect_i = b->type + (b->mass-1)*2;
	else
		rect_i = 4 + b->type;

	Color col = COLOR_WHITE;
	if(ghost) {
		float bt = 1.0f - clamp(0.0f, 1.0f, (t - b->birth_t) / ghost_lifetime);
		col &= (((byte)lrintf(bt * 255.0f)) << 24) | 0xFFFFFF;
	}

	float scale = 1.0f;
	float st = t - b->birth_t;
	if(!ghost && b->birth_t > 0.0f && st >= 0.0f && st < 1.0f) {
		scale = _ball_spawn_func(st);	
	}

	if(b->type > BT_WHITE)
		scale = b->r / 16.0f;

	float x_min = b->r;
	float x_max = SCREEN_WIDTH - x_min;
	float y_min = x_min;
	float y_max = SCREEN_HEIGHT - y_min;

	_render_ball_internal(b->p, b, rect_i, col, scale);

	WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, b->p,
			_render_ball_internal(npos, b, rect_i, col, scale));

	if(b->type > BT_WHITE) {
		_render_halo(b->p, b, scale, col);

		x_min = halo_radius * scale;
		x_max = SCREEN_WIDTH - x_min;
		y_min = x_min;
		y_max = SCREEN_HEIGHT - y_min;

		WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, b->p,
				_render_halo(npos, b, scale, col));
	}
}

void sim_render(void) {
	float t = time_s();

	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		_render_ball(&balls[i], false, t);
	}

	Ball* ghosts = DARRAY_DATA_PTR(ghosts_array, Ball);
	for(uint i = 0; i < ghosts_array.size; ++i) {
		_render_ball(&ghosts[i], true, t);
	}
}

int sim_count_alive(void) {
	return balls_array.size;
}

int sim_count_ghosts(void) {
	return ghosts_array.size;
}

int sim_total_mass(void) {
	int mass = 0;
	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		mass += balls[i].mass;
	}
	return mass;
}

void sim_destroy(void) {
	Ball* balls = DARRAY_DATA_PTR(balls_array, Ball);
	for(uint i = 0; i < balls_array.size; ++i) {
		effects_destroy(balls[i].p);
	}
}
