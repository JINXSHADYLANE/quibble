#ifndef LAYOUTS_H
#define LAYOUTS_H

void layouts_init(void);
void layouts_close(void);
void layouts_set(const char* name);
char layouts_map(char keyb_char);

#endif
