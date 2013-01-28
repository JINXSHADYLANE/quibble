#include "worldgen.h"

#include <mempool.h>

#include "mchains.h"
#include "obj_types.h"

extern ObjRabbit* rabbit;

#define page_width 1024.0f
static float fg_page_cursor = 0.0f;
static float bg_page_cursor = 0.0f;

static RndContext rnd = NULL;
static Chain* fg_chain;
static Chain* bg_chain;
static Chain* ground_chain;

static void _gen_bg_page(void) {
	SprHandle spr;	
	uint advance;
	
	// Add background mushrooms
	static float bg_x = page_width;
	bg_x -= page_width;	
	while(bg_x < page_width) {
		char sym = mchains_next(bg_chain, &rnd);
		mchains_symbol_info(bg_chain, sym, &advance, &spr);
		mchains_symbol_info(bg_chain, sym, &advance, &spr);

		if(spr) {
			Vector2 pos = vec2(bg_page_cursor + bg_x + 100.0f, 768.0f);
			objects_create(&obj_deco_desc, pos, (void*)spr);
		}

		bg_x += (float)advance;
	}

	bg_page_cursor += page_width;
}

static void _gen_fg_page(void) {
	SprHandle spr;	
	uint advance = 0;
	static uint prev_advance = 0;
	
	static Vector2 gaps[4];
	static const int max_gaps = 4;
	static int gaps_i = 0;
	static int previuos_gaps = 0;
	
	gaps_i = previuos_gaps;
	
	// Add ground
	static float ground_x = page_width;
	ground_x -= page_width;
	while(ground_x < page_width) {
		char sym = mchains_next(ground_chain, &rnd);
		mchains_symbol_info(ground_chain, sym, &advance, &spr);
		if(spr) {
			Vector2 pos = vec2(fg_page_cursor + ground_x + 100.0f, 768.0f);
			advance = (uint) sprsheet_get_size_h(spr).x;
			if(sym == 'a' || sym == 'h'){	// no collision for grass_start1 and grass_end2
				objects_create(&obj_fg_deco_desc, pos, (void*)spr);
				prev_advance = advance;	
			} else {
				objects_create(&obj_ground_desc, pos, (void*)spr);
				if(sym == 'j' || sym == 'k' || sym == 'l' || sym == 'm' || sym == 'n' || sym == 'o'){
					ObjSpeedTrigger* t = (ObjSpeedTrigger*)objects_create(&obj_speed_trigger_desc, pos, (void*)spr);
					t->drag_coef = 0.9;
				}
				prev_advance = 0;
			}		
		} else {
			if(sym == '_' || sym == '-' || sym == '='){
				Vector2 pos = vec2(fg_page_cursor + ground_x + 100.0f - prev_advance, 768.0f);
				
				// add gap to gap array
				gaps[gaps_i].x = pos.x;									// start of gap
				gaps[gaps_i].y = pos.x + advance + 120 + 50;			// end of gap

				if(gaps_i > max_gaps){ 
					gaps_i = 0;
					printf("gaps_i > max_gaps !\n");
				}
				
				gaps_i++;
				objects_create(&obj_fall_trigger_desc, pos, (void*)advance);
			}
		}
		ground_x += (float)advance;
	}
	previuos_gaps = gaps_i - previuos_gaps;
	
	// Add foreground mushrooms
	static float fg_x = page_width;
	fg_x -= page_width;
	while(fg_x < page_width) {
		char sym = mchains_next(fg_chain, &rnd);
		mchains_symbol_info(fg_chain, sym, &advance, &spr);
		Vector2 pos = vec2(fg_page_cursor + fg_x + 100.0f, 641.0f);
		
		bool place = false;
		
		if(spr){
			place = true;
			for(int i = 0; i < gaps_i;i++){
				if( (pos.x > gaps[i].x && pos.x < gaps[i].y) || (pos.x+advance > gaps[i].x && pos.x+advance < gaps[i].y) || (pos.x < gaps[i].x && pos.x+advance > gaps[i].y )){
					place = false;
				}
			}
		}
				
		if(place) {
			GameObject* g = objects_create(&obj_mushroom_desc, pos, (void*)spr);
			ObjMushroom* shroom = (ObjMushroom*)g;
			if(sym == 'x')
				shroom->damage = 1.0f;
			else
				shroom->damage = 0.0f;

		}
	
		fg_x += (float)advance;
	}

	fg_page_cursor += page_width;
	
	for(int i = 0; i < gaps_i - previuos_gaps; i++){
		gaps[i].x = gaps[i+1].x;
		gaps[i].y = gaps[i+1].y; 		
	}		
}
void worldgen_reset(uint seed) {
	if(!rnd) {
		// First time
		rand_init_ex(&rnd, seed);
	}
	else {
		// Not first time
		rand_seed_ex(&rnd, seed);
		fg_page_cursor = 0.0f;
		bg_page_cursor = 0.0f;
		mchains_del(fg_chain);
		mchains_del(bg_chain);
		mchains_del(ground_chain);
	}

	fg_chain = mchains_new("fg");
	bg_chain = mchains_new("bg");
	ground_chain = mchains_new("ground");

	_gen_bg_page();
	_gen_fg_page();
}

void worldgen_close(void) {
	mchains_del(fg_chain);
	mchains_del(bg_chain);
	mchains_del(ground_chain);
	rand_free_ex(&rnd);
}

void worldgen_update(float fg_camera_extent_max, float bg_camera_extent_max) {
	if(fg_camera_extent_max >= fg_page_cursor)
		_gen_fg_page();
	if(bg_camera_extent_max >= bg_page_cursor)
		_gen_bg_page();
}

