#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#define character_count 4

#include "obj_types.h"

typedef struct {
	const char* name;
	const char* icon;
	const char* shield_spr;	
	const char* spr_handle;
	const char* animation;
	const char* description;
	uint cost;
	float speed;
	float xjump;
	float yjump;
	float sprite_offset;
	Vector2 shield_offset;
	Vector2 shield_scale;		
} CharacterDefaultParams;

typedef struct {
	const char* name;
	const char* animation;	
	SprHandle sprite;
	SprHandle shield_spr;
	float speed;
	float xjump;
	float yjump;
	ControlCallback control;
	uint ai_max_combo;
	float sprite_offset;
	Vector2 shield_offset;
	Vector2 shield_scale;	
} CharacterParams;

extern CharacterDefaultParams default_characters[];

#endif
