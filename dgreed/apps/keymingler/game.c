#include "game.h"
#include "common.h"
#include <system.h>
#include <utils.h>
#include <font.h>
#include "sounds.h"

// Types
typedef struct {
	Vector2 pos;
	char letter;
	float fire_start_t;
	int fire_frame;
} Barrel;	

// Resources
TexHandle tex_background;
TexHandle tex_barrel;
TexHandle tex_fire[FIRE_FRAMES];
TexHandle tex_bottom;
TexHandle tex_water[WATER_FRAMES];
RectF rect_barrel = {0, 0, 70, 84};
RectF rect_fire = {0, 0, 119, 144};
RectF srcrect_water = {0, 3, 1024, 539};
FontHandle font;

// Tweakables
float barrel_fall_speed = 100.0f;
float barrel_drop_interval = 2.0f;
float barrel_drop_variance = 1.5f;
float barrel_fire_anim_speed = 0.04f;
float barrel_fire_length = 0.5f;
float bottom_y_low = 700.0f;
float bottom_y_high = 450.0f;
float water_y_low = 600.0f;
float water_y_high = 300.0f;
float water_anim_speed = 0.15f;

// Globals
#define MAX_BARRELS 64
uint barrel_count;
Barrel barrels[MAX_BARRELS];

uint hit_counter;
uint miss_counter;
uint sink_counter;
float water_t;
float water_line;

void _load_anim(uint n_frames, const char* path, TexHandle* out) {
	char filename[256];
	for(uint i = 0; i < n_frames; ++i) {
		sprintf(filename, path, i);
		out[i] = tex_load(filename);
	}
}

void _free_anim(uint n_frames, TexHandle* anim) {
	for(uint i = 0; i < n_frames; ++i) {
		tex_free(anim[i]);
	}
}

void _drop_barrel(void) {
	if(barrel_count == MAX_BARRELS)
		LOG_ERROR("Too many barrels!");

	barrels[barrel_count].pos = vec2(rand_float_range(30.0f, SCREEN_WIDTH - 30.0f),
		-40.0f);	
	barrels[barrel_count].letter = (char)rand_int((int)'a', (int)'z');	
	barrels[barrel_count].fire_frame = -1;
	barrel_count++;
}

void _draw_water(float level, float t) {
	uint frame = (uint)(t / water_anim_speed);
	frame %= WATER_FRAMES;

	Color water_clear = COLOR_RGBA(255, 255, 255, 128);	
	Color water_black = COLOR_RGBA(32, 32, 32, 128);	
	Color water_color = color_lerp(water_clear, water_black, level);


	float bottom_y = lerp(700.0f, 768.0f - 319.0f, level);
	float water1_y = lerp(650.0f, 768.0f - 590.0f, level);
	float water2_y = water1_y - 50.0f;
	water_line = water1_y + 20.0f;

	RectF dest = rectf(0.0f, bottom_y, 0.0f, 0.0f);
	video_draw_rect(tex_bottom, 1, NULL, &dest, COLOR_WHITE);
	
	dest.top = water1_y;
	dest.left = 1024.0f;
	dest.right = 0.0f;
	dest.bottom = dest.top + 590.0f;
	video_draw_rect(tex_water[frame], 2, &srcrect_water, &dest, water_color);

	dest.top = water2_y;

	water_color |= 0xFF000000;
	video_draw_rect(tex_water[(frame+16)%WATER_FRAMES], 0, &srcrect_water, &dest,
		water_color);
}

void _draw_barrel(Vector2 pos, char letter, int fire_frame) {
	char str[] = {letter, '\0'};

	float barrel_hwidth = (float)rect_barrel.right - (float)rect_barrel.left;
	float barrel_hheight = (float)rect_barrel.bottom - (float)rect_barrel.top;
	float letter_hwidth = font_width(font, str);
	float letter_hheight = font_height(font);
	barrel_hwidth /= 2.0f; barrel_hheight /= 2.0f;
	letter_hwidth /= 2.0f; letter_hheight /= 2.0f;

	RectF barrel_pos = rectf(pos.x - barrel_hwidth, pos.y - barrel_hheight,
		0.0f, 0.0f);
	Vector2 letter_pos = vec2(pos.x - letter_hwidth, pos.y - letter_hheight
		+ 3.0f);

	video_draw_rect(tex_barrel, 2, NULL, &barrel_pos, COLOR_WHITE);
	font_draw(font, str, 3, &letter_pos, COLOR_BLACK);

	if(fire_frame != -1) {
		float fire_hwidth = (float)rect_fire.right - (float)rect_fire.left;
		float fire_hheight = (float)rect_fire.bottom - (float)rect_fire.top;
		fire_hwidth /= 2.0f; fire_hheight /= 2.0f;

		RectF fire_pos = rectf(pos.x - fire_hwidth, pos.y - fire_hheight, 
			0.0f, 0.0f);
		
		video_draw_rect(tex_fire[fire_frame], 4, NULL, &fire_pos,
			COLOR_WHITE);
	}
}

