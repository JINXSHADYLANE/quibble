#include "game.h"
#include "objects.h"
#include "obj_types.h"

#include "worldgen.h"
#include "hud.h"

extern bool draw_gfx_debug;
extern bool draw_physics_debug;

// Game state

float camera_speed = 100.0f;
uint last_page = 0;

ObjRabbit* rabbit = NULL;
float camera_follow_weight;
float rabbit_remaining_time;
float rabbit_distance;
float rabbit_current_distance;
float bg_scroll = 0.0f;

bool game_over;
bool game_paused = false;

static void game_init(void) {
	objects_init();
	hud_init();

	game_reset();
}

bool game_is_paused(void) {
	return game_paused;
}

void game_pause(void) {
	game_paused = true;
}

void game_unpause(void) {
	game_paused = false;
}

void game_reset(void) {
	if(rabbit) {
		objects_destroy_all();
	}

	rabbit = (ObjRabbit*)objects_create(&obj_rabbit_desc, vec2(512.0f, 384.0f), NULL);
	worldgen_reset(rand_uint());

	camera_follow_weight = 0.2f;
	rabbit_remaining_time = 45.0f;
	game_over = false;
}

static void game_close(void) {
	worldgen_close();
	hud_close();
	objects_close();
}

static void game_enter(void) {
}

static void game_leave(void) {
}

static bool game_update(void) {
#ifndef NO_DEVMODE
	if(char_down('g'))
		draw_gfx_debug = !draw_gfx_debug;
	if(char_down('p'))
		draw_physics_debug = !draw_physics_debug;
#endif

	if(game_is_paused())
		return true;

	if(rabbit_remaining_time <= 0.0f) {
		rabbit_remaining_time = 0.0f;

		if(!game_over)
			rabbit_distance = rabbit_current_distance;

		game_over = true;

		// Game over
		camera_follow_weight *= 0.95f;
	}

	if(rabbit && rabbit->header.type) {
		// Make camera follow rabbit
		if(!game_over)
			rabbit_current_distance = rabbit->header.render->world_dest.left / (1024.0f / 3.0f) - 2.0f;
		float camera_x = (objects_camera[0].left*8.0f + objects_camera[0].right) / 9.0f;
		float new_camera_x = lerp(camera_x, rabbit->header.render->world_dest.left + 45.0f, camera_follow_weight);
		float camera_offset = new_camera_x - camera_x;
		objects_camera[0].left += camera_offset;
		objects_camera[0].right += camera_offset;
		objects_camera[1].left += camera_offset/2.0f;
		objects_camera[1].right += camera_offset/2.0f;
		bg_scroll += camera_offset/8.0f;
	}

	worldgen_update(objects_camera[0].right, objects_camera[1].right);

	rabbit_remaining_time -= time_delta() / 1000.0f;

	return true;
}

static bool game_render(float t) {
	// Draw scrolling background
	float off_x = fmodf(bg_scroll, 1024.0f);
	RectF dest = rectf(-off_x, 0.0f, 0.0f, 0.0f);
	spr_draw("background", 0, dest, COLOR_WHITE);
	dest = rectf(1024.0f - off_x, 0.0f, 0.0f, 0.0f);
	spr_draw("background", 0, dest, COLOR_WHITE);

	hud_render();
	objects_tick(game_is_paused());

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
