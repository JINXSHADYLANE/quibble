#include "game.h"
#include "common.h"
#include "sounds.h"
#include "layouts.h"
#include <system.h>
#include <utils.h>
#include <font.h>

// Types
typedef struct {
	Vector2 pos;
	char letter;
	float fire_start_t;
	int fire_frame;
	bool sinking;
	float sink_t;
	float hint_t;
} Barrel;	

typedef struct {
	float swim_in_t;
	float toughness;
	Vector2 pos;
	float avg_y;
	float avg_x;
	float speed;
	bool active;
	bool dir;
	bool dead;
	float death_t;
} Fish;	

// Resources
TexHandle tex_background;
TexHandle tex_barrel;
TexHandle tex_fire[FIRE_FRAMES];
TexHandle tex_bottom;
TexHandle tex_title_screen; 
TexHandle tex_water[WATER_FRAMES];
TexHandle tex_fish[FISH_FRAMES];
TexHandle tex_fish_dead[FISH_DEAD_FRAMES];
RectF rect_barrel = {0, 0, 70, 84};
RectF rect_fire = {0, 0, 119, 144};
RectF rect_fish = {0, 0, 121, 83};
RectF srcrect_water = {0, 3, 1024, 539};
FontHandle font;

// Tweakables
const float barrel_fall_speed = 100.0f;
float barrel_drop_interval = 2.0f;
float barrel_drop_variance = 1.5f;
const float barrel_drop_recalc_interval = 10.0f;
const float barrel_fire_anim_speed = 0.04f;
const float barrel_fire_length = 0.5f;
const float barrel_sinking_length = 1.0f;
const float bottom_y_low = 700.0f;
const float bottom_y_high = 450.0f;
const float water_y_low = 600.0f;
const float water_y_high = 300.0f;
const float water_anim_speed = 0.15f;
const float laser_length = 0.1f;
const float switch_initial_interval = 20.0f;
const float switch_interval_multiplier = 0.9f;
const float hint_length = 0.4f;
const float fish_xspeed = 50.0f;
const float fish_rise_speed = 70.0f;
const float fish_xspeed_variance = 20.0f;
const float fish_anim_speed = 0.3f;
const float fish_death_length = 1.0f;
const float title_screen_fadeout = 1.5f;

// Globals
#define MAX_BARRELS 64
#define MAX_FISHES 4
#define KEY_COUNT 34

char valid_keys[] = 
 {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 
 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '-', '='};
 
bool is_switched[KEY_COUNT] = {false}; 

uint barrel_count;
Barrel barrels[MAX_BARRELS];
Fish fishes[MAX_FISHES];
float laser_t;
Vector2 laser_pos;

uint hit_counter;
uint miss_counter;
uint sink_counter;
float water_t;
float water_line;

bool title_screen;
float game_start_t;
float barrel_droprate_recalc_t;

float _quadratic_t(float t) {
	return -4.0f*t*t + 4.0f*t;
}	

bool is_char_switched(char c) {
	for(uint i = 0; i < KEY_COUNT; ++i) {
		if(valid_keys[i] == c)
			return is_switched[i];
	}
	LOG_ERROR("Unsupported key wants to be pressed!");
	return false;
}

void _switch_keys(float t) {
	static float last_switch = 0.0f;
	// C doesn't do proper constant resolving, just copy value
	static float switch_interval = /*switch_initial_interval*/ 20.0f;

	if(t - last_switch > switch_interval) {
		last_switch = t;
		is_switched[rand_int(0, KEY_COUNT)] = true;
		switch_interval *= switch_interval_multiplier;
	}
}

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

void _gen_fishes(void) {
	for(uint i = 0; i < MAX_FISHES; ++i) {
		fishes[i].swim_in_t = rand_float_range(0.2f, 0.7f);
		fishes[i].toughness = rand_float_range(fishes[i].swim_in_t, 1.0f);
		fishes[i].active = false;
		fishes[i].dead = false;
		fishes[i].speed = fish_xspeed + 
			rand_float_range(-fish_xspeed_variance, fish_xspeed_variance);
	}
}

