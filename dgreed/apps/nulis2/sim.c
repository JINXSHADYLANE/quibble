#include "sim.h"

#include <sprsheet.h>
#include <coldet.h>
#include <darray.h>

#define MAX_BALL_SIZE 48.0f
#define DT (1.0f / 60.0f)
#define LEVEL_NAMELEN 16
#define MAX_SPAWNS 16

typedef enum {
	BT_WHITE = 1,
	BT_PAIR = 2,
	BT_GRAV = 4,
	BT_TIME = 8
} BallType;

typedef struct {
	Vector2 pos, old_pos, force;
	float angle, old_angle, radius, inv_mass;
	float t, ts;
	BallType type;
	CDObj* collider;
	bool remove;
} Ball;

typedef struct {
	Vector2 pos, vel;
	float t;
	BallType type;
} SpawnDef;

typedef struct {
	char name[LEVEL_NAMELEN];
	uint n_spawns;
	SpawnDef spawns[MAX_SPAWNS];
	uint spawn_random_at;
	float spawn_random_interval;
} LevelDef;

// Game state
static float screen_widthf, screen_heightf;
static DArray balls;
static DArray ghosts;
static CDWorld cd;
static LevelDef level;
static float last_spawn_t;

static bool ffield_push;
static Vector2 ffield_pos;
static Ball* grav_ball;
static Ball* time_ball;

// Tweaks
float ball_mass = 10.0f;
float ball_radius = 16.0f;
float pair_displacement = 1.5;
float linear_damp = 0.995f;
float angular_damp = 0.99f;

float ffield_strength = 10.0f;
float ffield_radius = 160.0f;

float gravity_max_dist = 160.0f;
float gravity_strength = 2.0f;

float spawn_anim_len = 0.5f;
float spawn_maxspeed = 30.0f;
float spawn_maxrot = 1.0f;


// Code

static void _load_level(const char* level_name) {
	// Todo
	strcpy(level.name, "testlevel");
	level.n_spawns = 3;

	level.spawns[0].pos = vec2(-50.0f, 0.0f);
	level.spawns[0].vel = vec2(0.0f, 0.0f);
	level.spawns[0].t = 0.0f;
	level.spawns[0].type = BT_WHITE;
	
	level.spawns[1].pos = vec2(0.0f, 0.0f);
	level.spawns[1].vel = vec2(0.0f, 0.0f);
	level.spawns[1].t = 1.0f;
	level.spawns[1].type = BT_WHITE;

	level.spawns[2].pos = vec2(50.0f, 0.0f);
	level.spawns[2].vel = vec2(0.0f, 0.0f);
	level.spawns[2].t = 2.0f;
	level.spawns[2].type = BT_WHITE;

	level.spawn_random_at = 0;
	level.spawn_random_interval = 0.0f;
}

void sim_init(uint screen_width, uint screen_height) {
	screen_widthf = (float)screen_width;
	screen_heightf = (float)screen_height;

	balls = darray_create(sizeof(Ball), 0);
	ghosts = darray_create(sizeof(Ball), 0);

	coldet_init_ex(&cd, MAX_BALL_SIZE, screen_widthf, screen_heightf, true, true);
}

void sim_close(void) {
	coldet_close(&cd);

	darray_free(&ghosts);
	darray_free(&balls);
}

void sim_reset(const char* level) {
	_load_level(level);

	last_spawn_t = time_s();
}

const char* sim_level(void) {
	return level.name;
}

