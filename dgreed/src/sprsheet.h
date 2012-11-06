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
	(scale 1)	# scale all sprites by this factor when rendering
	
	(img background
		(tex "background.png")
	)

	(img title
		(tex "atlas.png")
		(src 0,0,480,64)
	)

	(img off_center
		(tex "pix.png")
		(src 10,0,20,10)
		(cntr 3,3)
	)

	(anim robot
		(tex "robot.png")
		(frames 12)
		(src 0,0,32,48) # first frame
		(offset 34,50) # offset for next frames
		(grid 8,5) # frames grid size
	)
)

*/

typedef uint SprHandle;

void sprsheet_init(const char* desc);
void sprsheet_close(void);

SprHandle sprsheet_get_handle(const char* name);

void sprsheet_get(const char* name, TexHandle* tex, RectF* src);
void sprsheet_get_h(SprHandle handle, TexHandle* tex, RectF* src);

void sprsheet_get_ex(const char* name, TexHandle* tex, RectF* src, Vector2* cntr_off);
void sprsheet_get_ex_h(SprHandle handle, TexHandle* tex, RectF* src, Vector2* cntr_off);

void sprsheet_get_anim(const char* name, uint frame, TexHandle* tex, RectF* src);
void sprsheet_get_anim_h(SprHandle handle, uint frame, TexHandle* tex, RectF* src);

uint sprsheet_get_anim_frames(const char* name);
uint sprsheet_get_anim_frames_h(SprHandle handle);

Vector2 sprsheet_get_size(const char* name);
Vector2 sprsheet_get_size_h(SprHandle handle);

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
