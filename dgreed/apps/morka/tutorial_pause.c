#include "tutorial_pause.h"
#include "game.h"
#include "common.h"
#include "tutorials.h"
#include <uidesc.h>
#include <vfont.h>

extern bool button_click;

static void tutorial_pause_init(void) {
}

static void tutorial_pause_close(void) {
}

static void tutorial_pause_enter(void) {
}

static void tutorial_pause_leave(void) {
}

static bool tutorial_pause_update(void) {

	return true;
}

static bool tutorial_pause_render(float t) {
	// Game scene
	if(t != 0) {
		minimap_update_places();
		game_update();
	}
	if(t == 0) game_render(0);
	tutorials_render(t);

	return true;
}

StateDesc tutorial_pause_state = {
	.init = tutorial_pause_init,
	.close = tutorial_pause_close,
	.enter = tutorial_pause_enter,
	.leave = tutorial_pause_leave,
	.update = tutorial_pause_update,
	.render = tutorial_pause_render
};
