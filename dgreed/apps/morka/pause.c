#include "pause.h"
#include "game.h"
#include "hud.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

static void pause_init(void) {
}

static void pause_close(void) {
}

static void pause_enter(void) {
}

static void pause_preenter(void) {
}

static void pause_leave(void) {
}

static bool pause_update(void) {
	return true;
}

static bool pause_render(float t) {
	hud_render_regular_pause(t); 
	return true;
}

StateDesc pause_state = {
	.init = pause_init,
	.close = pause_close,
	.enter = pause_enter,
	.preenter = pause_preenter,
	.leave = pause_leave,
	.update = pause_update,
	.render = pause_render
};
