#ifndef DEVMODE_H
#define DEVMODE_H

void devmode_init(void);
void devmode_close(void);
void devmode_update(void);
void devmode_overload(const char* name);
void devmode_render(void);

#endif
