#ifndef POLYGONIZE_H
#define POLYGONIZE_H

#include <utils.h>
#include <darray.h>

#define RASTER_EMPTY 0xFFFFFFFF
#define RASTER_SOLID 0x0FFFFFFF
#define RASTER_ISLAND 0x00FFFFFF

// Public interface:

// Returns darray of triangles from b&w bitmap.
// Raster data is destroyed by this process!
// Optionally (if it's not null), fills segments 
// darray with line segments of arena geometry outline.
DArray poly_triangulate_raster(uint* raster, uint width, uint height,
	DArray* segments);

// Private interface: (visible for testing)

// Marks islands with unique ids and returns amount of them.
uint poly_mark_islands(uint* raster, uint width, uint height);

// Converts island to simple polygon (no holes)
DArray poly_simplify_island(uint* raster, uint width, uint height, uint island);

// Simplifies complex polygon
DArray poly_simplify(DArray polygon);

// Triangulates simple polygon
DArray poly_triangulate(DArray polygon);

#endif

