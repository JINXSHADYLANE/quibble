#include "ai_precalc.h"
#include "memory.h"
#include "game.h"

// Tweakables
float min_wall_distance = 17.0f;
float min_point_distance = 35.0f;
float max_edge_distance = 65.0f;

const uint nn_grid_width = 15;
const uint nn_grid_height = 10;
const uint nn_grid_cells = 15 * 10; // No constant propagation in C99..
const uint nn_samples = 10;

Vector2 wraparound_offsets[5] =
	{{10000.0f, 0.0f}, {0.0f, -10000.0f},
	 {-10000.0f, 0.0f}, {0.0f, 10000.0f}, {0.0f, 0.0f}};

float nn_cell_width = -1.0f;
float nn_cell_height = -1.0f;

Segment bounds[4];	 

uint ai_nn_grid_cell(Vector2 p) {
	assert(nn_cell_width > 0.0f && nn_cell_height > 0.0f);
	assert(p.x >= 0.0f && p.x < nn_grid_width * nn_cell_width);
	assert(p.y >= 0.0f && p.y < nn_grid_height * nn_cell_height);

	uint x = (uint)floorf(p.x / nn_cell_width);
	uint y = (uint)floorf(p.y / nn_cell_height);

	return y * nn_grid_width + x; 
}

Vector2 _rand_point_in_nn_grid_cell(uint cell) {
	assert(nn_cell_width > 0.0f && nn_cell_height > 0.0f);
	assert(cell < nn_grid_cells);

	uint x = cell % nn_grid_width;
	uint y = cell / nn_grid_width;
	
	float x_min = (float)x * nn_cell_width;
	float y_min = (float)y * nn_cell_height;
	
	return vec2(
		rand_float_range(x_min, x_min + nn_cell_width),
		rand_float_range(y_min, y_min + nn_cell_height));
}

float ai_wall_distance(Vector2 p, DArray geometry) {
	Segment* segments = DARRAY_DATA_PTR(geometry, Segment);
	float distance = 10000.0f;
	for(uint i = 0; i < geometry.size; ++i) {
		Segment seg = {segments[i].p2, segments[i].p1};

		for(uint j = 0; j < 4; ++j) {
			float wraparound_d;
			Segment wraparound_seg = segment(
				vec2_add(seg.p1, wraparound_offsets[j]),
				vec2_add(seg.p2, wraparound_offsets[j]));
			wraparound_d = segment_point_dist(wraparound_seg, p);
			if(wraparound_d > 0.0f && wraparound_d < fabsf(distance))
				distance = wraparound_d;
		}		

		float d = segment_point_dist(seg, p);
		if(fabsf(d) < fabsf(distance))
			distance = d;
		else 
			if(distance < 0.0f && d > 0.0f && feql((fabsf(distance)), d))
				distance = d;
	}
	return distance > 0.0f ? distance : 0.0f;
}

float _node_distance_sq(Vector2 p1, Vector2 p2) {
	float result = 10000000.0f;
	float d_sqr;

	for(uint i = 0; i < 5; ++i) {
		Vector2 wrap_p = vec2_add(p2, wraparound_offsets[i]);
		d_sqr = vec2_length_sq(vec2_sub(p1, wrap_p));
		result = MIN(result, d_sqr);
	}
	return result;
}

float ai_distance_sq(Vector2 p1, Vector2 p2) {
	return _node_distance_sq(p1, p2);
}

uint _nearest_navpoint(Vector2 p, DArray geometry, DArray navpoints) {
	float min_sq_d = 100000.0f;
	uint min_id = ~0;

	Vector2* vec2_navpoints = DARRAY_DATA_PTR(navpoints, Vector2);
	for(uint i = 0; i < navpoints.size; ++i) {
		float sq_d = _node_distance_sq(p, vec2_navpoints[i]);
		if(sq_d < min_sq_d) {
			min_sq_d = sq_d;
			min_id = i;
		}	
	}

	assert(min_id != ~0);

	return min_id;
}

Segment ai_shortest_path(Vector2 p1, Vector2 p2) {
	uint id = 5;
	float min_d = 10000000.0f;
	float d_sqr;

	for(uint i = 0; i < 5; ++i) {
		Vector2 wrap_p = vec2_add(p2, wraparound_offsets[i]);
		d_sqr = vec2_length_sq(vec2_sub(p1, wrap_p));
		if(d_sqr < min_d) {
			min_d = d_sqr;
			id = i;
		}	
	}

	return segment(p1, vec2_add(p2, wraparound_offsets[id])); 
}

