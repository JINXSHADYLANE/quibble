#include "game.h"
#include "common.h"
#include <system.h>
#include <font.h>

TexHandle tex_barrel;
RectF rect_barrel = {0, 0, 70, 84};

FontHandle font;

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

void game_init(void) {
	tex_barrel = tex_load(BARREL_FILE);

	font = font_load(FONT_FILE);
}

void game_close(void) {
	tex_free(tex_barrel);

	font_free(font);
}

void game_update(void) {
}

void game_render(void) {
	_draw_barrel(vec2(100.0f, 100.0f), 'a');
}
