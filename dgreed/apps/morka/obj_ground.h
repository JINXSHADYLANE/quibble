#ifndef OBJ_GROUND_H
#define OBJ_GROUND_H

#include "objects.h"

#define OBJ_GROUND_TYPE 2

typedef struct {
	GameObject header;
} ObjGround;

extern GameObjectDesc obj_ground_desc;

#endif
