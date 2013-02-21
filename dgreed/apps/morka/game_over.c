#include "game_over.h"
#include "obj_types.h"
#include "minimap.h"
#include "hud.h"
#include "common.h"
#include "game.h"
#include <uidesc.h>
#include <vfont.h>

static ScreenType screen = SCORES_SCREEN;

static void game_over_init(void) {

}

static void game_over_close(void) {

}

static void game_over_preenter(void) {

}

static void game_over_enter(void) {
}

static void game_over_leave(void) {
}

static bool game_over_update(void) {
	game_update_empty();
	minimap_update_places();	
	return true;
}

void game_over_set_screen(ScreenType s){
	screen = s;
}

static bool game_over_render(float t) {
	// Game scene
	if(t != 0) {
		game_update_empty();
		minimap_update_places();	
	}
	if(t == 0){
		game_render(0);
	}
	switch(screen){
		case TUTORIAL_SCREEN:
			hud_render_game_over_tut(t);
		break;
		case OUT_SCREEN:
			hud_render_game_over_out(t);
		break;
		case SCORES_SCREEN:
			hud_render_game_over_scores(t);
		break;
	} 

	return true;
}

StateDesc game_over_state = {
	.init = game_over_init,
	.close = game_over_close,
	.enter = game_over_enter,
	.preenter = game_over_preenter,
	.leave = game_over_leave,
	.update = game_over_update,
	.render = game_over_render
};
