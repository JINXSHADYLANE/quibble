#include "sim.h"

#include "common.h"
#include "levels.h"

#include <sprsheet.h>
#include <coldet.h>
#include <darray.h>
#include <sprsheet.h>
#include <async.h>
#include <malka/ml_states.h>

#define MAX_BALL_SIZE 51.2f
#define DT (1.0f / 60.0f)

typedef struct {
	Vector2 pos, old_pos, force;
	float angle, old_angle, radius, inv_mass;
	float t, ts;
	BallType type;
	CDObj* collider;
	bool remove;
} Ball;

// Sprites
static SprHandle spr_background, spr_vignette;
static SprHandle spr_balls[32];
static SprHandle spr_grav_b_in, spr_grav_b_back;
static SprHandle spr_grav_w_in, spr_grav_w_back;

// Game state
static float screen_widthf, screen_heightf;
static DArray balls;
static DArray spawns;
static DArray ghosts;
static CDWorld cd;
static LevelDef level;
static float start_t;
static float last_spawn_t;
static bool is_solved;
static bool reset_level;
static float reset_t;

static bool ffield_push;
static Vector2 ffield_pos;
static Ball* grav_ball;
static Ball* time_ball;
static Ball* new_ball;

// Tweaks
float max_speed = 100.0f;
float overlap_resolve_factor = 0.25f;
float ball_mass = 2.0f;
float grav_mass = 6.0f;
float ball_radius = 16.0f;
float pair_displacement = 1.5f;
float linear_damp = 0.997f;
float angular_damp = 0.99f;

float ffield_strength = 1500.0f;
float ffield_radius = 160.0f;

float gravity_max_dist = 320.0f;
float gravity_strength = 200000.0f;

float time_max_dist = 160.0f;
float time_strength = 3.0f;
float time_pulse_speed = 8.0f;

float spawn_anim_len = 0.5f;
float spawn_maxspeed = 30.0f;
float spawn_maxrot = 2.0f;
float ghost_lifetime = 1.0f;
float ghost_maxrot = 7.0f;

float vignette_delay = 1.0f;
float vignette_duration = 5.0f;


// Code

static void _load_level(const char* level_name) {
	levels_get(level_name, &level);
}

static Ball* _get_ball(uint i) {
	assert(i < balls.size);
	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	return &b[i];
}

void sim_init(uint screen_width, uint screen_height) {
	screen_widthf = (float)screen_width;
	screen_heightf = (float)screen_height;

	balls = darray_create(sizeof(Ball), 0);
	ghosts = darray_create(sizeof(Ball), 0);
	spawns = darray_create(sizeof(Ball), 0);

	coldet_init_ex(&cd, MAX_BALL_SIZE, screen_widthf, screen_heightf, true, true);

	spr_background = sprsheet_get_handle("background");
	spr_vignette = sprsheet_get_handle("vignette");

	spr_balls[0] = 						sprsheet_get_handle("bubble_b");
	spr_balls[BT_WHITE] = 				sprsheet_get_handle("bubble_w");
	spr_balls[BT_PAIR] = 				sprsheet_get_handle("double_bubble_b");
	spr_balls[BT_PAIR | BT_WHITE] = 	sprsheet_get_handle("double_bubble_w");
	spr_balls[BT_TRIPLE] = 				sprsheet_get_handle("triple_bubble_b");
	spr_balls[BT_TRIPLE | BT_WHITE] = 	sprsheet_get_handle("triple_bubble_w");
	spr_balls[BT_GRAV] = 				sprsheet_get_handle("gravity_b");
	spr_balls[BT_GRAV | BT_WHITE] = 	sprsheet_get_handle("gravity_w");
	spr_balls[BT_TIME] = 				sprsheet_get_handle("time_b");
	spr_balls[BT_TIME | BT_WHITE] = 	sprsheet_get_handle("time_w");

	spr_grav_b_in = sprsheet_get_handle("gravity_b_inside");
	spr_grav_b_back = sprsheet_get_handle("gravity_b_back");
	spr_grav_w_in = sprsheet_get_handle("gravity_w_inside");
	spr_grav_w_back = sprsheet_get_handle("gravity_w_back");

	reset_t = -100.0f;
}

void sim_close(void) {
	coldet_close(&cd);

	darray_free(&spawns);
	darray_free(&ghosts);
	darray_free(&balls);
}

