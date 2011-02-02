#ifndef BIND_H
#define BIND_H

/*

Interface:

- cobjects.obj_crate, cobjects.obj_beacon, cobjects.obj_button, cobjects.obj_battery
	object type constants

- cobjects.reset(tilemaphandle)

- cobjects.close()

- cobjects.add(type, pos)

- cobjects.seal(player_rect)

- cobjects.move_player(offset)
	returns new player rect and boolean indicating if battery was taken

- cobjects.get_crates(screen)
	returns table of visible crate positions

- cobjects.get_beacons(screen)
	returns table of beacons - each has members pos and intensity

- cobjects.get_batteries(screen)
	returns table of visible battery positions

- cobjects.get_buttons(screen)
	returns table of buttons - eache has members pos and state

- clighting.init(screen)

- clighting.close()

- clighting.render(layer, lights)
	lights is a table of tables with fields pos, radius and alpha
*/	

#include <malka/malka.h>

int bind_open_bulb(lua_State* l);
void bind_close_bulb(void);

#endif
