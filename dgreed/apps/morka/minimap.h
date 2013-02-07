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

#endif
