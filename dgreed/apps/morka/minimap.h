#ifndef MINIMAP_H
#define MINIMAP_H

#include "obj_types.h"
#include <darray.h>

void minimap_init(void);
void minimap_close(void);
void minimap_reset(uint distance);
void minimap_track(ObjRabbit* rabbit);
void minimap_draw(float t);
void minimap_draw_finish_line(void);
void minimap_update_places(void);

float minimap_min_x(void);
float minimap_max_x(void);

uint minimap_get_count(void);

ObjRabbit* minimap_get_rabbit(uint i);
ObjRabbit* minimap_get_rabbit_in_place(uint i);
uint minimap_get_place_of_rabbit(ObjRabbit* rabbit);

#endif
