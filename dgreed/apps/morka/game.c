#include "game.h"
#include "common.h"
#include "objects.h"
#include "obj_types.h"
#include "worldgen.h"
#include "hud.h"
#include "minimap.h"
#include "tutorials.h"
#include "game_over.h"

#include <mfx.h>
#include <particles.h>

bool camera_follow = false;

extern bool draw_gfx_debug;
extern bool draw_physics_debug;
bool draw_ground_debug = false;
bool draw_ai_debug = false;

// Game state
ObjRabbit* rabbit = NULL;
float camera_follow_weight;
float bg_scroll = 0.0f;

static bool game_need_reset = true;
static bool game_over = false;
static bool game_paused = false;

bool tutorial_level = true;

static void game_reset(void) {
	if(rabbit) {
		objects_destroy_all();
	}
	minimap_reset(levels_current_desc()->distance);

	// Player rabbit
	Vector2 pos = vec2(512.0f, 579.0f);
	rabbit = (ObjRabbit*)objects_create(&obj_rabbit_desc,pos,(void*)-1);
	minimap_track(rabbit);
	tutorials_reset(rabbit);

	// AI rabbits
	for(int i = 0;i < levels_current_desc()->ai_rabbit_num;i++){
		Vector2 pos = vec2(640.0f+128.0f*i,579.0f);
		ObjRabbit* ai_rabbit = (ObjRabbit*)objects_create(
			&obj_rabbit_desc, pos, (void*)(size_t)i
		);
		minimap_track(ai_rabbit);
	}	

	float p = 512.0f + 50.0f + levels_current_desc()->ai_rabbit_num * 128.0f;

	objects_camera[0].left = p;
	objects_camera[0].right = p + 1024.0f;
	objects_camera[1].left = p;
	objects_camera[1].right = p + 1024.0f;
	objects_camera[2].left = p;
	objects_camera[2].right = p + 1024.0f;
	bg_scroll = p;

	worldgen_reset(rand_uint(),levels_current_desc());

	camera_follow_weight = 0.2f;
	game_over = false;

	camera_follow = false;

	hud_trigger_combo(0);

	// Check if this level is a tutorial
	tutorial_level = !strcmp(levels_current_desc()->name, "level1");
}

static void game_init(void) {
	levels_init(ASSETS_DIR "levels.mml");
	objects_init();
	hud_init();
	minimap_init();
	tutorials_init();
	levels_reset("level1");
	game_reset();
}

static void game_enter(void) {
	if(game_need_reset){
		game_reset();
		game_need_reset = false;
	}
	game_unpause();
}

static void game_leave(void) {
}

void game_end(void){
	game_over = true;
}

void game_request_reset(void){
	game_need_reset = true;
}

void game_pause(void) {
	game_paused = true;
}

void game_unpause(void) {
	game_paused = false;
}

static void game_close(void) {
	worldgen_close();
	tutorials_close();
	minimap_close();
	hud_close();
	objects_close();
	levels_close();
}

static float _camera_x(void) {
	float camera_x = (objects_camera[0].left*8.0f + objects_camera[0].right) / 9.0f;
	return camera_x;
}

static void _move_camera(float new_pos_x, float follow_weight) {
	float camera_x = _camera_x();
	float new_camera_x = lerp(camera_x, new_pos_x, follow_weight);
	float camera_offset = new_camera_x - camera_x;
	if(camera_offset > 0.0f){
		camera_follow = true;
		objects_camera[0].left += camera_offset;
		objects_camera[0].right += camera_offset;
		objects_camera[1].left += camera_offset/2.0f;
		objects_camera[1].right += camera_offset/2.0f;
		objects_camera[2].left += camera_offset/8.0f;
		objects_camera[2].right += camera_offset/8.0f;
		bg_scroll += camera_offset/8.0f;
	}
}

bool game_update(void) {
	if(game_paused)
		return true;

	if(rabbit && rabbit->header.type) {

		float rabbit_pos = rabbit->header.render->world_dest.left;

		if(game_over)
			camera_follow_weight *= 0.95f;
		else {

			float rabbit_distance = rabbit_pos / (1024.0f / 3.0f) - 2.0f;
			float level_distance = levels_current_desc()->distance;

			if(level_distance > 0 && rabbit_distance >= level_distance) {
				game_over = true;
				game_over_set_screen(SCORES_SCREEN);
				malka_states_push("game_over");
			}

			if(rabbit->data->is_dead){
				game_over = true;
				game_over_set_screen(OUT_SCREEN);
				malka_states_push("game_over");
			}

		}

		_move_camera(rabbit_pos + 45.0f, camera_follow_weight);
	}

	float pos = minimap_max_x();
	worldgen_update( pos, pos );

	// spawn background dust particles
	static int delta = 0;	
	if(delta == 0){
		delta = rand_int(40,70); // new particle every 40-70 frames
		float x = rand_float_range(objects_camera[2].left,objects_camera[2].right);
		float y = rand_float_range(0.0f,579.0f);
		Vector2 pos = vec2(x,y);
		ObjParticleAnchor* anchor = (ObjParticleAnchor*)objects_create(&obj_bg_particle_anchor_desc, pos, NULL);
		mfx_trigger_follow("dusts",&anchor->screen_pos,NULL);
	}
	delta--;
	
	particles_update(time_s());

	return true;
}

bool game_update_empty(void) {
	float scroll_speed = 3.0;

	float camera_x = _camera_x();
	_move_camera(camera_x + scroll_speed, 0.2f);

	worldgen_update(objects_camera[0].right, objects_camera[1].right);
	
	if(!game_paused){
		particles_update(time_s());
	} 

	return true;
}

bool game_render(float t) {
	// Draw scrolling background
	float off_x = fmodf(bg_scroll, 1024.0f);
	RectF dest = rectf(-off_x, 0.0f, 0.0f, 0.0f);
	Color col = COLOR_WHITE;
	spr_draw_h(levels_current_desc()->background, background_layer, dest, col);
	dest = rectf(1024.0f - off_x, 0.0f, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, background_layer, dest, col); 

	objects_tick(game_paused);

	if(!game_paused && !game_over) 
		hud_render(t);
	
	if(draw_ground_debug) worldgen_debug_render();

	if(tutorials_are_enabled()){ 
		if(!game_paused) tutorial_event(AUTO);
		if(tutorials_during_gameplay()) tutorials_render(0);
	}

	minimap_draw_finish_line();

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
