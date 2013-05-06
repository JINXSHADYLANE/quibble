#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include <anim.h>

#include "objects.h"

#define collision_flag (1U << 30)

//1,2,4,8,10,20,40,80,100,200,400,800
enum {
	OBJ_GROUND_TYPE 		= 0x01 | collision_flag,
	OBJ_MUSHROOM_TYPE 		= 0x02 | collision_flag,
	OBJ_TRAMPOLINE_TYPE 	= 0x04 | collision_flag,
	OBJ_CACTUS_TYPE 		= 0x08 | collision_flag,
	OBJ_FALL_TRIGGER_TYPE 	= 0x10 | collision_flag,
	OBJ_BRANCH_TYPE 		= 0x20 | collision_flag,
	OBJ_SPIKE_BRANCH_TYPE 	= 0x40 | collision_flag,
	OBJ_SPRING_BRANCH_TYPE 	= 0x80 | collision_flag,
	OBJ_RABBIT_TYPE 		= 0x100 | collision_flag,
	OBJ_SPEED_TRIGGER_TYPE	= 0x200 | collision_flag,
	OBJ_ERASER_TYPE			= 0x400 | collision_flag,
	OBJ_POWERUP_TYPE		= 0x800 | collision_flag,
	OBJ_BOMB_TYPE			= 0x1000 | collision_flag
};
enum {
	OBJ_DECO_TYPE 				=  1 << 16,
	OBJ_FG_DECO_TYPE 			=  2 << 16,
	OBJ_TRUNK_TYPE				=  3 << 16,
	OBJ_PARTICLE_ANCHOR_TYPE	=  4 << 16,
	OBJ_BG_PARTICLE_ANCHOR_TYPE	=  5 << 16,
	OBJ_FLOATER_TYPE			=  6 << 16,	
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
	const char* name;
	const char* description;	
	const char* spr;
	const char* btn;
	const char* unlocked_spr;
	bool passive;
	PhysicsHitCallback hit_callback;
	PowerupCallback powerup_callback;
	RenderCallback update_pos;
	uint cost;	
} PowerupParams;

extern PowerupParams powerup_params[];

extern PowerupParams coin_powerup;

typedef struct {
	GameObject header;
	float prev_r;
} ObjPowerup;

extern GameObjectDesc obj_powerup_desc;

// Rabbit Data

typedef struct {
	// State parameters
	float last_keypress_t;
	float last_keyrelease_t;
	float jump_time;
	float mushroom_hit_time;
	bool virtual_key_up;
	bool virtual_key_down;
	bool virtual_key_pressed;
	Vector2 bounce_force;
	bool game_over;
	bool is_dead;
	bool falling_down;
	bool force_jump;
	bool force_dive;
	bool input_disabled;
	bool touching_ground;
	bool jump_off_mushroom;
	bool is_diving;
	bool on_water;	
	bool jump_out;
	bool jumped;
	bool dived;
	bool spike_hit;
	float respawn;
	uint last_frame;

	// Winter season
	bool over_branch;
	bool collision_update;	
	
	// AI
	bool player_control;
	int ai_max_combo;
	Vector2 land;
	Vector2 dive;
	GameObject* previuos_hit;

	// Stats
	const char* rabbit_name;
	int tokens; 
	float rabbit_time;
	uint combo_counter;
	float speed;
	float xjump;
	float yjump;
	uint boost;
	float sprite_offset;
	
	// Powerup parameters
	bool has_powerup[POWERUP_COUNT];
	bool trampoline_placed;
	bool rocket_start;
	float rocket_time;
	float shield_dh;
	float shield_h;
	SprHandle shield_spr;
	Vector2 shield_offset;
	Vector2 shield_scale;
	float skunk_gas_cd;

} ObjRabbitData;

typedef void (*ControlCallback)(GameObject* self);

void obj_rabbit_player_control(GameObject* self);

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

#endif
