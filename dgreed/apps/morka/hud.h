#ifndef HUD_H
#define HUD_H

#include <utils.h>
#include <uidesc.h>

void hud_init(void);
void hud_close(void);

void hud_trigger_combo(uint multiplier);
void _hud_render_ui(UIElement* element, uint layer);

void hud_render(float t);

void hud_render_game_over_tut(float t);
void hud_render_game_over_out(float t);
void hud_render_game_over_scores(float t);

bool hud_button(UIElement* element, Color col, float ts);

#endif
