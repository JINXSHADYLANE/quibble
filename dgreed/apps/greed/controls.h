#ifndef CONTROLS_H
#define CONTROLS_H

#include <utils.h>

void controls_init(void);
void controls_close(void);

void controls_draw(float fadein);
void controls_update(uint ship);
void controls_adjust(void);

#endif
