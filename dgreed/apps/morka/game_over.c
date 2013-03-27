#include "game_over.h"
#include "obj_types.h"
#include "minimap.h"
#include "hud.h"
#include "common.h"
#include "game.h"
#include "shop.h"
#include "level_select.h"
#include <keyval.h>
#include <uidesc.h>
#include <vfont.h>

static ScreenType screen = WIN_SCREEN;

static void game_over_init(void) {

}

static void game_over_close(void) {

}

static void game_over_preenter(void) {
	keyval_set_int("coins",coins);
	if(screen == WIN_SCREEN || screen == TUTORIAL_SCREEN ) levels_unlock_next();
}

static void game_over_enter(void) {

}

static void game_over_leave(void) {
}

static void game_over_postleave(void) {
	level_select_set_season(levels_current_desc()->season);
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
	switch(screen){
		case TUTORIAL_SCREEN:
			hud_render_game_over_tut(t);
		break;
		case OUT_SCREEN:
			hud_render_game_over_out(t);
		break;
		case WIN_SCREEN:
			hud_render_game_over_win(t);
		break;
		case LOSE_SCREEN:
			hud_render_game_over_lose(t);
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
	.postleave = game_over_postleave,
	.update = game_over_update,
	.render = game_over_render
};
