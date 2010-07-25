#include "polygonize.h"
#include "memory.h"

typedef struct {
	int x, y;
} Point;	

Point point(int x, int y) {
	Point result = {x, y};
	return result;
}	

bool _collide_rect_tri(const Triangle* tri, const RectF* rect);

DArray poly_triangulate_raster(uint* raster, uint width, uint height,
	DArray* segments) {
	assert(raster);
	assert(width);
	assert(height);

	DArray triangles = darray_create(sizeof(Triangle), 0);

	uint island_count = poly_mark_islands(raster, width, height);
	if(island_count == 0)
		return triangles;

	for(uint i = 0; i < island_count; ++i) {
		DArray simple_island = poly_simplify_island(raster, width, height, i);
		if(simple_island.size > 0) {
			if(segments) {
				Vector2* island_vertices = 
					DARRAY_DATA_PTR(simple_island, Vector2);
				for(uint j = 0; j < simple_island.size; ++j) {
					Segment new = {island_vertices[j], 
						island_vertices[(j+1)%simple_island.size]};
					
					// Make sure we are not adding 0 length segments
					assert(segment_length(new) > 0.00001f);

					darray_append(segments, &new);	
				}	
			}
			DArray island_tris = poly_triangulate(simple_island);
			darray_append_multi(&triangles, island_tris.data, island_tris.size);
			darray_free(&island_tris);
		}	
		darray_free(&simple_island);
	}

	float fwidth = (float)width;
	float fheight = (float)height;

	// Clone tris near the edges of screen, 
	// for proper wrap-around collision detection
	const float rect_thickness = 20.0f;
	RectF top = rectf(0.0f, 0.0f, fwidth, rect_thickness);
	RectF bottom = rectf(0.0f, fheight - rect_thickness, fwidth, fheight);
	RectF left = rectf(0.0f, 0.0f, rect_thickness, fheight);
	RectF right = rectf(fwidth - rect_thickness, 0.0f, fwidth, fheight);

	uint n_tris = triangles.size;
	for(uint i = 0; i < n_tris; ++i) {
		Triangle* tris = DARRAY_DATA_PTR(triangles, Triangle);

		Triangle new;
		if(_collide_rect_tri(&(tris[i]), &top)) {
			new = tris[i];
			new.p1.y += fheight; 
			new.p2.y += fheight;
			new.p3.y += fheight;
			darray_append(&triangles, &new);
		}	
		if(_collide_rect_tri(&(tris[i]), &bottom)) {
			new = tris[i];
			new.p1.y -= fheight; 
			new.p2.y -= fheight;
			new.p3.y -= fheight;
			darray_append(&triangles, &new);
		}
		if(_collide_rect_tri(&(tris[i]), &left)) {
			new = tris[i];
			new.p1.x += fwidth; 
			new.p2.x += fwidth;
			new.p3.x += fwidth;
			darray_append(&triangles, &new);
		}
		if(_collide_rect_tri(&(tris[i]), &right)) {
			new = tris[i];
			new.p1.x -= fwidth; 
			new.p2.x -= fwidth;
			new.p3.x -= fwidth;
			darray_append(&triangles, &new);
		}
	}	

	return triangles;
}	

int x_offsets[] = {-1,  0,  1, -1,  0, 1, -1, 1};
int y_offsets[] = {-1, -1, -1,  1,  1, 1,  0, 0};

// Returns true if pos is inside raster
bool _clip_pos(uint width, uint height, Point pos)
{
	if(pos.x < 0 || pos.y < 0)
		return false;
	if(pos.x >= (int)width || pos.y >= (int)height)
		return false;
	return true;	
}

