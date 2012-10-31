#include "game.h"
#include "objects.h"
#include "obj_types.h"

#include "worldgen.h"

extern bool draw_gfx_debug;
extern bool draw_physics_debug;

// Game state

float camera_speed = 100.0f;
uint last_page = 0;

ObjRabbit* rabbit = NULL;

static void game_init(void) {
	objects_init();

	video_set_blendmode(15, BM_MULTIPLY);

	rabbit = (ObjRabbit*)objects_create(&obj_rabbit_desc, vec2(512.0f, 384.0f), NULL);

	worldgen_reset(20);
}

static void game_close(void) {
	worldgen_close();
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
	
	// Make camera follow rabbit
	if(rabbit && rabbit->header.type) {
		float camera_x = (objects_camera[0].left*3.0f + objects_camera[0].right) / 4.0f;
		float new_camera_x = lerp(camera_x, rabbit->header.render->world_pos.x, 0.2f);
		float camera_offset = new_camera_x - camera_x;
		objects_camera[0].left += camera_offset;
		objects_camera[0].right += camera_offset;
		objects_camera[1].left += camera_offset/2.0f;
		objects_camera[1].right += camera_offset/2.0f;
	}

	worldgen_update(objects_camera[0].right, objects_camera[1].right);

	return true;
}

static bool game_render(float t) {
	spr_draw("background", 0, rectf_null(), COLOR_WHITE);

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
