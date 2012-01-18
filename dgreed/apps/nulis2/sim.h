#ifndef SIM_H
#define SIM_H

#include <utils.h>
#include <coldet.h>

void sim_init(uint screen_width, uint screen_height);
void sim_close(void);
void sim_reset(const char* level);
const char* sim_level(void);
void sim_update(void);
void sim_render(void);
void sim_render_ex(bool skip_vignette);

#endif
