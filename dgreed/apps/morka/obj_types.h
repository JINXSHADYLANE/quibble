#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include <anim.h>

#include "objects.h"

enum {
	OBJ_RABBIT_TYPE = 1,
	OBJ_GROUND_TYPE,
	OBJ_DECO_TYPE,
	OBJ_MUSHROOM_TYPE,
	OBJ_CLOCK_TYPE,
	OBJ_CLOCK_FADING_TYPE
};

// Rabbit

typedef struct {
	GameObject header;
	bool touching_ground;
	bool jump_off_mushroom;
	bool is_diving;
	Anim* anim;
	Vector2 bounce_force;
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
	float oh, h, dh;
	float damage;
} ObjMushroom;

extern GameObjectDesc obj_mushroom_desc;

#endif