void _flood_fill(uint* raster, uint width, uint height,
	Point pos, uint value) {
	assert(raster);
	assert(pos.x >= 0 && pos.x < width);
	assert(pos.y >= 0 && pos.y < height);
	assert(ARRAY_SIZE(x_offsets) == 8);
	assert(ARRAY_SIZE(y_offsets) == 8);

	uint orig_value = raster[IDX_2D(pos.x, pos.y, width)];
	raster[IDX_2D(pos.x, pos.y, width)] = value;

	// Use DArray as a queue
	DArray queue = darray_create(sizeof(Point), 100);	
	darray_append(&queue, &pos);

	while(queue.size > 0) {
		Point* queue_data = DARRAY_DATA_PTR(queue, Point);
		Point p = queue_data[queue.size-1];
		darray_remove(&queue, queue.size-1);

		// Insert neighbour pixels into queue
		for(uint i = 0; i < 8; ++i) {
			int x_off = x_offsets[i];
			int y_off = y_offsets[i];
			Point np = point(p.x + x_off, p.y + y_off);

			if(!_clip_pos(width, height, np))
				continue;
			if(raster[IDX_2D(np.x, np.y, width)] != orig_value)
				continue;

			raster[IDX_2D(np.x, np.y, width)] = value;
			darray_append(&queue, &np);
		}	
	}

	darray_free(&queue);
}	

uint poly_mark_islands(uint* raster, uint width, uint height) {
	uint island_count = 0;

	for(uint y = 0; y < height; ++y) {
		for(uint x = 0; x < width; ++x) {
			if(raster[IDX_2D(x, y, width)] != RASTER_SOLID)
				continue;

			_flood_fill(raster, width, height, point(x, y), island_count);
			island_count++;
		}
	}

	return island_count;
}

int cross_x_offsets[] = {1, -1, 0,  0};
int cross_y_offsets[] = {0,  0, 1, -1};

bool _is_empty(uint* raster, uint width, uint height, Point pos) {
	assert(raster);
	assert(pos.x >= -1 && pos.y >= -1);
	assert(pos.x <= (int)width && pos.y <= (int)height);

	if(!_clip_pos(width, height, pos))
		return true;
	return raster[IDX_2D(pos.x, pos.y, width)] == RASTER_EMPTY;	
}	

// Leaves only the outline of island, all inside pixels becomes RASTER_ISLAND
void _outline_island(uint* raster, uint width, uint height, uint island) {
	assert(raster);
	assert(ARRAY_SIZE(cross_x_offsets) == 4);
	assert(ARRAY_SIZE(cross_y_offsets) == 4);

	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			if(raster[IDX_2D(x, y, width)] != island)
				continue;
			
			uint empty_neighbours = 0;
			for(uint i = 0; i < 4; ++i) {
				Point np = point(x + cross_x_offsets[i], 
					y + cross_y_offsets[i]);
				if(_is_empty(raster, width, height, np))
					empty_neighbours++;
			}

			if(empty_neighbours == 0)
				raster[IDX_2D(x, y, width)] = RASTER_ISLAND;
		}
	}
}	

Vector2 _point_to_vec(Point p) {
	return vec2((float)p.x + 0.5f, (float)p.y + 0.5f);
}

bool _is_bridge(uint* raster, uint width, uint height, Point p) {
	assert(raster);
	assert(p.x >= 0 && p.y >= 0);
	assert(p.x < width && p.y < height);

	uint empty_neighbours = 0;
	for(uint i = 0; i < 8; ++i) {
		Point np = point(p.x + x_offsets[i], p.y + y_offsets[i]);
		if(_is_empty(raster, width, height, np))
			empty_neighbours++;
	}

	if(empty_neighbours < 6)
		return false;

	if(_is_empty(raster, width, height, point(p.x+1, p.y))
		&& _is_empty(raster, width, height, point(p.x-1, p.y)))
		return true;
	if(_is_empty(raster, width, height, point(p.x, p.y+1))
		&& _is_empty(raster, width, height, point(p.x, p.y-1)))
		return true;
	if(_is_empty(raster, width, height, point(p.x+1, p.y+1))
		&& _is_empty(raster, width, height, point(p.x-1, p.y-1)))
		return true;
	if(_is_empty(raster, width, height, point(p.x+1, p.y-1))
		&& _is_empty(raster, width, height, point(p.x-1, p.y+1)))
		return true;
	return false;	
}

