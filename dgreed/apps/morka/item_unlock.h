#ifndef ITEM_UNLOCK_H
#define ITEM_UNLOCK_H

#include "obj_types.h"
#include <malka/ml_states.h>

extern StateDesc item_unlock_state;

typedef struct {
	const char* spr;
	const char* text1;
	const char* text2;
	const char* text3;
	uint state_num;
} ObjItemUnlockParams;

void item_unlock_set(ObjItemUnlockParams * params);

#endif