void _remove_barrel(uint idx) {
	for(uint i = idx; i < barrel_count-1; ++i) {
		barrels[i] = barrels[i+1];	
	}
	barrel_count--;
}

void _update_barrels(float t, float dt) {
	// Count how many chars were pressed
	uint char_count = 0;
	for(char c = 'a'; c <= 'z'; ++c) {
		if(char_down(c))
			char_count++;
	}

	bool no_more_checks = false;
	for(int i = 0; i < barrel_count; ++i) {
		// Handle flaming barrels
		if(barrels[i].fire_frame != -1) {
			float passed_time = t - barrels[i].fire_start_t;
			uint passed_frames = (uint)(passed_time / barrel_fire_anim_speed);
			barrels[i].fire_frame = passed_frames % FIRE_FRAMES;

			barrels[i].pos.y += dt *  barrel_fall_speed;

			if(passed_time > barrel_fire_length) {
				_remove_barrel(i);
				i--;
			}
			
			continue;
		}

		// Check for correct keypresses	
		if(barrels[i].fire_frame == -1 && 
			!no_more_checks && char_down(barrels[i].letter)) {
			char_count--;
			barrels[i].fire_frame = 0;
			barrels[i].fire_start_t = t;

			no_more_checks = true;

			hit_counter++;
			
			sound_play(sound_burning);
		}

		barrels[i].pos.y += dt *  barrel_fall_speed;
		if(barrels[i].pos.y > water_line) {
			// Destroy barrel
			if (rand_int(0, 2)) sound_play(sound_bulbul);
			else sound_play(sound_sinked);

			_remove_barrel(i);
			i--;
			sink_counter++;
		}
	}

	if(char_count > 0) {
		// TODO: handle wrong keypresses
		miss_counter += char_count;
	}

	water_t = ((float)sink_counter + 0.06f * (float)hit_counter)/100.0f;
	water_t = MIN(water_t, 1.0f);
}

void _generate_barrels(float t) {
	static float last_drop = 0.0f;
	static float next_in = 0.0f;

	if(t - last_drop > next_in) {
		_drop_barrel();
		next_in = 0.0f;
		last_drop = t;
	}

	if(next_in == 0.0f) 
		next_in = barrel_drop_interval 
			+ rand_float_range(-barrel_drop_variance, barrel_drop_variance);
}

void game_init(void) {
	tex_background = tex_load(BACKGROUND_FILE);
	tex_barrel = tex_load(BARREL_FILE);
	tex_bottom = tex_load(BOTTOM_FILE);
	_load_anim(FIRE_FRAMES, FIRE_FILE, tex_fire);
	_load_anim(WATER_FRAMES, WATER_FILE, tex_water);

	font = font_load(FONT_FILE);

	barrel_count = 0;
	hit_counter = 0;
	miss_counter = 0;
	sink_counter = 0;
	water_t = 0.0f;
}

void game_close(void) {
	tex_free(tex_background);
	tex_free(tex_barrel);
	tex_free(tex_bottom);
	_free_anim(FIRE_FRAMES, tex_fire);
	_free_anim(WATER_FRAMES, tex_water);

	font_free(font);
}

void game_update(void) {
	float t = time_ms() / 1000.0f;
	float dt = time_delta() / 1000.0f;

	_generate_barrels(t);
	_update_barrels(t, dt);
}

void game_render(void) {
	float t = time_ms() / 1000.0f;
	RectF dest = rectf_null();
	video_draw_rect(tex_background, 0, NULL, &dest, COLOR_WHITE);
	_draw_water(water_t, t);

	for(uint i = 0; i < barrel_count; ++i)
		_draw_barrel(barrels[i].pos, barrels[i].letter, barrels[i].fire_frame);
}

