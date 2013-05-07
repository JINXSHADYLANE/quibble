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

extern ObjRabbit* player;

static ScreenType screen = WIN_SCREEN;

float game_over_anim_start = 0.0f;
float game_over_anim_end = 0.0f;

uint coins_earned = 0;

static void game_over_init(void) {

}

static void game_over_close(void) {

}

static void game_over_preenter(void) {
	if(screen == WIN_SCREEN || screen == TUTORIAL_SCREEN ) levels_unlock_next();
	game_over_anim_start = time_s() + 0.6f;
	game_over_anim_end = time_s() + 0.9f;
	coins_earned = 0;

	uint place = minimap_get_place_of_rabbit(player);
	player->data->tokens += 10 * (4-place);
	keyval_set_int("coins",coins + player->data->tokens);

	if(screen == TUTORIAL_SCREEN) levels_set_place(5);	
}

static void game_over_enter(void) {	
}

static void game_over_leave(void) {
	shop_reset();
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
	.postleave = NULL,
	.update = game_over_update,
	.render = game_over_render
};
