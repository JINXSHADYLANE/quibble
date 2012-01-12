#ifndef LEVELS_H
#define LEVELS_H

#include <utils.h>

#define LEVEL_NAMELEN 16
#define MAX_SPAWNS 16

/*
Example:

(levels _
	(level l1
		(random_at 5)
		(random_interval 1.0)
		(spawns _
			(b 100,100 (vel -1,-1) (t 2.1))
			(w -100,100 (t 0.1))
		)
	)
)

Ball types:
w, b, dw, db, gw, gb, tw, tb

*/

typedef enum {
	BT_WHITE = 1,
	BT_PAIR = 2,
	BT_GRAV = 4,
	BT_TIME = 8, 
	BT_TRIPLE = 16
} BallType;

typedef struct {
	Vector2 pos, vel;
	float t;
	BallType type;
} SpawnDef;

typedef struct {
	char name[LEVEL_NAMELEN];
	uint n_spawns;
	SpawnDef spawns[MAX_SPAWNS];
	uint spawn_random_at;
	float spawn_random_interval;
} LevelDef;

void levels_reset(const char* desc);
void levels_get(const char* name, LevelDef* def);
void levels_close(void);

#endif
