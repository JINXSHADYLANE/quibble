#ifndef EFFECTS_H
#define EFFECTS_H

#include <utils.h>

void effects_init(void);
void effects_close(void);
void effects_update(void);
void effects_render(void);
void effects_force_field(Vector2 p, float r, bool push);
void effects_collide_ab(Vector2 p, float dir);
void effects_collide_aab(Vector2 p);
void effects_collide_aa(Vector2 p);
void effects_collide_aaa(Vector2 p);
void effects_destroy(Vector2 p);
void effects_spawn(Vector2 p);
void effects_win(void);

#endif
