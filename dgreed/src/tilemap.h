#ifndef TILEMAP_H
#define TILEMAP_H

#include <system.h>

typedef struct {
	uint start, end, fps;
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
	const char* name;
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

void tilemap_render(Tilemap* t, RectF viewport, float time);

bool tilemap_collide(Tilemap* t, RectF rect);
int tilemap_collide_point(Tilemap* t, Vector2 point);
bool tilemap_collide_circle(Tilemap* t, Vector2 center, float radius);

RectF tilemap_world2screen(Tilemap* t, RectF rect);
Vector2 tilemap_world2screen_point(Tilemap* t, Vector2 point);
RectF tilemap_screen2world(Tilemap* t, RectF rect);
Vector2 tilemap_screen2world_point(Tilemap* t, Vector2 point);

#endif

