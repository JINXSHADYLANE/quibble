#ifndef SPRSHEET_H
#define SPRSHEET_H

#include <system.h>

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

void sprsheet_init(const char* desc);
void sprsheet_close(void);

void sprsheet_get(const char* name, TexHandle* handle, RectF* src);
void sprsheet_get_anim(const char* name, uint frame, TexHandle* handle, RectF* src);
int sprsheet_get_anim_frames(const char* name);

#endif
