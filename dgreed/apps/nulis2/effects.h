#ifndef EFFECTS_H
#define EFFECTS_H

#include <utils.h>

void effects_init(const char* desc);
void effects_close(void);

void effects_trigger(const char* name);
void effects_trigger_ex(const char* name, Vector2 pos, float dir);

#endif