static void _spawn(Vector2 pos, Vector2 vel, BallType type, float a) {

	float angular_vel = rand_float_range(-spawn_maxrot, spawn_maxrot);
	float radius = type & BT_PAIR ? ball_radius * pair_displacement : ball_radius; 
	float mass = type & BT_PAIR ? ball_mass * 2.0f : ball_mass;

	Ball new = {
		.pos = pos,
		.old_pos = vec2_sub(pos, vec2_scale(vel, DT)),
		.force = {0.0f, 0.0f},
		.angle = a,
		.old_angle = a - angular_vel * DT,
		.radius = radius,
		.inv_mass = 1.0f / mass,
		.t = time_s(),
		.ts = 1.0f,
		.type = type,
		.collider = NULL,
		.remove = false
	};

	int hits = coldet_query_circle(&cd, pos, radius, 1, NULL);
	if(hits == 0) {
		darray_append(&balls, &new);
		Ball* pballs = DARRAY_DATA_PTR(balls, Ball);
		Ball* pnew = &pballs[balls.size-1];
		CDObj* obj = coldet_new_circle(&cd, pos, radius, 1, pnew);
		pnew->collider = obj;
	}
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

static bool _balls_collide(const Ball* a, const Ball* b) {
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

static void _update_level(void) {
	float t = time_s();

	// Spawn predefined balls
	for(uint i = 0; i < level.n_spawns; ++i) {
		if(level.spawns[i].t <= t) {
			SpawnDef* s = &level.spawns[i];
			_spawn(s->pos, s->vel, s->type, 0.0f);
			last_spawn_t = t;

			// Move last spawn here, decrease count
			level.spawns[i] = level.spawns[--level.n_spawns];
			--i;
		}
	}

	// Spawn random balls
	if(balls.size < level.spawn_random_at) {
		if(t > last_spawn_t + level.spawn_random_interval) {
			BallType type = rand_uint() & BT_WHITE;

			Vector2 pos = vec2(
					rand_float_range(0.0f, screen_widthf),
					rand_float_range(0.0f, screen_heightf)
			);

			float v = rand_float_range(0.0f, spawn_maxspeed);
			float a = rand_float_range(0.0f, 2.0f*PI);
			Vector2 vel = vec2(
					cosf(a) * v,
					sinf(a) * v
			);

			_spawn(pos, vel, type, 0.0f);
			last_spawn_t = t;
		}
	}
}

void _apply_force_field_cb(CDObj* obj) {
	Ball* ball = (Ball*)obj->userdata;
	if(!ball->remove) {
		Vector2 path;
		float d = _distance(ffield_pos, ball->pos, &path);
		float r = ball_radius;
		if(ball->type & BT_PAIR)
			r *= pair_displacement;

		if(d < r*r) {
			float f = 1.0f - clamp(0.0f, 1.0f, sqrtf(d) / r);
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
	Ball* ball = (Ball*)obj->userdata;
	if(ball != grav_ball) {
		Vector2 path;
		float sqr_d = _distance(grav_ball->pos, ball->pos, &path);

		assert(sqr_d <= gravity_max_dist * gravity_max_dist);

		float d = sqrtf(d);

		// Turn off gravity when objects penetrate
		/*
		float g_r = ball_radius;
		float b_r = ball_radius;
		if(ball->type & BT_PAIR)
			b_r *= pair_displacement;
		if(d <= g_r + b_r)
			return;
		*/

		// Magic function to make force falloff to zero at max_dist,
		// allows to calculate gravity interactions in nearly linear time
		float decay = MIN(fabsf(d - gravity_max_dist), fabsf(d + gravity_max_dist));
		decay = powf(decay / gravity_max_dist, 0.25f);

		float f = (1.0f / (grav_ball->inv_mass * ball->inv_mass)) / sqr_d;
		if(grav_ball->type & BT_WHITE)
			f *= -1.0f;
		f *= gravity_strength;
		
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
}

void sim_update(void) {
	_update_level();

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
	Ball* b = DARRAY_DATA_PTR(balls, Ball);
	for(uint i = 0; i < balls.size; ++i) {
		if(b[i].type & BT_GRAV) {
			grav_ball = &b[i];
			coldet_query_circle(&cd, b[i].pos, ball_radius, 1,
					_apply_gravity_cb);
		}
		if(b[i].type & BT_TIME) {
			time_ball = &b[i];
			coldet_query_circle(&cd, b[i].pos, ball_radius, 1,
					_apply_timescale_cb);
		}
	}

	// Calculate next state using verlet integration
	for(uint i = 0; i < balls.size; ++i) {
		Ball* ball = &b[i];	

		float dt = DT * ball->ts;

		// Position
		float damp = powf(linear_damp, ball->ts);
		Vector2 acc = vec2_scale(ball->force, ball->inv_mass);
		Vector2 new_pos = {
			.x = damp*2.0f*ball->pos.x - ball->old_pos.x + acc.x * dt*dt,
			.y = damp*2.0f*ball->pos.y - ball->old_pos.y + acc.y * dt*dt
		};
		ball->old_pos = ball->pos;
		ball->pos = new_pos;

		// Rotation
		float ang_damp = powf(angular_damp, ball->ts);
		float new_angle = ang_damp*2.0f*ball->angle - ball->old_angle;
		ball->old_angle = ball->angle;
		ball->angle = new_angle;

		// Reset force and timescale
		ball->force = vec2(0.0f, 0.0f);
		ball->ts = 1.0f;

		// Set collider offset
		Vector2 offset = vec2_sub(ball->pos, ball->old_pos);
		ball->collider->offset = offset;
	}
}

