#include "game.h"

#include "common.h"
#include "levels.h"
#include "sim.h"

#include <system.h>

SoundHandle music;

StateDesc game_state = {
	.init = game_init,
	.close = game_close,
	.enter = game_enter,
	.leave = game_leave,
	.update = game_update,
	.render = game_render
};

void game_init(void) {
	levels_reset(ASSETS_PRE "levels.mml");
	
	sim_init(1024, 768);
	sim_reset("l1");
}

void game_close(void) {
	sim_close();

	levels_close();
}

void game_enter(void) {
}

void game_leave(void) {
}

bool game_update(void) {
	sim_update();
	return true;
}

bool game_render(float t) {
	sim_render();
	return true;
}
