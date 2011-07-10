#include "game.h"
#include <system.h>
#include <gfx_utils.h>

#include "arena.h"
#include "physics.h"
#include "sounds.h"
#include "ai.h"
#include "devmode.h"
#include "particles.h"
#include "menus.h"
#include "state.h"
#include "controls.h"
#include "objects.h"
#include "sounds.h"

#define ENERGYBAR_LAYER 6
#define FOREGROUND_LAYER 5
#define OBJECTS_LAYER 4
#define BACKGROUND_LAYER 3
#define SHADOWS_LAYER 2
#define PARTICLES_LAYER 5

#define SHIP_COLORS 4
#define SHIP_FRAMES 8
#define SHIP_SHARDS 9
#define VORTEX_FRAMES 7
#define PLATFORM_RINGS 8
#define PLATFORM_FRAMES 2
#define PLATFORM_RING_SHADOW 4
#define PLATFORM_RING_NEUTRAL 5
#define PLATFORM_RING_NEUTRAL_SHADOW 6

TexHandle shards;
TexHandle atlas1;
RectF ship_rects[SHIP_COLORS][SHIP_FRAMES];
RectF ship_shards[SHIP_COLORS][SHIP_SHARDS];
RectF ship_shadow_rect;
RectF vortex_rects[VORTEX_FRAMES];
RectF platform_ring_rects[PLATFORM_RINGS];
RectF platform_rects[PLATFORM_FRAMES];
RectF platform_shadow_rect;
RectF bullet_rect;
RectF bullet_shadow_rect;
RectF energybar_rects[3]; // start, end, middle
RectF shockwave_rect;

// Game state
uint n_ships;
uint n_platforms;
ShipState ship_states[MAX_SHIPS];
PlatformState platform_states[MAX_PLATFORMS];
float start_anim_t = 0.0f;

// Tweakables
float ship_circle_radius = 35.0f;
float ship_firing_speed = 10.0f;
float vortex_fade_in_length = 0.7f;
float vortex_fade_out_length = 0.4f;
float vortex_animation_speed = 1800.0f;
float platform_ring_rotation_speed = PI;
float platform_transition_length = 1.0f;
float platform_holding_time = 15.0f;
float platform_warning1_time = 5.0f;
float platform_warning2_time = 2.0f;
float platform_core_size = 0.7f;
float platform_active_core_size = 1.0f;
float ship_min_size = 0.5f;
float ship_max_size = 1.0f;
float ship_explosion_length = 0.5f;
float energy_max = 100.0f;
float energy_decrease_speed = 2.0f;
float energy_holding = 0.3f;
float energy_neutralization = 10.0f;
float energybar_alpha = 0.5f;
float energybar_spacing = 24.0f;
float energybar_y = 20.0f;
float energybar_warn_length = 0.2f;
float start_anim_length = 1.0f;
float start_anim_obj_fadein = 0.3f;
float start_anim_obj_size = 2.0f;
float bullet_cost = 1.0f;
float bullet_damage = 0.8f;
float shockwave_min_size = 0.5f;
float shockwave_max_size = 3.0f;

const Color ship_colors[] = {
	COLOR_RGBA(33, 48, 189, 255),
	COLOR_RGBA(74, 185, 46, 255),
	COLOR_RGBA(240, 214, 53, 255),
	COLOR_RGBA(143, 51, 140, 255)
};	

const Vector2 ship_shard_offsets[] = {
	{-25.0f + 5.0f, -36.0f - 12.0f},
	{-25.0f + 21.0f, -36.0f - 3.0f},
	{-25.0f + 21.0f, -36.0f + 21.0f},
	{-25.0f - 7.0f, -36.0f + 1.0f},
	{-25.0f + 6.0f, -36.0f + 5.0f},
	{-25.0f + 10.0f, -36.0f + 19.0f},
	{-25.0f - 5.0f, -36.0f + 23.0f},
	{-25.0f + 5.0f, -36.0f + 40.0f},
	{-25.0f + 8.0f, -36.0f + 35.0f}
};	

const float ship_shard_angles[] = {
	0.2f, -1.5f, 1.0f,
	2.0f, -0.6f, -1.8f,
	0.4f, 3.0f, -2.5f
};

// Devmode switches
bool draw_physics_debug = false;
bool draw_ai_debug = false;

