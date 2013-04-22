#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#define character_count 4

#include "obj_types.h"

typedef struct {
	const char* name;
	const char* icon;
	const char* spr_handle;
	const char* animation;
	const char* description;
	uint cost;
	Color minimap_color;
	float speed;
	float xjump;
	float yjump;
} CharacterDefaultParams;

typedef struct {
	const char* name;
	const char* animation;	
	SprHandle sprite;
	Color minimap_color;
	float speed;
	float xjump;
	float yjump;
	ControlCallback control;
	uint ai_max_combo;
} CharacterParams;

extern CharacterDefaultParams default_characters[];

#endif
