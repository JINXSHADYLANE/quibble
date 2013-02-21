#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "utils.h"

// Lightweight localization system.
//
// On close creates/updates base.loc file
// with all the translateable strings.
// Translate it to another language and give to
// loc_init to be able to select that language.

// Example of language definition file:
// (lang lt
// 		(OK Gerai)
// 		(Back Atgal)
// 		("No Filter" "Man be filtro")
// )

// Initializes localization system,
// takes list of comma-separated localization files.
// If production == true, base.loc will not be generated/updated
void loc_init(const char* loc_files, bool production);

// Closes localization system
void loc_close(void);

// Selects a language to use
void loc_select(const char* language);

// Translates a string
const char* loc_str(const char* str);

#endif

