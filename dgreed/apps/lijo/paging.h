#ifndef PAGING_H
#define PAGING_H

#include <tilemap.h>

// Higher level system for huge 2d tile world

// Each page has 512x512 tiles, 8x8 pixels each.
// It has two tile layers - foreground and background.
// It also has 1 bit per tile collision bitmap.
#define PAGE_SIZE 512
#define TILE_SIZE 8
#define MAX_TILESETS 8
#define MAX_TILESET_NAME 16

typedef struct {
	int x, int y;
} PageIdx;

// In-memory runtime page representation
typedef struct {
	int x, y;
	Tilemap* tilemap;
} Page;

// On-disk compressed page description
typedef struct {
	uint16 background[PAGE_SIZE*PAGE_SIZE];	
	uint16 foreground[PAGE_SIZE*PAGE_SIZE];	
	byte collision[PAGE_SIZE*PAGE_SIZE/8];
	char tileset_names[MAX_TILESETS*MAX_TILESET_NAME];
	uint32 n_tilesets;
} PageDesc;

void paging_init(const char* pagefile);
void paging_close(void);
void paging_save(const char* pagefile);

// World position to page idx
PageIdx paging_world_pt2page(int x, int y);
PageIdx paging_world_vec2page(Vector2 pos);

// World position to tile idx inside a page
int paging_world_pt2tile(int x, int y);
int paging_world_vec2tile(Vector2 pos);

// Page handling
void paging_new(int x, int y);
bool paging_exists(int x, int y);
bool paging_is_loaded(int x, int y);
void paging_load(int x, int y);
void paging_unload(int x, int y);
Page* paging_get(int x, int y); // NULL if page doesn't exist or not loaded

// Page tilemap access (for editor)
void paging_set_tile(int x, int y, int tile_idx, int layer, uint16 value);
uint16 paging_get_tile(int x, int y, int tile_idx, int layer);
void paging_set_col(int x, int y, int tile_idx, bool solid);
bool paging_get_col(int x, int y, int tile_idx);

// Rendering
void paging_render(RectF viewport, int camera_x, int camera_y,
		float zoom, float rotation);

// TODO: collision

#endif
