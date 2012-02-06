#ifndef SIM_H
#define SIM_H

#include <utils.h>
#include <coldet.h>

void sim_init(uint screen_width, uint screen_height, uint sim_width, uint sim_height);
void sim_close(void);
void sim_enter(void);
void sim_leave(void);
void sim_reset(const char* level);
const char* sim_level(void);
void sim_update(void);
void sim_render(void);
void sim_render_ex(bool skip_vignette);

#endif
