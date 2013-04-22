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
float bg_y = 0.0f;

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

	LevelDesc* lvl_desc = levels_current_desc();

	// Check if this level is a tutorial
	tutorial_level = !strcmp(lvl_desc->name, "level1");

	minimap_reset(lvl_desc->distance);

	uint ai_num = 0;

	for(int i = 0;i < character_count;i++){
		CharacterParams* cp = &character_params[i];

		cp->sprite = sprsheet_get_handle(default_characters[i].spr_handle);
		cp->minimap_color = default_characters[i].minimap_color;
		cp->animation = default_characters[i].animation;

		if(i == player){
			// Player character
			Vector2 pos = vec2(512.0f, v_height-128.0f);

			cp->name = "You";
			cp->speed = default_characters[i].speed;
			cp->xjump = default_characters[i].xjump;
			cp->yjump = default_characters[i].yjump;
			cp->control = obj_rabbit_player_control;

			rabbit = (ObjRabbit*)objects_create(
				&obj_rabbit_desc, pos, (void*)&character_params[i]
			);

			minimap_track(rabbit);
			tutorials_reset(rabbit);
		} else if(ai_num < lvl_desc->ai_rabbit_num) {
			// AI character
			Vector2 pos = vec2(640.0f+128.0f*ai_num,v_height-128.0f);

			cp->name = default_characters[i].name;

			switch(lvl_desc->season){
				case AUTUMN: cp->control = ai_control_autumn;
				break;
				case WINTER: cp->control = ai_control_winter;
				break;
				case SPRING: cp->control = ai_control_spring;
				break;
				case SUMMER: cp->control = ai_control_summer;
				break;
			}
			cp->ai_max_combo = lvl_desc->ai_max_combo[ai_num];
			cp->speed = lvl_desc->ai_rabbit_speeds[ai_num];
			cp->xjump = lvl_desc->ai_rabbit_xjumps[ai_num];
			cp->yjump = lvl_desc->ai_rabbit_yjumps[ai_num];
			ai_num++;

			ObjRabbit* ai_rabbit = (ObjRabbit*)objects_create(
				&obj_rabbit_desc, pos, (void*)&character_params[i]
			);

			minimap_track(ai_rabbit);
		}
		
	}	

	float p = 512.0f + 50.0f + levels_current_desc()->ai_rabbit_num * 128.0f;
	for(uint i = 0; i < N_CAMERAS; ++i) {
		objects_camera[i].left = p;
		objects_camera[i].right = p + v_width;
		objects_camera[i].bottom = v_height;
	}

	bg_scroll = p;

	camera_follow_weight = 0.2f;
	game_over = false;
	camera_follow = false;

	worldgen_reset(rand_uint(),levels_current_desc());
	worldgen_update(objects_camera[0].right, objects_camera[1].right);

	hud_reset();

	game_need_reset = false;
}