#define WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos, action) \
	if(pos.x < x_min) { 										 \
		Vector2 npos = vec2(pos.x + (float)SCREEN_WIDTH, pos.y); \
		action;													 \
		if(pos.y < y_min) {										 \
			npos.y += (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
		if(pos.y > y_max) {										 \
			npos.y -= (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
	}															 \
	if(pos.x > x_max) {											 \
		Vector2 npos = vec2(pos.x - (float)SCREEN_WIDTH, pos.y); \
		action;													 \
		if(pos.y < y_min) {										 \
			npos.y += (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
		if(pos.y > y_max) {										 \
			npos.y -= (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
	}															 \
	if(pos.y < y_min) {											 \
		Vector2 npos = vec2(pos.x, pos.y + (float)SCREEN_HEIGHT);\
		action;													 \
	}															 \
	if(pos.y > y_max) {											 \
		Vector2 npos = vec2(pos.x, pos.y - (float)SCREEN_HEIGHT);\
		action;													 \
	}												


#define GAME_TWEAK(name, min, max) \
	tweaks_float(tweaks, #name, &name, min, max)

void game_register_tweaks(Tweaks* tweaks) {
	tweaks_group(tweaks, "game");
	GAME_TWEAK(ship_circle_radius, 1.0f, 50.0f);
	GAME_TWEAK(ship_firing_speed, 1.0f, 32.0f);
	GAME_TWEAK(vortex_fade_in_length, 0.1f, 2.0f);
	GAME_TWEAK(vortex_fade_out_length, 0.1f, 0.4f);
	GAME_TWEAK(vortex_animation_speed, 500.0f, 4000.0f);
	GAME_TWEAK(platform_ring_rotation_speed, -PI*4.0f, PI*4.0f);
	GAME_TWEAK(platform_transition_length, 0.1f, 4.0f);
	GAME_TWEAK(platform_holding_time, 2.0f, 30.0f);
	GAME_TWEAK(platform_warning1_time, 1.0f, 10.0f);
	GAME_TWEAK(platform_warning2_time, 0.5f, 9.0f);
	GAME_TWEAK(platform_core_size, 0.1f, 2.0f);
	GAME_TWEAK(platform_active_core_size, 0.1f, 2.0f);
	GAME_TWEAK(ship_min_size, 0.1f, 1.5f);
	GAME_TWEAK(ship_max_size, 0.2f, 2.0f);
	GAME_TWEAK(ship_explosion_length, 0.2f, 3.0f);
	GAME_TWEAK(energy_max, 10.0f, 100.0f);
	GAME_TWEAK(energy_decrease_speed, 0.1f, 10.0f);
	GAME_TWEAK(energy_holding, 0.1f, 10.0f);
	GAME_TWEAK(energy_neutralization, 1.0f, 100.0f);
	GAME_TWEAK(energybar_alpha, 0.0f, 1.0f);
	GAME_TWEAK(energybar_y, 0.0f, 320.0f);
	GAME_TWEAK(energybar_warn_length, 0.0f, 1.0f);
	GAME_TWEAK(energybar_spacing, 0.0f, 100.0f);
	GAME_TWEAK(start_anim_length, 0.5f, 5.0f);
	GAME_TWEAK(start_anim_obj_fadein, 0.1f, 1.0f);
	GAME_TWEAK(start_anim_obj_size, 0.1f, 10.0f);
	GAME_TWEAK(bullet_cost, 0.0f, 10.0f);
	GAME_TWEAK(bullet_damage, 0.0f, 10.0f);
	GAME_TWEAK(shockwave_min_size, 0.1f, 5.0f);
	GAME_TWEAK(shockwave_max_size, 0.1f, 5.0f);
}

void game_init(void) {
	#ifndef NO_DEVMODE
	devmode_init();
	#endif

	state_init();
	controls_init();
	menus_init();
	particles_init("greed_assets/", PARTICLES_LAYER);
	arenas_init();
	arena_init();
	physics_init();
	objects_init("greed_assets/object_defs.mml");

	atlas1 = tex_load("greed_assets/atlas1.png");
	shards = tex_load("greed_assets/shards.png");

	// Generate texture source rentangles
	for(uint y = 0; y < SHIP_COLORS; ++y) {
		for(uint x = 0; x < SHIP_FRAMES; ++x) {
			float px = (float)x * 50.0f;
			float py = (float)y * 72.0f;
			ship_rects[y][x] = rectf(px, py, px + 50.0f, py + 72.0f);
		}
	}	
	ship_shadow_rect = rectf(429.0f, 0.0f, 429.0f + 58.0f, 4.0f + 76.0f);

	for(uint x = 0; x < VORTEX_FRAMES; ++x) {
		float px = (float)x * 50.0f;
		vortex_rects[x] = rectf(px, 291.0f, px + 50.0f, 291.0f + 50.0f);
	}

	// These are misaligned, so set coords manually
	platform_ring_rects[0] = rectf(0.0f, 359.0f, 52.0f, 359.0f + 52.0f);
	platform_ring_rects[1] = rectf(71.0f, 359.0f, 71 + 52.0f, 359.0f + 52.0f);
	platform_ring_rects[2] = rectf(143.0f, 359.0f, 143 + 52.0f, 359.0f + 52.0f);
	platform_ring_rects[3] = rectf(215.0f, 359.0f, 215 + 52.0f, 359.0f + 52.0f);
	platform_ring_rects[4] = rectf(287.0f, 359.0f, 287 + 56.0f, 359.0f + 56.0f);
	platform_ring_rects[5] = rectf(360.0f, 360.0f, 360.0f + 52.0f, 360.0f + 52.0f);
	platform_ring_rects[6] = rectf(433.0f, 360.0f, 433.0f + 52.0f, 360.0f + 52.0f);

	platform_rects[0] = rectf(1.0f, 433.0f, 1.0f + 40.0f, 433.0f + 40.0f);
	platform_rects[1] = rectf(73.0f, 433.0f, 73.0f + 40.0f, 433.0f + 40.0f);

	platform_shadow_rect = rectf(144.0f, 431.0f, 144.0f + 45.0f, 431.0f + 45.0f);

	bullet_rect = rectf(217.0f, 432.0f, 217.0f + 12.0f, 432.0f + 12.0f);
	bullet_shadow_rect = rectf(288.0f, 432.0f, 288.0f + 16.0f, 432.0f + 16.0f);

	energybar_rects[0] = rectf(0.0f, 480.0f, 11.0f, 480.0f + 21.0f);
	energybar_rects[1] = rectf(11.0f, 480.0f, 21.0f, 480.0f + 21.0f);
	energybar_rects[2] = rectf(21.0f, 480.0f, 32.0f, 480.0f + 21.0f);

	shockwave_rect = rectf(400.0f, 80.0f, 400.0f + 110.f, 80.0f + 110.0f);

	// Shards
	for(uint i = 0; i < SHIP_COLORS; ++i) {
		float sx = (float)(i%2) * 105.0f;
		float sy = (float)(i/2) * 105.0f;
		for(uint j = 0; j < SHIP_SHARDS; ++j) {
			float dx = (float)(j%3) * 35.0f;
			float dy = (float)(j/3) * 35.0f;
			uint ii = i;
			// First two colors are mixed up
			if(i == 0) 
				ii = 1;
			else 
				if(i == 1) 
					ii = 0;
			ship_shards[ii][j] = rectf(sx+dx, sy+dy, sx+dx+35.0f, sy+dy+35.0f);
		}
	}

	n_ships = 0;
	n_platforms = 0;
}	

void game_close(void) {
	tex_free(atlas1);
	tex_free(shards);

	objects_close();
	physics_close();
	arena_close();
	arenas_close();
	particles_close();
	menus_close();
	controls_close();
	state_close();
	
	#ifndef NO_DEVMODE
	devmode_close();
	#endif
}

void game_reset(const char* arena, uint n_players) {
	arena_reset(arena, n_players);
	physics_reset(n_players);
	ai_reset(480.0f, 320.0f);

	n_ships = n_players;
	n_platforms = current_arena_desc.n_platforms;

	// Reset ships
	for(uint i = 0; i < n_players; ++i) {
		ship_states[i].is_exploding = false;
		ship_states[i].explode_t = 0.0f;
		ship_states[i].energy = energy_max;
		ship_states[i].last_bullet_t = 0.0f;
		ship_states[i].vortex_opacity = 0.0f;
		ship_states[i].vortex_angle = 0.0f;
		ship_states[i].is_accelerating = false;
	}	

	// Reset platforms
	for(uint i = 0; i < n_platforms; ++i) {
		platform_states[i].color = PLATFORM_NEUTRAL;
		platform_states[i].last_color = PLATFORM_NEUTRAL;
		platform_states[i].color_fade = 0.0f;
		platform_states[i].ring_angle = rand_float_range(0.0f, 360.0f);
		platform_states[i].is_beeping = false;
	}	

	// Works as long as arenas are named "cx_arenay"
	uint chapter = arena[14] - '0';
	sounds_event_ex(MUSIC, chapter);
}	

void game_shoot(uint ship) {
	assert(ship < n_ships);

	float t = time_ms();
	if(ship_states[ship].last_bullet_t + (1000.0f/ship_firing_speed) < t) {
		ship_states[ship].last_bullet_t = t;
		ship_states[ship].energy -= bullet_damage;
		ship_states[ship].energy = MAX(0.0f, ship_states[ship].energy);
		physics_spawn_bullet(ship);
		sounds_event(SHOT);
	}
}

void game_platform_taken(uint ship_id, uint platform_id) {
	assert(ship_id < n_ships);
	assert(platform_id < n_platforms);
	
	Vector2 pos = current_arena_desc.platforms[platform_id];
	particles_spawn("ptransition_in", &pos, 0.0f);
	sounds_event(PLATFORM_TAKEN);
}	

void game_bullet_hit(uint ship) {
	assert(ship < n_ships);

	ship_states[ship].energy -= bullet_damage;
	ship_states[ship].energy = MAX(0.0f, ship_states[ship].energy);
	sounds_event(COLLISION_BULLET_SHIP);
}

void _control_keyboard1(uint ship) {
	assert(ship < n_ships);

	ship_states[ship].is_accelerating = key_pressed(KEY_UP);

	if (key_down(KEY_UP))
		sounds_event_ex(SHIP_ACC, 1);
	if (key_up(KEY_UP))
		sounds_event_ex(SHIP_ACC, 2);

	physics_control_ship(ship,
		key_pressed(KEY_LEFT),
		key_pressed(KEY_RIGHT),
		key_pressed(KEY_UP));
	
	if(key_pressed(KEY_A))
		game_shoot(ship);
}	

bool _is_paused(void) {
	return menu_state == MENU_PAUSE || menu_transition == MENU_PAUSE;
}

float _game_time(void) {
	static float pause_t = -1.0f;
	static float pause_acc = 0.0f;
	static float t = 0.0f;
	static bool was_paused = false;
	bool is_paused = _is_paused();
	
	if(!was_paused && is_paused)
		pause_t = time_ms() / 1000.0f;
	if(was_paused && !is_paused) {
		assert(pause_t > 0.0f);
		pause_acc += (time_ms() / 1000.0f) - pause_t;
	}	

	if(!is_paused)
		t = (time_ms() / 1000.0f) - pause_acc;
	was_paused = is_paused;
	return t;
}

float _game_dt(void) {
	if(_is_paused())
		return 0.0f;
	else
		return time_delta() / 1000.0f;
}

bool _in_start_anim(void) {
	float t = _game_time();
	return start_anim_t + start_anim_length >= t;
}

void game_update(void) {
	#ifndef NO_DEVMODE
	//devmode_update();
	#endif

	sounds_update();
	menus_update();

	if(menu_state != MENU_GAME && menu_state != MENU_GAMEOVER) {
		#ifndef NO_DEVMODE
		devmode_overload(NULL);
		#endif
		return;
	}
	else {
		#ifndef NO_DEVMODE
		devmode_overload(arena_get_current());
		#endif
	}

	if(_in_start_anim())
		return;

	if(_is_paused())
		return;

	// Paused state might change here
	controls_draw(0.0f);
	if(menu_state == MENU_GAME && !ship_states[0].is_exploding) {
		_control_keyboard1(0);
		controls_update(0);
	}

	if(_is_paused())
		return;

	float dt = _game_dt();
	float time = _game_time();

	physics_update(dt);
	particles_update(time);

	// Update ships
	for(uint ship = 0; ship < n_ships; ++ship) {
		if(ship_states[ship].is_exploding)
			continue;

		// Count platforms taken
		uint platforms_taken = 0;
		for(uint platform = 0; platform < n_platforms; ++platform) {
			if(platform_states[platform].color == ship) 
				platforms_taken++;
		}

		// Update energy
		float energy_delta = 
			-energy_decrease_speed + (float)platforms_taken * energy_holding;
		ship_states[ship].energy += energy_delta * dt;	
		ship_states[ship].energy = 
			clamp(0.0f, energy_max, ship_states[ship].energy); 

		if(ship_states[ship].energy == 0.0f && menu_transition == MENU_GAME) {
			ship_states[ship].is_exploding = true;
			ship_states[ship].explode_t = time;
			
			sounds_event(SHIP_KILL);

			particles_spawn("explosion1", &physics_state.ships[ship].pos, 0.0f);	
			particles_spawn("explosion2", &physics_state.ships[ship].pos, 0.0f);	

			physics_shockwave(ship);

			uint ships_left = 0;
			for(uint i = 0; i < n_ships; ++i)
				ships_left += ship_states[i].is_exploding ? 0 : 1;

			// Show game over screen if only 1 ship is left, or player exploded
			if(ship == 0 || ships_left == 1) {
				menu_transition = MENU_GAMEOVER;
				menu_transition_t = time_ms() / 1000.0f;
				menu_last_game_did_win = !ship_states[0].is_exploding;
			}
		}

		// Update size & mass
		float percent_taken = (float)platforms_taken / (float)n_platforms;
		float target_size = lerp(ship_min_size, ship_max_size,
			percent_taken);

		if(fabs(physics_state.ships[ship].scale - target_size) > 0.01) {
			physics_state.ships[ship].scale =
				lerp(physics_state.ships[ship].scale, target_size, 0.05);
			physics_set_ship_size(ship, physics_state.ships[ship].scale);	
		}
	}

	// Update vortices
	for(uint ship = 0; ship < n_ships; ++ship) {
		ShipState* state = &(ship_states[ship]);

		if(state->is_accelerating)
			state->vortex_opacity += dt * (1.0f / vortex_fade_in_length);
		else
			state->vortex_opacity -= dt * (1.0f / vortex_fade_out_length);
		state->vortex_opacity = MAX(MIN(state->vortex_opacity, 1.0f), 0.0f);

		if(state->vortex_opacity == 0.0f)
			continue;

		state->vortex_angle += vortex_animation_speed * dt;	
		if(state->vortex_angle > 360.0f)
			state->vortex_angle -= 360.0f;
	}		

	// Update platforms
	for(uint platform = 0; platform < n_platforms; ++platform) {
		PlatformState* state = &platform_states[platform];

		// Rotate ring
		state->ring_angle += dt * platform_ring_rotation_speed;	
		if(state->ring_angle > 360.0f)
			state->ring_angle -= 360.0f;
		
		// Check if platform is in color transition
		if(state->color != state->last_color) {
			state->color_fade += dt * (1.0f / platform_transition_length);

			if(state->color_fade >= 1.0f) {
				state->last_color = state->color;
				state->color_fade = 0.0f;
			}	
		}	

		if(state->color != PLATFORM_NEUTRAL) {
			// Check if platform needs to go back to neutral state
			if(time >= state->activation_t + platform_holding_time) {
				ship_states[state->color].energy += energy_neutralization;
				state->color = PLATFORM_NEUTRAL;

				Vector2 pos = current_arena_desc.platforms[platform];
				particles_spawn("ptransition_out", &pos, 0.0f);
				sounds_event(PLATFORM_NEUTRALIZED);
			}
		}	
	}	

	ai_update();
}	

void _draw_ship(const Vector2* pos, uint ship) {
	assert(pos);

	float zrot = physics_state.ships[ship].zrot;
	float rot = physics_state.ships[ship].rot;
	float scale = physics_state.ships[ship].scale;

	uint frame = (uint)(zrot / (360.0f / (float)SHIP_FRAMES));
	frame = frame % SHIP_FRAMES;
	assert(frame < SHIP_FRAMES);

	// Ship
	gfx_draw_textured_rect(atlas1, OBJECTS_LAYER,
		&(ship_rects[ship][frame]),	pos, rot, scale, COLOR_WHITE);

	// Vortex
	if(ship_states[ship].vortex_opacity > 0.0f) {
		float vortex_rot = ship_states[ship].vortex_angle;
		uint vortex_frame = 
			(uint)(vortex_rot / (360.0f / (float)VORTEX_FRAMES));
		
		Vector2 shift = vec2_rotate(vec2(0.0f, 10.0f * scale), rot);
		Vector2 vortex_pos = vec2_add(*pos, shift); 	
		assert(vortex_frame < VORTEX_FRAMES);	

		Color vortex_color = color_lerp(COLOR_TRANSPARENT, COLOR_WHITE,
			ship_states[ship].vortex_opacity);
		
		gfx_draw_textured_rect(atlas1, FOREGROUND_LAYER, 
			&(vortex_rects[vortex_frame]), &vortex_pos, rot, scale, vortex_color);
	}		

	// Shadow
	Vector2 shadow_pos = vec2_add(*pos, current_arena_desc.shadow_shift);
	gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
		&ship_shadow_rect, &shadow_pos, rot, scale, COLOR_WHITE);
}		

void _draw_shattered_ship(uint ship, float t) {
	assert(ARRAY_SIZE(ship_shard_offsets) == SHIP_SHARDS);
	assert(ship < n_ships);
	assert(t >= 0.0f && t <= 1.0f);

	Vector2 pos = physics_state.ships[ship].pos;
	float rot = physics_state.ships[ship].rot;
	float scale = physics_state.ships[ship].scale;

	// Shockwave
	Color shockwave_color = color_lerp(
		COLOR_WHITE, COLOR_WHITE & 0xFFFFFF,
		sinf(t * 3.1415f / 2.0f)
	);
	float shockwave_size = lerp(shockwave_min_size, shockwave_max_size, t);
	gfx_draw_textured_rect(atlas1, OBJECTS_LAYER, &shockwave_rect,
		&pos, rot, shockwave_size, shockwave_color);

	float x_min = shockwave_size * rectf_width(&shockwave_rect) / 2.0f;
	float x_max = (float)SCREEN_WIDTH - x_min;
	float y_min = x_min;
	float y_max = (float)SCREEN_HEIGHT - y_min;

	// Virtual shockwaves on screen boundries
	WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos, 
		gfx_draw_textured_rect(atlas1, OBJECTS_LAYER, &shockwave_rect,
			&npos, rot, shockwave_size, shockwave_color));

	float escale = scale + sqrtf(t) * 1.5f;

	Vector2 rot_offset = vec2_rotate(vec2(17.5f*escale, 17.5f*escale), rot);
	Vector2 center_offset = vec2(-35.0f*scale, -35.0f*scale);
	pos = vec2_add(pos, vec2_scale(center_offset, 0.5f));
	pos = vec2_add(pos, rot_offset);

	Vector2 offsets[SHIP_SHARDS];
	memcpy(offsets, ship_shard_offsets, sizeof(offsets));
	gfx_transform(offsets, SHIP_SHARDS, &pos, rot, escale);

	x_min = 0.0f;
	x_max = (float)SCREEN_WIDTH;
	y_min = 0.0f;
	y_max = (float)SCREEN_HEIGHT;

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, t*t*t*t);
	float s = 35.0f * scale;
	for(uint i = 0; i < SHIP_SHARDS; ++i) {
		RectF dest = {
			offsets[i].x, offsets[i].y, 
			offsets[i].x+s, offsets[i].y+s
		};

		// Adjust for wrap-around screen, not 100% correct -
		// single shard is always drawn once, but I doubt
		// anyone will notice
		if(dest.left > x_max) {
			dest.left -= x_max - x_min;
			dest.right -= x_max - x_min;
		}
		if(dest.right < x_min) {
			dest.left += x_max - x_min;
			dest.right += x_max - x_min;
		}
		if(dest.bottom > y_max) {
			dest.bottom -= y_max - y_min;
			dest.top -= y_max - y_min;
		}
		if(dest.top < y_min) {
			dest.bottom += y_max - y_min;
			dest.top += y_max - y_min;
		}

		float a = rot + lerp(0.0f, ship_shard_angles[i], t);
		video_draw_rect_rotated(shards, OBJECTS_LAYER,
			&ship_shards[ship][i], &dest, a, c);
	}
}	

void _draw_energybar(const Vector2* pos, float length, Color color) {
	assert(pos);
	assert(length >= 0.0f);

	float w = rectf_width(&energybar_rects[0]);
	float h = rectf_height(&energybar_rects[0]);

	RectF dest = rectf(pos->x - length/2.0f - w, pos->y - h/2.0f, 0.0f, 0.0f);
	video_draw_rect(atlas1, ENERGYBAR_LAYER, &energybar_rects[0], &dest, color);  
	dest.left += length + w;
	video_draw_rect(atlas1, ENERGYBAR_LAYER, &energybar_rects[2], &dest, color);  

	dest.bottom = dest.top + h;
	dest.left = pos->x - length/2.0f;
	dest.right = dest.left + length;
	video_draw_rect(atlas1, ENERGYBAR_LAYER, &energybar_rects[1], &dest, color);
}

float _calc_core_shrink_ratio(uint platform) {
	assert(platform < n_platforms);

	float time = _game_time();
	PlatformState* state = &(platform_states[platform]);

	float core_shrink = 1.0f;
	float neutralization_t = 
		platform_holding_time + state->activation_t;

	if(time >= neutralization_t - platform_warning1_time) {
		if(!state->is_beeping) {
			sounds_event(PLATFORM_BEEP);
			state->is_beeping = true;
		}

		float sin_t = sin(time*PI*2.0f);
		core_shrink = sin_t*sin_t;
		if(time >= neutralization_t - platform_warning2_time) {
			float sin_3t = sin(time*PI*6.0f);
			core_shrink *= sin_3t*sin_3t * 0.2f;
		}
		else {
			core_shrink *= 0.1f;
		}
		
		// Fade-in animation in first second
		float t = time - (neutralization_t - platform_warning1_time);
		if(t < 1.0f)
			core_shrink = lerp(0.0f, core_shrink, t);

		core_shrink = 1.0f - core_shrink;
	}	
	return core_shrink;
}		

void game_render(void) {
	#ifndef NO_DEVMODE
	Vector2 p = vec2(400.0f, 1.0f);
	if(gui_switch(&p, "devmode"))
		devmode_render();
	#endif

	menus_render();

	if(menu_state != MENU_GAME && menu_state != MENU_GAMEOVER 
		&& menu_state != MENU_PAUSE)
		return;

	if(draw_physics_debug)
		physics_debug_draw();

	if(draw_ai_debug)
		ai_debug_draw();

	float time = _game_time();
	if(_in_start_anim()) {
		float t = (time - start_anim_t) / start_anim_length;
		game_render_startanim(t);
		return;
	}	
	
	particles_draw();

	RectF obstructions[4];
	uint obstruction_count = 0;

	// Draw ships & energy bars
	for(uint ship = 0; ship < n_ships; ++ship) {
		if(ship_states[ship].is_exploding) {
			float t = (time - ship_states[ship].explode_t) 
				/ ship_explosion_length; 
			if(t <= 1.0f)
				_draw_shattered_ship(ship, t);
			continue;
		}

		// Ship
		Vector2 pos = physics_state.ships[ship].pos;
		_draw_ship(&pos, ship);

		float scale = physics_state.ships[ship].scale;
		float x_min = ship_circle_radius * scale;
		float x_max = (float)SCREEN_WIDTH - x_min;
		float y_min = x_min;
		float y_max = (float)SCREEN_HEIGHT - y_min;

		float r = (ship_circle_radius + 10.0f) * scale;
		obstructions[obstruction_count++] = rectf(
			pos.x - r, pos.y - r,
			pos.x + r, pos.y + r
		);

		// Virtual ships on screen boundries
		WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos, _draw_ship(&npos, ship));
	
		// Energy bar
		float max_len = (float)(SCREEN_WIDTH/n_ships) - energybar_spacing;
		float normalized_energy = ship_states[ship].energy / energy_max;
		float length = normalized_energy * max_len;
		pos = vec2(
				(float)(SCREEN_WIDTH/n_ships) * ((float)ship + 0.5f),
				energybar_y
		);
		float warn_t = MIN(1.0f, normalized_energy / energybar_warn_length);
		Color c = ship_colors[ship];
		Color ca = c & 0x00FFFFFF;
		float ct = lerp(
				energybar_alpha + (1.0f - energybar_alpha) * sin(time * PI * 6.0f),
				energybar_alpha, 
				warn_t
		);		
		c = color_lerp(c, ca, ct);
		_draw_energybar(&pos, length, c);
	}		

	// Reduce obstructions
	for(uint i = 0; i < obstruction_count; ++i) {
		for(uint j = i + 1; j < obstruction_count; ++j) {
			if(rectf_rectf_collision(&obstructions[i], &obstructions[j])) {
				obstructions[i] = rectf_bbox(&obstructions[i], &obstructions[j]);
				obstructions[j] = obstructions[obstruction_count-1];
				obstruction_count--;
				i--;
				j--;
				break;
			}
		}
	}

	arena_draw(obstructions, obstruction_count);

	// Draw bullets
	for(uint bullet = 0; bullet < physics_state.n_bullets; ++bullet) {
		Vector2 pos = physics_state.bullets[bullet].pos;

		// Bullet
		gfx_draw_textured_rect(atlas1, OBJECTS_LAYER,
			&bullet_rect, &pos, 0.0f, 1.0f, COLOR_WHITE);
		
		// Shadow
		pos = vec2_add(pos, current_arena_desc.shadow_shift);
		gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
			&bullet_shadow_rect, &pos, 0.0f, 1.0f, COLOR_WHITE);
	}		

	// Draw platforms
	for(uint platform = 0; platform < n_platforms; ++platform) {
		PlatformState* state = &(platform_states[platform]);
		Vector2 pos = current_arena_desc.platforms[platform];
		Vector2 shadow_pos = vec2_add(pos, current_arena_desc.shadow_shift);

		// Is transitioning?
		if(state->color != state->last_color) {
			float t = state->color_fade;

			float core_size, last_core_size;
			float ring_angle, last_ring_angle;
			RectF* ring;
			RectF* ring_shadow;
			RectF* last_ring;
			RectF* last_ring_shadow;

			sound_stop(PLATFORM_BEEP);

			if(state->color == PLATFORM_NEUTRAL) {
				core_size = platform_core_size;
				ring = &(platform_ring_rects[PLATFORM_RING_NEUTRAL]);
				ring_shadow =
					&(platform_ring_rects[PLATFORM_RING_NEUTRAL_SHADOW]);
				ring_angle = state->ring_angle;
				state->is_beeping = false;
			}
			else {
				core_size = platform_active_core_size;
				ring = &(platform_ring_rects[state->color]);
				ring_shadow = &(platform_ring_rects[PLATFORM_RING_SHADOW]);
				ring_angle = -state->ring_angle;
			}	

			if(state->last_color == PLATFORM_NEUTRAL) {
				last_core_size = platform_core_size;
				last_ring = &(platform_ring_rects[PLATFORM_RING_NEUTRAL]);
				last_ring_shadow =
					&(platform_ring_rects[PLATFORM_RING_NEUTRAL_SHADOW]);
				last_ring_angle = state->ring_angle;	
			}
			else {
				last_core_size = 
					platform_active_core_size * _calc_core_shrink_ratio(platform);
				last_ring = &(platform_ring_rects[state->last_color]);
				last_ring_shadow = &(platform_ring_rects[PLATFORM_RING_SHADOW]);
				last_ring_angle = -state->ring_angle;
			}	

			float current_core_size = smoothstep(last_core_size, core_size, t);	

			Color last_ring_color = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, t);
			Color ring_color = color_lerp(COLOR_TRANSPARENT, COLOR_WHITE, t);

			float ring_size = smoothstep(0.1f, 1.0f, t);
			float last_ring_size = smoothstep(1.0f, 0.1f, t);

			// TODO: correct drawing of transitioning platforms
			// is only possible when stable sorting is used in system_***.c
			// Either guarantee that, or use more layers in the future.

			// Core
			gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
				&(platform_rects[0]), &pos, 0.0f, current_core_size,
				COLOR_WHITE);
			
			// Core shadow
			gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
				&platform_shadow_rect, &shadow_pos, 0.0f,
				current_core_size, COLOR_WHITE);

			// Last ring
			gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
				last_ring, &pos, last_ring_angle, last_ring_size, 
				last_ring_color);

			// Last ring shadow
			gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
				last_ring_shadow, &shadow_pos, last_ring_angle, 
				last_ring_size, last_ring_color);

			// Ring
			gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
				ring, &pos, ring_angle, ring_size, ring_color);
			
			// Ring shadow
			gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
				ring_shadow, &shadow_pos, ring_angle, ring_size,
				ring_color);
		}	
		else {
			if(state->color == PLATFORM_NEUTRAL) {
				// Core
				gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER, 
					&(platform_rects[0]), &pos, 0.0f, platform_core_size, 
					COLOR_WHITE);

				// Core shadow
				gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
					&platform_shadow_rect, &shadow_pos, 0.0f, 
					platform_core_size, COLOR_WHITE);
				
				// Ring
				gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
					&(platform_ring_rects[PLATFORM_RING_NEUTRAL]), &pos,
					state->ring_angle, 1.0f, COLOR_WHITE);

				// Ring shadow	
				gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
					&(platform_ring_rects[PLATFORM_RING_NEUTRAL_SHADOW]),
					&shadow_pos, state->ring_angle, 1.0f, COLOR_WHITE);
			}		
			else {
				float core_shrink = _calc_core_shrink_ratio(platform);
						
				// Core
				gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
					&(platform_rects[0]), &pos, 0.0f, 
					platform_active_core_size * core_shrink, COLOR_WHITE);

				// Core shadow
				gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
					&platform_shadow_rect, &shadow_pos, 0.0f,
					platform_active_core_size, COLOR_WHITE);

				// Ring
				gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
					&(platform_ring_rects[state->color]), &pos,
					-state->ring_angle, 1.0f, COLOR_WHITE);

				// Ring shadow
				gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
					&(platform_ring_rects[PLATFORM_RING_SHADOW]),
					&shadow_pos, -state->ring_angle, 1.0f, COLOR_WHITE);
			}
		}	
	}	
}	

