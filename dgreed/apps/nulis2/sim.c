#include "sim.h"

#include "common.h"
#include "levels.h"

#include <sprsheet.h>
#include <coldet.h>
#include <darray.h>
#include <sprsheet.h>
#include <async.h>
#include <mfx.h>
#include <particles.h>
#include <tweakables.h>
#include <malka/ml_states.h>

#define MAX_BALL_SIZE_IPAD 51.2f
#define MAX_BALL_SIZE_IPHONE 53.3333333333f
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
static SprHandle spr_grav_in;
static SprHandle spr_ffield;

// Game state
static Tweaks* tweaks;
static bool show_tweaks = false;
static FontHandle tweaks_font;
static float screen_widthf, screen_heightf;
static float sim_widthf, sim_heightf;
static Vector2 screen_offset;
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

static float time_volume;
static float grav_volume;

// ffield state
typedef struct {
	Vector2 center;
	float radius;
	float birth_time;
	bool shrink;
	Color color;
} FFieldCircle;

#define MAX_FFIELD_CIRCLES 32 
FFieldCircle ffield_circles[MAX_FFIELD_CIRCLES];
uint ffield_circle_count = 0;

// Tweaks
float max_speed = 15.0f;
float ball_mass = 2.0f;
float grav_mass = 6.0f;
float mass_increase_factor = 1.3f;
float ball_radius = 16.0f;
float pair_displacement = 1.5f;
float linear_damp = 0.997f;
float angular_damp = 0.99f;
float collission_trigger_sqr_d = 50.0f;
float overlap_resolve_factor = 1.5f;

float ffield_strength = 1500.0f;
float ffield_radius = 160.0f;

float gravity_max_dist = 200.0f;
float gravity_strength = 200000.0f;

float time_max_dist = 160.0f;
float time_strength = 2.0f;
float time_pulse_speed = 8.0f;

float spawn_anim_len = 0.5f;
float spawn_maxspeed = 30.0f;
float spawn_maxrot = 2.0f;
float ghost_lifetime = 1.0f;
float ghost_maxrot = 7.0f;

float vignette_delay = 1.0f;
float vignette_duration = 5.0f;

float ffield_freq = 0.2f;
float ffield_lifetime = 0.5f;

float touch_push_dist = 130.0f;

// Code

