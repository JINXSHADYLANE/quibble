#ifndef SIM_H
#define SIM_H

#include <utils.h>

void sim_init(void);
void sim_close(void);
void sim_reset(float spawn_interval, uint start_spawning_at);
void sim_spawn(Vector2 p, Vector2 v, uint type, float t);
void sim_spawn_random(void);
void sim_force_field(Vector2 p, float r, float strength);
void sim_update(void);
void sim_render(void);
int sim_count_alive(void);
int sim_count_ghosts(void);
int sim_total_mass(void);
void sim_destroy(void);

#endif
