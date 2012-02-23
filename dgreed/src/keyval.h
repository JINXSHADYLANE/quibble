#ifndef KEYVAL_H
#define KEYVAL_H

#include "utils.h"

// Fast and persistent key -> value store for game saves,
// settings, etc.
// Flushes data to disk every N writes and on close.
// You can also force flush manually.

void keyval_init(const char* file);
void keyval_close(void);

const char* keyval_get(const char* key, const char* def);
int keyval_get_int(const char* key, int def);
bool keyval_get_bool(const char* key, bool def);
float keyval_get_float(const char* key, float def);

void keyval_set(const char* key, const char* val);
void keyval_set_int(const char* key, int val);
void keyval_set_bool(const char* key, bool val);
void keyval_set_float(const char* key, float val);

void keyval_flush(void);
void keyval_gc(void);

void keyval_app_suspend(void);

#endif