bool ai_split_path(Segment path, Segment* out_p1, Segment* out_p2) {
	assert(out_p1);
	assert(out_p2);

	for(uint i = 0; i < 4; ++i) {
		Vector2 middle;
		if(segment_intersect(path, bounds[i], &middle)) {
			*out_p1 = segment(path.p1, middle); 
			Vector2 off = wraparound_offsets[i];
			*out_p2 = segment(vec2_add(middle, off), vec2_add(path.p2, off));
			return true;
		}	
	}
	
	*out_p1 = path;
	return false;
}

float _point_distance(Vector2 p, DArray points) {
	Vector2* points_vec2 = DARRAY_DATA_PTR(points, Vector2);
	float distance_sqr = 10000000.0f;
	for(uint i = 0; i < points.size; ++i) {
		float d = _node_distance_sq(p, points_vec2[i]);
		distance_sqr = MIN(distance_sqr, d);
	}	

	return sqrtf(distance_sqr);
}

DArray _gen_navpoints(DArray geometry, DArray platforms) {
	DArray points = darray_create(sizeof(Vector2), 0);
	darray_append_multi(&points, platforms.data, platforms.size);

	uint fail_count = 0;
	do {
		Vector2 navpoint = vec2(rand_float_range(0.0f, SCREEN_WIDTH),
			rand_float_range(0.0f, SCREEN_HEIGHT));

		float wdist = ai_wall_distance(navpoint, geometry);
		float pdist = _point_distance(navpoint, points);

		if(wdist >= min_wall_distance && pdist >= min_point_distance) {
			darray_append(&points, (void*)&navpoint);
			fail_count = 0;
		}
		else {
			fail_count++;
		}	
	} while(fail_count < 1200);	
	return points;
}	

DArray _gen_edges(DArray points, DArray geometry) {
	DArray edges = darray_create(sizeof(Edge), 0);
	Vector2* points_vec2 = DARRAY_DATA_PTR(points, Vector2);
	for(uint i = 0; i < points.size; ++i) {
		for(uint j = 0; j < points.size; ++j) {
			if(i >= j)
				continue;

			float sqr_dist = _node_distance_sq(points_vec2[i], points_vec2[j]);

			if(sqr_dist <= max_edge_distance*max_edge_distance) {
				Edge n = {i, j};

				Segment s = ai_shortest_path(points_vec2[i], points_vec2[j]);
				Segment s1, s2;
				bool split = ai_split_path(s, &s1, &s2);

				// Check for wall intersection
				Segment* geometry_s = DARRAY_DATA_PTR(geometry, Segment);
				bool intersects_arena = false;
				for(uint k = 0; k < geometry.size; ++k) {
					if(segment_intersect(s1, geometry_s[k], NULL)) {
						intersects_arena = true;
						break;
					}
					if(split && segment_intersect(s2, geometry_s[k], NULL)) {
						intersects_arena = true;
						break;
					}	
				}

				// Check distance to walls at midpoint
				Vector2 midpoint = vec2_scale(vec2_add(s1.p1, s1.p2), 0.5f);
				if(ai_wall_distance(midpoint, geometry) < min_wall_distance)
					intersects_arena = true;

				if(!intersects_arena)
					darray_append(&edges, (void*)&n);
			}	
		}
	}
	return edges;
}	

void ai_precalc_bounds(float width, float height) {
	wraparound_offsets[0] = vec2(width, 0.0f);
	wraparound_offsets[1] = vec2(0.0f, -height);
	wraparound_offsets[2] = vec2(-width, 0.0f);
	wraparound_offsets[3] = vec2(0.0f, height);

	bounds[0] = segment(vec2(0.0f, 0.0f), vec2(0.0f, height));
	bounds[1] = segment(vec2(0.0f, height), vec2(width, height));
	bounds[2] = segment(vec2(width, height), vec2(width, 0.0f));
	bounds[3] = segment(vec2(width, 0.0f), vec2(0.0f, 0.0f));

	nn_cell_width = width / (float)nn_grid_width;
	nn_cell_height = height / (float)nn_grid_height;
}	