void game_render_transition(float t) {
	if(menu_state == MENU_PAUSE || menu_transition == MENU_PAUSE) {
		controls_draw(t);
		return;
	}	

	if(t < 0.0f) {
		arena_draw(NULL, 0);
		start_anim_t = _game_time();
	}
}

void _startanim_platform(uint platform, float t) {
	PlatformState* state = &platform_states[platform];
	Vector2 pos = current_arena_desc.platforms[platform];
	Vector2 shadow_pos = vec2_add(pos, current_arena_desc.shadow_shift);

	Color c = color_lerp(COLOR_WHITE&0xFFFFFF, COLOR_WHITE, t);
	float size = smoothstep(start_anim_obj_size, 1.0f, t);

	// Core
	gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER, 
		&(platform_rects[0]), &pos, 0.0f, platform_core_size * size, c); 

	// Core shadow
	gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
		&platform_shadow_rect, &shadow_pos, 0.0f, 
		platform_core_size * size, c);
	
	// Ring
	gfx_draw_textured_rect(atlas1, BACKGROUND_LAYER,
		&(platform_ring_rects[PLATFORM_RING_NEUTRAL]), &pos,
		state->ring_angle, size, c);

	// Ring shadow	
	gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
		&(platform_ring_rects[PLATFORM_RING_NEUTRAL_SHADOW]),
		&shadow_pos, state->ring_angle, size, c);
}					

