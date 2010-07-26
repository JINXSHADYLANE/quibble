#ifndef PHYSICS_H
#define PHYSICS_H

#include <utils.h>
#include <tweakables.h>
#include "game.h"

typedef struct {
	Vector2 pos;
	Vector2 vel;
	float rot;
	float ang_vel;
	float scale;
	float mass;
	float zrot;
	float zrot_vel;
} ShipPhysicalState; 	

typedef struct {
	Vector2 pos;
	float rot;
} BulletPhysicalState;	

typedef struct {
	ShipPhysicalState ships[MAX_SHIPS];
	BulletPhysicalState bullets[MAX_BULLETS];

	uint n_ships;
	uint n_bullets;
} PhysicsState;	

extern PhysicsState physics_state;

void physics_register_tweaks(Tweaks* tweaks);
void physics_init(void);
void physics_close(void);

// TODO: Update this
void physics_reset(uint n_ships);
void physics_spawn_bullet(uint ship);
void physics_set_ship_size(uint ship, float size);
void physics_control_ship(uint ship, bool rot_left, bool rot_right, bool acc);

void physics_update(float dt);
void physics_debug_draw(void);

Vector2 physics_wraparound(Vector2 in);

#endif