#define GAME_TWEAK(name, min, max) \
	tweaks_float(tweaks, #name, &name, min, max)

static void _register_tweaks(void) {
	tweaks_group(tweaks, "sim");

	GAME_TWEAK(max_speed, 5.0f, 50.0f);
	GAME_TWEAK(ball_mass, 0.5f, 10.0f);
	GAME_TWEAK(grav_mass, 0.5f, 10.0f);
	GAME_TWEAK(linear_damp, 0.99f, 0.999f);
	GAME_TWEAK(angular_damp, 0.90f, 0.99f);
	GAME_TWEAK(mass_increase_factor, 1.0f, 3.0f);
	GAME_TWEAK(collission_trigger_sqr_d, 10.0f, 100.0f);
	GAME_TWEAK(overlap_resolve_factor, 0.5f, 4.0f);
	GAME_TWEAK(ffield_strength, 500.0f, 5000.0f);
	GAME_TWEAK(ffield_radius, 50.0f, 300.0f);
	GAME_TWEAK(ffield_freq, 0.05f, 1.0f);
	GAME_TWEAK(ffield_lifetime, 0.1f, 1.0f);
	GAME_TWEAK(gravity_max_dist, 50.0f, 300.0f);
	GAME_TWEAK(gravity_strength, 100000.0f, 500000.0f);
	GAME_TWEAK(time_max_dist, 50.0f, 300.0f);
	GAME_TWEAK(time_strength, 1.1f, 3.0f);
	GAME_TWEAK(time_pulse_speed, 1.0f, 20.0f);
	GAME_TWEAK(spawn_anim_len, 0.1f, 1.0f);
	GAME_TWEAK(spawn_maxspeed, 10.0f, 100.0f);
	GAME_TWEAK(spawn_maxrot, 0.0f, 5.0f);
	GAME_TWEAK(ghost_lifetime, 0.1f, 3.0f);
	GAME_TWEAK(ghost_maxrot, 0.0f, 10.0f);
	GAME_TWEAK(vignette_delay, 0.0f, 3.0f);
	GAME_TWEAK(vignette_duration, 1.0f, 10.0f);
	GAME_TWEAK(touch_push_dist, 50.0f, 250.0f);
}

static void _load_level(const char* level_name) {
	levels_get(level_name, &level);
}

static Ball* _get_ball(uint i) {
	assert(i < balls.size);
	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	return &b[i];
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

static bool _is_solved(void) {
	uint n_balls = _count_alive_balls();

	if(n_balls == 0 && spawns.size == 0 && level.n_spawns == 0)
		return true;
	return false;
}

static bool _is_unsolvable(float t) {
	if(spawns.size != 0 || level.n_spawns != 0)
		return false;

	if(level.spawn_random_at > 2)
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

void sim_init(uint screen_width, uint screen_height, uint sim_width, uint sim_height) {
	screen_widthf = (float)screen_width;
	screen_heightf = (float)screen_height;
	sim_widthf = (float)sim_width;
	sim_heightf = (float)sim_height;
	screen_offset = vec2(
			(screen_widthf - sim_widthf) / 2.0f, 
			(screen_heightf - sim_heightf) / 2.0f
	);

	balls = darray_create(sizeof(Ball), 0);
	ghosts = darray_create(sizeof(Ball), 0);
	spawns = darray_create(sizeof(Ball), 0);

	float max_size = sim_widthf < 1024.0f ? 
		MAX_BALL_SIZE_IPHONE : MAX_BALL_SIZE_IPAD;
	coldet_init_ex(&cd, max_size, sim_widthf, sim_heightf, true, true);

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

	spr_grav_in = sprsheet_get_handle("g_sparkles");

	spr_ffield = sprsheet_get_handle("circle");

	reset_t = -100.0f;

	tweaks_font = font_load(ASSETS_PRE "varela.bft");
	tweaks = tweaks_init(ASSETS_PRE "tweaks.mml", rectf(10.0f, 40.0f, 1000.0f, 700.0f), 9, 0);
	tweaks->y_spacing = 90.0f;
	tweaks->items_per_page = 7;
	_register_tweaks();

}

void sim_close(void) {
	font_free(tweaks_font);
	tweaks_close(tweaks);

	coldet_close(&cd);

	darray_free(&spawns);
	darray_free(&ghosts);
	darray_free(&balls);
}

void sim_reset(const char* level) {
	is_solved = reset_level = false;
	start_t = malka_state_time("game");
	last_spawn_t = -100.0f;
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

	if(type & BT_GRAV || type & BT_TIME) {
		radius = 16.0f + s * 2.0f;

		mass = powf(mass_increase_factor, s);
		mass *= (type & BT_GRAV ? grav_mass : ball_mass);
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
		{sim_widthf, 0.0f},
		{-sim_widthf, 0.0f},
		{0.0f, sim_heightf},
		{0.0f, -sim_heightf},
		{sim_widthf, sim_heightf},
		{-sim_widthf, sim_heightf},
		{sim_widthf, -sim_heightf},
		{-sim_widthf, -sim_heightf}
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

	float rr = a->radius + b->radius;
	if((a->type & BT_PAIR) || (b->type & BT_PAIR)) {
		Vector2 a_centers[2];
		Vector2 b_centers[2];

		_ball_centers(a, a_centers);
		_ball_centers(b, b_centers);

		for(uint i = 0; i < 2; ++i) {
			for(uint j = 0; j < 2; ++j) {
				if(_distance(a_centers[i], b_centers[j], NULL) < rr*rr)
					return true;
			}
		}

		return false;
	}
	else {
		return _distance(a->pos, b->pos, NULL) <= rr*rr;
	}
}

static void _update_level(void) {
	float t = malka_state_time("game") - start_t;

	// Spawn predefined balls
	for(uint i = 0; i < level.n_spawns; ++i) {
		if(level.spawns[i].t <= t) {
			SpawnDef* s = &level.spawns[i];
			Vector2 p = vec2(
					sim_widthf/2.0f + s->pos.x,
					sim_heightf/2.0f + s->pos.y
			);

			if(p.x < 0.0f || p.x >= sim_widthf || p.y < 0.0f || p.y >= sim_heightf)
				continue;

			_spawn(p, s->vel, s->type, 0.0f, t, s->s);
			last_spawn_t = t;

			// Move last spawn here, decrease count
			level.spawns[i] = level.spawns[--level.n_spawns];
			--i;

			mfx_trigger_ex("spawn", vec2_add(p, screen_offset), 0.0f);
		}
	}

	// Spawn random balls
	if(_count_alive_balls() < level.spawn_random_at) {
		if(t > last_spawn_t + level.spawn_random_interval) {
			BallType type = rand_uint() & BT_WHITE;

			Vector2 pos;
			uint hits = 0;
			uint tries = 0;
			
			do {
				pos = vec2(
						rand_float_range(0.0f, sim_widthf),
						rand_float_range(0.0f, sim_heightf)
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

			mfx_trigger_ex("random_spawn", vec2_add(pos, screen_offset), 0.0f);
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
		float r_lim = (g_r + b_r);
		if(d <= r_lim)
			return;

		float volume = 1.0f - clamp(0.0f, 1.0f, d / gravity_max_dist); 
		grav_volume = MIN(1.0f, grav_volume + volume/2.0f); 

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
		float volume = 1.0f - f;
		time_volume = MIN(1.0f, time_volume + volume/2.0f);
		float ts = time_strength;
	
		f = lerp(ts * m, 1.0f, f);
		if(time_ball->type & BT_WHITE)
			f = 1.0f / f;

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
static Vector2 _move_ball_ex(Ball* a, Ball* b, Vector2* pba) {

	/*
	// Add a little randomness to solve cases where a and b perfectly overlap
	Vector2 bpos = vec2_add(b->pos, vec2(
				rand_float_range(-1.0f, 1.0f),
				rand_float_range(-1.0f, 1.0f)
	));
	*/

	Vector2 ba;
	float d_sqr = _distance(b->pos, a->pos, &ba);
	if(pba) *pba = ba;
	float rr = a->radius + b->radius;
	if(d_sqr <= rr*rr) {
		float d = sqrtf(d_sqr);
		Vector2 dir = vec2_normalize(ba);
		float f = (rr - d) * overlap_resolve_factor;
		Vector2 offset = vec2_scale(dir, f); 
		Vector2 new_pos = vec2_add(a->pos, vec2_scale(offset, a->inv_mass));
		return new_pos;
	}
	return a->pos;
}

static Vector2 _move_ball(Ball* a, Ball* b) {
	return _move_ball_ex(a, b, NULL);
}

// Perfect inelastic collission
void _balls_join(Ball* a, Ball* b) {
	Vector2 va = vec2_sub(a->pos, a->old_pos);
	Vector2 vb = vec2_sub(b->pos, b->old_pos);

	float ma = 1.0f / a->inv_mass;
	float mb = 1.0f / b->inv_mass;
	float inv_mab = 1.0f / (ma + mb);

	Vector2 ba;
	a->pos = _move_ball_ex(a, b, &ba);

	Vector2 old_ba;
	_distance(a->old_pos, b->old_pos, &old_ba);

	// Due to time effects, we need higher effect
	// trigger threshold for time balls
	float trigger_sqr_d = collission_trigger_sqr_d;
	if((a->type | b->type) & BT_TIME) {
		trigger_sqr_d *= 6.0f;
	}

	if(fabsf(vec2_length_sq(ba) - vec2_length_sq(old_ba)) > trigger_sqr_d) {
		Vector2 p = vec2_add(b->pos, vec2_scale(ba, a->radius / (a->radius + b->radius)));
		float dir = atan2f(ba.y, ba.x);
		mfx_trigger_ex("grav_join", vec2_add(p, screen_offset), dir);
	}

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
	Vector2 ab, old_ab;
	_distance(a->pos, b->pos, &ab);
	_distance(a->old_pos, b->old_pos, &old_ab);
	Vector2 a_pos = a->pos;
	Vector2 b_pos = vec2_add(a->pos, ab);
	Vector2 a_old_pos = a->old_pos;
	Vector2 b_old_pos = vec2_sub(b_pos, vb);
	
	// Trigger effect
	if(fabsf(vec2_length_sq(ab) - vec2_length_sq(old_ab)) > collission_trigger_sqr_d) {
		Vector2 p = vec2_scale(vec2_add(a_pos, b_pos), 0.5f);
		float dir = atan2f(ab.y, ab.x);
		const char* event = "a+b";
		if((a->type | b->type) & BT_GRAV)
			event = "grav+b";
		if((a->type | b->type) & BT_TIME)
			event = "time+b";
		mfx_trigger_ex(event, vec2_add(p, screen_offset), dir);	
	}

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


	BallType ta = ball_a->type, tb = ball_b->type;

	if((ta | tb) & BT_PAIR)
		if(!_do_balls_collide(ball_a, ball_b))
			return;

	if(((ta | tb) & (BT_GRAV | BT_TIME))) {
		// Gravity/time shrinking and growing

		Ball* normal = (ta & (BT_GRAV | BT_TIME)) ? ball_b : ball_a;
		Ball* fancy = (ta & (BT_GRAV | BT_TIME)) ? ball_a : ball_b;

		// Swap if time & grav, to make time ball stick to grav
		if(normal->type & BT_GRAV) {
			if(fancy->type & BT_TIME) {
				Ball* temp = normal;
				normal = fancy;
				fancy = temp;
			}
		}

		if(normal->type & BT_PAIR) {
			Vector2 vf = vec2_sub(fancy->pos, fancy->old_pos);
			vf = vec2_scale(vf, 1.0f / DT);

			// Same color, grow
			if(~(ta ^ tb) & BT_WHITE) {
				normal->remove = true;
				fancy->inv_mass /= mass_increase_factor;
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

					mfx_trigger_ex(
							fancy->type & BT_TIME ? "time_explode" : "grav_explode",
							vec2_add(fancy->pos, screen_offset), fancy->angle
					);
				}
				else {
					mfx_trigger_ex(
							fancy->type & BT_TIME ? "time_grow" : "grav_grow", 
							vec2_add(fancy->pos, screen_offset), fancy->angle
					);
				}
			}
			// Shrink
			else {
				normal->remove = true;
				fancy->inv_mass *= mass_increase_factor;
				fancy->radius -= 2.0f;
				fancy->collider->size.radius -= 2.0f;
				fancy->collider->dirty = true;

				// To small, become a normal ball
				if(fancy->radius < 16.0f) {
					fancy->remove = true;
					_spawn(fancy->pos, vf, fancy->type & BT_WHITE, 0.0f, -1.0f, 0.0f);

					mfx_trigger_ex(
							fancy->type & BT_TIME ? "time_vanish" : "grav_vanish",
							vec2_add(fancy->pos, screen_offset), fancy->angle
					);
				}
				else {
					mfx_trigger_ex(
							fancy->type & BT_TIME ? "time_shrink" : "grav_shrink",
							vec2_add(fancy->pos, screen_offset), fancy->angle
					);
				}
			}
			return;
		}
		else {
			if(!(normal->type & BT_GRAV)) {
				if((fancy->type & BT_GRAV) && !(fancy->type & BT_WHITE)) {
					_balls_join(ball_a, ball_b);
					//_balls_bounce(ball_a, ball_b);
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
			mfx_trigger_ex("a+a", vec2_add(new.pos, screen_offset), new.angle);
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
			new.t = malka_state_time("game");
			new.collider = NULL;
			darray_append(&ghosts, &new);

			// XX + XX = X
			if((ta & BT_PAIR) && (tb & BT_PAIR)) {
				Vector2 vel = vec2_sub(new.old_pos, new.pos);
				vel = vec2_scale(vel, 1.0f / DT);
				mfx_trigger_ex("aa+aa", vec2_add(new.pos, screen_offset), new.angle);
				_spawn(new.pos, vel, ta & BT_WHITE, 0.0f, -1.0f, 0.0f);
			}
			else {
				mfx_trigger_ex("aa+a", vec2_add(new.pos, screen_offset), new.angle);

				if(_is_solved())
					mfx_trigger_ex("win", vec2_add(new.pos, screen_offset), new.angle);
			}
			return;
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

				mfx_trigger_ex("aa+b", vec2_add(p, screen_offset), a);
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

				mfx_trigger_ex("aa+bb", vec2_add(p, screen_offset), a);
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
		v = vec2_scale(vec2_normalize(v), max_speed); 
	}
	ball->pos = vec2_add(ball->pos, vec2_scale(v, ball->ts));
	ball->old_pos = vec2_sub(ball->pos, v);

	// Wrap around
	if(ball->pos.x < 0.0f || ball->pos.x >= sim_widthf) {
		float new_x = ball->pos.x;
		while(new_x < 0.0f)
			new_x += sim_widthf;
		new_x = fmodf(new_x, sim_widthf);
		ball->old_pos.x += new_x - ball->pos.x;
		ball->pos.x = new_x;
	}
	if(ball->pos.y < 0.0f || ball->pos.y >= sim_heightf) {
		float new_y = ball->pos.y;
		while(new_y < 0.0f)
			new_y += sim_heightf;
		new_y = fmodf(new_y, sim_heightf);
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


void _next_level(void* userdata) {
	const char* next = levels_next(level.name);
	sim_reset(next);
}

void _reset_level(void* userdata) {
	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	for(uint i = 0; i < balls.size; ++i) {
		if(!b[i].remove)
			mfx_trigger_ex("lose_vanish", vec2_add(b[i].pos, screen_offset), b[i].angle);
	}

	sim_reset(level.name);
}

static void _show_ffield(Vector2 p, float r, bool push) {
	const Color ffield_color_start = COLOR_RGBA(0, 129, 255, 255);
	const Color ffield_color_end = COLOR_RGBA(255, 0, 236, 255);

	float t = malka_state_time("game");

	for(uint i = 0; i < ffield_circle_count; ++i) {
		float d_sqr = _distance(p, ffield_circles[i].center, NULL);
		if(d_sqr < touch_push_dist*touch_push_dist)
			if(t - ffield_circles[i].birth_time < ffield_freq)
				return;
	}

	FFieldCircle new = { p, r, t, !push, 0 };
	new.color = color_lerp(
			ffield_color_start, ffield_color_end, rand_float()
	);

	assert(ffield_circle_count < MAX_FFIELD_CIRCLES);
	ffield_circles[ffield_circle_count++] = new;
}

static void _update_ffield(void) {
	float t = malka_state_time("game");
	for(uint i = 0; i < ffield_circle_count; ++i) {
		FFieldCircle* circle = &ffield_circles[i];

		float ct = (t - circle->birth_time) / ffield_lifetime;
		if(ct >= 1.0f) {
			ffield_circles[i] = ffield_circles[--ffield_circle_count];
			--i;
			continue;
		}
	}
}

static void _render_ffield(void) {
	float t = malka_state_time("game");
	for(uint i = 0; i < ffield_circle_count; ++i) {
		FFieldCircle* circle = &ffield_circles[i];
		float ct = (t - circle->birth_time) / ffield_lifetime;
		ct = clamp(0.0f, 1.0f, ct);

		float start_r = circle->shrink ? circle->radius : circle->radius * 0.5f;
		float end_r = circle->shrink ? circle->radius * 0.5f : circle->radius;
		float scale = lerp(start_r, end_r, ct) / (scr_size == SCR_IPAD ? 128.0f : 64.0f);

		Color transparent = circle->color & 0xFFFFFF;
		Color col = color_lerp(transparent, circle->color, sinf(ct * PI));

		Vector2 pos = circle->center;

		spr_draw_cntr_h(spr_ffield, 4, vec2_add(pos, screen_offset), 0.0f, scale, col);

		float x_min = circle->radius;
		float x_max = sim_widthf - x_min;
		float y_min = x_min;
		float y_max = sim_heightf - y_min;

		WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos,
			spr_draw_cntr_h(spr_ffield, 4, vec2_add(npos, screen_offset),
			0.0f, scale, col));
	}
}

#ifdef TARGET_IOS
static void _process_touch(void) {
	uint tcount = touches_count();

	if(tcount && tcount < 5) {
		bool processed[4] = {false, false, false, false};
		Touch* t = touches_get();	

		for(uint i = 0; i < tcount; ++i) {
			if(processed[i])
				continue;

			bool push = false;
			float min_sq_dist = touch_push_dist * touch_push_dist;
			uint min_j = ~0;
			Vector2 pos;

			// Find a touch close to this one
			for(uint j = i+1; j < tcount; ++j) {
				if(processed[j])
					continue;

				float sq_dist = vec2_length_sq(vec2_sub(t[i].pos, t[j].pos));
				if(sq_dist < min_sq_dist) {
					// Touches are close, let's push
					push = true;
					min_sq_dist = sq_dist;
					min_j = j;
					pos = vec2_scale(vec2_add(t[i].pos, t[j].pos), 0.5f);
				}
			}

			if(push) {
				assert(tcount >= 2);
				assert(min_j < tcount);
				processed[i] = processed[min_j] = true;
			}
			else {
				processed[i] = true;
				pos = t[i].pos;
			}

			ffield_pos = vec2_sub(pos, screen_offset); 
			if(ffield_pos.x < 0.0f || ffield_pos.x >= sim_widthf 
					|| ffield_pos.y < 0.0f || ffiel_pos.y >= sim_heightf)
				continue;

			ffield_push = push;
			
			coldet_query_circle(&cd, pos, ffield_radius, 1,
					_apply_force_field_cb);

			_show_ffield(pos, ffield_radius, push);
			mfx_trigger_ex("force_field", vec2_add(pos, screen_offset), 0.0f);
		}
	}
}
#else
static void _process_mouse(void) {
	bool pull = mouse_pressed(MBTN_PRIMARY);
	bool push = mouse_pressed(MBTN_SECONDARY);

	if(push || pull) {
		Vector2 pos = vec2_sub(mouse_vec(), screen_offset);

		if(pos.x < 0.0f || pos.x >= sim_widthf || pos.y < 0.0f || pos.y >= sim_heightf)
			return;

		ffield_pos = pos;
		ffield_push = push;

		coldet_query_circle(&cd, pos, ffield_radius, 1,
			_apply_force_field_cb);

		_show_ffield(pos, ffield_radius, push);
		mfx_trigger_ex("force_field", vec2_add(pos, screen_offset), 0.0f);
	}
}
#endif

void sim_update(void) {
	if(touches_count() == 5 || key_up(KEY_QUIT)) {
		show_tweaks = false;
		malka_states_push("menu");
	}

	if(scr_size == SCR_IPAD) {
		if(touches_count() == 6 || char_up('e'))
			malka_states_push("editor");

		if(show_tweaks && char_up('t'))
			show_tweaks = false;

		if(touches_count() == 7 || char_up('t'))
			show_tweaks = true;
	}

	float t = malka_state_time("game");

	particles_update(t);

	_update_level();
	_update_ffield();

	if(!is_solved && _is_solved()) {
		is_solved = true;
		// Level finished!
		async_schedule(_next_level, lrintf(ghost_lifetime * 1000.0f) + 500, NULL);
	}

	if(!is_solved && !reset_level && _is_unsolvable(t)) {
		// Unsolvable, fade in vignette and reset after some time
		reset_level = true;
		reset_t = t + vignette_delay;
		uint t_off = lrintf((vignette_delay + vignette_duration/2.0f) * 1000.0f);
		async_schedule(_reset_level, t_off, NULL);
	}

	// Accumulate forces and time scale factors
	
	// Force field
#ifdef TARGET_IOS
	_process_touch();
#else
	_process_mouse();
#endif

	// Gravity & time scale
	time_volume = 0.0f;
	grav_volume = 0.0f;
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
	if(grav_volume > 0.1f)
		mfx_snd_set_ambient("GravityField.wav", grav_volume);
	if(time_volume > 0.1f)
		mfx_snd_set_ambient("TimeWarp.wav", time_volume);

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

	// Simple balls, grav and time are always at 0 angle
	float angle = (b->type & ~BT_WHITE) ? b->angle : 0.0f;
	if(b->type & (BT_TIME | BT_GRAV))
		angle = 0.0f;

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
	spr_draw_cntr_h(spr_balls[b->type], layer, vec2_add(b->pos, screen_offset),
			angle, scale, c);

	float x_min = radius * scale;
	float x_max = sim_widthf - x_min;
	float y_min = x_min;
	float y_max = sim_heightf - x_min;

	// Draw virtual copies on screen boundries
	WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, b->pos,
		spr_draw_cntr_h(spr_balls[b->type], layer, vec2_add(npos, screen_offset),
		angle, scale, c));

	if(b->type & BT_GRAV) {
		SprHandle spr = spr_grav_in;
		float off = t + b->t;

		const int n_rings = 6;
		for(uint i = 0; i < n_rings; ++i) {
			float sn = sinf(t + i*(PI*2.0f / n_rings))/ 2.0f + 0.5f;
			float cn = cosf(t + i*(PI*2.0f / n_rings));

			if(cn > 0.0f) {
				float r = lerp(scale*0.2f, scale, sn);
				float a = off * (float)(i+1) / 6.0f;
				Color c = (b->type & BT_WHITE ? COLOR_BLACK : COLOR_WHITE); 
				Color col = c & 0xFFFFFF;
				col = color_lerp(col, c, cn);

				spr_draw_cntr_h(spr, layer+1, vec2_add(screen_offset, b->pos),
					a, r, col);

				WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, b->pos,
					spr_draw_cntr_h(spr, layer+1, vec2_add(npos, screen_offset),
					a, r, col));
			}
		}
	}
}

void sim_render(void) {
	if(show_tweaks)
		tweaks_render(tweaks);
	sim_render_ex(false);
}

void sim_render_ex(bool skip_vignette) {
    float t = malka_state_time("game");

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

	_render_ffield();
}

