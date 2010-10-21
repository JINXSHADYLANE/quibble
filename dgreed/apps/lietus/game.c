#include "game.h"

#include <system.h>
#include <tilemap.h>

const int atlas_width = 256;
const int atlas_height = 256;
const int tile_width = 32;
const int tile_height = 32;
#define atlas_horiz_tiles (atlas_width / tile_width)
#define atlas_vert_tiles (atlas_height / tile_height)

float apelsinas_framerate = 15.0f;
float apelsinas_move_acc = 0.8f;
float apelsinas_move_damp = 0.85f;
float apelsinas_jump_acc = 8.0f;
float apelsinas_gravity = 0.5f;
float apelsinas_height = 26.0f;
float apelsinas_width = 24.0f;

TexHandle char_atlas;
Tilemap* world;

RectF viewport = {0.0f, 0.0f, 480.0f, 320.0f};

struct {
	Vector2 p, v;
	bool dir;
	bool touching_ground;
	enum {
		standing = 0,
		running,
		stand_jump,
		run_jump,
		jumping
	} state;
	int frame;
	float last_frame_t;
} apelsinas;	
	
RectF _frame_src(uint frame) {
	RectF res;
	res.left = (float)(frame % atlas_horiz_tiles) * (float)tile_width;
	res.top = (float)(atlas_vert_tiles-1 - frame / atlas_horiz_tiles) 
		* (float)tile_height;
	res.right = res.left + (float)tile_width;
	res.bottom = res.top + (float)tile_height;
	return res;
}

void _apelsinas_reset(void) {
	memset(&apelsinas, 0, sizeof(apelsinas));
	apelsinas.touching_ground = true;
	apelsinas.last_frame_t = time_ms() / 1000.0f;

	for(uint i = 0; i < world->n_objects; ++i) {
		if(world->objects[i].id == 0) 
			apelsinas.p = world->objects[i].p;
	}

	apelsinas.p.y -= 50.0f;
};	

void _apelsinas_update(void) {
	float t = time_ms() / 1000.0f;
	float dt = time_delta() / 1000.0f;

	if(apelsinas.state == jumping && fabs(apelsinas.v.y) < 0.1f)
		apelsinas.state = standing;

	if(key_pressed(KEY_LEFT)) {
		apelsinas.v.x -= apelsinas_move_acc;
		apelsinas.dir = true;
		if(apelsinas.state == standing) 
			apelsinas.state = running;
	}
	if(key_pressed(KEY_RIGHT)) {
		apelsinas.v.x += apelsinas_move_acc;
		apelsinas.dir = key_pressed(KEY_LEFT) ? apelsinas.dir : false;
		if(apelsinas.state == standing)
			apelsinas.state = running;
	}
	if(key_down(KEY_UP) && apelsinas.touching_ground) {
		apelsinas.touching_ground = false;
		apelsinas.v.y = -apelsinas_jump_acc;
		apelsinas.state = jumping;
	}
	if(t - apelsinas.last_frame_t > 1.0f / apelsinas_framerate) {
		apelsinas.frame++;
		if(apelsinas.frame == 8) {
			apelsinas.frame = 0;
		}
		apelsinas.last_frame_t = t;
	}
	
	apelsinas.v.y += apelsinas_gravity;
	apelsinas.v.x *= apelsinas_move_damp;

	RectF bbox = rectf ( 
		apelsinas.p.x + ((float)tile_width - apelsinas_width)/2.0f,
		apelsinas.p.y + ((float)tile_height - apelsinas_height),
		0.0f, 0.0f
	);		
	bbox.right = bbox.left + apelsinas_width;
	bbox.bottom = bbox.top + apelsinas_height;

	Vector2 dx = tilemap_collide_swept_rectf(world, bbox, vec2(apelsinas.v.x, 0.0f));
	apelsinas.p = vec2_add(apelsinas.p, dx);
	bbox.left += dx.x; bbox.right += dx.x;
	apelsinas.v.x = dx.x;

	Vector2 dy = tilemap_collide_swept_rectf(world, bbox, vec2(0.0f, apelsinas.v.y));
	apelsinas.p = vec2_add(apelsinas.p, dy);
	apelsinas.v.y = dy.y;
	if(dy.y == 0.0f)
		apelsinas.touching_ground = true;
}

void _apelsinas_render(void) {
	RectF dest = rectf(apelsinas.p.x, apelsinas.p.y, 
			apelsinas.p.x + (float)tile_width, apelsinas.p.y + (float)tile_height);
	dest = tilemap_world2screen(world, &viewport, dest);
	if(apelsinas.dir) {
		dest.right = dest.left;
		dest.left = dest.right + (float)tile_width;
		dest.bottom = dest.top + (float)tile_height;
	}
	int frame = apelsinas.frame;
	if(apelsinas.state == standing || apelsinas.state == running)
		frame = fabs(apelsinas.v.x) > 0.1f ? apelsinas.frame : 0;
	if(apelsinas.state == stand_jump)
		frame += 16;
	if(apelsinas.state == run_jump)
		frame += 8;
	if(apelsinas.state == jumping)
		frame += 24;
	
	RectF src = _frame_src(frame);
	video_draw_rect(char_atlas, 0, &src, &dest, COLOR_WHITE); 
}

void game_init(void) {
	char_atlas = tex_load("lietus_assets/apelsinas.png");
	world = tilemap_load("lietus_assets/world.btm");
	_apelsinas_reset();
}

void game_close(void) {
	tex_free(char_atlas);
	tilemap_free(world);
}

void game_update(void) {

	world->camera.center = vec2_add(apelsinas.p, vec2(16.0f, 16.0f));

	_apelsinas_update();
}

void game_render(void) {
	tilemap_render(world, viewport, 0.0f);
	_apelsinas_render();
}

