#ifndef TWEAKABLES_H
#define TWEAKABLES_H

#include "font.h"
#include "darray.h"

typedef enum {
	TWEAK_FLOAT,
	TWEAK_INT,
	TWEAK_BOOL
} TwVarType;

typedef struct {
	TwVarType type;

	char* group;
	char* name;
	char* overload;

	union {
		struct {
			float value;
			float* addr;
			float min;
			float max;
		} _float;
		struct {
			int value;
			int* addr;
			int min;
			int max;
		} _int;
		struct {
			bool value;
			bool* addr;
		} _bool;
	} t;
} TwVar;	

typedef struct {
	const char* filename;
	RectF dest;
	uint layer;
	FontHandle font;
	Color color;
	Color overload_color;

	uint items_per_page;
	float y_spacing;
	float widest_name;

	const char* group;
	const char* overload;

	uint last_drawn_page;
	const char* last_page_name;
	uint last_page_first_var;

	DArray vars;
} Tweaks;	
	
Tweaks* tweaks_init(const char* filename, RectF dest, uint layer,
	FontHandle font);
void tweaks_close(Tweaks* tweaks);

void tweaks_group(Tweaks* tweaks, const char* name);
void tweaks_overload(Tweaks* tweaks, const char* overload);
void tweaks_float(Tweaks* tweaks, const char* name, float* addr, 
	float min, float max);
void tweaks_int(Tweaks* tweaks, const char* name, int* addr,
	int min, int max);
void tweaks_bool(Tweaks* tweaks, const char* name, bool* addr);

void tweaks_render(Tweaks* tweaks);

#endif

