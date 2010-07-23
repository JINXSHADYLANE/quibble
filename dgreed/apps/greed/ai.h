#ifndef AI_H
#define AI_H

#include "ai_precalc.h"
#include "arena.h"

typedef enum {
	AI_DEFEND,
	AI_ATTACK,
	AI_CAPTURE
} AgentState;	

typedef struct {
	// Always try to control at least this fraction of platforms in arena
	float platforms_frac; 
} AgentPersonality;	

typedef struct {
	uint ship_id;
	AgentPersonality* personality;

	float last_think_t;
	float last_steer_t;
	AgentState state;
	uint dest_node;

	// Steer target
	Vector2 steer_tg_pos;
	uint steer_tg_next_node; 

	// Shoot target
	Vector2 shoot_tg_pos;
	bool shoot;
} Agent;	
	
void ai_reset(float width, float height);

void ai_debug_draw(void);

void ai_update(void);

void ai_init_agent(uint player_id);
void ai_close_agent(uint player_id);

#endif

