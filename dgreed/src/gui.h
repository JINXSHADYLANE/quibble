#ifndef GUI_H
#define GUI_H

#include "system.h"
#include "font.h"

typedef struct {
	TexHandle texture;
	FontHandle font;

	Color text_color;

	uint first_layer;
	uint second_layer;
	uint text_layer;

	RectF src_button_down;
	RectF src_button_up;

	RectF src_switch_on_up;
	RectF src_switch_on_down;
	RectF src_switch_off_up;
	RectF src_switch_off_down;

	RectF src_slider;
	RectF src_slider_knob_down;
	RectF src_slider_knob_up;
} GuiDesc;

GuiDesc gui_default_style(const char* assets_prefix);
bool gui_validate_desc(const GuiDesc* desc);

void gui_init(const GuiDesc* desc);
void gui_close(void);

void gui_label(const Vector2* pos, const char* text);
bool gui_button(const Vector2* pos, const char* text);
bool gui_switch(const Vector2* pos, const char* text);
float gui_slider(const Vector2* pos);

bool gui_getstate_switch(const Vector2* pos);
float gui_getstate_slider(const Vector2* pos);

void gui_setstate_switch(const Vector2* pos, bool val);
void gui_setstate_slider(const Vector2* pos, float val);

#endif
