#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include "objects.h"

enum {
	OBJ_RABBIT_TYPE = 1,
	OBJ_GROUND_TYPE,
	OBJ_DECO_TYPE
};

// Rabbit

typedef struct {
	GameObject header;
	bool touching_ground;
} ObjRabbit;

extern GameObjectDesc obj_rabbit_desc;

// Ground

typedef struct {
	GameObject header;
} ObjGround;

extern GameObjectDesc obj_ground_desc;

// Deco

typedef struct {
	GameObject header;
} ObjDeco;

extern GameObjectDesc obj_deco_desc;

#endif

