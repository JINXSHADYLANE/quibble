#include "ai.h"
#include "gfx_utils.h"

#include "physics.h"
#include "game.h"
#include "arena.h"

#define MAX_AGENTS MAX_SHIPS
#define DEBUG_DRAW_LAYER 8 

// Tweakables
float agent_think_interval = 1000.0f / 5.0f;
float agent_steer_interval = 1000.0f / 30.0f;

Agent agents[MAX_AGENTS];

const uint ai_personality_count = 4;
const char* ai_personality_names[MAX_AI_PERSONALITIES] = {
	"multivac",
	"microvac",
	"galactic_ac",
	"universal_ac",
	"cosmic_ac",
	"ac"
};	

AgentPersonality ai_personalities[] = {
	{0.6f,0.20f,0.5f,70.0f,120.0f,45.0f,30.0f,15.0f,150.0f,1.0f,40.0f,0.6f,80.0f,100.0f},
	{0.6f,0.20f,0.5f,70.0f,120.0f,45.0f,30.0f,15.0f,150.0f,1.0f,40.0f,0.6f,80.0f,100.0f},
	{0.6f,0.20f,0.5f,70.0f,120.0f,45.0f,30.0f,15.0f,150.0f,1.0f,40.0f,0.6f,80.0f,100.0f},
	{0.6f,0.20f,0.5f,70.0f,120.0f,45.0f,30.0f,15.0f,150.0f,1.0f,40.0f,0.6f,80.0f,100.0f}
};	

#define AI_TWEAK(name, min, max) \
	tweaks_float(tweaks, #name, name, min, max)

void ai_register_tweaks(Tweaks* tweaks) {
	for(uint i = 0; i < ai_personality_count; ++i) {
		char group_name[128];
		sprintf(group_name, "ai/%s", ai_personality_names[i]);
		tweaks_group(tweaks, group_name);

		float* platforms_frac = &ai_personalities[i].platforms_frac;
		float* steer_tg_angle_tolerance = 
			&ai_personalities[i].steer_tg_angle_tolerance;
		float* steer_tg_aggressive_angle = 
			&ai_personalities[i].steer_tg_aggressive_angle;
		float* steer_tg_aggressive_dist = 
			&ai_personalities[i].steer_tg_aggressive_distance;
		float* steer_tg_max_dist = 
			&ai_personalities[i].steer_tg_max_distance;
		float* steer_tg_min_dist = 
			&ai_personalities[i].steer_tg_min_distance;
		float* steer_tg_coast_dist =
			&ai_personalities[i].steer_tg_coast_distance;
		float* steer_tg_no_steer_dist =
			&ai_personalities[i].steer_tg_no_steer_dist;
		float* no_oversteer_dist =
			&ai_personalities[i].no_oversteer_dist;
		float* oversteer_factor =
			&ai_personalities[i].oversteer_factor;
		float* min_defend_target_dist =
			&ai_personalities[i].min_defend_target_dist;
		float* shoot_angle =
			&ai_personalities[i].shoot_angle;
		float* shoot_dist =
			&ai_personalities[i].shoot_distance;
		float* estimated_speed = 
			&ai_personalities[i].estimated_speed;

		AI_TWEAK(platforms_frac, 0.0f, 1.0f);
		AI_TWEAK(steer_tg_angle_tolerance, 0.0f, PI);
		AI_TWEAK(steer_tg_aggressive_angle, 0.0f, PI);
		AI_TWEAK(steer_tg_aggressive_dist, 0.0f, 300.0f);
		AI_TWEAK(steer_tg_max_dist, 0.0f, 300.0f);
		AI_TWEAK(steer_tg_min_dist, 0.0f, 300.0f);
		AI_TWEAK(steer_tg_coast_dist, 0.0f, 300.0f);
		AI_TWEAK(steer_tg_no_steer_dist, 0.0f, 100.0f);
		AI_TWEAK(no_oversteer_dist, 0.0f, 400.0f);
		AI_TWEAK(oversteer_factor, 0.0f, PI/2.0f);
		AI_TWEAK(min_defend_target_dist, 0.0f, 100.0f);
		AI_TWEAK(shoot_angle, 0.0f, PI);
		AI_TWEAK(shoot_dist, 0.0f, 300.0f);
		AI_TWEAK(estimated_speed, 0.0f, 300.0f);
	}
}

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
				video_draw_line(DEBUG_DRAW_LAYER, &s1.p1, &s1.p2, c);
				video_draw_line(DEBUG_DRAW_LAYER, &s2.p1, &s2.p2, c);
			}
			else {
				video_draw_line(DEBUG_DRAW_LAYER, &s1.p1, &s1.p2, c);
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

	// Draw visibility grid
	ai_debug_vis_draw(nav_mesh, DEBUG_DRAW_LAYER);
}

