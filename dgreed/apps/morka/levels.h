#ifndef LEVELS_H
#define LEVELS_H

#include "utils.h"
#include <darray.h>
#include <sprsheet.h>

/*
(levels morka
	(level first
		(distance 100)
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
} LevelDesc;

extern DArray levels_descs;

void levels_init(const char* filename);
void levels_close(void);

void levels_reset(const char* level_name);
LevelDesc* levels_current_desc(void);

#endif
