#ifndef LEVELS_H
#define LEVELS_H

#include "utils.h"
#include <darray.h>
#include <sprsheet.h>

/*
(levels morka
	(level first
		(distance 100)
		(rabbits _
			(spr rabbit_2)
		)
		(background background)
		(bg_chain bg)
		(fg_chain fg)
		(ground_chain ground)
	)
)
*/

typedef struct {
	const char* name;
	int distance;
	SprHandle background;
	const char* bg_chain;
	const char* fg_chain;
	const char* ground_chain;
	uint ai_rabbit_num;
	// ai rabbit data
	const char* ai_rabbit_names[3];
	SprHandle ai_rabbit_spr[3];
	Color ai_rabbit_colors[3];
	float ai_rabbit_speeds[3];
	float ai_rabbit_xjumps[3];
	float ai_rabbit_yjumps[3];
	int ai_max_combo[3];

} LevelDesc;

void levels_init(const char* filename);
void levels_close(void);

void levels_reset(const char* level_name);
LevelDesc* levels_current_desc(void);

void levels_set_next(void);
bool levels_is_final(void);

#endif
