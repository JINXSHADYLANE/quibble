#include "effects.h"
#include "common.h"

#include <system.h>
#include <particles.h>
#include <gfx_utils.h>

// Tweakables
const float ffield_freq = 0.2;
const float ffield_lifetime = 0.5;
const int ffield_layer = 1;
const int particles_layer = 5;

TexHandle ffield_tex;
SoundHandle ffield_sound;
SourceHandle ffield_source;
RectF ffield_rect = {2.0f, 770.0f, 254.0f, 768.0f + 254.0f};
Color ffield_color_start;
Color ffield_color_end;

// State
typedef struct {
	Vector2 center;
	float radius;
	float birth_time;
	bool shrink;
	Color color;
} FFieldCircle;

#define MAX_FFIELD_CIRCLES 8
FFieldCircle ffield_circles[MAX_FFIELD_CIRCLES];
uint ffield_circle_count = 0;
float ffield_last_spawn = 0.0f;
float ffield_volume = 0.0f;

// Sounds
typedef struct {
	const char* name;
	float volume;
	SoundHandle handle;
} SfxDef;

SfxDef sfx[] = {
	{"bounce.wav", 0.4f, 0},
	{"bad_string.wav", 1.0f, 0},
	{"windup+click.wav", 1.0f, 0},
	{"hit.wav", 1.0f, 0},
	{"vanish.wav", 1.0f, 0},
	{"appear.wav", 0.3f, 0},
	{"win.wav", 0.5f, 0}
};

static void _sfx_load(void) {
	char filename[64];
	for(uint i = 0; i < ARRAY_SIZE(sfx); ++i) {
		strcpy(filename, ASSET_PRE);
		strcat(filename, sfx[i].name);
		sfx[i].handle = sound_load_sample(filename);
		sound_set_volume(sfx[i].handle, sfx[i].volume);
	}
}

static void _sfx_unload(void) {
	for(uint i = 0; i < ARRAY_SIZE(sfx); ++i) {
		sound_free(sfx[i].handle);
	}
}

static void _sfx_play(const char* name) {
	for(uint i = 0; i < ARRAY_SIZE(sfx); ++i) {
		if(strcmp(name, sfx[i].name) == 0) {
			sound_play(sfx[i].handle);
			return;
		}
	}
	assert(0 && "Bad sound name!");
}

void effects_init(void) {
	ffield_color_start = COLOR_RGBA(0, 129, 255, 255);
	ffield_color_end = COLOR_RGBA(255, 0, 236, 255);

	particles_init(ASSET_PRE, particles_layer);	
	ffield_tex = tex_load(ASSET_PRE "background.png");

	_sfx_load();

	ffield_sound = sound_load_sample(ASSET_PRE "ffield.wav");
	ffield_source = sound_play_ex(ffield_sound, true);
	sound_set_volume_ex(ffield_source, ffield_volume);
}

void effects_close(void) {
	particles_close();
	tex_free(ffield_tex);

	_sfx_unload();
	sound_free(ffield_sound);
}

void effects_force_field(Vector2 p, float r, bool push) {
	ffield_volume = MIN(1.0f, ffield_volume + 0.1f);
	float t = time_s();
	if(t - ffield_last_spawn > ffield_freq) {
		ffield_last_spawn = t;

		FFieldCircle new = { p, r, t, !push, 0 };
		new.color = color_lerp(
				ffield_color_start, ffield_color_end, rand_float()
		);

		assert(ffield_circle_count < MAX_FFIELD_CIRCLES);
		ffield_circles[ffield_circle_count++] = new;
	}
}

static void _update_ffield(void) {
	ffield_volume = MAX(0.0f, ffield_volume - 0.05f);
	sound_set_volume_ex(ffield_source, ffield_volume);
	float t = time_s();
	for(uint i = 0; i < ffield_circle_count; ++i) {
		FFieldCircle* circle = &ffield_circles[i];

		float ct = (t - circle->birth_time) / ffield_lifetime;
		if(ct >= 1.0f) {
			ffield_circles[i] = ffield_circles[--ffield_circle_count];
			--i;
			continue;
		}
	}
}

static void _render_ffield(void) {
	float t = time_s();
	for(uint i = 0; i < ffield_circle_count; ++i) {
		FFieldCircle* circle = &ffield_circles[i];
		float ct = (t - circle->birth_time) / ffield_lifetime;
		ct = clamp(0.0f, 1.0f, ct);

		float start_r = circle->shrink ? circle->radius : circle->radius * 0.5f;
		float end_r = circle->shrink ? circle->radius * 0.5f : circle->radius;
		float scale = lerp(start_r, end_r, ct) / 128.0f;

		Color transparent = circle->color & 0xFFFFFF;
		Color col = color_lerp(transparent, circle->color, sinf(ct * PI));

		Vector2 pos = circle->center;
		gfx_draw_textured_rect(ffield_tex, ffield_layer, 
				&ffield_rect, &pos, 0.0f, scale, col);

		float x_min = circle->radius;
		float x_max = SCREEN_WIDTH - x_min;
		float y_min = x_min;
		float y_max = SCREEN_HEIGHT - y_min;

		WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos,
			gfx_draw_textured_rect(ffield_tex, ffield_layer, 
				&ffield_rect, &npos, 0.0f, scale, col));
	}
}

void effects_collide_ab(Vector2 p, float dir) {
	static float last_collide = 0.0f;
	float t = time_s();
	if(t - last_collide < 0.05f) 
		return;
	last_collide = t;

	particles_spawn("collision0", &p, dir + PI/2.0f);
	particles_spawn("collision0", &p, dir - PI/2.0f);
	_sfx_play("bounce.wav");
}

void effects_collide_aab(Vector2 p) {
	particles_spawn("collision2", &p, 0.0f);
	particles_spawn("collision2", &p, PI/2.0f);
	particles_spawn("collision2", &p, PI);
	particles_spawn("collision2", &p, PI/3.0f * 2.0f);
	particles_spawn("diffusion", &p, 0.0f);
	_sfx_play("hit.wav");
}

void effects_collide_aa(Vector2 p) {
	particles_spawn("blast0", &p, 0.0f);
	particles_spawn("fusion", &p, 0.0f);
	_sfx_play("bad_string.wav");
}

void effects_collide_aaa(Vector2 p) {
	particles_spawn("wicked_blast0", &p, 0.0f);
	_sfx_play("windup+click.wav");
}

void effects_destroy(Vector2 p) {
	particles_spawn("gameover", &p, 0.0f);
		
	static float last_destroy = 0.0f;
	float t = time_s();
	if(t - last_destroy < 0.1f)
		return;
	last_destroy = t;
	_sfx_play("vanish.wav");
}

void effects_spawn(Vector2 p) {
	particles_spawn("blast1", &p, 0.0f);
	_sfx_play("appear.wav");
}

void effects_win(void) {
	_sfx_play("win.wav");
}

void effects_update(void) {
	_update_ffield();
	particles_update(time_s());
}

void effects_render(void) {
	_render_ffield();
	particles_draw();
}

