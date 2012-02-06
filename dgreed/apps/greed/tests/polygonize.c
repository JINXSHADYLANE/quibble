#include "../../apps/greed/polygonize.h"
#include "darray.h"
#include "memory.h"

void _draw_rect(uint* raster, uint width, uint height, 
	uint x, uint y, uint w, uint h, uint value) {
	assert(raster);
	assert(x + w < width);
	assert(y + h < height);

	for(uint cy = y; cy < y + h; ++cy)
		for(uint cx = x; cx < x + w; ++cx)
			raster[IDX_2D(cx, cy, width)] = value;
}

void _draw_circle(uint* raster, uint width, uint height,
	uint x, uint y, uint r, uint value) {
	assert(raster);
	assert(x + r < width);
	assert(x - r < x);
	assert(y + r < height);
	assert(y - r < y);

	uint bbox_x1 = x - r;
	uint bbox_x2 = x + r + 1;
	uint bbox_y1 = y - r;
	uint bbox_y2 = y + r + 1;

	for(uint cy = bbox_y1; cy < bbox_y2; ++cy) {
		for(uint cx = bbox_x1; cx < bbox_x2; ++cx) {
			int dx = (int)x - (int)cx;
			int dy = (int)y - (int)cy;
			if(dx*dx + dy*dy <= r*r)
				raster[IDX_2D(cx, cy, width)] = value;
		}
	}
}	

TEST_(mark_islands) {
	const uint width = 256;
	const uint height = 256;
	uint* raster = (uint*)MEM_ALLOC(width * height * sizeof(uint));
	memset(raster, RASTER_EMPTY, width * height * sizeof(uint));

	ASSERT_(poly_mark_islands(raster, width, height) == 0);

	_draw_rect(raster, width, height, 0, 10, 100, 15, RASTER_SOLID);
	_draw_rect(raster, width, height, 103, 1, 60, 60, RASTER_SOLID);
	_draw_rect(raster, width, height, 50, 100, 80, 80, RASTER_SOLID);
	_draw_circle(raster, width, height, 90, 140, 30, RASTER_EMPTY);
	_draw_circle(raster, width, height, 200, 200, 10, RASTER_SOLID);

	ASSERT_(poly_mark_islands(raster, width, height) == 4);

	uint island_sizes[] = {0, 0, 0, 0};
	for(int y = 0; y < width; ++y) {
		for(int x = 0; x < height; ++x) {
			uint pixel = raster[IDX_2D(x, y, width)];
			if(pixel == RASTER_EMPTY)
				continue;

			ASSERT_(pixel < 4);
			island_sizes[pixel]++;
		}
	}	

	ASSERT_(island_sizes[0] == 3840);
	ASSERT_(island_sizes[1] == 1715);
	ASSERT_(island_sizes[2] == 4067);
	ASSERT_(island_sizes[3] == 377);

	MEM_FREE(raster);
}	

TEST_(simplify_island) {
	const uint width = 256;
	const uint height = 256;
	uint* raster = (uint*)MEM_ALLOC(width * height * sizeof(uint));
	memset(raster, RASTER_EMPTY, width * height * sizeof(uint));

	_draw_rect(raster, width, height, 10, 10, 200, 200, 0);
	_draw_rect(raster, width, height, 50, 50, 50, 50, RASTER_EMPTY);

	DArray island_poly = poly_simplify_island(raster, width, height, 0);
	// Should be less, change
	//printf("%d\n", island_poly.size);
	ASSERT_(island_poly.size == 14);

	darray_free(&island_poly);

	MEM_FREE(raster);
}	

bool _vec2_eql(Vector2 a, Vector2 b) {
	return a.x == b.x && a.y == b.y;
}	

