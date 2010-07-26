#include "ai.h"
#include "gfx_utils.h"

#include "physics.h"
#include "game.h"
#include "arena.h"

#define MAX_AGENTS MAX_SHIPS
#define DEBUG_DRAW_LAYER 12

// Tweakables
float agent_think_interval = 1000.0f / 5.0f;
float agent_steer_interval = 1000.0f / 30.0f;

Agent agents[MAX_AGENTS];

AgentPersonality default_personality = 
	{0.6f, 0.20f, 0.5f, 70.0f, 120.0f, 45.0f, 30.0f, 15.0f, 0.3f, 0.6f, 80.0f};

void ai_reset(float width, float height) {
	ai_precalc_bounds(width, height);

	for(uint i = 0; i < MAX_AGENTS; ++i)
		agents[i].ship_id = ~0;
}

void ai_debug_draw() {
	NavMesh* nav_mesh = &current_arena_desc.nav_mesh;
	uint n = nav_mesh->n_nodes;

	Color c = COLOR_RGBA(196, 196, 212, 128);

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
					&s1.p1, &s1.p2, c);
				video_draw_line(DEBUG_DRAW_LAYER + i%2, 
					&s2.p1, &s2.p2, c);
			}
			else {
				video_draw_line(DEBUG_DRAW_LAYER + i%2,
					&s1.p1, &s1.p2, c);
			}		
		}
	}

	// Draw steer targets & destinations
	for(uint i = 0; i < MAX_AGENTS; ++i) {
		if(agents[i].ship_id != ~0) {
			Color c = ship_colors[agents[i].ship_id];
			
			Vector2 steer_tg_pos = nav_mesh->navpoints[agents[i].steer_tg_node];
			gfx_draw_point(DEBUG_DRAW_LAYER, &steer_tg_pos, c);

			Vector2 dest_pos = nav_mesh->navpoints[agents[i].dest_node];
			gfx_draw_point(DEBUG_DRAW_LAYER, &dest_pos, c);
		}
	}

}

void _agent_set_state(uint id, AgentState new_state, uint prey_id) {
	assert(id < MAX_AGENTS);

	Vector2 steer_tg_pos;
	Agent* agent = &agents[id];
	//ArenaDesc* arena = &current_arena_desc;

	// No change, keep on doing same stuff
	if(new_state == agent->state)
		return;

	agent->state = new_state;
	if(new_state == AI_CAPTURE) {
		// TODO: Determine which platform is close / easy to capture

		uint target_platform = game_random_free_platform();
		// Go to attack if no free platforms
		if(target_platform == MAX_UINT32) {
			new_state = AI_ATTACK;
		}
		else {
			agent->dest_node = arena_platform_navpoint(target_platform);
			steer_tg_pos = physics_state.ships[agent->ship_id].pos;
			agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
		}
	}

	if(new_state == AI_DEFEND) {
		// TODO: Determine which platform is most insecure

		uint target_platform = game_random_taken_platform(agent->ship_id);
		agent->dest_node = arena_platform_navpoint(target_platform);
		steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
	}

	if(new_state == AI_ATTACK) {
		uint target_platform = game_random_taken_platform(prey_id);
		agent->dest_node = arena_platform_navpoint(target_platform);
		steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
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
	ShipPhysicalState* phys_state = &physics_state.ships[agent->ship_id];
	Vector2 steer_tg_pos = arena->nav_mesh.navpoints[agent->steer_tg_node];

	float distance = ai_distance_sq(steer_tg_pos, phys_state->pos);

	float tg_advance_sq = agent->personality->steer_tg_min_distance;
	tg_advance_sq *= tg_advance_sq;

	if(distance < tg_advance_sq) {
		agent->steer_tg_node = ai_find_next_path_node(&arena->nav_mesh, 
			agent->steer_tg_node, agent->dest_node);
	}

	// Steer
	bool can_accelerate = false;

	steer_tg_pos = arena->nav_mesh.navpoints[agent->steer_tg_node];
	Vector2 ship_dir = vec2(sinf(phys_state->rot), -cosf(phys_state->rot));
	Segment path = ai_shortest_path(phys_state->pos, steer_tg_pos);
	Vector2 ship_to_steer_tg = vec2_sub(path.p2, path.p1);

	// Distance between ship and steer target
	distance = vec2_length(ship_to_steer_tg);

	if(distance > agent->personality->steer_tg_no_steer_dist) {
		// Angle between ship direction ant steer target
		float angle = vec2_angle(ship_dir, ship_to_steer_tg); 
			
		if(fabs(angle) > agent->personality->steer_tg_angle_tolerance) {
			bool aggressive = 
				fabs(angle) > agent->personality->steer_tg_aggressive_angle;

			// Steer towards target
			if(angle > 0.0f)
				agent->steer_right = aggressive ? 3 : 1;
			else
				agent->steer_left = aggressive ? 3 : 1;
		}
		else {
			// Angle is OK, now just stabilize
			can_accelerate = true;
			if(fabs(phys_state->ang_vel) > 
				agent->personality->acceptable_angular_velocity) {
				if(phys_state->ang_vel > 0.0f)
					agent->steer_left = 1;
				else
					agent->steer_right = 1;
			}
		}
	}	

	if(distance > agent->personality->steer_tg_max_distance) {
		// "Recalc" path, steer tg is too far
		steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
	}
	else {
		if(distance > agent->personality->steer_tg_coast_distance &&
			can_accelerate) {

			bool aggressive = 
				distance > agent->personality->steer_tg_aggressive_distance;
			agent->accelerate = aggressive ? 3 : 1;
		}	
	}

	// Look for ships to shoot
	agent->shoot = false;
	for(uint i = 0; i < n_ships; ++i) {
		if(i == agent->ship_id)
			continue;

		Vector2 pos = physics_state.ships[i].pos;	
		Vector2 ship_to_ship = vec2_sub(pos, phys_state->pos);
		float distance_sq = vec2_length_sq(ship_to_ship);
		float req_distance_sq = agent->personality->shoot_distance;
		req_distance_sq *= req_distance_sq;
		if(distance_sq < req_distance_sq) {
			float angle = vec2_angle(ship_dir, ship_to_ship);
			if(fabs(angle) < agent->personality->shoot_angle)
				agent->shoot = true;
		}
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

		physics_control_ship(agents[i].ship_id,
			agents[i].steer_left-- > 0,
			agents[i].steer_right-- > 0,
			agents[i].accelerate-- > 0);

		agents[i].steer_left = MAX(agents[i].steer_left, 0);
		agents[i].steer_right = MAX(agents[i].steer_right, 0);
		agents[i].accelerate = MAX(agents[i].accelerate, 0);

		if(agents[i].shoot)
			game_shoot(agents[i].ship_id);
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

	agent->steer_left = 0;
	agent->steer_right = 0;
	agent->accelerate = 0;

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

