#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <datastruct.h>
#include "objects.h"

typedef struct {
	GameObjectDesc* desc;
	Vector2 pos;
	void* userdata;
	ListHead list;
} WorldElement;

typedef struct {
	ListHead background;
	ListHead mushrooms;
} WorldPage;

void worldgen_reset(uint seed);
void worldgen_close(void);

WorldPage* worldgen_current(void);
WorldPage* worldgen_next(void);

WorldPage* worldgen_new_page(void);

void worldgen_show(WorldPage* page);

#endif