void game_force_reset(void){
	game_reset();
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
	if(game_need_reset) game_reset();		
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

static void _move_camera(Vector2 new_pos, Vector2 follow_weight) {
	Vector2 camera = vec2(_camera_x(),0.0f);
	Vector2 new_camera;
	new_camera.x = lerp(camera.x, new_pos.x, follow_weight.x);
	new_camera.y = lerp(camera.y, new_pos.y, follow_weight.y);

	Vector2 offset = vec2_sub(new_camera,camera);

	if(offset.x > 0.0f){
		if(!game_paused) camera_follow = true;

		objects_camera[0].left += offset.x;
		objects_camera[0].right += offset.x;
		objects_camera[0].top += offset.y;
		objects_camera[0].bottom += offset.y;

		if(objects_camera[0].bottom > v_height){
			offset.y -= objects_camera[0].top;
			objects_camera[0].top = 0.0f;
			objects_camera[0].bottom = v_height;			
		} else if(objects_camera[0].top < -110.0f) {
			offset.y += -objects_camera[0].top - 110.0f;
			objects_camera[0].top = -110.0f;
			objects_camera[0].bottom = objects_camera[0].top + v_height;			
		}

		objects_camera[1].left += offset.x / 2.0f;
		objects_camera[1].right += offset.x / 2.0f;
		objects_camera[1].top += offset.y / 2.0f;
		objects_camera[1].bottom += offset.y / 2.0f;

		objects_camera[2].left += offset.x / 8.0f;
		objects_camera[2].right += offset.x / 8.0f;
		objects_camera[2].top += offset.y / 8.0f;
		objects_camera[2].bottom += offset.y / 8.0f;

		bg_scroll += offset.x/8.0f;
		//bg_y += offset.y/8.0f;
	}
}

bool game_update(void) {
	if(game_paused)
		return true;

	if(rabbit && rabbit->header.type) {
		Vector2 camera;
		Vector2 follow = vec2(camera_follow_weight,0.0f);

		Vector2 pos = rabbit->header.physics->cd_obj->pos;

		camera.x = rabbit->header.render->world_dest.left + 45.0f;
		//camera.y = rabbit->header.physics->vel.y;

		//printf("%f\n", rabbit->header.physics->vel.x);
		float vel_x = rabbit->header.physics->vel.x;
		float target_z = 1.0f + clamp(0.0f, 1.0f, (vel_x - 1000.0f) / 500.0f);
		for(uint i = 0; i < N_CAMERAS; ++i) {
			objects_camera_z[i] = lerp(objects_camera_z[i], target_z, 0.01f);
		}

		if(camera.y == 0.0f && objects_camera[0].top < 0.0f){
			camera.y = -objects_camera[0].top * 1000.0f;
		}
		//printf("%f %f\n",camera.y,objects_camera[0].top );

		float c = normalize(v_height-128.0f - pos.y, 0.0f,v_height-128.0f);
		c = MAX(0.0f,c);
		follow.y = 0.005 * c * c;

		_move_camera(camera, follow);

		//printf("pos.y: %f c: %f fy: %f\n",579.0f - pos.y,c,follow.y );

		if(!game_over && rabbit->data->game_over){
			game_over = true;

			uint place = minimap_get_place_of_rabbit(rabbit);

			if(rabbit->data->is_dead) 
				game_over_set_screen(OUT_SCREEN);
			else if(place <= 3)
				game_over_set_screen(WIN_SCREEN);
			else
				game_over_set_screen(LOSE_SCREEN);

			levels_set_place(place);
			malka_states_push("game_over");
		}

	}

	float pos = minimap_max_x();
	worldgen_update( pos, pos );

	// spawn background dust particles
	static int delta = 0;	
	if(delta == 0){
		delta = rand_int(40,70); // new particle every 40-70 frames
		float x = rand_float_range(objects_camera[2].left,objects_camera[2].right);
		float y = rand_float_range(0.0f,v_height-128.0f);
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

	Vector2 camera = vec2(_camera_x() + scroll_speed,-objects_camera[0].top);
	_move_camera(camera, vec2(0.2f,0.01f) );

	worldgen_update(objects_camera[0].right, objects_camera[1].right);
	
	particles_update(time_s());

	return true;
}

void game_render_level(void){
	// Draw scrolling background
	float off_x = fmodf(bg_scroll, 1024.0f);
	float off_y = bg_y;
	RectF dest = rectf(-off_x, -off_y, 0.0f, 0.0f);
	Color col = COLOR_WHITE;
	spr_draw_h(levels_current_desc()->background, background_layer, dest, col);
	dest = rectf(1024.0f - off_x, -off_y, 0.0f, 0.0f);
	spr_draw_h(levels_current_desc()->background, background_layer, dest, col);
	if(v_width > 1024.0f){
		dest = rectf(2048.0f - off_x, -off_y, 0.0f, 0.0f);
		spr_draw_h(levels_current_desc()->background, background_layer, dest, col);
	}
	
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
