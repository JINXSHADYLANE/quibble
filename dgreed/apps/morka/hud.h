#ifndef HUD_H
#define HUD_H

#include <utils.h>
#include <uidesc.h>

void hud_init(void);
void hud_close(void);

void hud_trigger_combo(uint multiplier);
void _hud_render_ui(UIElement* element, uint layer);

void hud_render(void);

#endif
