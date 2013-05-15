#include "fadeout.h"
#include "common.h"

#include <sprsheet.h>

bool fadeout_update(void) {
	malka_states_replace("game");
	return true;
}

bool fadeout_render(float t) {
	Color c = COLOR_FTRANSP(1.0f - fabsf(t));
	c &= 0xFF000000;

	RectF dest = {0.0f, 0.0f, v_width, v_height};
	spr_draw("empty", fadeout_layer, dest, c);
	
	return true;
}

StateDesc fadeout_state = {
	.update = fadeout_update,
	.render = fadeout_render
};
