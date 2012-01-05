#ifndef SIM_H
#define SIM_H

#include <utils.h>
#include <coldet.h>

typedef enum {
	BT_WHITE = 1,
	BT_PAIR = 2,
	BT_GRAV = 4
	BT_TIME = 8
} BallType;

typedef struct {
	Vector2 pos, old_pos, force;
	float angle, old_angle, radius, inv_mass;
	BallType type;
	CDObj* collider;
} Ball;

void sim_init(void);
void sim_close(void);
void sim_reset(uint screen_width, uint screen_height, const char* level);
const char* sim_level(void);
void sim_update(void);
void sim_render(void);

#endif
