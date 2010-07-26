#include "game.h"
#include <system.h>
#include <gfx_utils.h>

#include "arena.h"
#include "physics.h"
#include "sounds.h"
#include "ai.h"
#include "devmode.h"

#define ENERGYBAR_LAYER 7
#define FOREGROUND_LAYER 6
#define OBJECTS_LAYER 5
#define BACKGROUND_LAYER 4
#define SHADOWS_LAYER 3

#define SHIP_COLORS 4
#define SHIP_FRAMES 8
#define VORTEX_FRAMES 7
#define PLATFORM_RINGS 8
#define PLATFORM_FRAMES 2
#define PLATFORM_RING_SHADOW 4
#define PLATFORM_RING_NEUTRAL 5
#define PLATFORM_RING_NEUTRAL_SHADOW 6

TexHandle atlas1;
RectF ship_rects[SHIP_COLORS][SHIP_FRAMES];
RectF ship_shadow_rect;
RectF vortex_rects[VORTEX_FRAMES];
RectF platform_ring_rects[PLATFORM_RINGS];
RectF platform_rects[PLATFORM_FRAMES];
RectF platform_shadow_rect;
RectF bullet_rect;
RectF bullet_shadow_rect;

// Game state
uint n_ships;
uint n_platforms;
ShipState ship_states[MAX_SHIPS];
PlatformState platform_states[MAX_PLATFORMS];

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
float energy_max = 100.0f;
float energy_decrease_speed = 2.0f;
float energy_holding = 0.3f;
float energy_neutralization = 10.0f;
float energybar_length = 400.0f;

const Color ship_colors[] = {
	COLOR_RGBA(33, 48, 189, 255),
	COLOR_RGBA(74, 185, 46, 255),
	COLOR_RGBA(240, 214, 53, 255),
	COLOR_RGBA(143, 51, 140, 255)
};	

const float energybar_y_pos[] = {
	10.0f, 20.0f, 30.0f, 40.0f
};	

// Devmode switches
bool draw_physics_debug = false;
bool draw_ai_debug = false;

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
	GAME_TWEAK(energy_max, 10.0f, 100.0f);
	GAME_TWEAK(energy_decrease_speed, 0.1f, 10.0f);
	GAME_TWEAK(energy_holding, 0.1f, 10.0f);
	GAME_TWEAK(energy_neutralization, 1.0f, 100.0f);
	GAME_TWEAK(energybar_length, 50.0f, 400.0f);
}

void game_init(void) {
	#ifndef NO_DEVMODE
	devmode_init();
	#endif

	arena_init();
	physics_init();

	atlas1 = tex_load("greed_assets/atlas1.png");

	// Generate texture source rentangles
	for(uint y = 0; y < SHIP_COLORS; ++y) {
		for(uint x = 0; x < SHIP_FRAMES; ++x) {
			float px = (float)x * 50.0f;
			float py = (float)y * 72.0f;
			ship_rects[y][x] = rectf(px, py, px + 50.0f, py + 72.0f);
		}
	}	
	ship_shadow_rect = rectf(433.0f, 0.0f, 433.0f + 50.0f, 72.0f);

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
}	

void game_close(void) {
	physics_close();
	arena_close();
	
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
	}	
}	

void game_shoot(uint ship) {
	assert(ship < n_ships);

	float t = time_ms();
	if(ship_states[ship].last_bullet_t + (1000.0f/ship_firing_speed) < t) {
		ship_states[ship].last_bullet_t = t;
		physics_spawn_bullet(ship);
		sounds_event(SHOT);
	}
}

void _control_keyboard1(uint ship) {
	assert(ship < n_ships);

	ship_states[ship].is_accelerating = key_pressed(KEY_UP);

	physics_control_ship(ship,
		key_pressed(KEY_LEFT),
		key_pressed(KEY_RIGHT),
		key_pressed(KEY_UP));
	
	if(key_pressed(KEY_A))
		game_shoot(ship);
}	

void game_update(void) {
	#ifndef NO_DEVMODE
	devmode_update();
	#endif

	_control_keyboard1(0);

	float dt = time_delta() / 1000.0f;
	float time = time_ms() / 1000.0f;

	physics_update(dt);

	// Update ships
	for(uint ship = 0; ship < n_ships; ++ship) {
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
		PlatformState* state = &(platform_states[platform]);

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

				sounds_event(PLATFORM_NEUTRALIZED);
			}
		}	
	}	

	ai_update();
}	

void game_platform_taken(uint ship_id, uint platform_id) {
	assert(ship_id < n_ships);
	assert(platform_id < n_platforms);
	
	sounds_event(PLATFORM_TAKEN);
}	

void _draw_ship(const Vector2* pos, uint ship) {
	assert(pos);

	float zrot = physics_state.ships[ship].zrot;
	float rot = physics_state.ships[ship].rot;
	float scale = physics_state.ships[ship].scale;

	uint frame = (uint)(zrot / (360.0f / (float)SHIP_FRAMES));
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

float _calc_core_shrink_ratio(uint platform) {
	assert(platform < n_platforms);

	float time = time_ms() / 1000.0f;
	PlatformState* state = &(platform_states[platform]);

	float core_shrink = 1.0f;
	float neutralization_t = 
		platform_holding_time + state->activation_t;

	if(time >= neutralization_t - platform_warning1_time) {
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

	if(draw_physics_debug)
		physics_debug_draw();

	if(draw_ai_debug)
		ai_debug_draw();
	
	arena_draw();

	// Draw ships & energy bars
	for(uint ship = 0; ship < n_ships; ++ship) {
		// Ship
		Vector2 pos = physics_state.ships[ship].pos;
		_draw_ship(&pos, ship);

		// Virtual ships on screen boundries
		if(pos.x < ship_circle_radius * 1.0f) {
			Vector2 npos = vec2(pos.x + (float)SCREEN_WIDTH, pos.y);
			_draw_ship(&npos, ship);
		}
		if(pos.x > (float)SCREEN_WIDTH - ship_circle_radius * 1.0f) {
			Vector2 npos = vec2(pos.x - (float)SCREEN_WIDTH, pos.y);
			_draw_ship(&npos, ship);
		}
		if(pos.y < ship_circle_radius * 1.0f) {
			Vector2 npos = vec2(pos.x, pos.y + (float)SCREEN_HEIGHT);
			_draw_ship(&npos, ship);
		}
		if(pos.y > (float)SCREEN_HEIGHT - ship_circle_radius * 1.0f) {
			Vector2 npos = vec2(pos.x, pos.y - (float)SCREEN_HEIGHT);
			_draw_ship(&npos, ship);
		}	

		// Energy bar
		float normalized_energy = ship_states[ship].energy / energy_max;
		float length = normalized_energy * energybar_length;
		float x_pos = ((float)SCREEN_WIDTH - energybar_length) / 2.0f;
		Vector2 v1 = vec2(x_pos, energybar_y_pos[ship]);
		Vector2 v2 = vec2(x_pos + length, v1.y);
		video_draw_line(ENERGYBAR_LAYER, &v1, &v2, ship_colors[ship]);
	}		

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

			if(state->color == PLATFORM_NEUTRAL) {
				core_size = platform_core_size;
				ring = &(platform_ring_rects[PLATFORM_RING_NEUTRAL]);
				ring_shadow =
					&(platform_ring_rects[PLATFORM_RING_NEUTRAL_SHADOW]);
				ring_angle = state->ring_angle;	
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