NavMesh ai_precalc_navmesh(DArray geometry, DArray platforms,
	float width, float height) {
	NavMesh res;

	ai_precalc_bounds(width, height);

	DArray navpoints = _gen_navpoints(geometry, platforms);
	DArray edges = _gen_edges(navpoints, geometry);

	Vector2* navpoints_v = DARRAY_DATA_PTR(navpoints, Vector2);
	Edge* edges_e = DARRAY_DATA_PTR(edges, Edge);

	uint n = navpoints.size;

	res.n_nodes = n;
	res.navpoints = MEM_ALLOC(sizeof(Vector2) * n);
	res.neighbour_count = MEM_ALLOC(sizeof(uint) * n);
	res.neighbours_start = MEM_ALLOC(sizeof(uint) * n);
	float* adjacency = MEM_ALLOC(sizeof(float) * n*n);
	res.distance = MEM_ALLOC(sizeof(float) * n*n);
	res.radius = MEM_ALLOC(sizeof(float) * n);
	res.nn_grid_count = MEM_ALLOC(sizeof(uint) * nn_grid_cells);
	res.nn_grid_start = MEM_ALLOC(sizeof(uint) * nn_grid_cells);

	memset(res.neighbour_count, 0, sizeof(uint) * n);
	memset(adjacency, 0, sizeof(float) * n*n);
	memset(res.distance, 0, sizeof(float) * n*n);
	memset(res.nn_grid_count, 0, sizeof(uint) * nn_grid_cells);
	memset(res.nn_grid_start, 0, sizeof(uint) * nn_grid_cells);
		
	for(uint i = 0; i < navpoints.size; ++i) {
		res.navpoints[i] = 	navpoints_v[i];
		res.radius[i] = ai_wall_distance(navpoints_v[i], geometry);
	}

	for(uint i = 0; i < edges.size; ++i) {
		float dist = vec2_length(vec2_sub(
			navpoints_v[edges_e[i].v1], navpoints_v[edges_e[i].v2]));
		adjacency[IDX_2D(edges_e[i].v1, edges_e[i].v2, n)] = dist;
		adjacency[IDX_2D(edges_e[i].v2, edges_e[i].v1, n)] = dist;
		res.neighbour_count[edges_e[i].v1]++;
		res.neighbour_count[edges_e[i].v2]++;
	}

	// Pracalc node neighbour lists
	uint total_neighbours = res.neighbour_count[0];
	res.neighbours_start[0] = 0;
	for(uint i = 1; i < n; ++i) {
		res.neighbours_start[i] = 
			res.neighbours_start[i-1] + res.neighbour_count[i-1];
		total_neighbours +=	res.neighbour_count[i];
	}		
	res.neighbours = MEM_ALLOC(sizeof(uint) * total_neighbours);
	for(uint i = 0; i < n; ++i) {
		uint idx = res.neighbours_start[i];
		for(uint j = 0; j < n; ++j) {
			if(adjacency[IDX_2D(i, j, n)] > 0.0f)
				res.neighbours[idx++] = j;
		}
		assert(idx == res.neighbours_start[i] + res.neighbour_count[i]);
	}

	memcpy(res.distance, adjacency, sizeof(float) * n*n);
	// Change zero distances to infinities
	for(uint i = 0; i < n*n; ++i)
		if(res.distance[i] == 0.0f)
			res.distance[i] = 10000000.0f;

	// Floyd-Warshall
	for(uint i = 0; i < n; ++i)
		for(uint j = 0; j < n; ++j)
			for(uint k = 0; k < n; ++k)
				res.distance[IDX_2D(j, k, n)] = MIN(res.distance[IDX_2D(j, k, n)],
					res.distance[IDX_2D(j, i, n)]+res.distance[IDX_2D(i, k, n)]);

	// Precalc nearest-navpoint grid
	DArray grid_cell;
	DArray nn_grid;

	res.nn_grid_start[0] = 0;

	nn_grid = darray_create(sizeof(uint), 0);
	for(uint i = 0; i < nn_grid_cells; ++i) {
		grid_cell = darray_create(sizeof(uint), 0);
		
		for(uint j = 0; j < nn_samples; ++j) {
			Vector2 p = _rand_point_in_nn_grid_cell(i);
			uint nearest_navpoint = _nearest_navpoint(p, geometry, navpoints);
			if(nearest_navpoint >= n) // Point was inside wall, skip
				continue;

			bool unique = true;

			uint* existing_navpoints = DARRAY_DATA_PTR(grid_cell, uint);
			for(uint k = 0; k < grid_cell.size; ++k)
				if(existing_navpoints[k] == nearest_navpoint)
					unique = false;
			
			if(unique)
				darray_append(&grid_cell, &nearest_navpoint);
		}

		res.nn_grid_count[i] = grid_cell.size;
		if(i > 0)
			res.nn_grid_start[i] = 
				res.nn_grid_start[i-1] + res.nn_grid_count[i-1];
		
		uint* cell_navpoints = DARRAY_DATA_PTR(grid_cell, uint);
		darray_append_multi(&nn_grid, cell_navpoints, grid_cell.size);
		darray_free(&grid_cell);
	}	

	// Hack, works properly as long as darray_free only does
	// MEM_FREE(darray.data);
	res.nn_grid = (uint*)nn_grid.data;

	darray_free(&navpoints);
	darray_free(&edges);
	MEM_FREE(adjacency);

	return res;
}

