#ifndef SAVES_H
#define SAVES_H

#include <utils.h>

// Loads or creates save file
void saves_init(void);
// Closes save file
void saves_close(void);

// Returns true if puzzle is solved
bool saves_get_state(const char* puzzle_name);
// Sets new puzzle state, writes changes to file
void saves_set_state(const char* puzzle_name, bool solved);

#endif

