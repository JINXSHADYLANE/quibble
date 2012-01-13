#ifndef SPRSHEET_H
#define SPRSHEET_H

#include "system.h"

// A layer on top of textures to stop worrying about loading, freeing 
// and source rectangles.

/*
Example description:

(sprsheet bulb
	(prefix "bulb_assets/")
	(preload background,title,robot)
	
	(img background
		(tex "background.png")
	)

	(img title
		(tex "atlas.png")
		(src 0,0,480,64)
	)

	(anim robot
		(tex "robot.png")
		(frames 12)
		(src 0,0,32,48) # first frame
		(offset 34,50) # offset for next frames
	)
)

*/

typedef uint SprHandle;

void sprsheet_init(const char* desc);
void sprsheet_close(void);

SprHandle sprsheet_get_handle(const char* name);

void sprsheet_get(const char* name, TexHandle* tex, RectF* src);
void sprsheet_get_h(SprHandle handle, TexHandle* tex, RectF* src);
void sprsheet_get_anim(const char* name, uint frame, TexHandle* tex, RectF* src);
void sprsheet_get_anim_h(SprHandle handle, uint frame, TexHandle* tex, RectF* src);
uint sprsheet_get_anim_frames(const char* name);
uint sprsheet_get_anim_frames_h(SprHandle handle);

// Helper rendering methods:

void spr_draw(const char* name, uint layer, RectF dest, Color tint);
void spr_draw_h(SprHandle handle, uint layer, RectF dest, Color tint);
void spr_draw_anim(const char* name, uint frame, uint layer, RectF dest, Color tint);
void spr_draw_anim_h(SprHandle handle, uint frame, uint layer, RectF dest, Color tint);
void spr_draw_cntr(const char* name, uint layer, Vector2 dest, float rot, 
		float scale, Color tint);
void spr_draw_cntr_h(SprHandle handle, uint layer, Vector2 dest, float rot, 
		float scale, Color tint);
void spr_draw_anim_cntr(const char* name, uint frame, uint layer, Vector2 dest, 
		float rot, float scale, Color tint);
void spr_draw_anim_cntr_h(SprHandle handle, uint frame, uint layer, Vector2 dest, 
		float rot, float scale, Color tint);

#endif
