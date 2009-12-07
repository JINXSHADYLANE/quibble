#ifndef GUI_H
#define GUI_H

#include "utils.h"

void gui_init(void);
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
