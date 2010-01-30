#include "game.h"
#include "common.h"
#include <system.h>
#include <utils.h>
#include <font.h>

// Types
typedef struct {
	Vector2 pos;
	char letter;
} Barrel;	

// Resources
TexHandle tex_barrel;
RectF rect_barrel = {0, 0, 70, 84};

FontHandle font;

// Tweakables
float barrel_fall_speed = 100.0f;

// Globals
#define MAX_BARRELS 64
uint barrel_count;
Barrel barrels[MAX_BARRELS];

void _drop_barrel(void) {
	if(barrel_count == MAX_BARRELS)
		LOG_ERROR("Too many barrels!");

	barrels[barrel_count].pos = vec2(rand_float_range(20.0f, SCREEN_WIDTH - 20.0f),
		-40.0f);	
	barrels[barrel_count].letter = (char)rand_int((int)'a', (int)'z');	
	barrel_count++;
}

void _draw_barrel(Vector2 pos, char letter) {
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

	video_draw_rect(tex_barrel, 0, NULL, &barrel_pos, COLOR_WHITE);
	font_draw(font, str, 1, &letter_pos, COLOR_BLACK);
}

void _update_barrels(float dt) {
	
	for(int i = 0; i < barrel_count; ++i) {
		barrels[i].pos.y += dt *  barrel_fall_speed;
		if(barrels[i].pos.y > SCREEN_HEIGHT - 100.0f) {
			// Destroy barrel
			barrels[i] = barrels[barrel_count-1];
			barrel_count--;
			i--;
		}
	}
}

void game_init(void) {
	tex_barrel = tex_load(BARREL_FILE);

	font = font_load(FONT_FILE);

	barrel_count = 0;
}

void game_close(void) {
	tex_free(tex_barrel);

	font_free(font);
}

void game_update(void) {
	float dt = time_delta() / 1000.0f;

	_update_barrels(dt);
	if(char_down('a'))
		_drop_barrel();
}

void game_render(void) {
	for(uint i = 0; i < barrel_count; ++i)
		_draw_barrel(barrels[i].pos, barrels[i].letter);
}
