#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include <anim.h>

#include "objects.h"

enum {
	OBJ_RABBIT_TYPE = 1,
	OBJ_GROUND_TYPE,
	OBJ_DECO_TYPE,
	OBJ_MUSHROOM_TYPE
};

// Rabbit

typedef struct {
	GameObject header;
	bool touching_ground;
	bool can_double_jump;
	float jump_time;
	float mushroom_hit_time;
	Anim* anim;
} ObjRabbit;

extern GameObjectDesc obj_rabbit_desc;

// Ground

typedef struct {
	GameObject header;
} ObjGround;

extern GameObjectDesc obj_ground_desc;

// Deco (static image, destroys itself when not visible)

typedef struct {
	GameObject header;
} ObjDeco;

extern GameObjectDesc obj_deco_desc;

// Mushroom

typedef struct {
	GameObject header;
} ObjMushroom;

extern GameObjectDesc obj_mushroom_desc;

#endif

