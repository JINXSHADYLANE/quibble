#ifndef SHOP_H
#define SHOP_H

#include "obj_types.h"
#include <malka/ml_states.h>

extern StateDesc shop_state;

extern uint coins;
extern bool powerups[POWERUP_COUNT];

void shop_reset(void);

#endif
