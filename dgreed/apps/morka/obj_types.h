#ifndef OBJ_TYPES_H
#define OBJ_TYPES_H

#include <anim.h>

#include "objects.h"

enum {
	OBJ_RABBIT_TYPE = 1,
	OBJ_GROUND_TYPE,
	OBJ_DECO_TYPE,
	OBJ_FG_DECO_TYPE,
	OBJ_MUSHROOM_TYPE,
	OBJ_TRAMPOLINE_TYPE,
	OBJ_CACTUS_TYPE,
	OBJ_PIN,
	OBJ_SPEED_TRIGGER_TYPE,
	OBJ_FALL_TRIGGER_TYPE,
	OBJ_PARTICLE_ANCHOR_TYPE,
	OBJ_BG_PARTICLE_ANCHOR_TYPE,
	OBJ_TOKEN_TYPE
};

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
	float land;
	float rabbit_time;
	const char* rabbit_name; 
	bool game_over;
	bool rubber_band;
	int ai_max_combo;
	int boost;
	Color minimap_color;
	float speed;
	float xjump;
	float yjump;
	int tokens;
	bool has_trampoline;
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

// Race marker pin

typedef struct {
	GameObject header;
	float speed;
} ObjPin;

extern GameObjectDesc obj_pin_desc;

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

// Token (coin)

typedef struct {
	GameObject header;
	int value;
} ObjToken;

extern GameObjectDesc obj_token_desc;

#endif

