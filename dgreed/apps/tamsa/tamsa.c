#include <system.h>

#include "common.h"
#include "kdtree.h"
#include "world.h"

static Color backbuffer[scr_size] = {0};
static TexHandle backbuffer_tex;

static World world;
static Vector2 pos, vel, dir_vec;
static float dir, rot;

void tm_init(void) {
	backbuffer_tex = tex_create(
		next_pow2(scr_height), 
		next_pow2(scr_width)
	);

	KdSeg segs[] = {
		{0, 0, 6, 0, 7},
		{6, 0, 6, 5, 7},
		{6, 5, 0, 5, 7},
		{0, 5, 0, 0, 7},
		{2, 2, 2, 3, 13},
		{2, 3, 4, 3, 13},
		{4, 3, 4, 2, 13},
		{4, 2, 2, 2, 13}
	};

	world.floor = 0.0f;
	world.ceiling = 1.0f;
	kdtree_init(&world.walls, segs, 8);

	pos = vec2(2.571671, 1.256600);
	dir_vec = vec2(0.998971, -0.045358);
	vel = vec2(0.0f, 0.0f);
	dir = vec2_angle(vec2(1.0f, 0.0f), dir_vec);
	rot = 0.0f;
}

extern float eps;

void tm_update(void) {
	if(char_down('o')) {
		eps *= 10.0f;
		printf("eps = %f\n", eps);
	}

	if(char_down('l')) {
		eps *= 0.1f;
		printf("eps = %f\n", eps);
	}

	if(key_pressed(KEY_LEFT))
		rot -= 0.01f;
	if(key_pressed(KEY_RIGHT))
		rot += 0.01f;
	dir += rot;
	rot *= 0.3f;

	dir_vec = vec2_rotate(vec2(1.0f, 0.0f), dir);

	if(char_down('p')) {
		printf("pos = vec2(%f, %f);\n", pos.x, pos.y);
		printf("dir = vec2(%f, %f);\n", dir_vec.x, dir_vec.y);
	}

	if(key_pressed(KEY_UP))
		vel = vec2_add(vel, vec2_scale(dir_vec, 0.01f));
	if(key_pressed(KEY_DOWN))
		vel = vec2_add(vel, vec2_scale(dir_vec, -0.01f));

	pos = vec2_add(pos, vel);
	vel = vec2_scale(vel, 0.3f);
}

bool tm_render(void) {
	// Motion blur
	for(uint i = 0; i < scr_width * scr_height; ++i) {
		byte r, g, b, a;
		COLOR_DECONSTRUCT(backbuffer[i], r, g, b, a);
		r <<= 2;
		g <<= 2;
		b <<= 2;
		backbuffer[i] = COLOR_RGBA(r, g, b, a);
	}

	world_render(
		&world, pos, dir_vec,
		PI/3.0f, scr_width, scr_height, backbuffer
	);

	// Transfer backbuffer to gpu
	tex_blit(backbuffer_tex, backbuffer, 0, 0, scr_height, scr_width);

	// Draw backbuffer, rotated 90 degrees
	RectF src = rectf(0.0f, 0.0f, scr_height, scr_width);
	RectF dest = rectf(60.0f, -60.0f, 0.0f, 0.0f);
	video_draw_rect_rotated(backbuffer_tex, 1, &src, &dest, -PI/2.0f, COLOR_WHITE);

	return !key_up(KEY_QUIT) && !char_up('q');
}

void tm_close(void) {
	kdtree_free(&world.walls);
	tex_free(backbuffer_tex);
}

int dgreed_main(int argc, const char** argv) {
	video_init_exr(
		scr_width * scr_scale, scr_height * scr_scale,
		scr_width, scr_height, "tamsa", false
	);

	tm_init();

	while(system_update()) {
		tm_update();
		if(!tm_render())
			break;
		video_present();
	}

	tm_close();

	video_close();
	return 0;
}

