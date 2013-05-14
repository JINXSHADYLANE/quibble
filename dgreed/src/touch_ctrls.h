#ifndef TOUCH_CTRLS
#define TOUCH_CTRLS

#include "utils.h"
#include "system.h"

// Multitouch (where it's supported) control helpers

// Draws interactive joystick, with a reasonable interaction area and
// a deadzone. Returns offset vector with length less than 1.0
Vector2 touch_joystick(TexHandle tex, uint layer, const RectF* back_src, 
	const RectF* nub_src, const Vector2* pos);

// Draws interactive button, returns true when it's being pressed
bool touch_button(TexHandle tex, uint layer, const RectF* src, 
	const Vector2* pos);

// Invisible movable object, useful for adjustable controls
Vector2 touch_movable(const Vector2* pos, float radius);

#endif