void sim_reset(const char* level) {
	is_solved = reset_level = false;
	start_t = last_spawn_t = time_s();
	ghosts.size = 0;
	spawns.size = 0;
	if(balls.size != 0) {
		Ball* b = DARRAY_DATA_PTR(balls, Ball);
		for(uint i = 0; i < balls.size; ++i) {
			if(b[i].collider)
				coldet_remove_obj(&cd, b[i].collider);
		}
		balls.size = 0;
	}

	_load_level(level);

}

const char* sim_level(void) {
	return level.name;
}

static void _spawn_ex(Ball* b) {
	darray_append(&spawns, b);
}

static void _spawn(Vector2 pos, Vector2 vel, BallType type, float a, float t, float s) {

	float angular_vel = rand_float_range(-spawn_maxrot, spawn_maxrot);
	float radius = type & BT_PAIR ? ball_radius * pair_displacement : ball_radius; 
	float mass = type & BT_PAIR ? ball_mass * 2.0f : ball_mass;

	if(type & BT_GRAV) {
		mass = grav_mass;
	}

	if(type & BT_GRAV || type & BT_TIME) {
		radius = 16.0f + s * 2.0f;
	}

	Ball new = {
		.pos = pos,
		.old_pos = vec2_sub(pos, vec2_scale(vel, DT)),
		.force = {0.0f, 0.0f},
		.angle = a,
		.old_angle = a - angular_vel * DT,
		.radius = radius,
		.inv_mass = 1.0f / mass,
		.t = t,
		.ts = 1.0f,
		.type = type,
		.collider = NULL,
		.remove = false
	};

	_spawn_ex(&new);
}