void ai_free_navmesh(NavMesh mesh) {
	assert(mesh.n_nodes);
	assert(mesh.navpoints);
	assert(mesh.neighbour_count);
	assert(mesh.neighbours_start);
	assert(mesh.neighbours);
	assert(mesh.distance);
	assert(mesh.radius);
	assert(mesh.nn_grid_count);
	assert(mesh.nn_grid_start);
	assert(mesh.nn_grid);

	MEM_FREE(mesh.navpoints);
	MEM_FREE(mesh.neighbour_count);
	MEM_FREE(mesh.neighbours_start);
	MEM_FREE(mesh.neighbours);
	MEM_FREE(mesh.distance);
	MEM_FREE(mesh.radius);
	MEM_FREE(mesh.nn_grid_count);
	MEM_FREE(mesh.nn_grid_start);
	MEM_FREE(mesh.nn_grid);
}

void ai_save_navmesh(FileHandle file, const NavMesh* navmesh) {
	assert(file);
	assert(navmesh);
	assert(navmesh->n_nodes > 3);

	file_write_uint32(file, navmesh->n_nodes);

	size_t n = navmesh->n_nodes;

	for(uint i = 0; i < n; ++i) {
		file_write_float(file, navmesh->navpoints[i].x);
		file_write_float(file, navmesh->navpoints[i].y);
	}

	uint n_neighbours = 0;
	for(uint i = 0; i < n; ++i) {
		file_write_uint32(file, navmesh->neighbour_count[i]);
		n_neighbours += navmesh->neighbour_count[i];
	}	

	for(uint i = 0; i < n_neighbours; ++i)
		file_write_uint32(file, navmesh->neighbours[i]);

	for(uint i = 0; i < n*n; ++i)
		file_write_float(file, navmesh->distance[i]);

	for(uint i = 0; i < n; ++i)
		file_write_float(file, navmesh->radius[i]);

	uint n_nn_grid_nodes = 0;
	for(uint i = 0; i < nn_grid_cells; ++i) {
		file_write_uint32(file, navmesh->nn_grid_count[i]);
		n_nn_grid_nodes += navmesh->nn_grid_count[i];
	}

	for(uint i = 0; i < n_nn_grid_nodes; ++i)
		file_write_uint32(file, navmesh->nn_grid[i]);
}

