#include "menu.h"
#include "background.h"
#include "common.h"
#include "puzzles.h"
#include <font.h>

FontHandle font;

void menu_init(void) {
	font = font_load(FONT_NOVA);
}

void menu_close(void) {
	font_free(font);
}

void menu_update(void) {
	background_update();
}

void menu_render(void) {
	background_render();

	for(uint i = 0; i < puzzle_count; ++i) {
		float fi = (float)i;
		Vector2 dest = vec2(500.0f - fi * 40.0f, 100.0f + fi * 40.0f);
		font_draw(font, puzzle_descs[i].name, 2, &dest, COLOR_WHITE);
	}
}

