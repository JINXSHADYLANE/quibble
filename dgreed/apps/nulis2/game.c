#include "game.h"

#include "sim.h"

StateDesc game_state = {
	.init = game_init,
	.close = game_close,
	.enter = game_enter,
	.leave = game_leave,
	.update = game_update,
	.render = game_render
};

void game_init(void) {
	sim_init(1024, 768);
	sim_reset("level");
}

void game_close(void) {
	sim_close();
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
