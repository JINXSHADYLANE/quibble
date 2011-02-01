#ifndef OBJECTS_H
#define OBJECTS_H

#include <utils.h>
#include <darray.h>
#include <tilemap.h>

typedef enum {
	obj_crate = 0,
	obj_beacon,
	obj_button,
	obj_battery,
	obj_count
} ObjectType;

typedef struct {
	ObjectType type;
	RectF rect;
	uint button_beacon;
	bool beacon_taken;
	union {
		bool button_state;
		float beacon_intensity;
		bool battery_taken;
	} data;	
} Object;

void objects_reset(Tilemap* map);
void objects_close(void);
void objects_add(ObjectType type, Vector2 pos);
void objects_seal(RectF player);
RectF objects_move_player(Vector2 offset, bool* battery);
void objects_get(ObjectType type, RectF screen, DArray dest);

#endif