TEST_(triangulate1) {
	Vector2 v;
	DArray poly1 = darray_create(sizeof(Vector2), 3);
	v = vec2(-1.0f, -1.0f);
	darray_append(&poly1, &v);
	v = vec2(0.0f, 1.0f);
	darray_append(&poly1, &v);
	v = vec2(1.0f, -1.0f);
	darray_append(&poly1, &v);

	DArray triangulated = poly_triangulate(poly1);
	ASSERT_(triangulated.size == 1);
	Triangle* tri = DARRAY_DATA_PTR(triangulated, Triangle);
	ASSERT_(tri);
	ASSERT_(_vec2_eql(tri->p1, vec2(1.0f, -1.0f)));
	ASSERT_(_vec2_eql(tri->p2, vec2(0.0f, 1.0f)));
	ASSERT_(_vec2_eql(tri->p3, vec2(-1.0f, -1.0f)));

	darray_free(&triangulated);
	darray_free(&poly1);
}

TEST_(triangulate2) {
	Vector2 v;
	DArray poly2 = darray_create(sizeof(Vector2), 4);
	v = vec2(-1.0f, 1.0f);
	darray_append(&poly2, &v);
	v = vec2(1.0f, 1.0f);
	darray_append(&poly2, &v);
	v = vec2(1.0f, -1.0f);
	darray_append(&poly2, &v);
	v = vec2(-1.0f, -1.0f);
	darray_append(&poly2, &v);

	DArray triangulated = poly_triangulate(poly2);
	ASSERT_(triangulated.size == 2);
	Triangle* tri = DARRAY_DATA_PTR(triangulated, Triangle);
	ASSERT_(tri);
	ASSERT_(_vec2_eql(tri[0].p1, vec2(-1.0f, -1.0f)));
	ASSERT_(_vec2_eql(tri[0].p2, vec2(1.0f, -1.0f)));
	ASSERT_(_vec2_eql(tri[0].p3, vec2(1.0f, 1.0f)));
	ASSERT_(_vec2_eql(tri[1].p1, vec2(-1.0f, -1.0f)));
	ASSERT_(_vec2_eql(tri[1].p2, vec2(1.0f, 1.0f)));
	ASSERT_(_vec2_eql(tri[1].p3, vec2(-1.0f, 1.0f)));

	darray_free(&triangulated);
	darray_free(&poly2);
}	

TEST_(triangulate3) {
	Vector2 v;
	DArray poly3 = darray_create(sizeof(Vector2), 100);
	for(uint i = 0; i < 100; ++i) {
		float a = ((float)i / 100.0f) * PI * 2.0f;
		v = vec2(sin(a) * 10.0f, cos(a) * 3.0f);
		darray_append(&poly3, &v);
	}

	DArray triangulated = poly_triangulate(poly3);
	ASSERT_(triangulated.size == 98);
	Triangle* tri = DARRAY_DATA_PTR(triangulated, Triangle);
	ASSERT_(tri);

	darray_free(&triangulated);
	darray_free(&poly3);
}	

TEST_(simplify) {
	Vector2 v;
	DArray poly = darray_create(sizeof(Vector2), 3);
	v = vec2(-1.0f, 1.0f);
	darray_append(&poly, &v);
	v = vec2(-1.0f, -1.0f);
	darray_append(&poly, &v);
	v = vec2(1.0f, -1.0f);
	darray_append(&poly, &v);
	v = vec2(1.0f, 1.0f);
	darray_append(&poly, &v);

	DArray hole = darray_create(sizeof(Vector2), 4);
	v = vec2(-0.3f, 0.3f);
	darray_append(&hole, &v);
	v = vec2(-0.3f, -0.3f);
	darray_append(&hole, &v);
	v = vec2(0.3f, -0.3f);
	darray_append(&hole, &v);
	v = vec2(0.3f, 0.3f);
	darray_append(&hole, &v);

	DArray complex_polygon = darray_create(sizeof(DArray), 2);
	darray_append(&complex_polygon, &poly);
	darray_append(&complex_polygon, &hole);

	DArray simple_polygon = poly_simplify(complex_polygon);
	ASSERT_(simple_polygon.size == 10);

	darray_free(&simple_polygon);
	darray_free(&complex_polygon);
	darray_free(&hole);
	darray_free(&poly);
}	