DArray poly_simplify_island(uint* raster, uint width, uint height, 
	uint island) {
	assert(raster);

	_outline_island(raster, width, height, island);

	DArray island_polys = darray_create(sizeof(DArray), 0);

	const int walk_x_offsets[] = {-1, -1, 0, 1, 1,  1,  0, -1}; 
	const int walk_y_offsets[] = { 0,  1, 1, 1, 0, -1, -1, -1};

	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			if(raster[IDX_2D(x, y, width)] != island)
				continue;

			DArray outline = darray_create(sizeof(Vector2), 100);
			
			Point start_pos = point(x, y);
			Point current_pos = start_pos;
			while(true) {

				//if(_is_bridge(raster, width, height, current_pos))
				//	LOG_ERROR("Collision regions are too thin.");

				Vector2 vertex = _point_to_vec(current_pos);
				darray_append(&outline, &vertex);

				Point next_pos = {0, 0};
				bool next_pos_found = false;
				for(uint i = 0; i < 8; ++i) {
					Point np = point(current_pos.x + walk_x_offsets[i],
						current_pos.y + walk_y_offsets[i]);

					if(!_clip_pos(width, height, np))
						continue;
					if(_is_bridge(raster, width, height, np))
						continue;
					if(raster[IDX_2D(np.x, np.y, width)] == island) {
						next_pos = np;
						next_pos_found = true;
						break;
					}	
				}

				raster[IDX_2D(current_pos.x, current_pos.y, width)] 
						= RASTER_ISLAND;

				if(next_pos_found) {
					current_pos = next_pos;
					continue;
				}		

				break;
			}	
			// Insert starting pos once more
			Vector2 vertex = _point_to_vec(start_pos);
			darray_append(&outline, &vertex);

			// Skip holes with perimeter smaller than 100 pixels
			if(outline.size < 100 && island_polys.size > 0) {
				darray_free(&outline);
				continue;
			}	

			Vector2* complex_outline = DARRAY_DATA_PTR(outline, Vector2);
			DArray simple_outline = darray_create(sizeof(Vector2), 100);
			darray_append(&simple_outline, &complex_outline[0]);
			uint start = 0;
			uint end = 1;

			const float delta_dir_tolerance[] = 
				{50.0f, 38.0f, 30.0f, 18.0f, 6.0f};

			float direction = 0.0f;
			while(true) {
				if(end == outline.size-1)
					break;
					
				uint path_len = end - start;

				Vector2 v_start = complex_outline[start];
				Vector2 v_end = complex_outline[end];
				
				if(path_len < 5)
					direction = vec2_dir(vec2_sub(v_end, v_start)) * RAD_TO_DEG;

				Vector2 v_pre_end = complex_outline[end-1];
				Vector2 v_post_end = complex_outline[end+1];
				float pre_end_dir = vec2_dir(vec2_sub(v_end, v_pre_end));
				float post_end_dir = vec2_dir(vec2_sub(v_post_end, v_end));
				float new_direction = vec2_dir(vec2_sub(v_post_end, v_start));
				pre_end_dir *= RAD_TO_DEG;
				post_end_dir *= RAD_TO_DEG;
				new_direction *= RAD_TO_DEG;

				uint min_delta_dir_idx = ARRAY_SIZE(delta_dir_tolerance)-1;
				float dir_change_tolerance = 
					delta_dir_tolerance[MIN(path_len-1, min_delta_dir_idx)];
				if(path_len > 20)
					dir_change_tolerance = 0.3f;

				if(abs(pre_end_dir - post_end_dir) > 80.0f
					|| abs(direction - new_direction) > dir_change_tolerance) {
					darray_append(&simple_outline, &v_end);
					start = end;
					end = start+1;
					continue;
				}	

				end++;
			}	
			darray_free(&outline);
			
			// TODO: investigate why so many small outlines are generated
			if(simple_outline.size > 2)
				darray_append(&island_polys, &simple_outline);
			else
				darray_free(&simple_outline);
		}
	}	

	DArray result;
	if(island_polys.size > 0) {
		result = poly_simplify(island_polys);
		DArray* outline = DARRAY_DATA_PTR(island_polys, DArray);
		for(uint i = 0; i < island_polys.size; ++i) {
			darray_free(&outline[i]);
		}
	}
	else {
		result = darray_create(sizeof(Vector2), 0);
	}	

	darray_free(&island_polys);

	return result;
}	


