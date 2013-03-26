#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include <anim.h>

#include "objects.h"

//1,2,4,8,10,20,40,80,100,200,400,800
enum {
	OBJ_GROUND_TYPE = 0x01,
	OBJ_MUSHROOM_TYPE = 0x02,
	OBJ_TRAMPOLINE_TYPE = 0x04,
	OBJ_CACTUS_TYPE = 0x08,
	OBJ_FALL_TRIGGER_TYPE = 0x10,
	OBJ_BRANCH_TYPE = 0x20,
	OBJ_SPIKE_BRANCH_TYPE = 0x40,
	OBJ_SPRING_BRANCH_TYPE = 0x80
};
enum {
	OBJ_RABBIT_TYPE 			=  1 << 8,
	OBJ_DECO_TYPE 				=  2 << 8,
	OBJ_FG_DECO_TYPE 			=  3 << 8,
	OBJ_SPEED_TRIGGER_TYPE		=  4 << 8,
	OBJ_ERASER_TYPE				=  5 << 8,
	OBJ_TRUNK_TYPE				=  6 << 8,
	OBJ_PARTICLE_ANCHOR_TYPE	=  7 << 8,
	OBJ_BG_PARTICLE_ANCHOR_TYPE	=  8 << 8,
	OBJ_FLOATER_TYPE			=  9 << 8,
	OBJ_POWERUP_TYPE			= 10 << 8,
	OBJ_BOMB_TYPE				= 11 << 8			
};

// Powerup

typedef enum{
	TRAMPOLINE = 0,
	SHIELD,
	BOMB,
	ROCKET,

	POWERUP_COUNT 
} PowerupType;

typedef void (*PowerupCallback)(GameObject* other);

typedef struct {
	const char* spr;
	const char* btn;
	bool passive;
	PhysicsHitCallback hit_callback;
	PowerupCallback powerup_callback;
} PowerupParams;

extern PowerupParams powerup_params[];

extern PowerupParams coin_powerup;

typedef struct {
	GameObject header;
} ObjPowerup;

extern GameObjectDesc obj_powerup_desc;

// Rabbit Data

typedef struct {
	uint combo_counter;
	float last_keypress_t;
	float last_keyrelease_t;
	float jump_time;
	float mushroom_hit_time;

	bool virtual_key_up;
	bool virtual_key_down;
	bool virtual_key_pressed;

	bool touching_ground;
	bool jump_off_mushroom;
	bool is_diving;
	bool is_dead;
	bool on_water;
	Vector2 bounce_force;
	bool player_control;
	bool falling_down;
	Vector2 land;
	Vector2 dive;
	GameObject* previuos_hit;
	float rabbit_time;
	const char* rabbit_name; 
	bool game_over;
	int ai_max_combo;
	int boost;
	Color minimap_color;
	float speed;
	float xjump;
	float yjump;
	int tokens;
	bool force_jump;
	bool force_dive;
	bool input_disabled;

	bool has_powerup[POWERUP_COUNT];
	bool trampoline_placed;
	bool rocket_start;
	float rocket_time;
	float shield_dh;
	float shield_h;

	bool jumped;
	bool dived;
	bool spike_hit;

	uint last_frame;
} ObjRabbitData;

typedef void (*ControlCallback)(GameObject* self);

// Rabbit

typedef struct {
	GameObject header;
	Anim* anim;
	ControlCallback control;
	ObjRabbitData* data;
} ObjRabbit;

extern GameObjectDesc obj_rabbit_desc;

// Ground

typedef struct {
	GameObject header;
} ObjGround;

extern GameObjectDesc obj_ground_desc;

// Background deco (static image, destroys itself when not visible)

typedef struct {
	GameObject header;
} ObjDeco;

extern GameObjectDesc obj_deco_desc;

// Foreground Deco (static image, destroys itself when not visible)

typedef struct {
	GameObject header;
} ObjFgDeco;

extern GameObjectDesc obj_fg_deco_desc;

// Tree trunk

typedef struct {
	GameObject header;
} ObjTrunk;

extern GameObjectDesc obj_trunk_desc;

// Mushroom

typedef struct {
	GameObject header;
	float oh, h, dh;
} ObjMushroom;

extern GameObjectDesc obj_mushroom_desc;

// Cactus (Spikeshroom)

typedef struct {
	GameObject header;
	float t0,t1,d;
	RectF original;
	float damage;
} ObjCactus;

extern GameObjectDesc obj_cactus_desc;

// Trampoline

typedef struct {
	GameObject header;
	float oh, h, dh, db;
	GameObject * owner;
} ObjTrampoline;

extern GameObjectDesc obj_trampoline_desc;

// Fall trigger

typedef struct {
	GameObject header;
} ObjFallTrigger;

extern GameObjectDesc obj_fall_trigger_desc;

// Speed trigger

typedef struct {
	GameObject header;
	float drag_coef;
} ObjSpeedTrigger;

extern GameObjectDesc obj_speed_trigger_desc;

// Particle anchor

typedef struct {
	GameObject header;
	Vector2 pos;
	Vector2 screen_pos;
} ObjParticleAnchor;

extern GameObjectDesc obj_particle_anchor_desc;

// Background Particle anchor

typedef struct {
	GameObject header;
	Vector2 pos;
	Vector2 screen_pos;
} ObjBgParticleAnchor;

extern GameObjectDesc obj_bg_particle_anchor_desc;

// Floating text/img object

typedef struct {
	const char* spr;
	const char* text;
	float duration;
} ObjFloaterParams;

typedef struct {
	GameObject header;
	Vector2 txt_pos;
	float duration, t0;
	char text[16];
} ObjFloater;

extern GameObjectDesc obj_floater_desc;

// Bomb

typedef struct {
	GameObject header;
	GameObject * owner;
} ObjBomb;

extern GameObjectDesc obj_bomb_desc;

// Eraser

typedef struct {
	GameObject header;
} ObjEraser;

extern GameObjectDesc obj_eraser_desc;

// Branch

typedef struct {
	GameObject header;
	float oh, h, dh;
} ObjBranch;

extern GameObjectDesc obj_branch_desc;

// Spike Branch

typedef struct {
	GameObject header;
	float oh, h, dh;
} ObjSpikeBranch;

extern GameObjectDesc obj_spike_branch_desc;

// Spring Branch

typedef struct {
	GameObject header;
	float oh, h, dh;
} ObjSpringBranch;

extern GameObjectDesc obj_spring_branch_desc;

#endif