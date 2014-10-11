#ifndef BIND_H
#define BIND_H

/*

Interface:

- clighting.init(screen)

- clighting.close()

- clighting.render(layer, lights)
	lights is a table of tables with fields pos, radius and alpha

*/	

#include <malka/malka.h>

int bind_open_kovas(lua_State* l);
void bind_close_kovas(void);

#endif
