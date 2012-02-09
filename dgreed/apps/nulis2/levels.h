#ifndef LEVELS_H
#define LEVELS_H

#include <utils.h>

#define LEVEL_NAMELEN 4 
#define MAX_SPAWNS 32 

/*
Example:

(levels _
	(level l1
		(par_time 200)
		(par_reactions 20)
		(random_at 5)
		(random_interval 1.0)
		(spawns _
			(b 100,100 (vel -1,-1) (t 2.1))
			(w -100,100 (t 0.1))
			(gw 0,0 (t 3) (s 2))
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
	float t, s;
	BallType type;
} SpawnDef;

typedef struct {
	char name[LEVEL_NAMELEN];
	uint n_spawns;
	SpawnDef spawns[MAX_SPAWNS];
	uint spawn_random_at;
	float spawn_random_interval;
	uint par_time;
	uint par_reactions;
} LevelDef;

void levels_reset(const char* desc);
void levels_parse_ed(const char* desc);
void levels_get(const char* name, LevelDef* def);
const char* levels_next(const char* current);
void levels_close(void);

bool level_is_unlocked(const char* name);
bool level_is_unlocked_n(uint n);
bool level_is_solved(const char* name);
void level_solve(const char* name, uint reactions, uint time);
float level_score(const char* name);
float level_score_n(uint n);
uint levels_total_score(void);
const char* level_first_unsolved(void);

#endif