void _update_fishes(float t, float dt) {
	for(uint i = 0; i < MAX_FISHES; ++i) {
		if(water_t < fishes[i].swim_in_t)
			continue;

		if(!fishes[i].active) {
			fishes[i].active = true;

			fishes[i].pos = vec2(rand_int(0, 2) == 0 ? -184.0f : 1024.0f + 52.0f,
				rand_float_range(768.0f - 150.0f, water_line + 30.0f));
			fishes[i].avg_y = fishes[i].pos.y;	
			fishes[i].dir = fishes[i].pos.x < 512.0f ? false : true;	
		}

		bool visible = fishes[i].pos.x > 70.0f && fishes[i].pos.x < 900.0f;

		if(!fishes[i].dead && water_t > fishes[i].toughness && visible) {
			fishes[i].dead = true;
			fishes[i].death_t = t;
			fishes[i].avg_x = fishes[i].pos.x;
		}	

		if(!fishes[i].dead) {
			fishes[i].pos.y = fishes[i].avg_y + 
				sin(fishes[i].pos.x / 60.0f + fishes[i].speed) * 10.0f;
			fishes[i].pos.x += fishes[i].dir ? 
				-dt * fishes[i].speed : dt * fishes[i].speed; 

			if(fishes[i].pos.x > 1024.0f + 53.0f || fishes[i].pos.x < -184.0f) {
				fishes[i].avg_y = 
					rand_float_range(768.0f - 150.0f, water_line + 30.0f);
				fishes[i].pos.x = !fishes[i].dir ? 1024.0f + 52.0f : -182.0f;
				fishes[i].dir = !fishes[i].dir;	
			}
		}

		if(fishes[i].dead) {
			if(fishes[i].avg_y > water_line - 50.0f) {
				fishes[i].avg_y -= dt * fish_rise_speed;
			}

			fishes[i].pos.x = fishes[i].avg_x + sin(t/1.2f + 
				fishes[i].swim_in_t * 10.0f) * 14.0f;
			fishes[i].pos.y = fishes[i].avg_y + sin(t*1.4f + 
				fishes[i].swim_in_t * 10.0f) * 7.0f;
		}
	}
}

void _draw_fishes(float t) {
	for(uint i = 0; i < MAX_FISHES; ++i) {
		if(!fishes[i].active)
			continue;

		float fish_hwidth = rect_fish.right / 2.0f;
		float fish_hheight = rect_fish.bottom / 2.0f;

		RectF dest = rectf(fishes[i].pos.x + fish_hwidth, 
			fishes[i].pos.y + fish_hheight, 0.0f, 0.0f);

		if(fishes[i].dir) {
			dest.right = dest.left + rect_fish.right;
			float tmp = dest.left;
			dest.left = dest.right;
			dest.right = tmp;

			dest.bottom = dest.top + rect_fish.bottom;
		}

		if(!fishes[i].dead) {	
			uint frame = ((uint)(t / fish_anim_speed) + i) % FISH_FRAMES; 
			video_draw_rect(tex_fish[frame], 3, NULL, &dest, COLOR_WHITE);
		}
		else {
			uint frame = (t - fishes[i].death_t) / 
					(fish_death_length / FISH_DEAD_FRAMES);
			frame = MIN(frame, FISH_DEAD_FRAMES-1);	
			video_draw_rect(tex_fish_dead[frame], 3, NULL, &dest, COLOR_WHITE);
		}	
	}
}

void _drop_barrel(void) {
	if(barrel_count == MAX_BARRELS)
		LOG_ERROR("Too many barrels!");

	barrels[barrel_count].pos = vec2(rand_float_range(30.0f, SCREEN_WIDTH - 30.0f),
		-40.0f);	
	
	char letter = 0;
	do {
		// Don't generate dash and equal
		letter = valid_keys[rand_int(0, sizeof(valid_keys)-2)];
	} while(letter == 0);

	barrels[barrel_count].letter = letter;	
	barrels[barrel_count].fire_frame = -1;
	barrels[barrel_count].sinking = false;
	barrels[barrel_count].hint_t = -1.0f;
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
	video_draw_rect(tex_bottom, 2, NULL, &dest, COLOR_WHITE);
	
	dest.top = water1_y;
	dest.left = 1024.0f;
	dest.right = 0.0f;
	dest.bottom = dest.top + 590.0f;
	video_draw_rect(tex_water[frame], 8, &srcrect_water, &dest, water_color);

	dest.top = water2_y;

	water_color |= 0xFF000000;
	video_draw_rect(tex_water[(frame+16)%WATER_FRAMES], 1, &srcrect_water, &dest,
		water_color);
}