TEST_(empty) {
	uint collision_mask[60*60];
	memset(collision_mask, RASTER_EMPTY, 60 * 60 * sizeof(uint));
	
	DArray triangles = poly_triangulate_raster(collision_mask, 60, 60, NULL);
	ASSERT_(triangles.size == 0);

	darray_free(&triangles);
}
	

TEST_(simplify_and_triangulate) {
	Vector2 v;
	DArray poly = darray_create(sizeof(Vector2), 4);
	v = vec2(-1.0f, 1.0f);
	darray_append(&poly, &v);
	v = vec2(1.0f, 1.0f);
	darray_append(&poly, &v);
	v = vec2(1.0f, -1.0f);
	darray_append(&poly, &v);
	v = vec2(-1.0f, -1.0f);
	darray_append(&poly, &v);

	DArray hole = darray_create(sizeof(Vector2), 4);
	v = vec2(-0.5f, 0.5f);
	darray_append(&hole, &v);
	v = vec2(0.5f, 0.5f);
	darray_append(&hole, &v);
	v = vec2(0.5f, -0.5f);
	darray_append(&hole, &v);
	v = vec2(-0.5f, -0.5f);
	darray_append(&hole, &v);

	DArray complex_polygon = darray_create(sizeof(DArray), 2);
	darray_append(&complex_polygon, &poly);
	darray_append(&complex_polygon, &hole);

	DArray simple_polygon = poly_simplify(complex_polygon);
	ASSERT_(simple_polygon.size == 10);

	DArray triangles = poly_triangulate(simple_polygon);
	ASSERT_(triangles.size == 8);

	darray_free(&poly);
	darray_free(&hole);
	darray_free(&complex_polygon);
	darray_free(&simple_polygon);
	darray_free(&triangles);
}

TEST_(simplify_and_triangulate2) {
	Vector2 v;
	DArray poly = darray_create(sizeof(Vector2), 4);
	v = vec2(-3.0f, 3.0f);
	darray_append(&poly, &v);
	v = vec2(3.0f, 3.0f);
	darray_append(&poly, &v);
	v = vec2(3.0f, -3.0f);
	darray_append(&poly, &v);
	v = vec2(-3.0f, -3.0f);
	darray_append(&poly, &v);

	DArray hole1 = darray_create(sizeof(Vector2), 20);
	DArray hole2 = darray_create(sizeof(Vector2), 20);
	DArray hole3 = darray_create(sizeof(Vector2), 20);

	for(uint i = 0; i < 20; ++i) {
		float a = ((float)i / 20.0f) * PI * 2.0f;
		v = vec2(-1.0f + sin(a) * 0.5f, 1.0f + cos(a) * 0.5f);
		darray_append(&hole1, &v);
		v = vec2(1.0f + sin(a) * 0.5f, 1.0f + cos(a) * 0.5f);
		darray_append(&hole2, &v);
		v = vec2(sin(a) * 0.5f, -1.0f + cos(a) * 0.5f);
		darray_append(&hole3, &v);
	}

	DArray complex_polygon = darray_create(sizeof(DArray), 4);
	darray_append(&complex_polygon, &poly);
	darray_append(&complex_polygon, &hole1);
	darray_append(&complex_polygon, &hole2);
	darray_append(&complex_polygon, &hole3);

	DArray simple_polygon = poly_simplify(complex_polygon);
	ASSERT_(simple_polygon.size == 70);

	DArray triangulated_polygon = poly_triangulate(simple_polygon);
	ASSERT_(triangulated_polygon.size == 68);

	darray_free(&triangulated_polygon);
	darray_free(&simple_polygon);
	darray_free(&complex_polygon);
	darray_free(&hole1);
	darray_free(&hole2);
	darray_free(&hole3);
	darray_free(&poly);
}	

