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
	uint ship_id;
	uint last_think_t;
	uint last_steer_t;
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

void ai_debug_draw();

void ai_update();

void ai_init_agent(uint player_id);
void ai_close_agent(uint player_id);

#endif

