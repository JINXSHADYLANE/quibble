#ifndef OBJECTS_H
#define OBJECTS_H

#include "utils.h"
#include "darray.h"
#include "system.h"
#include "mml.h"

typedef enum {
	OBJANIM_NONE = 0,
	OBJANIM_LOOP,
	OBJANIM_BLINK,
	OBJANIM_COUNT
} ObjectAnim;

typedef struct {
	const char* name;
	TexHandle texture;
	uint start_frame, n_frames; 
	ObjectAnim anim_type;
	float fps;
} ObjectDef;

typedef struct {
	ObjectDef* def;
	Vector2 pos;
	float size, rot, phase;
	uint layer;
	Color color;
} Object;

extern uint n_object_defs;
extern ObjectDef* object_defs;
extern DArray* objects;

void objects_init(const char* defs_file);
void objects_close(void);
void objects_reset(DArray* objs);
void objects_load(MMLObject* mml, NodeIdx node, DArray* objs);
void objects_save(MMLObject* mml, NodeIdx node, DArray* objs);
void objects_render(float t, uint alpha);

void objects_edit(void);
void objects_soft_save(void);
void objects_hard_save(void);

#endif

