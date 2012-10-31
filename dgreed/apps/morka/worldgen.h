#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <utils.h>

void worldgen_reset(uint seed);
void worldgen_close(void);

void worldgen_update(float camera_extent_max);

#endif

