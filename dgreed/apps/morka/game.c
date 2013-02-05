#include "game.h"
#include "objects.h"
#include "obj_types.h"

#include "worldgen.h"
#include "hud.h"

#include "minimap.h"

#include <mfx.h>
#include <particles.h>

#include "common.h"

int level_id = 1;

extern bool draw_gfx_debug;
extern bool draw_physics_debug;
bool draw_ground_debug = false;
bool draw_ai_debug = false;

// Game state

float camera_speed = 100.0f;
uint last_page = 0;

ObjRabbit* rabbit = NULL;
ObjRabbit* ai_rabbit = NULL;
float camera_follow_weight;
float rabbit_current_distance;
float bg_scroll = 0.0f;

bool game_over;
bool game_was_reset = false;
bool game_paused = false;

DArray levels_descs;

static void game_init(void) {
	levels_init(ASSETS_DIR "levels.mml");
	objects_init();

	hud_init();
	minimap_init();

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
	switch(level_id){
		// temp level select
		case 1:
			levels_reset("level1");
		break;
		case 2:
			levels_reset("level2");
		break;
		case 3:
			levels_reset("level3");
		break;
		case 4:
			levels_reset("level4");
		break;
		case 5:
			levels_reset("level5");
		break;
	}
	minimap_reset(levels_current_desc()->distance);

	SprHandle spr = sprsheet_get_handle("rabbit");

	// Player rabbit
	rabbit = (ObjRabbit*)objects_create(&obj_rabbit_desc, vec2(512.0f, 384.0f), (void*)spr);
	minimap_track(rabbit);

	// AI rabbits
	for(int i = 0;i < levels_current_desc()->ai_rabbit_num;i++){
		spr = levels_current_desc()->ai_rabbit_spr[i];
		ai_rabbit = (ObjRabbit*)objects_create(&obj_rabbit_desc, vec2(502.0f+rand_float_range(-32.0f,32.0f),
											 284.0f+rand_float_range(-32.0f,32.0f)), (void*)spr);
		minimap_track(ai_rabbit);
	}	

	worldgen_reset(rand_uint(),levels_current_desc());

	camera_follow_weight = 0.2f;
	rabbit_current_distance = 0.0f;
	game_over = false;
}

static void game_close(void) {
	worldgen_close();
	minimap_close();
	hud_close();
	objects_close();
	levels_close();
}

static void game_enter(void) {
	printf("entering game\n");
}

static void game_leave(void) {
	printf("leaving game\n");	
}

bool game_update(void) {
#ifndef NO_DEVMODE
	if(char_down('g'))
		draw_gfx_debug = !draw_gfx_debug;
	if(char_down('p'))
		draw_physics_debug = !draw_physics_debug;
	if(char_down('l'))
		draw_ground_debug = !draw_ground_debug;
	if(char_down('a'))
		draw_ai_debug = !draw_ai_debug;
#endif

	if(game_is_paused())
		return true;

	if(rabbit_current_distance >= levels_current_desc()->distance && !game_over) {
		game_over = true;
		game_was_reset = false;
		malka_states_push("game_over");
	}

	if(rabbit && rabbit->header.type) {
		if(rabbit->data->is_dead && !game_over){
			game_over = true;
			game_was_reset = false;
			malka_states_push("game_over");
		}
				
		if(game_over)
			camera_follow_weight *= 0.95f;
	
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
	
	float t = malka_state_time("game");
	particles_update(t);

	return true;
}
bool game_update_empty(void) {
	float scroll_speed = 3.0;

	float camera_x = (objects_camera[0].left*8.0f + objects_camera[0].right) / 9.0f;
	float new_camera_x = lerp(camera_x, camera_x + scroll_speed, 0.2f);
	float camera_offset = new_camera_x - camera_x;
	objects_camera[0].left += camera_offset;
	objects_camera[0].right += camera_offset;
	objects_camera[1].left += camera_offset/2.0f;
	objects_camera[1].right += camera_offset/2.0f;
	bg_scroll += camera_offset/8.0f;

	worldgen_update(objects_camera[0].right, objects_camera[1].right);
	
	float t = malka_state_time("game");
	particles_update(t);

	return true;
}

bool game_render_empty(float t) {
	// Draw scrolling background
	float off_x = fmodf(bg_scroll, 1024.0f);
	RectF dest = rectf(-off_x, 0.0f, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, 0, dest, COLOR_WHITE);
	dest = rectf(1024.0f - off_x, 0.0f, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, 0, dest, COLOR_WHITE); 

	objects_tick(game_is_paused());
	
	return true;
}

bool game_render(float t) {
	// Draw scrolling background
	float off_x = fmodf(bg_scroll, 1024.0f);
	RectF dest = rectf(-off_x, 0.0f, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, 0, dest, COLOR_WHITE);
	dest = rectf(1024.0f - off_x, 0.0f, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, 0, dest, COLOR_WHITE); 

	if(!game_paused && !game_over)hud_render();
	objects_tick(game_is_paused());
	
	if(draw_ground_debug) worldgen_debug_render();
	
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