void _agent_set_state(uint id, AgentState new_state, uint prey_id) {
	assert(id < MAX_AGENTS);

	Vector2 steer_tg_pos;
	Agent* agent = &agents[id];
	ArenaDesc* arena = &current_arena_desc;

	// No change, keep on doing same stuff
	if(new_state == agent->state)
		return;

	agent->state = new_state;
	if(new_state == AI_CAPTURE) {

		// Find closest platform
		float closest_dist = 10000.0f;
		uint closest_id = MAX_UINT32;

		for(uint i = 0; i < arena->n_platforms; ++i) {
			if(platform_states[i].color != MAX_UINT32)
				continue;
			
			float dist = ai_navmesh_distance(&arena->nav_mesh,
				physics_state.ships[agent->ship_id].pos, arena->platforms[i]);

			if(dist < closest_dist) {
				closest_dist = dist;
				closest_id = i;
			}
		}

		// Go to attack if no free platforms
		if(closest_id == MAX_UINT32) {
			new_state = AI_ATTACK;
		}
		else {
			agent->platform = closest_id;
			agent->dest_node = arena_platform_navpoint(closest_id);
			steer_tg_pos = physics_state.ships[agent->ship_id].pos;
			agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
		}
	}

	if(new_state == AI_DEFEND) {

		// Choose platform which will neutralize first
		float quickest_t = 100000.0f;
		uint quickest_id = MAX_UINT32;
		for(uint i = 0; i < arena->n_platforms; ++i) {
			if(platform_states[i].color != agent->ship_id) 
				continue;
			
			float t = game_time_till_neutralization(i);
			if(t < quickest_t) {
				float distance = ai_navmesh_distance(&arena->nav_mesh,
					physics_state.ships[agent->ship_id].pos,
					arena->platforms[i]);
				if(distance / agent->personality->estimated_speed > t) {	
					quickest_t = t;
					quickest_id = i;
				}
			}
		}	

		if(quickest_id == MAX_UINT32)
			quickest_id = game_random_taken_platform(agent->ship_id);
		if(quickest_id == MAX_UINT32)
			new_state = AI_ATTACK;

		agent->platform = quickest_id;
		agent->dest_node = arena_platform_nearby_navpoint(quickest_id);
		steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
	}

	if(new_state == AI_ATTACK) {

		// Choose platform which will neutralize first
		float quickest_t = 100000.0f;
		uint quickest_id = MAX_UINT32;
		for(uint i = 0; i < arena->n_platforms; ++i) {
			if(platform_states[i].color != prey_id) 
				continue;
			
			float t = game_time_till_neutralization(i);
			if(t < quickest_t) {
				quickest_t = t;
				quickest_id = i;
			}
		}	

		if(quickest_id == MAX_UINT32)
			quickest_id = rand_int(0, arena->n_platforms);

		agent->dest_node = arena_platform_navpoint(quickest_id);
		steer_tg_pos = physics_state.ships[agent->ship_id].pos;
		agent->steer_tg_node = arena_closest_navpoint(steer_tg_pos);
	}
}

void _agent_think(uint id) {
	assert(id < MAX_AGENTS);

	Agent* agent = &agents[id];
	ArenaDesc* arena = &current_arena_desc;

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

	if(agent->state == AI_DEFEND) {
		// If we're defending, and we're close to the target node,
		// choose new target node
		Vector2 ship_pos = physics_state.ships[agent->ship_id].pos;
		Vector2 target_pos = arena->nav_mesh.navpoints[agents->steer_tg_node];
		float min_dist = agent->personality->min_defend_target_dist;
		if(vec2_length_sq(vec2_sub(ship_pos, target_pos)) < min_dist * min_dist) {
			agent->dest_node = arena_platform_nearby_navpoint(agent->platform);
			agent->steer_tg_node = arena_closest_navpoint(ship_pos);
		}
	}

	if(agent->state == AI_CAPTURE) {
		// See if we captured target platform
		if(platform_states[agent->platform].color == agent->ship_id)
			_agent_set_state(id, AI_DEFEND, 0);
	}
}

