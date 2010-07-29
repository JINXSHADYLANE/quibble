#ifndef AI_H
#define AI_H

#include "ai_precalc.h"
#include "arena.h"
#include <tweakables.h>

typedef enum {
	AI_DEFEND,
	AI_ATTACK,
	AI_CAPTURE
} AgentState;	

typedef struct {
	// Always try to control at least this fraction of platforms in arena
	float platforms_frac; 
	// Try to keep angle to steer target smaller than this
	float steer_tg_angle_tolerance;
	// If angle to target is bigger than this, use more aggressive steering
	float steer_tg_aggressive_angle;
	// If target is far enough, accelerate aggresively
	float steer_tg_aggressive_distance;
	// If distance between ship ant steer target is more than this, recalc path
	float steer_tg_max_distance;
	// If distance less than this, move steer tg
	float steer_tg_min_distance;
	// If distance to steer target is smaller than this, do not accelerate
	float steer_tg_coast_distance;
	// If target is closer than this, do not steer
	float steer_tg_no_steer_dist;
	// Do not try to stabilize if angular velocity is less than this
	float acceptable_angular_velocity;

	// If there is a ship in front within this distance and angle, shoot
	float shoot_angle;
	float shoot_distance;

	// When calculating if agent can get somewhere quickly enough,
	// use this average speed
	float estimated_speed;
} AgentPersonality;	

typedef struct {
	uint ship_id;
	AgentPersonality* personality;

	float last_think_t;
	float last_steer_t;
	AgentState state;
	uint dest_node;

	// Steer target
	uint steer_tg_node; 

	// Shoot target
	Vector2 shoot_tg_pos;
	bool shoot;

	// Steering
	int steer_left;
	int steer_right;
	int accelerate;
} Agent;	

#define MAX_AI_PERSONALITIES 8

extern const uint ai_personality_count;
extern const char* ai_personality_names[MAX_AI_PERSONALITIES];
	
void ai_register_tweaks(Tweaks* tweaks);

void ai_reset(float width, float height);

void ai_debug_draw(void);

void ai_update(void);

void ai_init_agent(uint player_id, uint personality);
void ai_close_agent(uint player_id);

#endif

