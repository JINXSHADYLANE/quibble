#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <utils.h>

#include "levels.h"

void worldgen_reset(uint seed, const LevelDesc* desc);
void worldgen_close(void);

void worldgen_update(float fg_camera_extent_max, float bg_camera_extent_max);
void worldgen_debug_render();

#endif