void _agent_steer(uint id) {
	assert(id < MAX_AGENTS);

	float t = time_ms() / 1000.0f;

	Agent* agent = &agents[id];
	ArenaDesc* arena = &current_arena_desc;
		
	// Update steer target
	ShipPhysicalState* phys_state = &physics_state.ships[agent->ship_id];
	Vector2 steer_tg_pos = arena->nav_mesh.navpoints[agent->steer_tg_node];

	float distance = ai_distance_sq(steer_tg_pos, phys_state->pos);

	float tg_advance_sq = agent->personality->steer_tg_min_distance;
	tg_advance_sq *= tg_advance_sq;

	// Steer tg visibility
	bool visible = ai_vis_query(&arena->nav_mesh, phys_state->pos, steer_tg_pos);

	if(visible) {
		agent->last_tg_seen_t = t;

		uint old_steer_tg_node = agent->steer_tg_node;
		agent->steer_tg_node = ai_find_next_path_node(&arena->nav_mesh, 
			agent->steer_tg_node, agent->dest_node);

		bool new_visible = ai_vis_query(&arena->nav_mesh, phys_state->pos, steer_tg_pos); 
		if(!new_visible)
			agent->steer_tg_node = old_steer_tg_node;
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
			// Wait till angle is better
		}
		else {
			// Angle is OK, accelerate 
			can_accelerate = true;
		}
	}	

	if((!visible && distance > agent->personality->steer_tg_max_distance) ||
		t - agent->last_tg_seen_t > 1.0f) {
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

	// If we're standing still - something is not right;
	// accelerate just for the fun of it!
	if(vec2_length_sq(phys_state->vel) < 1.5f)
		agent->accelerate = 3;

	// Look for ships to shoot
	agent->shoot = false;
	for(uint i = 0; i < n_ships; ++i) {
		if(i == agent->ship_id)
			continue;

		if(physics_state.ships[i].exploded)
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
	ArenaDesc* arena = &current_arena_desc;
	Vector2 unit_vector = {0.0f, -1.0f};

	for(uint i = 0; i < MAX_AGENTS; ++i) {
		if(agents[i].ship_id == ~0)
			continue;

		if(ship_states[agents[i].ship_id].is_exploding)
			continue;
		
		if(t - agents[i].last_think_t > agent_think_interval) {
			_agent_think(i);
			agents[i].last_think_t = t;
		}	

		if(t - agents[i].last_steer_t > agent_think_interval) {
			_agent_steer(i);
			agents[i].last_steer_t = t;
		}	

		ShipPhysicalState* phys_state = &physics_state.ships[agents[i].ship_id];
		
		Vector2 steer_tg_pos = arena->nav_mesh.navpoints[agents[i].steer_tg_node];
		Vector2 ship_dir = vec2(sinf(phys_state->rot), -cosf(phys_state->rot));
		Segment path = ai_shortest_path(phys_state->pos, steer_tg_pos);
		Vector2 ship_to_steer_tg = vec2_sub(path.p2, path.p1);

		float angle, distance = vec2_length(ship_to_steer_tg);
		float ship_angle = angle = vec2_angle(unit_vector, ship_dir);
		if(distance > agents[i].personality->steer_tg_no_steer_dist) { 
			angle = vec2_angle(unit_vector, ship_to_steer_tg);

			// Oversteer a bit, more closer to the target
			float dangle = angle - ship_angle;
			float ovrsteer_dir = dangle / fabsf(dangle); 
			float min_ovrsteer_dist = agents[i].personality->steer_tg_no_steer_dist;
			float max_ovrsteer_dist = agents[i].personality->no_oversteer_dist;
			float ovrsteer_t = 1.0f - clamp(0.0, 1.0f, (distance - min_ovrsteer_dist) 
				/ (max_ovrsteer_dist - min_ovrsteer_dist)); 

			angle += ovrsteer_dir * ovrsteer_t * agents[i].personality->oversteer_factor;
		}	
		else
			angle = vec2_angle(unit_vector, ship_dir);

		physics_control_ship_ex(agents[i].ship_id,
			angle,
			agents[i].accelerate-- > 0
		);

		agents[i].accelerate = MAX(agents[i].accelerate, 0);

		if(agents[i].shoot)
			game_shoot(agents[i].ship_id);
	}
}

void ai_init_agent(uint player_id, uint personality) {
	uint agent_id = 0;
	while(agent_id < MAX_AGENTS && agents[agent_id].ship_id != ~0) {
		agent_id++;
	}

	if(agent_id == MAX_AGENTS)
		LOG_ERROR("Unable to find free ai agent slot");

	Agent* agent = &agents[agent_id];

	agent->ship_id = player_id;
	agent->personality = &ai_personalities[personality];
	agent->last_think_t = time_ms();
	agent->last_steer_t = time_ms();

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