void _draw_barrel(Vector2 pos, char letter, int fire_frame, float sink_t, float hint_t) {
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

	if(sink_t >= 0.0f) {
		Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, sink_t);
		video_draw_rect(tex_barrel, 4, NULL, &barrel_pos, c);
		c &= 0xFF000000;
		font_draw(font, str, 5, &letter_pos, c);
		return;
	}
	else {
		video_draw_rect(tex_barrel, 4, NULL, &barrel_pos, COLOR_WHITE);
		if(hint_t < 0.0f) {
			font_draw(font, str, 5, &letter_pos, COLOR_BLACK);
		}
		else {
			hint_t = MIN(hint_t, 1.0f);

			float qt = _quadratic_t(hint_t);
			Color transp_black = COLOR_RGBA(0, 0, 0, 0);
			Color c1 = color_lerp(transp_black, COLOR_BLACK, qt);
			Color c2 = color_lerp(COLOR_BLACK, transp_black, qt);

			char nstr[] = {'\0', '\0'};
			nstr[0] = layouts_map(letter);

			font_draw(font, str, 5, &letter_pos, c2);
			font_draw(font, nstr, 5, &letter_pos, c1);
		}
	}

	if(fire_frame != -1) {
		float fire_hwidth = (float)rect_fire.right - (float)rect_fire.left;
		float fire_hheight = (float)rect_fire.bottom - (float)rect_fire.top;
		fire_hwidth /= 2.0f; fire_hheight /= 2.0f;

		RectF fire_pos = rectf(pos.x - fire_hwidth, pos.y - fire_hheight, 
			0.0f, 0.0f);
		
		video_draw_rect(tex_fire[fire_frame], 7, NULL, &fire_pos,
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
	for(uint i = 0; i < KEY_COUNT; ++i) {
		if(char_down(valid_keys[i]))
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

		// Check for correct keypresses when layout is mixed
		if(!barrels[i].sinking && barrels[i].fire_frame == -1 &&
			!no_more_checks && is_char_switched((size_t)barrels[i].letter) &&
			char_down(layouts_map(barrels[i].letter))) {
			goto shoot;
		}	

		// Check for correct keypresses	
		if(!barrels[i].sinking && barrels[i].fire_frame == -1 && 
			!no_more_checks && char_down(barrels[i].letter)) {
			// Show hint if letter is switched
			if(is_char_switched((size_t)barrels[i].letter) && 
				layouts_map(barrels[i].letter) != barrels[i].letter) {
				barrels[i].hint_t = t;
				goto fall;
			}

		shoot:	

			char_count--;
			barrels[i].fire_frame = 0;
			barrels[i].fire_start_t = t;

			no_more_checks = true;

			hit_counter++;

			laser_t = t;
			laser_pos = vec2(barrels[i].pos.x, barrels[i].pos.y+15.0f);
			
			sound_play(sound_burning);
		}
	
	fall:

		barrels[i].pos.y += dt *  barrel_fall_speed;
		
		if(barrels[i].hint_t > 0.0f && t - barrels[i].hint_t > hint_length)
			barrels[i].hint_t = -1.0f;

		if(!barrels[i].sinking && barrels[i].pos.y > water_line) {
			if (rand_int(0, 2)) sound_play(sound_bulbul);
			else sound_play(sound_sinked);
			barrels[i].sinking = true;
			barrels[i].sink_t = t;

			sink_counter++;
		}

		if(barrels[i].sinking && t - barrels[i].sink_t > barrel_sinking_length) {
			_remove_barrel(i);
			i--;
		}
	}

	if(char_count > 0) {
		// TODO: handle wrong keypresses
		miss_counter += char_count;

		laser_t = t;
		float x = rand_float_range(50.0f, 1024.0f - 50.0f);
		float y = rand_float_range(10.0f, water_line - 20.0f);
		laser_pos = vec2(x, y);
	}

	water_t = ((float)sink_counter - 0.03f * (float)hit_counter)/100.0f;
	water_t = MIN(MAX(water_t, 0.0f), 1.0f);
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
	tex_title_screen = tex_load(TITLE_SCREEN_FILE);
	_load_anim(FIRE_FRAMES, FIRE_FILE, tex_fire);
	_load_anim(WATER_FRAMES, WATER_FILE, tex_water);
	_load_anim(FISH_FRAMES, FISH_FILE, tex_fish);
	_load_anim(FISH_DEAD_FRAMES, FISH_DEAD_FILE, tex_fish_dead);

	font = font_load(FONT_FILE);

	title_screen = true;
}

void game_reset(void) {
	memset(is_switched, 0, sizeof(is_switched));

	barrel_count = 0;
	hit_counter = 0;
	miss_counter = 0;
	sink_counter = 0;
	water_t = 0.0f;
	laser_t = -100.0f;

	_gen_fishes();
}

void game_close(void) {
	tex_free(tex_background);
	tex_free(tex_barrel);
	tex_free(tex_bottom);
	tex_free(tex_title_screen);
	_free_anim(FIRE_FRAMES, tex_fire);
	_free_anim(WATER_FRAMES, tex_water);
	_free_anim(FISH_FRAMES, tex_fish);
	_free_anim(FISH_DEAD_FRAMES, tex_fish_dead);

	font_free(font);
}

void game_update(void) {
	float t = time_ms() / 1000.0f;
	float dt = time_delta() / 1000.0f;

	if(title_screen) {
		if(char_down(' ')) {
			title_screen = false;
			barrel_droprate_recalc_t = game_start_t = t;
			game_reset();
		}
		return;
	}

	if(t - barrel_droprate_recalc_t > barrel_drop_recalc_interval) {
		barrel_droprate_recalc_t = t;
		
		float ratio = (float)hit_counter / (float)MAX(miss_counter, 1);
		ratio /= 300.0f;
		ratio = MIN(MAX(0.0f, ratio), 10.0f);
		
		float inv_droprate = 1.0f / barrel_drop_interval;
		inv_droprate += ratio;
		barrel_drop_interval = 1.0f / inv_droprate;
		barrel_drop_variance = 0.75f * barrel_drop_interval;
	}

	_generate_barrels(t);
	_update_barrels(t, dt);
	_update_fishes(t, dt);
	_switch_keys(t);
}

void game_render(void) {
	float t = time_ms() / 1000.0f;

	if(title_screen || t - game_start_t < title_screen_fadeout) {
		float fade = 0.0f;
		if(!title_screen)
			fade = (t - game_start_t) / title_screen_fadeout;

		RectF dest = rectf_null();
		Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fade);
		video_draw_rect(tex_title_screen, 10, NULL, &dest, c);
		if(title_screen)
			return;
	}

	RectF dest = rectf_null();
	video_draw_rect(tex_background, 0, NULL, &dest, COLOR_WHITE);
	_draw_water(water_t, t);
	_draw_fishes(t);

	for(uint i = 0; i < barrel_count; ++i)
		_draw_barrel(barrels[i].pos, barrels[i].letter, barrels[i].fire_frame,
			barrels[i].sinking ? (t - barrels[i].sink_t) / barrel_sinking_length : -1.0f,
			barrels[i].hint_t > 0.0f ? (t - barrels[i].hint_t) / hint_length : -1.0f);

	if(t - laser_t < laser_length) {
		Vector2 pos1 = vec2(laser_pos.x > 512.0f ? 1024.0f : 0.0f,
			laser_pos.y + 15.0f);

		video_draw_line(6, &pos1, &laser_pos, COLOR_RGBA(196, 16, 20, 196));
	}
}

