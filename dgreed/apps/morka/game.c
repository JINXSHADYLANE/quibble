#include "game.h"
#include "objects.h"
#include "obj_rabbit.h"
#include "obj_ground.h"

// Game state

static void game_init(void) {
	objects_init();

	video_set_blendmode(15, BM_MULTIPLY);

	objects_create(&obj_rabbit_desc, vec2(512.0f, 384.0f), NULL);

	objects_create(&obj_ground_desc, vec2(128.0f, 683.0f), (void*)0);
	objects_create(&obj_ground_desc, vec2(384.0f, 683.0f), (void*)1);
	objects_create(&obj_ground_desc, vec2(640.0f, 683.0f), (void*)2);
	objects_create(&obj_ground_desc, vec2(896.0f, 683.0f), (void*)3);
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
	spr_draw("background", 0, rectf_null(), COLOR_WHITE);
	spr_draw("vignette", 15, rectf_null(), COLOR_WHITE);

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