static float _distance(Vector2 start, Vector2 end, Vector2* out_path) {
	Vector2 path = vec2_sub(end, start);
	float min_d = vec2_length_sq(path);

	const Vector2 wrap_offsets[] = {
		{screen_widthf, 0.0f},
		{-screen_widthf, 0.0f},
		{0.0f, screen_heightf},
		{0.0f, -screen_heightf}
	};

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

static void _ball_centers(const Ball* a, Vector2* c) {
	assert(a && c);

	if(a->type & BT_PAIR) {
		float radius = ball_radius * pair_displacement;
		Vector2 d = vec2_rotate(vec2(radius*0.5f, 0.0f), a->angle);
		c[0] = vec2_add(a->pos, d);
		c[1] = vec2_sub(a->pos, d);
		return;
	}
	else {
		c[0] = c[1] = a->pos;
	}
}

static bool _do_balls_collide(const Ball* a, const Ball* b) {
	assert(a && b);

	float r = ball_radius * 2.0f;
	if((a->type & BT_PAIR) || (b->type & BT_PAIR)) {
		Vector2 a_centers[2];
		Vector2 b_centers[2];

		_ball_centers(a, a_centers);
		_ball_centers(b, b_centers);

		for(uint i = 0; i < 2; ++i) {
			for(uint j = 0; j < 2; ++j) {
				if(_distance(a_centers[i], b_centers[j], NULL) < r*r)
					return true;
			}
		}

		return false;
	}
	else {
		return _distance(a->pos, b->pos, NULL) <= r*r;
	}
}

static uint _count_alive_balls(void) {
	uint n_balls = 0;
	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	for(uint i = 0; i < balls.size; ++i) {
		if(!b[i].remove)
			n_balls++;
	}
	return n_balls;
}

static void _update_level(void) {
	float t = time_s() - start_t;

	// Spawn predefined balls
	for(uint i = 0; i < level.n_spawns; ++i) {
		if(level.spawns[i].t <= t) {
			SpawnDef* s = &level.spawns[i];
			Vector2 p = vec2(
					screen_widthf/2.0f + s->pos.x,
					screen_heightf/2.0f + s->pos.y
			);
			_spawn(p, s->vel, s->type, 0.0f, t, s->s);
			last_spawn_t = t;

			// Move last spawn here, decrease count
			level.spawns[i] = level.spawns[--level.n_spawns];
			--i;
		}
	}

	// 

	// Spawn random balls
	if(_count_alive_balls() < level.spawn_random_at) {
		if(t > last_spawn_t + level.spawn_random_interval) {
			BallType type = rand_uint() & BT_WHITE;

			Vector2 pos;
			uint hits = 0;
			uint tries = 0;
			
			do {
				pos = vec2(
						rand_float_range(0.0f, screen_widthf),
						rand_float_range(0.0f, screen_heightf)
				);

				hits = coldet_query_circle(&cd, pos, ball_radius, 1, NULL);
				tries++;
			} while(tries < 8 && hits > 0);

			float v = rand_float_range(0.0f, spawn_maxspeed);
			float a = rand_float_range(0.0f, 2.0f*PI);
			Vector2 vel = vec2(
					cosf(a) * v,
					sinf(a) * v
			);

			_spawn(pos, vel, type, 0.0f, t, 0.0f);
			last_spawn_t = t;
		}
	}
}

void _apply_force_field_cb(CDObj* obj) {
	Ball* ball = _get_ball((size_t)obj->userdata);
	assert(!ball->remove);

	if(!ball->remove) {
		Vector2 path;
		float d = _distance(ffield_pos, ball->pos, &path);
		float r = ball->radius + ffield_radius;

		if(d < r*r) {
			float f = -1.0f + clamp(0.0f, 1.0f, sqrtf(d) / r);
			if(ffield_push)
				f *= -1.0f;
			path = vec2_normalize(path);
			ball->force = vec2_add(ball->force,
					vec2_scale(path, ffield_strength * f)
			);
		}
	}
}

void _apply_gravity_cb(CDObj* obj) {
	Ball* ball = _get_ball((size_t)obj->userdata);
	assert(!ball->remove);

	if(ball != grav_ball) {
		Vector2 path;
		float sqr_d = _distance(grav_ball->pos, ball->pos, &path);

		float d = sqrtf(sqr_d);

		/*
		// Create "electric" push force, when objects are close 
		float f = 0.0f;
		if(d <= r_lim) {
			f = (((r_lim*r_lim) / (d*d)) - 1.0f) * 5000.0f;
		}
		*/

		float g_r = grav_ball->radius; 
		float b_r = ball->radius;
		float r_lim = (g_r + b_r) * 1.1f;
		if(d <= r_lim)
			return;

		// Magic function to make force falloff to zero at max_dist,
		// allows to calculate gravity interactions in nearly linear time
		float decay = MIN(fabsf(d - gravity_max_dist), fabsf(d + gravity_max_dist));
		decay = powf(decay / gravity_max_dist, 0.25f);

		float f = (gravity_strength / (grav_ball->inv_mass * ball->inv_mass)) / sqr_d;
		if(grav_ball->type & BT_WHITE)
			f *= -1.0f;
		
		path = vec2_normalize(path);
		grav_ball->force = vec2_add(grav_ball->force,
				vec2_scale(path, f * decay)
		);
		ball->force = vec2_add(ball->force,
				vec2_scale(path, -f * decay)
		);
	}
}

void _apply_timescale_cb(CDObj* obj) {
	Ball* ball = _get_ball((size_t)obj->userdata);
	assert(!ball->remove);

	if(ball != time_ball) {
		float m = (1.0f / time_ball->inv_mass) / ball_mass;
		float sqr_d = _distance(time_ball->pos, ball->pos, NULL);
		float f = clamp(0.0f, 1.0f, sqrtf(sqr_d) / time_max_dist);
		float ts = time_strength;
		if(time_ball->type & BT_WHITE)
			ts = 1.0f / time_strength;

		f = lerp(powf(ts, m), 1.0f, f);

		ball->ts *= f;
	}
}

Ball _balls_merge(Ball* a, Ball* b) {
	// Momentum times dt
	Vector2 pa = vec2_scale(vec2_sub(a->pos, a->old_pos), 1.0f / a->inv_mass);
	Vector2 pb = vec2_scale(vec2_sub(b->pos, b->old_pos), 1.0f / a->inv_mass);
	Vector2 pn = vec2_add(pa, pb);

	Ball new;

	Vector2 path;
	_distance(a->pos, b->pos, &path);
	new.pos = vec2_add(a->pos, vec2_scale(path, 0.5f));
	new.inv_mass = (a->inv_mass * b->inv_mass) / (a->inv_mass + b->inv_mass);
	new.old_pos = vec2_sub(new.pos, vec2_scale(pn, new.inv_mass));
	new.force = vec2(0.0f, 0.0f);
	new.angle = atan2f(
			b->pos.y - a->pos.y,
			b->pos.x - a->pos.x
	);
	new.old_angle = new.angle - (a->angle - a->old_angle) - (b->angle - b->old_angle);
	new.radius = a->radius * pair_displacement;
	new.t = -1.0f;
	new.ts = (a->ts + b->ts) * 0.5f;
	new.force = vec2(0.0f, 0.0f);
	new.remove = false;		

	return new;
}

// Returns a new position for ball a, for it not to collide with b
static Vector2 _move_ball(Ball* a, Ball* b) {

	// Add a little randomness to solve cases where a and b perfectly overlap
	Vector2 bpos = vec2_add(b->pos, vec2(
				rand_float_range(-1.0f, 1.0f),
				rand_float_range(-1.0f, 1.0f)
	));

	Vector2 ba;
	float d_sqr = _distance(bpos, a->pos, &ba);
	float rr = a->radius + b->radius;
	if(d_sqr <= rr*rr) {
		float d = sqrtf(d_sqr);
		Vector2 dir = vec2_normalize(ba);
		float f = (rr - d + 0.1f) * overlap_resolve_factor;
		Vector2 offset = vec2_scale(dir, f); 
		Vector2 new_pos = vec2_add(a->pos, offset);
		return new_pos;
	}
	return a->pos;
}

// Perfect inelastic collission
void _balls_join(Ball* a, Ball* b) {
	Vector2 va = vec2_sub(a->pos, a->old_pos);
	Vector2 vb = vec2_sub(b->pos, b->old_pos);

	float ma = 1.0f / a->inv_mass;
	float mb = 1.0f / b->inv_mass;
	float inv_mab = 1.0f / (ma + mb);

	a->pos = _move_ball(a, b);

	Vector2 nv = vec2_add(
			vec2_scale(va, ma * inv_mab),
			vec2_scale(vb, mb * inv_mab)
	);

	a->old_pos = vec2_sub(a->pos, nv);
	b->old_pos = vec2_sub(b->pos, nv);
}

// Perfect elastic collission
void _balls_bounce(Ball* a, Ball* b) {
	// Velocities
	Vector2 va = vec2_sub(a->pos, a->old_pos);
	Vector2 vb = vec2_sub(b->pos, b->old_pos);

	// Calculate new position for ball, taking world 
	// wrap-around into account
	Vector2 ab;
	_distance(a->pos, b->pos, &ab);
	Vector2 a_pos = a->pos;
	Vector2 b_pos = vec2_add(a->pos, ab);
	Vector2 a_old_pos = a->old_pos;
	Vector2 b_old_pos = vec2_sub(b_pos, vb);

	// Normal and tangent vectors of collission space
	Vector2 sn = vec2_normalize(vec2_sub(b_pos, a_pos));
	Vector2 st = vec2(sn.y, -sn.x);
	
	// Normal velocities
	Vector2 n_va = vec2_scale(sn, vec2_dot(sn, va));
	Vector2 n_vb = vec2_scale(sn, vec2_dot(sn, vb));

	// Tangent velocities
	Vector2 t_va = vec2_scale(st, vec2_dot(st, va));
	Vector2 t_vb = vec2_scale(st, vec2_dot(st, vb));

	// Find exact moment in time collission occured 
	float t = circle_circle_test(
			a_old_pos, ball_radius, va,
			b_old_pos, ball_radius, vb 
	);
	if(t <= 0.0f) {
		a->pos = _move_ball(a, b);
		return;
	}

	assert(t >= 0.0f);

	// Positions of balls at the moment of collission
	Vector2 pa = vec2_lerp(a->old_pos, a->pos, t);
	Vector2 pb = vec2_lerp(b->old_pos, b->pos, t);

	// Calculate new speeds, preserving momentum and kinetic energy
	float ma = 1.0f / a->inv_mass;
	float mb = 1.0f / b->inv_mass;
	float inv_mab = 1.0f / (ma + mb);
	Vector2 nva = vec2_add(
			vec2_scale(n_va, (ma - mb) * inv_mab),
			vec2_scale(n_vb, 2.0f * mb * inv_mab)
	);
	Vector2 nvb = vec2_add(
			vec2_scale(n_vb, (mb - ma) * inv_mab),
			vec2_scale(n_va, 2.0f * ma * inv_mab)
	);

	// Add tangent velocities, which did not change
	nva = vec2_add(nva, t_va);
	nvb = vec2_add(nvb, t_vb);

	// Calculate current position by moving balls with new speed
	// for all the remaining time after collission
	assert(1.0f - t >= 0.0f);
	a->pos = vec2_add(pa, vec2_scale(nva, 1.0f - t));
	a->old_pos = vec2_sub(a->pos, nva);
	b->pos = vec2_add(pb, vec2_scale(nvb, 1.0f - t));
	b->old_pos = vec2_sub(b->pos, nvb);
}

void _collission_cb(CDObj* a, CDObj* b) {
	Ball* ball_a = _get_ball((size_t)a->userdata);
	Ball* ball_b = _get_ball((size_t)b->userdata);

	assert(ball_a != ball_b);
	assert(ball_a && ball_b);

	if(ball_a->remove || ball_b->remove)
		return;

	if(!_do_balls_collide(ball_a, ball_b))
		return;

	BallType ta = ball_a->type, tb = ball_b->type;

	if(((ta | tb) & (BT_GRAV | BT_TIME))) {
		// Gravity/time shrinking and growing

		Ball* normal = (ta & (BT_GRAV | BT_TIME)) ? ball_b : ball_a;
		Ball* fancy = (ta & (BT_GRAV | BT_TIME)) ? ball_a : ball_b;

		if(normal->type & BT_PAIR) {
			Vector2 vf = vec2_sub(fancy->pos, fancy->old_pos);
			vf = vec2_scale(vf, 1.0f / DT);

			// Same color, grow
			if(~(ta ^ tb) & BT_WHITE) {
				normal->remove = true;
				fancy->inv_mass /= 2.0f;
				fancy->radius += 2.0f;
				fancy->collider->size.radius += 2.0f;
				fancy->collider->dirty = true;

				// Too big, explode into 5 balls
				if(fancy->radius > 24.0f) {
					fancy->remove = true;
					fancy->collider->size.radius = 24.0f;

					float a = fancy->angle;
					float r = sqrtf(2.0f) * ball_radius * 1.4f;
					for(uint i = 0; i < 5; ++i) {
						float t = (float)i / 5.0f;
						Vector2 offset = vec2(
							cosf(a + t * 2.0f * PI) * r,
							sinf(a + t * 2.0f * PI) * r
						);	
						Vector2 pos = vec2_add(fancy->pos, offset);
						Vector2 v = vec2_add(vec2_scale(offset, 0.5), vf);
						_spawn(pos, v, ta & BT_WHITE, 0.0f, -1.0f, 0.0f);
					}
				}
			}
			// Shrink
			else {
				normal->remove = true;
				fancy->inv_mass *= 2.0f;
				fancy->radius -= 2.0f;
				fancy->collider->size.radius -= 2.0f;
				fancy->collider->dirty = true;

				// To small, become a normal ball
				if(fancy->radius < 16.0f) {
					fancy->remove = true;
					_spawn(fancy->pos, vf, fancy->type & BT_WHITE, 0.0f, -1.0f, 0.0f);
				}
			}
			return;
		}
		else {
			if(!(normal->type & BT_GRAV)) {
				if((fancy->type & BT_GRAV) && !(fancy->type & BT_WHITE)) {
					_balls_join(ball_a, ball_b);
					return;
				}
			}
		}
	}


	if((ta & BT_WHITE) == (tb & BT_WHITE)) {
		// X + X = XX
		if(((ta & ~BT_WHITE) == 0) && ((tb & ~BT_WHITE) == 0)) {
			ball_a->remove = ball_b->remove = true;
			Ball new = _balls_merge(ball_a, ball_b);
			new.type = ta | BT_PAIR;
			_spawn_ex(&new);
			return;
		}

		if((ta & BT_PAIR) || (tb & BT_PAIR)) {
			// XX + X = 0
			ball_a->remove = ball_b->remove = true;
			Ball new = _balls_merge(ball_a, ball_b);
			new.type = (ta & BT_WHITE) | BT_TRIPLE;
			float rot = rand_float_range(-ghost_maxrot, ghost_maxrot);
			new.old_angle = new.angle - rot * DT;
			new.t = time_s();
			new.collider = NULL;
			darray_append(&ghosts, &new);

			// XX + XX = X
			if((ta & BT_PAIR) && (tb & BT_PAIR)) {
				Vector2 vel = vec2_sub(new.old_pos, new.pos);
				vel = vec2_scale(vel, 1.0f / DT);
				_spawn(new.pos, vel, ta & BT_WHITE, 0.0f, -1.0f, 0.0f);
			}
		}
	}
	else {
		if((ta & BT_PAIR) || (tb & BT_PAIR)) {
			ball_a->remove = ball_b->remove = true;
			Vector2 path;
			_distance(ball_a->pos, ball_b->pos, &path);
			Vector2 p = vec2_add(ball_a->pos, vec2_scale(path, 0.5f));

			Vector2 vab = vec2_add(
					vec2_sub(ball_a->pos, ball_a->old_pos),
					vec2_sub(ball_b->pos, ball_b->old_pos)
			);
			vab = vec2_scale(vab, 1.0f / DT);

			float a = rand_float_range(0.0f, 2.0f*PI);

			// XX + YY = X + X + X + Y + Y + Y
			if((ta & BT_PAIR) && (tb & BT_PAIR)) {
				float r = sqrtf(2.0f) * ball_radius * 1.7f;
				for(uint i = 0; i < 6; ++i) {
					float t = (float)i / 6.0f;
					float ri = i % 2 ? r : r * 0.5f;
					Vector2 offset = vec2(
						cosf(a + t * 2.0f * PI) * ri,
						sinf(a + t * 2.0f * PI) * ri
					);	
					Vector2 pos = vec2_add(p, offset);
					Vector2 v = vec2_add(vec2_scale(offset, 0.5), vab);
					_spawn(pos, v, i%2 ? BT_WHITE : 0, 0.0f, -1.0f, 0.0f);
				}
			}
			// XX + Y = X + X + Y + Y
			else {
				float r = sqrtf(2.0f) * ball_radius;
				for(uint i = 0; i < 4; ++i) {
					float t = (float)i / 4.0f;
					Vector2 offset = vec2(
						cosf(a + t * 2.0f * PI) * r,
						sinf(a + t * 2.0f * PI) * r
					);	
					Vector2 pos = vec2_add(p, offset);
					Vector2 v = vec2_add(vec2_scale(offset, 0.5f), vab);
					_spawn(pos, v, i%2 ? BT_WHITE : 0, 0.0f, -1.0f, 0.);
				}
			}
			return;
		}
	}
	// Other cases - bounce off
	_balls_bounce(ball_a, ball_b);
}

static void _spawn_cb(CDObj* obj) {
	Ball* a = _get_ball((size_t)obj->userdata);
	if(!a->remove && !(a->type & BT_GRAV)) {
		a->pos = _move_ball(a, new_ball);
	}
}

static void _update_ball(Ball* ball) {

	// Position
	float damp = powf(linear_damp, ball->ts);
	Vector2 acc = vec2_scale(ball->force, ball->inv_mass);
	Vector2 new_pos = {
		.x = (1.0f+damp)*ball->pos.x - damp*ball->old_pos.x + acc.x * DT*DT,
		.y = (1.0f+damp)*ball->pos.y - damp*ball->old_pos.y + acc.y * DT*DT 
	};
	Vector2 v = vec2_sub(new_pos, ball->pos);
	float l = vec2_length_sq(v);
	if(l > max_speed*max_speed) {
		ball->ts = sqrtf(max_speed / l); 
	}
	ball->pos = vec2_add(ball->pos, vec2_scale(v, ball->ts));
	ball->old_pos = vec2_sub(ball->pos, v);

	// Wrap around
	if(ball->pos.x < 0.0f || ball->pos.x >= screen_widthf) {
		float new_x = ball->pos.x;
		while(new_x < 0.0f)
			new_x += screen_widthf;
		new_x = fmodf(new_x, screen_widthf);
		ball->old_pos.x += new_x - ball->pos.x;
		ball->pos.x = new_x;
	}
	if(ball->pos.y < 0.0f || ball->pos.y >= screen_heightf) {
		float new_y = ball->pos.y;
		while(new_y < 0.0f)
			new_y += screen_heightf;
		new_y = fmodf(new_y, screen_heightf);
		ball->old_pos.y += new_y - ball->pos.y;
		ball->pos.y = new_y;
	}

	// Rotation
	float ang_damp = powf(angular_damp, ball->ts);
	float new_angle = (1.0f+ang_damp)*ball->angle - ang_damp*ball->old_angle;
	ball->old_angle = ball->angle;
	ball->angle = new_angle;

	// Reset force and timescale
	ball->force = vec2(0.0f, 0.0f);
	ball->ts = 1.0f;

	// Set collider offset
	if(ball->collider) {
		Vector2 offset = vec2_sub(ball->pos, ball->collider->pos);
		ball->collider->offset = offset;
	}
}

static bool _is_solved(void) {
	uint n_balls = _count_alive_balls();

	if(n_balls == 0 && spawns.size == 0 && level.n_spawns == 0)
		return true;
	return false;
}

static bool _is_unsolvable(void) {
	if(spawns.size != 0 || level.n_spawns != 0)
		return false;

	uint n_white = 0, n_black = 0;

	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	for(uint i = 0; i < balls.size; ++i) {
		if(b[i].remove || (b[i].type & (BT_GRAV | BT_TIME)))
			continue;
		
		uint n = (b[i].type & BT_PAIR) ? 2 : 1;
		if(b[i].type & BT_WHITE)
			n_white += n;
		else
			n_black += n;
	}
	
	return n_white + n_black < 3;
}

void _next_level(void* userdata) {
	const char* next = levels_next(level.name);
	sim_reset(next);
}

void _reset_level(void* userdata) {
	sim_reset(level.name);
}

void sim_update(void) {
	if(touches_count() == 4 || char_up('e'))
		malka_states_push("editor");

	if(key_up(KEY_QUIT))
		malka_states_push("menu");

	float t = time_s();

	_update_level();

	if(!is_solved && _is_solved()) {
		is_solved = true;
		// Level finished!
		async_schedule(_next_level, lrintf(ghost_lifetime * 1000.0f) + 500, NULL);
	}

	if(!is_solved && !reset_level && _is_unsolvable()) {
		// Unsolvable, fade in vignette and reset after some time
		reset_level = true;
		reset_t = time_s() + vignette_delay;
		uint t_off = lrintf((vignette_delay + vignette_duration/2.0f) * 1000.0f);
		async_schedule(_reset_level, t_off, NULL);
	}

	// Accumulate forces and time scale factors
	
	// Force field
	uint tcount = touches_count();
	bool pull = tcount == 1 || mouse_pressed(MBTN_PRIMARY);
	bool push = tcount == 2 || mouse_pressed(MBTN_SECONDARY);
	if(push || pull) {
		Vector2 pos;
		if(tcount) {
			Touch* t = touches_get();
			assert(t);
			pos = t[0].pos;
			if(tcount == 2)
				pos = vec2_scale(vec2_add(pos, t[1].pos), 0.5f);
		}
		else {
			pos = mouse_vec();
		}

		ffield_pos = pos;
		ffield_push = push;
		coldet_query_circle(&cd, pos, ffield_radius, 1, 
				_apply_force_field_cb);
	}

	// Gravity & time scale
	for(uint i = 0; i < balls.size; ++i) {
		Ball* b = _get_ball(i);
		if(b->remove)
			continue;

		if(b->type & BT_GRAV) {
			grav_ball = b;
			coldet_query_circle(&cd, b->pos, gravity_max_dist, 1,
					_apply_gravity_cb);
		}
		if(b->type & BT_TIME) {
			time_ball = b;
			coldet_query_circle(&cd, b->pos, time_max_dist, 1,
					_apply_timescale_cb);
		}
	}

	// Calculate next state using verlet integration
	for(uint i = 0; i < balls.size; ++i) {
		Ball* b = _get_ball(i);	
		if(b->remove)
			continue;

		_update_ball(b);
	}
	
	// Remove old ghosts and update all the others
	Ball* b = DARRAY_DATA_PTR(ghosts, Ball);
	for(uint i = 0; i < ghosts.size; ++i) {
		Ball* ghost = &b[i];

		if(t - ghost->t >= ghost_lifetime) {
			b[i] = b[ghosts.size-1];
			ghosts.size--;
			i--;
		}
		else {
			_update_ball(ghost);
		}
	}

	// Process all collissions
	coldet_process(&cd, _collission_cb);

	// Remove balls marked for removal
	b = DARRAY_DATA_PTR(balls, Ball);
	for(uint i = 0; i < balls.size; ++i) {
		if(b[i].remove && b[i].collider) {
			coldet_remove_obj(&cd, b[i].collider);
			b[i].collider = NULL;
		}
	}

	// Spawn new balls 
	b = DARRAY_DATA_PTR(spawns, Ball);
	for(uint i = 0; i < spawns.size; ++i) {
		new_ball = &b[i];
		coldet_query_circle(&cd, b[i].pos, b[i].radius, 1, _spawn_cb);

		// Find a ball which was removed (can be made O(log n))
		Ball* pnew = NULL;
		Ball* pballs = DARRAY_DATA_PTR(balls, Ball);
		size_t idx = ~0;
		for(uint j = 0; j < balls.size; ++j) {
			if(pballs[j].remove) {
				assert(pballs[j].collider == NULL);
				idx = j;
				break;
			}
		}
		if(idx != ~0) {
			// Write new ball over the removed one
			pnew = &pballs[idx];
			*pnew = b[i];
		}
		else {
			// Append new ball otherwise
			darray_append(&balls, &b[i]);
			pnew = _get_ball(balls.size-1);
			idx = balls.size-1;
		}
		assert(pnew);

		CDObj* obj = coldet_new_circle(&cd, pnew->pos, pnew->radius, 1, (void*)idx);
		pnew->collider = obj;
	}
	spawns.size = 0;
}

static void _render_ball(Ball* b, float t) {
	assert(spr_balls[b->type]);

	float scale = 1.0f;
	
	// Spawn animation
	if(b->t >= 0.0f && (t - b->t) <= spawn_anim_len) {
		float nt = (t - b->t) / spawn_anim_len;
		scale = nt * (sinf(nt*4.0f*PI)*(1.0f-nt)+1.0f);
	}

	// Simple balls are always at 0 angle
	float angle = (b->type & ~BT_WHITE) ? b->angle : 0.0f;

	Color c = COLOR_WHITE;
	uint layer = 2;

	float radius = b->radius;

	if(b->type & BT_TRIPLE) {
		// Alpha fadeout for ghosts
		float a = clamp(0.0f, 1.0f, 1.0f - (t - b->t) / ghost_lifetime);
		c &= ((byte)lrintf(a * 255.0f) << 24) | 0xFFFFFF;
		scale = 1.0f;
		radius *= 2.0f;
	}

	if(b->type & BT_GRAV) {
		scale *= b->radius / 24.0f;
	}

	if(b->type & BT_TIME) {

		radius = 64.0f;
		scale *= 0.5f;

		// Pulsating animation for time balls
		float f = (1.0f / b->inv_mass) / ball_mass;
		if(b->type & BT_WHITE)
			f = 1.0f / f;
		float anim = (cosf(t*time_pulse_speed * f) + 1.0f) / 2.0f;
		scale = lerp(scale, scale*0.85f, anim);

		scale *= b->radius / ball_radius;
	}

	// Draw
	spr_draw_cntr_h(spr_balls[b->type], layer, b->pos, angle, scale, c);

	float x_min = radius * scale;
	float x_max = screen_widthf - x_min;
	float y_min = x_min;
	float y_max = screen_heightf - x_min;

	// Draw virtual copies on screen boundries
	WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, b->pos,
		spr_draw_cntr_h(spr_balls[b->type], layer, npos, angle, scale, c));

	if(b->type & BT_GRAV) {
		SprHandle spr = b->type & BT_WHITE ? spr_grav_w_in : spr_grav_b_in;
		float off = t + b->t;

		const int n_rings = 6;
		for(uint i = 0; i < n_rings; ++i) {
			float sn = sinf(t + i*(PI*2.0f / n_rings))/ 2.0f + 0.5f;
			float cn = cosf(t + i*(PI*2.0f / n_rings));

			if(cn > 0.0f) {
				float r = lerp(scale*0.2f, scale, sn);
				float a = off * (float)(i+1) / 6.0f;
				Color col = c & 0xFFFFFF;
				col = color_lerp(col, c, cn);

				spr_draw_cntr_h(spr, layer-1, b->pos, a, r, col);

				WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, b->pos,
					spr_draw_cntr_h(spr, layer-1, npos, a, r, col));
			}
		}
	}
}

void sim_render(void) {
	sim_render_ex(false);
}

void sim_render_ex(bool skip_vignette) {
	float t = time_s();

	RectF fullscr = rectf(0.0f, 0.0f, screen_widthf, screen_heightf);

	// Render background
	spr_draw_h(spr_background, 0, fullscr, COLOR_WHITE);
	
	// Render vignette
	if(!skip_vignette) {
		if(t - reset_t < vignette_duration) {
			float tt = clamp(0.0f, 1.0f, (t - reset_t) / vignette_duration);
			tt = sinf(tt * PI);
			Color c = COLOR_FTRANSP(tt);
			spr_draw_h(spr_vignette, 3, fullscr, c);
		}
	}
	
	// Render balls
	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	for(uint i = 0; i < balls.size; ++i) {
		if(!b[i].remove)
			_render_ball(&b[i], t - start_t);
	}

	// Render ghosts
	b = DARRAY_DATA_PTR(ghosts, Ball);
	for(uint i = 0; i < ghosts.size; ++i) {
		_render_ball(&b[i], t);
	}
}

