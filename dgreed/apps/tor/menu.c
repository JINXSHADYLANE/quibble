#include "menu.h"
#include "background.h"
#include "common.h"
#include "puzzles.h"
#include "game.h"
#include <font.h>
#include <memory.h>

FontHandle font;
extern TexHandle atlas1;
RectF minipuzzle_back_src = {484, 2, 484+64, 2+64};

// Increases if mouse is over puzzle name, decreases otherwise
float* puzzle_hotness; 

// Tweakables
int names_per_screen = 13;			// Puzzle names
float names_horiz_shift = -70.0f;
float names_horiz_diff = 40.0f;
float names_vert_diff = 40.0f;
float select_fadein_speed = 4.0f;	// Selector segments
float select_fadeout_speed = 1.0f;
float select_length = 20.0f;
float select_min_dist = 4.0f;
float select_max_dist = 20.0f;
float minipuzzle_size = 128.0f;
float minipuzzle_inner_size = 120.0f;
float minipuzzle_fadein_start = 3.0f;
float minipuzzle_fadein_end = 5.0f;

RectF _puzzle_name_rect(int i) {
	assert(i >= 0 && i < names_per_screen);

	float fi = (float)(i - names_per_screen/2);
	float half_width = floor(font_width(font, puzzle_descs[i].name) / 2.0f);
	float half_height = floor(font_height(font) / 2.0f);
	RectF result = {
		floor(SCREEN_WIDTH / 2.0f - fi * names_horiz_diff) + names_horiz_shift, 
		floor(SCREEN_HEIGHT / 2.0f + fi * names_vert_diff), 0.0f, 0.0f};
	result.right = result.left;
	result.bottom = result.top;
	result.left -= half_width;
	result.right += half_width;
	result.top -= half_height;
	result.bottom += half_height;
	return result;
}

void menu_init(void) {
	font = font_load(FONT_NOVA);
	puzzle_hotness = (float*)MEM_ALLOC(sizeof(float) * puzzle_count);
	memset((void*)puzzle_hotness, 0, sizeof(float) * puzzle_count);
}

void menu_close(void) {
	font_free(font);
	MEM_FREE(puzzle_hotness);
}

void menu_update(void) {
	float dt = time_delta() / 1000.0f;

	background_update();

	uint x, y;
	mouse_pos(&x, &y);
	Vector2 m_pos = {(float)x, (float)y};

	for(uint i = 0; i < puzzle_count; ++i) {
		RectF rect = _puzzle_name_rect(i);	
		if(rectf_contains_point(&rect, &m_pos)) {
			puzzle_hotness[i] = puzzle_hotness[i] + dt * select_fadein_speed;
			if(mouse_up(MBTN_RIGHT)) {
				game_reset(i);
				game_state = PUZZLE_STATE;
				background_switch();
			}
		}	
		else {
			puzzle_hotness[i] = 
				MAX(MIN(puzzle_hotness[i], 1.0f) - dt * select_fadeout_speed, 0.0f);
		}
	}
}

void _draw_minipuzzle(uint i, const Vector2* pos) {
	// Draw border/background
	RectF dest = {pos->x, pos->y, 
		pos->x + minipuzzle_size, pos->y + minipuzzle_size};
	float t = clamp(minipuzzle_fadein_start, minipuzzle_fadein_end, puzzle_hotness[i]);
	t = (t - minipuzzle_fadein_start) / 
		(minipuzzle_fadein_end - minipuzzle_fadein_start);
	Color c = color_lerp(COLOR_TRANSPARENT, COLOR_WHITE, t);
	video_draw_rect(atlas1, 3, &minipuzzle_back_src, &dest, c);

	// Draw scaled solved puzzle
	assert(i < puzzle_count);
	PuzzleDesc* desc = &puzzle_descs[i];
	float puzzle_width = (float)desc->width * desc->tile_size.x;
	float puzzle_height = (float)desc->height * desc->tile_size.y;
	float scale_ratio = minipuzzle_inner_size / MAX(puzzle_width, puzzle_height);

	Vector2 top_left = {(dest.left + dest.right) / 2.0f, 
		(dest.top + dest.bottom) / 2.0f};
	top_left.x -= puzzle_width * scale_ratio / 2.0f;	
	top_left.y -= puzzle_height * scale_ratio / 2.0f;

	float cursor_x, cursor_y = top_left.y;
	for(uint j = 0; j < desc->height; ++j) {
		cursor_x = top_left.x;  
		for(uint k = 0; k < desc->width; ++k) {
			RectF source = 
				puzzle_get_tile_rect(i, desc->solved[IDX_2D(k, j, desc->width)]);
			RectF dest = {cursor_x, cursor_y, 
				cursor_x + desc->tile_size.x * scale_ratio,
				cursor_y + desc->tile_size.y * scale_ratio};
			video_draw_rect(desc->image, 4, &source, &dest, c); 	
			cursor_x += desc->tile_size.x * scale_ratio;
		}
		cursor_y += desc->tile_size.y * scale_ratio;
	}
}

void menu_render(void) {
	background_render();

	uint x, y;
	mouse_pos(&x, &y);
	Vector2 m_pos = {(float)x, (float)y};

	for(uint i = 0; i < puzzle_count; ++i) {
		RectF rect = _puzzle_name_rect(i);
		Vector2 dest = {rect.left, rect.top};
		float t = MIN(puzzle_hotness[i], 1.0f);
		if(t > 0.0f) {
			t *= t;
			Color c = color_lerp(COLOR_RGBA(255, 255, 255, 0), 
				COLOR_RGBA(255, 255, 255, 255), t);
			float dist = lerp(select_max_dist, select_min_dist, t);	
			Vector2 v1 = {rect.left - dist, (rect.top + rect.bottom) / 2.0f};	
			Vector2 v2 = {v1.x - select_length, v1.y};
			video_draw_line(2, &v1, &v2, c);
			v1.x = rect.right + dist;
			v2.x = v1.x + select_length;
			video_draw_line(2, &v1, &v2, c);

			font_draw(font, puzzle_descs[i].name, 2, &dest, COLOR_WHITE);
			if(puzzle_hotness[i] > 3.0f) 
				_draw_minipuzzle(i, &m_pos);
		}
		else {
			font_draw(font, puzzle_descs[i].name, 2, &dest, COLOR_WHITE);
		}
	}
}

