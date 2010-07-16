#include "ai_precalc.h"
#include "memory.h"
#include "game.h"

// Tweakables
float min_wall_distance = 1.0f;
float min_point_distance = 30.0f;
float max_edge_distance = 55.0f;

float _wall_distance(Vector2 p, DArray geometry) {
	Segment* segments = DARRAY_DATA_PTR(geometry, Segment);
	float distance = 10000.0f;
	for(uint i = 0; i < geometry.size; ++i) {
		float d = segment_point_dist(segments[i], p);
		if(abs(d) < abs(distance))
			distance = -d;
	}
	return distance > 0.0f ? distance : 0.0f;
}

float _point_distance(Vector2 p, DArray points) {
	Vector2* points_vec2 = DARRAY_DATA_PTR(points, Vector2);
	float distance_sqr = 10000000.0f;
	for(uint i = 0; i < points.size; ++i) {
		float dx = points_vec2[i].x - p.x;
		float dy = points_vec2[i].y - p.y;
		float d_sqr = dx*dx + dy*dy;
		distance_sqr = MIN(distance_sqr, d_sqr); 
	}
	return sqrtf(distance_sqr);
}

DArray _gen_navpoints(DArray geometry) {
	DArray points = darray_create(sizeof(Vector2), 0);
	uint count = 100000;
	do {
		Vector2 navpoint = vec2(rand_float_range(0.0f, SCREEN_WIDTH),
			rand_float_range(0.0f, SCREEN_HEIGHT));

		float wdist = _wall_distance(navpoint, geometry);
		float pdist = _point_distance(navpoint, points);

		if(wdist >= min_wall_distance && pdist >= min_point_distance) {
			darray_append(&points, (void*)&navpoint);
		}
	} while(count--);	
	return points;
}	

DArray _gen_edges(DArray points, DArray geometry) {
	DArray edges = darray_create(sizeof(Edge), 0);
	Vector2* points_vec2 = DARRAY_DATA_PTR(points, Vector2);
	for(uint i = 0; i < points.size; ++i) {
		for(uint j = 0; j < points.size; ++j) {
			if(i == j)
				continue;

			float sqr_dist = vec2_length_sq(vec2_sub(points_vec2[i], 
				points_vec2[j]));

			if(sqr_dist <= max_edge_distance*max_edge_distance) {
				Edge n = {i, j};

				// TODO: check distance to walls, not intersection
				Segment s = {points_vec2[i], points_vec2[j]};
				Segment* geometry_s = DARRAY_DATA_PTR(geometry, Segment);
				bool intersects_arena = false;
				for(uint k = 0; k < geometry.size; ++k) {
					if(segment_intersect(s, geometry_s[k], NULL)) {
						intersects_arena = true;
						break;
					}
				}

				if(!intersects_arena)
					darray_append(&edges, (void*)&n);
			}	
		}
	}
	return edges;
}	

NavMesh ai_precalc_navmesh(DArray geometry) {
	NavMesh res;

	DArray navpoints = _gen_navpoints(geometry);
	DArray edges = _gen_edges(navpoints, geometry);

	Vector2* navpoints_v = DARRAY_DATA_PTR(navpoints, Vector2);
	Edge* edges_e = DARRAY_DATA_PTR(edges, Edge);

	uint n = navpoints.size;

	res.n_nodes = n;
	res.navpoints = MEM_ALLOC(sizeof(Vector2) * n);
	res.adjacency = MEM_ALLOC(sizeof(float) * n*n);
	res.distance = MEM_ALLOC(sizeof(float) * n*n);
	res.radius = MEM_ALLOC(sizeof(float) * n);

	memset(res.adjacency, 0, sizeof(float) * n*n);
	memset(res.distance, 0, sizeof(float) * n*n);
		
	for(uint i = 0; i < navpoints.size; ++i) {
		res.navpoints[i] = 	navpoints_v[i];
		res.radius[i] = _wall_distance(navpoints_v[i], geometry);
	}

	for(uint i = 0; i < edges.size; ++i) {
		float dist = vec2_length(vec2_sub(
			navpoints_v[edges_e[i].v1], navpoints_v[edges_e[i].v2]));
		res.adjacency[IDX_2D(edges_e[i].v1, edges_e[i].v2, n)] = dist;
		res.adjacency[IDX_2D(edges_e[i].v2, edges_e[i].v1, n)] = dist;
	}

	// Floyd-Warshall
	memcpy(res.distance, res.adjacency, sizeof(float) * n*n);
	for(uint i = 0; i < n; ++i)
		for(uint j = 0; j < n; ++j)
			for(uint k = 0; k < n; ++k)
				res.distance[IDX_2D(j, k, n)] = MIN(res.distance[IDX_2D(j, k, n)],
					res.distance[IDX_2D(j, i, n)]+res.distance[IDX_2D(i, k, n)]);

	darray_free(&navpoints);
	darray_free(&edges);

	return res;
}

void ai_free_navmesh(NavMesh mesh) {
	assert(mesh.n_nodes);
	assert(mesh.navpoints);
	assert(mesh.adjacency);
	assert(mesh.distance);
	assert(mesh.radius);

	MEM_FREE(mesh.navpoints);
	MEM_FREE(mesh.adjacency);
	MEM_FREE(mesh.distance);
	MEM_FREE(mesh.radius);
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

	for(uint i = 0; i < n*n; ++i)
		file_write_float(file, navmesh->adjacency[i]);

	for(uint i = 0; i < n*n; ++i)
		file_write_float(file, navmesh->distance[i]);

	for(uint i = 0; i < n; ++i)
		file_write_float(file, navmesh->radius[i]);
}

NavMesh ai_load_navmesh(FileHandle file) {
	assert(file);

	NavMesh r;

	r.n_nodes = file_read_uint32(file);
	assert(r.n_nodes > 3 && r.n_nodes < 1024);

	size_t n = r.n_nodes;

	r.navpoints = (Vector2*)MEM_ALLOC(sizeof(Vector2)*n);
	r.adjacency = (float*)MEM_ALLOC(sizeof(float)*n*n);
	r.distance = (float*)MEM_ALLOC(sizeof(float)*n*n);
	r.radius = (float*)MEM_ALLOC(sizeof(float)*n);

	for(uint i = 0; i < n; ++i) {
		r.navpoints[i].x = file_read_float(file);
		r.navpoints[i].y = file_read_float(file);
	}

	for(uint i = 0; i < n*n; ++i)
		r.adjacency[i] = file_read_float(file);

	for(uint i = 0; i < n*n; ++i)
		r.distance[i] = file_read_float(file);

	for(uint i = 0; i < n; ++i)
		r.radius[i] = file_read_float(file);

	return r;	
}

