#ifndef TILEMAP_H
#define TILEMAP_H

#include <system.h>

typedef struct {
	uint start, end;
	float fps;
} TileAnimDef;

typedef struct {
	TexHandle texture;

	uint n_animdefs;
	TileAnimDef* animdefs;
} TilesetDef;

typedef struct {
	uint id;
	Vector2 p;
} TilemapObject;

typedef struct {
	char* name;
	uint16* data;
	uint render_layer;
} TilemapLayer;	

typedef struct {
	Vector2 center;
	float z, rot;
} TilemapCamera;	

typedef struct {
	uint tile_width, tile_height;
	uint width, height;

	uint n_tilesets;
	TilesetDef* tilesets;

	uint n_objects;
	TilemapObject* objects;

	uint n_layers;
	TilemapLayer* layers;

	byte* collission;

	TilemapCamera camera;
} Tilemap;

Tilemap* tilemap_load(const char* filename);
void tilemap_save(Tilemap* t, const char* filename);
void tilemap_free(Tilemap* t);

// Renders tilemap. Viewport is screen rect where current camera view will be
// rendered. Some tiles might be partially outside of viewport! 
void tilemap_render(Tilemap* t, RectF viewport, float time);

// Collissions
bool tilemap_collide(Tilemap* t, RectF rect);
bool tilemap_collide_point(Tilemap* t, Vector2 point);
// Actually, ray segment cast. Returns end if no intersection.
Vector2 tilemap_raycast(Tilemap* t, Vector2 start, Vector2 end);
// Returns offset, along which rect can move without collission. 
// Initial rect is assumed to not be colliding with anything! 
Vector2 tilemap_collide_swept_rectf(Tilemap* t, RectF rect, Vector2 offset);

// Coordinate system conversion
RectF tilemap_world2screen(Tilemap* t, const RectF* viewport, RectF rect);
Vector2 tilemap_world2screen_point(Tilemap* t, const RectF* viewport, Vector2 point);
RectF tilemap_screen2world(Tilemap* t, const RectF* viewport, RectF rect);
Vector2 tilemap_screen2world_point(Tilemap* t, const RectF* viewport, Vector2 point);

#endif

