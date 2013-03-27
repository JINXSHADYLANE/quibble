#include "shop.h"
#include "obj_types.h"
#include "minimap.h"
#include "hud.h"
#include "common.h"
#include "game.h"
#include "level_select.h"
#include <keyval.h>
#include <uidesc.h>
#include <vfont.h>

uint coins = 0;

static void shop_init(void) {

}

static void shop_close(void) {

}

static void shop_preenter(void) {
	coins = keyval_get_int("coins",0);
}

static void shop_enter(void) {
	game_pause();
}

static void shop_leave(void) {
}

static bool shop_update(void) {
	game_update_empty();
	minimap_update_places();	
	return true;
}

static bool shop_render(float t) {
	if(t != 0.0f) game_update_empty();

	hud_render_shop(t);

	return true;
}

StateDesc shop_state = {
	.init = shop_init,
	.close = shop_close,
	.enter = shop_enter,
	.preenter = shop_preenter,
	.leave = shop_leave,
	.update = shop_update,
	.render = shop_render
};
