#ifndef SPRSHEET_H
#define SPRSHEET_H

#include <system.h>

// A layer on top of textures to stop worrying about loading, freeing 
// and source rectangles.

void sprsheet_init(const char* desc);
void sprsheet_close(void);

bool sprsheet_get(const char* name, TexHandle* handle, RectF* src);

#endif