NavMesh ai_load_navmesh(FileHandle file) {
	assert(file);

	NavMesh r;

	r.n_nodes = file_read_uint32(file);
	assert(r.n_nodes > 3 && r.n_nodes < 1024);

	size_t n = r.n_nodes;

	r.navpoints = (Vector2*)MEM_ALLOC(sizeof(Vector2)*n);
	r.neighbour_count = (uint*)MEM_ALLOC(sizeof(uint)*n);
	r.neighbours_start = (uint*)MEM_ALLOC(sizeof(uint)*n);
	r.distance = (float*)MEM_ALLOC(sizeof(float)*n*n);
	r.radius = (float*)MEM_ALLOC(sizeof(float)*n);
	r.nn_grid_start = (uint*)MEM_ALLOC(sizeof(uint)*nn_grid_cells);
	r.nn_grid_count = (uint*)MEM_ALLOC(sizeof(uint)*nn_grid_cells);

	for(uint i = 0; i < n; ++i) {
		r.navpoints[i].x = file_read_float(file);
		r.navpoints[i].y = file_read_float(file);
	}

	uint n_neighbours = 0;
	for(uint i = 0; i < n; ++i) {
		r.neighbour_count[i] = file_read_uint32(file);
		n_neighbours += r.neighbour_count[i];
	}

	r.neighbours_start[0] = 0;
	for(uint i = 1; i < n; ++i)
		r.neighbours_start[i] = 
			r.neighbours_start[i-1] + r.neighbour_count[i-1];

	r.neighbours = (uint*)MEM_ALLOC(sizeof(uint)*n_neighbours);
	for(uint i = 0; i < n_neighbours; ++i)
		r.neighbours[i] = file_read_uint32(file);

	for(uint i = 0; i < n*n; ++i)
		r.distance[i] = file_read_float(file);

	for(uint i = 0; i < n; ++i)
		r.radius[i] = file_read_float(file);

	uint n_nn_grid_nodes = 0;
	for(uint i = 0; i <	nn_grid_cells; ++i) {
		r.nn_grid_count[i] = file_read_uint32(file);
		n_nn_grid_nodes += r.nn_grid_count[i];
	}

	r.nn_grid = (uint*)MEM_ALLOC(sizeof(uint)*n_nn_grid_nodes);
	r.nn_grid_start[0] = 0;
	for(uint i = 1; i < nn_grid_cells; ++i)
		r.nn_grid_start[i] = r.nn_grid_start[i-1] + r.nn_grid_count[i-1];

	for(uint i = 0; i < n_nn_grid_nodes; ++i)
		r.nn_grid[i] = file_read_uint32(file);

	return r;	
}

uint ai_nearest_navpoint(NavMesh* navmesh, Vector2 pos) {
	assert(navmesh);

	uint cell = ai_nn_grid_cell(pos);

	float min_sqr_d = 10000000.0f;
	uint min_id = ~0;
	for(uint i = 0; i < navmesh->nn_grid_count[cell]; ++i) {
		uint idx = i + navmesh->nn_grid_start[cell];
		uint navpoint = navmesh->nn_grid[idx];
		Vector2 navpoint_pos = navmesh->navpoints[navpoint];
		float sqr_d = _node_distance_sq(pos, navpoint_pos);
		if(sqr_d < min_sqr_d) {
			min_sqr_d = sqr_d;
			min_id = navpoint;
		}	
	}
	assert(min_id != ~0);
	return min_id;
}

uint ai_find_next_path_node(NavMesh* navmesh, uint current, uint dest) {
	assert(navmesh);
	assert(current < navmesh->n_nodes);
	assert(dest < navmesh->n_nodes);

	if(current == dest)
		return dest;

	uint current_dist_idx = IDX_2D(current, dest, navmesh->n_nodes);
	float current_dist = navmesh->distance[current_dist_idx];

	float best_dist = current_dist;
	uint next_node = ~0;
	for(uint i = 0; i < navmesh->neighbour_count[current]; ++i) {
		uint idx = i + navmesh->neighbours_start[current];
		uint node_id = navmesh->neighbours[idx];
		if(node_id == dest)
			return dest;

		uint new_dist_idx = IDX_2D(node_id, dest, navmesh->n_nodes);
		float new_dist = navmesh->distance[new_dist_idx];
		if(new_dist < best_dist) {
			best_dist = new_dist;
			next_node = node_id;
		}
	}
	assert(next_node != ~0);
	return next_node;
}

float ai_navmesh_distance(NavMesh* navmesh, Vector2 p1, Vector2 p2) {
	assert(navmesh);

	uint navpoint1 = ai_nearest_navpoint(navmesh, p1);
	uint navpoint2 = ai_nearest_navpoint(navmesh, p2);

	float distance = vec2_length(vec2_sub(p1, navmesh->navpoints[navpoint1]));
	distance += vec2_length(vec2_sub(p2, navmesh->navpoints[navpoint2]));

	uint idx = IDX_2D(navpoint1, navpoint2, navmesh->n_nodes);
	distance += navmesh->distance[idx];

	return distance;
}