void _startanim_ship(uint ship, float t) {
	Vector2 pos = physics_state.ships[ship].pos;
	float zrot = physics_state.ships[ship].zrot;
	float rot = physics_state.ships[ship].rot;
	float scale = physics_state.ships[ship].scale;

	uint frame = (uint)(zrot / (360.0f / (float)SHIP_FRAMES));
	assert(frame < SHIP_FRAMES);

	Color c = color_lerp(COLOR_WHITE&0xFFFFFF, COLOR_WHITE, t);
	float size = smoothstep(start_anim_obj_size, 1.0f, t);

	// Ship
	gfx_draw_textured_rect(atlas1, OBJECTS_LAYER,
		&(ship_rects[ship][frame]),	&pos, rot, scale * size, c);

	// Shadow
	Vector2 shadow_pos = vec2_add(pos, current_arena_desc.shadow_shift);
	gfx_draw_textured_rect(atlas1, SHADOWS_LAYER,
		&ship_shadow_rect, &shadow_pos, rot, scale * size, c);
}

void _startanim_energybar(uint ship, float t) {
	float max_len = (float)(SCREEN_WIDTH/n_ships) - energybar_spacing;
	float length = sqrtf(t) * max_len;
	Vector2 pos = vec2(
			(float)(SCREEN_WIDTH/n_ships) * ((float)ship + 0.5f),
			energybar_y
	);
	Color c = ship_colors[ship];
	Color ca = c & 0x00FFFFFF;
	_draw_energybar(&pos, length, 
			color_lerp(ca, c, t * (1.0f - energybar_alpha)));
}

