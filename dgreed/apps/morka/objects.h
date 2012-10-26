#ifndef OBJECTS_H
#define OBJECTS_H

#include <sprsheet.h>
#include <coldet.h>

#define PHYSICS_DT (1.0 / 60.0)

// Game object

typedef struct {
	uint32 type;
	struct FwdPhysicsComponent* physics;
	struct FwdRenderComponent* render;
	struct FwdUpdateComponent* update;
} GameObject;

// Physics component

typedef void (*PhysicsHitCallback)(GameObject* self, GameObject* collider);

typedef struct FwdPhysicsComponent {
	// Data
	GameObject* owner;
	CDObj* cd_obj;
	float inv_mass;
	Vector2 vel, acc;
	
	// Methods
	PhysicsHitCallback hit_callback;
} PhysicsComponent;

// Render component

typedef void (*RenderCallback)(GameObject* self);

typedef struct FwdRenderComponent {
	// Data
	GameObject* owner;
	float extent_min, extent_max;
	Vector2 world_pos;
	float scale, angle;
	uint16 layer;
	uint16 anim_frame;
	SprHandle spr;
	bool was_visible;

	// Methods
	RenderCallback update_pos;
	RenderCallback pre_render;
	RenderCallback post_render;
	RenderCallback became_visible;
	RenderCallback became_invisible;
} RenderComponent;

// Update component

typedef void (*UpdateCallback)(GameObject* self, float ts, float dt);

typedef struct FwdUpdateComponent {
	// Data
	GameObject* owner;
	uint interval;
	uint last_update_tick;

	// Methods
	UpdateCallback update;
} UpdateComponent;

// GameObject descriptor

typedef void (*GameObjectConstruct)(GameObject* self, Vector2 pos, void* user_data);
typedef void (*GameObjectDestruct)(GameObject* self);

typedef struct {
	uint32 type;
	uint32 size;

	bool has_physics;
	bool has_render;
	bool has_update;

	GameObjectConstruct construct;
	GameObjectDestruct destruct;
	
} GameObjectDesc;

// Public api:

extern CDWorld* objects_cdworld;
extern RectF objects_camera;

void objects_init(void);
void objects_close(void);

GameObject* objects_create(GameObjectDesc* desc, Vector2 pos, void* user_data);
void objects_destroy(GameObject* obj);

void objects_apply_force(GameObject* obj, Vector2 f);

void objects_tick(void);

#endif
