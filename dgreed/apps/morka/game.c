#include "game.h"
#include "objects.h"
#include "obj_rabbit.h"

// Game state

static void game_init(void) {
	objects_init();

	objects_create(&obj_rabbit_desc, vec2(512.0f, 384.0f), NULL);
}

static void game_close(void) {
	objects_close();
}

static void game_enter(void) {
}

static void game_leave(void) {
}

static bool game_update(void) {
	return true;
}

static bool game_render(float t) {
	objects_tick();
	return true;
}

StateDesc game_state = {
	.init = game_init,
	.close = game_close,
	.enter = game_enter,
	.leave = game_leave,
	.update = game_update,
	.render = game_render
};
