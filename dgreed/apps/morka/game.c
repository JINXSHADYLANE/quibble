#include "game.h"

#include "ai.h"
#include "characters.h"
#include "common.h"
#include "objects.h"
#include "obj_types.h"
#include "worldgen.h"
#include "hud.h"
#include "minimap.h"
#include "tutorials.h"
#include "game_over.h"
#include "placement.h"
#include "shop.h"

#include <keyval.h>
#include <mfx.h>
#include <particles.h>

extern bool draw_gfx_debug;
extern bool draw_physics_debug;
bool draw_ai_debug = false;

ObjRabbit* rabbit = NULL;
float camera_follow_weight;
bool camera_follow = false;
float bg_scroll = 0.0f;

static bool game_need_reset = true;
static bool game_over = false;
static bool game_paused = false;
bool tutorial_level = true;

uint player = 0;

CharacterParams character_params[character_count];

static void game_reset(void) {
	if(rabbit) {
		objects_destroy_all();
	}

	LevelDesc * lvl_desc = levels_current_desc();

	// Check if this level is a tutorial
	tutorial_level = !strcmp(lvl_desc->name, "level1");

	minimap_reset(lvl_desc->distance);

	uint ai_num = 0;

	for(int i = 0;i < character_count;i++){

		character_params[i].sprite = sprsheet_get_handle(default_characters[i].spr_handle);
		character_params[i].minimap_color = default_characters[i].minimap_color;

		if(i == player){
			// Player character
			Vector2 pos = vec2(512.0f, 579.0f);

			character_params[i].name = "You";
			character_params[i].speed = default_characters[i].speed;
			character_params[i].xjump = default_characters[i].xjump;
			character_params[i].yjump = default_characters[i].yjump;
			character_params[i].control = obj_rabbit_player_control;

			rabbit = (ObjRabbit*)objects_create(
				&obj_rabbit_desc, pos, (void*)&character_params[i]
			);

			minimap_track(rabbit);
			tutorials_reset(rabbit);
		} else if(ai_num < lvl_desc->ai_rabbit_num) {
			// AI character
			Vector2 pos = vec2(640.0f+128.0f*ai_num,579.0f);

			character_params[i].name = default_characters[i].name;

			switch(lvl_desc->season){
				case AUTUMN: character_params[i].control = ai_control_autumn;
				break;
				case WINTER: character_params[i].control = ai_control_winter;
				break;
				case SPRING: character_params[i].control = ai_control_spring;
				break;
				case SUMMER: character_params[i].control = ai_control_summer;
				break;
			}
			character_params[i].ai_max_combo = lvl_desc->ai_max_combo[ai_num];
			character_params[i].speed = lvl_desc->ai_rabbit_speeds[ai_num];
			character_params[i].xjump = lvl_desc->ai_rabbit_xjumps[ai_num];
			character_params[i].yjump = lvl_desc->ai_rabbit_yjumps[ai_num];
			ai_num++;

			ObjRabbit* ai_rabbit = (ObjRabbit*)objects_create(
				&obj_rabbit_desc, pos, (void*)&character_params[i]
			);

			minimap_track(ai_rabbit);
		}
		
	}	

	float p = 512.0f + 50.0f + levels_current_desc()->ai_rabbit_num * 128.0f;
	objects_camera[0].left = p;
	objects_camera[0].right = p + 1024.0f;
	objects_camera[1].left = p;
	objects_camera[1].right = p + 1024.0f;
	objects_camera[2].left = p;
	objects_camera[2].right = p + 1024.0f;
	bg_scroll = p;

	camera_follow_weight = 0.2f;
	game_over = false;
	camera_follow = false;

	worldgen_reset(rand_uint(),levels_current_desc());
	worldgen_update(objects_camera[0].right, objects_camera[1].right);

	hud_reset();

	game_need_reset = false;
}

static void game_init(void) {
	levels_init(ASSETS_DIR "levels.mml");
	objects_init();
	hud_init();
	minimap_init();
	tutorials_init();
	levels_reset("level1");
	placement_init();
	game_reset();
}

static void game_enter(void) {
	game_paused = tutorials_paused();	
}

static void game_preenter(void) {

}
static void game_postleave(void) {
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
	placement_close();
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
		if(!game_paused) camera_follow = true;
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


			if(!game_over && rabbit->data->game_over){
				game_over = true;
				if(rabbit->data->is_dead) 
					game_over_set_screen(OUT_SCREEN);
				else if(minimap_get_place_of_rabbit(rabbit) <= 3)
					game_over_set_screen(WIN_SCREEN);
				else
					game_over_set_screen(LOSE_SCREEN);

				if(!rabbit->data->is_dead)
					keyval_set_int("coins",coins + rabbit->data->tokens);

				malka_states_push("game_over");
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
	
	particles_update(time_s());

	return true;
}

void game_render_level(void){
	if(game_need_reset) game_reset();
	// Draw scrolling background
	float off_x = fmodf(bg_scroll, 1024.0f);
	RectF dest = rectf(-off_x, 0.0f, 0.0f, 0.0f);
	Color col = COLOR_WHITE;
	spr_draw_h(levels_current_desc()->background, background_layer, dest, col);
	dest = rectf(1024.0f - off_x, 0.0f, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, background_layer, dest, col);
	
	objects_tick(game_paused);

	minimap_draw_finish_line();
}

static bool game_render(float t) {
	// Keep updating camera during transitions
	if(t != 0){
		if(!game_over) 
			game_update();
		else 
			game_update_empty();
	}	
	hud_render(t);

	if(tutorials_are_enabled()){ 
		if(rabbit && rabbit->header.type && !game_paused) tutorial_event(AUTO);
		tutorials_render(t);
	}

	return true;
}

StateDesc game_state = {
	.init = game_init,
	.close = game_close,
	.enter = game_enter,
	.preenter = game_preenter,	
	.leave = game_leave,
	.postleave = game_postleave,
	.update = game_update,
	.render = game_render
};