void game_render_startanim(float t) {
	arena_draw(NULL, 0);

	controls_draw(1.0f - t);

	uint n_objs = n_ships + n_platforms;
	float norm_fadein = start_anim_obj_fadein / start_anim_length;

	float obj_interval = (1.0f - norm_fadein) / (float)n_objs;

	float curr_obj_start = 0.0f;

	// Platforms
	for(uint i = 0; i < n_platforms; ++i) {
		float obj_t = normalize(t, curr_obj_start, 
			curr_obj_start + norm_fadein);
		obj_t = clamp(0.0f, 1.0f, obj_t);	
		_startanim_platform(i, obj_t);
		curr_obj_start += obj_interval;
	}

	// Ships
	for(uint i = 0; i < n_ships; ++i) {
		float obj_t = normalize(t, curr_obj_start, 
			curr_obj_start + norm_fadein);
		obj_t = clamp(0.0f, 1.0f, obj_t);	
		_startanim_ship(i, obj_t);
		_startanim_energybar(i, obj_t);
		curr_obj_start += obj_interval;
	}
}

float game_taken_platforms_frac(uint color) {
	uint count = 0;
	for(uint i = 0; i < n_platforms; ++i) {
		if(platform_states[i].color == color)
			count++;
	}
	return (float)count / (float)n_platforms;
}

uint game_random_free_platform(void) {
	return game_random_taken_platform(MAX_UINT32);
}	

uint game_random_taken_platform(uint color) {
	uint count = 0;
	for(uint i = 0; i < n_platforms; ++i) {
		if(platform_states[i].color == color)
			count++;
	}
	if(count) {
		uint id;
		do {
			id = rand_int(0, n_platforms);
		} while(platform_states[id].color != color);	
		return id;
	}
	return MAX_UINT32;
}

float game_time_till_neutralization(uint platform) {
	if(platform_states[platform].color == MAX_UINT32)
		return 0.0f;

	float time = time_ms() / 1000.0f;	

	return (time + platform_holding_time) - 
		platform_states[platform].activation_t;	
}

