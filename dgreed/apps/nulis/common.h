#ifndef COMMON_H
#define COMMON_H

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

#define ASSET_PRE "nulis_assets/"

#define WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos, action) \
	if(pos.x < x_min) { 										 \
		Vector2 npos = vec2(pos.x + (float)SCREEN_WIDTH, pos.y); \
		action;													 \
		if(pos.y < y_min) {										 \
			npos.y += (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
		if(pos.y > y_max) {										 \
			npos.y -= (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
	}															 \
	if(pos.x > x_max) {											 \
		Vector2 npos = vec2(pos.x - (float)SCREEN_WIDTH, pos.y); \
		action;													 \
		if(pos.y < y_min) {										 \
			npos.y += (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
		if(pos.y > y_max) {										 \
			npos.y -= (float)SCREEN_HEIGHT;						 \
			action;												 \
		}														 \
	}															 \
	if(pos.y < y_min) {											 \
		Vector2 npos = vec2(pos.x, pos.y + (float)SCREEN_HEIGHT);\
		action;													 \
	}															 \
	if(pos.y > y_max) {											 \
		Vector2 npos = vec2(pos.x, pos.y - (float)SCREEN_HEIGHT);\
		action;													 \
	}	

#endif
