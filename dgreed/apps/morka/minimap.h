#ifndef MINIMAP_H
#define MINIMAP_H

#include "obj_types.h"
#include <darray.h>

void minimap_init();
void minimap_close(void);

void minimap_track(ObjRabbit* rabbit);

void minimap_draw();

void minimap_reset(uint distance);

float minimap_max_x();
float minimap_min_x();

void minimap_update_places();

uint minimap_get_count();
ObjRabbit* minimap_get_place(uint i);

#endif
