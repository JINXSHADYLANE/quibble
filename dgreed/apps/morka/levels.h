#ifndef LEVELS_H
#define LEVELS_H

#include "utils.h"
#include "obj_types.h"
#include <darray.h>
#include <sprsheet.h>

/*
(levels morka
	(level first
		(distance 100)
		(rabbits _
			(name Tori
				(spr rabbit_2)
				(speed 459)
				(xjump 100)
				(yjump 450)
				(max_combo 2)
			)
		)
		(powerups _
			(rocket _
				(num 3)
				(xmin 30)
				(xmax 60)
			)
		)		
		(background background)
		(bg_chain bg)
		(fg_chain fg)
		(ground_chain ground)
	)
)
*/

typedef enum{
	AUTUMN = 0,
	WINTER,
	SPRING,
	SUMMER
} TilesetType;

typedef struct {
	TilesetType tileset;
	const char* name;
	int distance;
	SprHandle background;
	const char* bg_chain;
	const char* fg_chain;
	const char* ground_chain;
	const char* tree_chain;
	uint ai_rabbit_num;
	// ai rabbit data
	const char* ai_rabbit_names[3];
	SprHandle ai_rabbit_spr[3];
	Color ai_rabbit_colors[3];
	float ai_rabbit_speeds[3];
	float ai_rabbit_xjumps[3];
	float ai_rabbit_yjumps[3];
	int ai_max_combo[3];
	// powerup data
	uint powerup_num[POWERUP_COUNT];
	Vector2 powerup_pos[POWERUP_COUNT];

} LevelDesc;

void levels_init(const char* filename);
void levels_close(void);

void levels_reset(const char* level_name);
LevelDesc* levels_current_desc(void);

void levels_set_next(void);
bool levels_is_final(void);

uint levels_get_powerup_count(void);

#endif
