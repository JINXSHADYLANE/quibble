#include "pause.h"
#include "game.h"
#include "hud.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

typedef enum{
	REGULAR_PAUSE = 0,
	TUTORIAL_PAUSE,
} PauseScreenType;

static PauseScreenType screen = REGULAR_PAUSE;

extern bool button_click;

static void pause_init(void) {
}

static void pause_close(void) {
}

static void pause_enter(void) {
}

static void pause_preenter(void) {
	const char* parent =  malka_states_at(1);
	if(strcmp(parent,"tutorial_pause") == 0) screen = TUTORIAL_PAUSE;
	else screen = REGULAR_PAUSE;
}

static void pause_leave(void) {
}

static bool pause_update(void) {
	return true;
}

static bool pause_render(float t) {
	switch(screen){
		case TUTORIAL_PAUSE:
			hud_render_tutorial_pause(t); 
		break;
		default:
			hud_render_regular_pause(t); 
		break;
	}	
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
