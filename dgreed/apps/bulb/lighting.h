#ifndef LIGHTING_H
#define LIGHTING_H

#include <utils.h>
#include <darray.h>

typedef struct {
	Vector2 pos;
	float radius;
	float alpha;
} Light;

void lighting_init(RectF screen);
void lighting_close(void);
void lighting_render(uint layer, DArray* lights);

#endif
