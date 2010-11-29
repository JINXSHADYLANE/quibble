#ifndef GAME_H
#define GAME_H

#include <utils.h>
#include <gui.h>
#include <tweakables.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define MAX_SHIPS 4
#define MAX_BULLETS 128
#define MAX_PLATFORMS 16

typedef struct {
	float energy;
	float last_bullet_t;
	float vortex_opacity;
	float vortex_angle;
	bool is_accelerating;
} ShipState;	

#define PLATFORM_NEUTRAL MAX_UINT32

typedef struct {
	// Color is ship number, MAX_UINT32 is neutral;
	uint color;
	uint last_color;
	float color_fade;
	float activation_t;
	float ring_angle;
} PlatformState;

extern uint n_ships;
extern uint n_platforms;
extern ShipState ship_states[MAX_SHIPS];
extern PlatformState platform_states[MAX_PLATFORMS];
extern const Color ship_colors[MAX_SHIPS];

extern bool draw_physics_debug;
extern bool draw_ai_debug;

void game_register_tweaks(Tweaks* tweaks);
void game_init(void);
void game_close(void);

void game_reset(const char* arena, uint n_players);
void game_update(void);
void game_platform_taken(uint ship, uint platform);
void game_render(void);
void game_render_transition(float t);
void game_render_startanim(float t);

void game_shoot(uint ship);

// Helpers for ai code:
float game_taken_platforms_frac(uint color);

uint game_random_free_platform(void);
uint game_random_taken_platform(uint color);

float game_time_till_neutralization(uint platform);

#endif

