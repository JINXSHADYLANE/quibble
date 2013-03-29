#ifndef LEVEL_SELECT_H
#define LEVEL_SELECT_H

#include "levels.h"

#include <malka/ml_states.h>

extern StateDesc level_select_state;

void level_select_set_season(SeasonType season);

#endif
