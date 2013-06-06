#ifndef HUD_H
#define HUD_H

#include <utils.h>
#include <uidesc.h>

void hud_init(void);
void hud_close(void);
void hud_reset(void);

bool hud_touch_powerup(void);

void hud_trigger_combo(uint multiplier);
void _hud_render_ui(UIElement* element, uint layer,Color col);

void hud_render(float t);

void hud_render_game_over_tut(float t);
void hud_render_game_over_out(float t);
void hud_render_game_over_win(float t);
void hud_render_game_over_lose(float t);

void hud_render_tutorial_pause(float t);
void hud_render_regular_pause(float t);

bool hud_button(UIElement* element, Color col, float ts);
// Button with circle collision
bool hud_button_ex(SprHandle spr, Vector2 pos, float r, Color col, float ts);
// Button with rectangle collision
bool hud_button_rect(SprHandle spr, Vector2 pos, Vector2 size, Color col, float ts);

// If there are unlockables, pushes the item_unlock state
bool hud_unlock_check(uint state_num);

// Scrolling selections
float hud_scroll(float xpos, float inc, uint elements, float t);
uint hud_scroll_get_selection(float value, float increment, uint elements);


extern bool hud_click;

#endif
