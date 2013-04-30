#ifndef IN_APP_H
#define IN_APP_H

#include "obj_types.h"
#include <malka/ml_states.h>

extern StateDesc in_app_state;

typedef struct {
	const char* name;
	const char* icon;
	float cost;
	uint coins; // placeholder, should be changed to actual effect of purchase	
} StoreItemParams;

#endif