bool _intersect_segments(Vector2 p1, Vector2 p2, Vector2 q1, Vector2 q2) {
	const float epsilon = 0.001f;

	float a1 = p2.y - p1.y;
	float b1 = p1.x - p2.x;
	float c1 = a1*p1.x + b1*p1.y;
	float a2 = q2.y - q1.y;
	float b2 = q1.x - q2.x;
	float c2 = a2*q1.x + b2*q1.y;

	float det = a1*b2 - a2*b1;
	if(abs(det) < epsilon)
		return false;
	
	float x = (b2*c1 - b1*c2) / det;
	
	if(MIN(p1.x, p2.x) <= (x-epsilon) && (x+epsilon) <= MAX(p1.x, p2.x)
		&& MIN(q1.x, q2.x) <= (x-epsilon) && (x+epsilon) <= MAX(q1.x, q2.x))
		return true;
	return false;	
}	

bool _polygon_segment_intersect(DArray polygon, Vector2 p1, Vector2 p2) {
	Vector2* vertices = DARRAY_DATA_PTR(polygon, Vector2);

	for(uint i = 0; i < polygon.size; ++i) {
		Vector2 q1 = vertices[i];
		Vector2 q2 = vertices[(i+1)%polygon.size];

		if(_intersect_segments(p1, p2, q1, q2))
			return true;
	}
	return false;
}	

typedef struct {
	uint i1, i2;
} IndexedSegment;	
	
DArray poly_simplify(DArray polygon) {
	assert(polygon.size > 0);

	DArray* outlines = DARRAY_DATA_PTR(polygon, DArray);

	if(polygon.size == 1) {
		// Clone polygon
		DArray new_polygon = darray_create(sizeof(Vector2), outlines[0].size);
		darray_append_multi(&new_polygon, outlines[0].data, outlines[0].size);
		return new_polygon;
	}	

	DArray new_polygon = darray_create(sizeof(Vector2), outlines[0].size * 3 / 2);
	darray_append_multi(&new_polygon, outlines[0].data, outlines[0].size);
	Vector2* new_poly_vertices = DARRAY_DATA_PTR(new_polygon, Vector2);

	for(uint hole = 1; hole < polygon.size; ++hole) {
		Vector2* hole_vertices = DARRAY_DATA_PTR(outlines[hole], Vector2);

		float min_distance = 10000.0f;
		uint min_poly_idx = ~0;
		uint min_hole_idx = ~0;

		for(uint poly_idx = 0; poly_idx < new_polygon.size; ++poly_idx) {
			for(uint hole_idx = 0; hole_idx < outlines[hole].size; ++hole_idx) {
				Vector2 poly_vertex = new_poly_vertices[poly_idx];
				Vector2 hole_vertex = hole_vertices[hole_idx]; 

				float dist = vec2_length(vec2_sub(poly_vertex, hole_vertex));
				if(dist < min_distance) {
					if(_polygon_segment_intersect(new_polygon, 
						poly_vertex, hole_vertex))
						continue;
					if(_polygon_segment_intersect(outlines[hole],
						poly_vertex, hole_vertex))
						continue;

					min_distance = dist;
					min_poly_idx = poly_idx;
					min_hole_idx = hole_idx;
				}	
			}
		}	

		assert(min_poly_idx != ~0);
		assert(min_hole_idx != ~0);
		assert(min_distance < 10000.0f);

		uint old_vertex_count = new_polygon.size;
		darray_reserve(&new_polygon, 
			old_vertex_count + outlines[hole].size + 2);
		new_poly_vertices = DARRAY_DATA_PTR(new_polygon, Vector2);
		new_polygon.size = old_vertex_count + outlines[hole].size + 2;	

		for(int i = old_vertex_count-1; i >= (int)min_poly_idx; --i) {
			new_poly_vertices[i + outlines[hole].size + 2] =
				new_poly_vertices[i];
		}		

		for(uint i = min_poly_idx+1, j = min_hole_idx;
			i < min_poly_idx + outlines[hole].size + 2;
			i++, j = (j > 0 ? j-1 : outlines[hole].size - 1)) {
			
			new_poly_vertices[i] = hole_vertices[j];
		}
	}

	return new_polygon;
}	

