#include "ai.h"
#include "gfx_utils.h"

#include "physics.h"
#include "game.h"
#include "arena.h"

#define MAX_AGENTS MAX_SHIPS
#define DEBUG_DRAW_LAYER 12

// Tweakables
float steer_tg_advance_dist = 4.0f;
float steer_tg_speed = 200.0f;
float agent_think_interval = 1000.0f / 5.0f;
float agent_steer_interval = 1000.0f / 30.0f;

Agent agents[MAX_AGENTS];

AgentPersonality default_personality = {0.6f};

void ai_reset(float width, float height) {
	ai_precalc_bounds(width, height);

	for(uint i = 0; i < MAX_AGENTS; ++i)
		agents[i].ship_id = ~0;
}

void ai_debug_draw() {
	NavMesh* nav_mesh = &current_arena_desc.nav_mesh;
	uint n = nav_mesh->n_nodes;

	// Draw grid
	for(uint i = 0; i < n; ++i) {
		Vector2 v1 = nav_mesh->navpoints[i];
		uint start = nav_mesh->neighbours_start[i];
		uint count = nav_mesh->neighbour_count[i];
		for(uint j = start; j < start + count; ++j) {
			uint i2 = nav_mesh->neighbours[j];
			if(i2 < i)
				continue;
			Vector2 v2 = nav_mesh->navpoints[i2];
			Segment s = ai_shortest_path(v1, v2);
			Segment s1, s2;
			if(ai_split_path(s, &s1, &s2)) {
				video_draw_line(DEBUG_DRAW_LAYER + i%2, 
					&s1.p1, &s1.p2, COLOR_WHITE);
				video_draw_line(DEBUG_DRAW_LAYER + i%2, 
					&s2.p1, &s2.p2, COLOR_WHITE);
			}
			else {
				video_draw_line(DEBUG_DRAW_LAYER + i%2,
					&s1.p1, &s1.p2, COLOR_WHITE);
			}		
		}
	}

	// Draw steer targets
	for(uint i = 0; i < MAX_AGENTS; ++i) {
		if(agents[i].ship_id != ~0) {
			Color c = ship_colors[agents[i].ship_id];
			gfx_draw_point(DEBUG_DRAW_LAYER, &(agents[i].steer_tg_pos), c);
		}
	}

}

void _agent_set_state(uint id, AgentState new_state, uint prey_id) {
	assert(id < MAX_AGENTS);

	Agent* agent = &agents[id];
	//ArenaDesc* arena = &current_arena_desc;

	// No change, keep on doing same stuff
	if(new_state == agent->state)
		return;

	agent->state = new_state;
	if(new_state == AI_CAPTURE) {
		// TODO: Determine which platform is close / easy to capture

		uint target_platform = game_random_free_platform();
		agent->dest_node = arena_platform_navpoint(target_platform);
		agent->steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_next_node = arena_closest_navpoint(agent->steer_tg_pos);
	}

	if(new_state == AI_DEFEND) {
		// TODO: Determine which platform is most insecure

		uint target_platform = game_random_taken_platform(agent->ship_id);
		agent->dest_node = arena_platform_navpoint(target_platform);
		agent->steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_next_node = arena_closest_navpoint(agent->steer_tg_pos);
	}

	if(new_state == AI_ATTACK) {
		uint target_platform = game_random_taken_platform(prey_id);
		agent->dest_node = arena_platform_navpoint(target_platform);
		agent->steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_next_node = arena_closest_navpoint(agent->steer_tg_pos);
	}
}

void _agent_think(uint id) {
	assert(id < MAX_AGENTS);

	Agent* agent = &agents[id];

	// Determine agent state
	float taken_platforms[MAX_SHIPS];
	float max_taken_platforms = -1.0f;
	float max_taken_ship = ~0;
	for(uint i = 0; i < n_ships; ++i) {
		taken_platforms[i] = game_taken_platforms_frac(i);
		if(max_taken_platforms < taken_platforms[i]) {
			max_taken_platforms = taken_platforms[i];
			max_taken_ship = i;
		}	
	}
	float my_platforms = taken_platforms[agent->ship_id];
	if(my_platforms == max_taken_platforms) {
		if(my_platforms < agent->personality->platforms_frac) 
			_agent_set_state(id, AI_CAPTURE, 0);
		else 
			_agent_set_state(id, AI_DEFEND, 0);
	}
	else {
		assert(max_taken_ship < n_ships);
		_agent_set_state(id, AI_ATTACK, max_taken_ship);
	}
}

void _agent_steer(uint id) {
	assert(id < MAX_AGENTS);

	Agent* agent = &agents[id];
	ArenaDesc* arena = &current_arena_desc;
		
	// Update steer target
	Vector2 next_pos = arena->nav_mesh.navpoints[agent->steer_tg_next_node];
	float distance = ai_distance_sq(agent->steer_tg_pos, next_pos);
	if(distance < steer_tg_advance_dist * steer_tg_advance_dist) {
		agent->steer_tg_next_node = ai_find_next_path_node(&arena->nav_mesh, 
			agent->steer_tg_next_node, agent->dest_node);
	}
	else {
		Segment path = ai_shortest_path(agent->steer_tg_pos, next_pos);
		Segment p1, p2;
		ai_split_path(path, &p1, &p2);
		Vector2 dir = vec2_normalize(vec2_sub(p1.p2, p1.p1));
		agent->steer_tg_pos = vec2_add(agent->steer_tg_pos, 
			vec2_scale(dir, steer_tg_speed * agent_steer_interval/1000.0f));

		agent->steer_tg_pos = physics_wraparound(agent->steer_tg_pos);	
	}
}

void ai_update(void) {
	float t = time_ms();

	for(uint i = 0; i < MAX_AGENTS; ++i) {
		if(agents[i].ship_id == ~0)
			continue;
		
		if(t - agents[i].last_think_t > agent_think_interval) {
			_agent_think(i);
			agents[i].last_think_t = t;
		}	

		if(t - agents[i].last_steer_t > agent_think_interval) {
			_agent_steer(i);
			agents[i].last_steer_t = t;
		}	
	}
}

void ai_init_agent(uint player_id) {
	uint agent_id = 0;
	while(agent_id < MAX_AGENTS && agents[agent_id].ship_id != ~0) {
		agent_id++;
	}

	if(agent_id == MAX_AGENTS)
		LOG_ERROR("Unable to find free ai agent slot");

	Agent* agent = &agents[agent_id];

	agent->ship_id = player_id;
	agent->personality = &default_personality;
	agent->last_think_t = time_ms();
	agent->last_steer_t = time_ms();

	_agent_think(agent_id);
}

void ai_close_agent(uint player_id) {
	uint agent_id = 0;
	while(agent_id < MAX_AGENTS && agents[agent_id].ship_id != player_id) {
		agent_id++;
	}	

	if(agent_id == MAX_AGENTS)
		LOG_ERROR("Provided player_id is not controlled by ai agent");

	agents[agent_id].ship_id = ~0;	
}

