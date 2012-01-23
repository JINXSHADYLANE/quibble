#ifndef COMMON_H
#define COMMON_H

#define ASSETS_PRE "nulis2_assets/"
#define SCRIPTS_PRE "nulis2_scripts/"

#define WRAPAROUND_DRAW(x_min, x_max, y_min, y_max, pos, action) \
	if(pos.x < x_min) { 										 \
		Vector2 npos = vec2(pos.x + screen_widthf, pos.y); 		 \
		action;													 \
		if(pos.y < y_min) {										 \
			npos.y += screen_heightf;							 \
			action;												 \
		}														 \
		if(pos.y > y_max) {										 \
			npos.y -= screen_heightf;							 \
			action;												 \
		}														 \
	}															 \
	if(pos.x > x_max) {											 \
		Vector2 npos = vec2(pos.x - screen_widthf, pos.y);		 \
		action;													 \
		if(pos.y < y_min) {										 \
			npos.y += screen_heightf;							 \
			action;												 \
		}														 \
		if(pos.y > y_max) {										 \
			npos.y -= screen_heightf;							 \
			action;												 \
		}														 \
	}															 \
	if(pos.y < y_min) {											 \
		Vector2 npos = vec2(pos.x, pos.y + screen_heightf);		 \
		action;													 \
	}															 \
	if(pos.y > y_max) {											 \
		Vector2 npos = vec2(pos.x, pos.y - screen_heightf);		 \
		action;													 \
	}	

#endif