bool _point_in_triangle(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p) {
	Vector2 a = vec2_sub(p3, p2);
	Vector2 b = vec2_sub(p1, p3);
	Vector2 c = vec2_sub(p2, p1);
	Vector2 ap = vec2_sub(p, p1);
	Vector2 bp = vec2_sub(p, p2);
	Vector2 cp = vec2_sub(p, p3);

	float abp = a.x*bp.y - a.y*bp.x;
	float cap = c.x*ap.y - c.y*ap.x;
	float bcp = b.x*cp.y - b.y*cp.x;

	return (abp >= 0.0f) && (cap >= 0.0f) && (bcp >= 0.0f);
}

bool _can_cut(DArray polygon, uint ip1, uint ip2, uint ip3, 
	const uint* vertices, uint v_count) {
	assert(vertices);
	assert(v_count > 2);

	Vector2* poly = DARRAY_DATA_PTR(polygon, Vector2);
	Vector2 p1 = poly[vertices[ip1]];
	Vector2 p2 = poly[vertices[ip2]];
	Vector2 p3 = poly[vertices[ip3]];

	float area = (
		p1.x * (p2.y - p3.y) +
		p2.x * (p3.y - p1.y) +
		p3.x * (p1.y - p2.y)) / 2.0f;

	if(area	< 0.00001f)
		return false;

	for(uint i = 0; i < v_count; ++i) {
		if(i == ip1 || i == ip2 || i == ip3)
			continue;

		if(p1.x == poly[vertices[i]].x && p1.y == poly[vertices[i]].y)
			continue;
		if(p2.x == poly[vertices[i]].x && p2.y == poly[vertices[i]].y)
			continue;
		if(p3.x == poly[vertices[i]].x && p3.y == poly[vertices[i]].y)
			continue;
		if(_point_in_triangle(p1, p2, p3, poly[vertices[i]]))
			return false;
	}
	return true;
}	

bool _collide_rect_tri(const Triangle* tri, const RectF* rect) {
	assert(tri);
	assert(rect);

	if(rectf_contains_point(rect, &(tri->p1)) 
		|| rectf_contains_point(rect, &(tri->p2))
		|| rectf_contains_point(rect, &(tri->p3)))
		return true;

	Vector2 p1 = vec2(rect->left, rect->top);
	Vector2 p2 = vec2(rect->left, rect->bottom);
	Vector2 p3 = vec2(rect->right, rect->top);
	Vector2 p4 = vec2(rect->right, rect->bottom);

	if(_point_in_triangle(tri->p1, tri->p2, tri->p3, p1)
		|| _point_in_triangle(tri->p1, tri->p2, tri->p3, p2)
		|| _point_in_triangle(tri->p1, tri->p2, tri->p3, p3)
		|| _point_in_triangle(tri->p1, tri->p2, tri->p3, p4))
		return true;

	return false;
}	
	
DArray poly_triangulate(DArray polygon) {
	assert(polygon.size >= 3);

	Vector2* poly = DARRAY_DATA_PTR(polygon, Vector2);

	uint vertex_count = polygon.size;
	uint* vertices = (uint*)MEM_ALLOC(vertex_count * sizeof(uint));

	// Triangulation algorithm works on clockwise polygon,
	// so reverse it.
	for(uint i = 0; i < vertex_count; ++i)
		vertices[i] = vertex_count-i - 1;

	DArray triangulated = darray_create(sizeof(Triangle), vertex_count - 2);	

	uint cuts_to_do = vertex_count - 2;
	while(cuts_to_do > 0) {
		uint v1 = 0, v2 = 1, v3 = 2;
		while(!_can_cut(polygon, v1, v2, v3, vertices, vertex_count)) {
			v1++; v2++; v3++;
			if(v1 >= vertex_count) {
				LOG_ERROR("Bad polygon, unable to triangulate");
			}	
			if(v2 >= vertex_count) v2 = 0;
			if(v3 >= vertex_count) v3 = 0;
		}
		Triangle new = {poly[vertices[v1]], poly[vertices[v2]], 
			poly[vertices[v3]]};
		darray_append(&triangulated, &new);

		for(uint i = v2; i+1 < vertex_count; ++i)
			vertices[i] = vertices[i+1];
		vertex_count--;
		cuts_to_do--;
	}	

	MEM_FREE(vertices);

	return triangulated;
}	

