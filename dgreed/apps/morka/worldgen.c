#include "worldgen.h"
#include "mchains.h"
#include "common.h"
#include "obj_types.h"
#include "placement.h"

#include <mfx.h>
#include <mempool.h>

extern bool tutorial_level;

static float page_width = 0.0f;
static float fg_page_cursor = 0.0f;
static float bg_page_cursor = 0.0f;

static RndContext rnd = NULL;
static Chain* fg_chain;
static Chain* bg_chain;
static Chain* ground_chain;

static int coins = 3;
static int coins_cd = 2;

static uint powerups[POWERUP_COUNT];

static void _gen_bg_page(void) {
	SprHandle spr;	
	int advance;
	
	// Add background mushrooms
	static float bg_x = 0;
		
	while(bg_x < page_width) {
		char sym = mchains_next(bg_chain, &rnd);
		mchains_symbol_info(bg_chain, sym, &advance, &spr);

		if(spr) {
			Vector2 pos = vec2(bg_page_cursor + bg_x + 100.0f, v_height);
			objects_create(&obj_deco_desc, pos, (void*)spr);
		}

		bg_x += (float)advance;
	}
	bg_x -= page_width;
	bg_page_cursor += page_width;
}

static bool place_powerup(GameObjectDesc* desc, Vector2 pos,PowerupType type){
	uint num = powerups[type];

	if(num){
		float min = (float) (levels_current_desc()->distance + 2.0f) *
							(v_width/3.0f) *
							levels_current_desc()->powerup_pos[type].x;

		float max = (float) (levels_current_desc()->distance + 2.0f) *
							(v_width/3.0f) *
							levels_current_desc()->powerup_pos[type].y;

		float d = (float)num / levels_current_desc()->powerup_num[type];
		float place = max - ((max-min) * d);


		if(pos.x > place && pos.x < max){
			objects_create(desc, pos,(void*)&powerup_params[type]);
			powerups[type]--;
			return true;
		}
	}
	return false;
}

static void _gen_ground(void){
	SprHandle spr;	
	int advance = 0;
	static float ground_x = 0;


	while(ground_x < page_width) {
		char sym = mchains_next(ground_chain, &rnd);
		mchains_symbol_info(ground_chain, sym, &advance, &spr);
		Vector2 pos = vec2(fg_page_cursor + ground_x, v_height);		
		if(spr) {
			advance = (uint) sprsheet_get_size_h(spr).x;

			// no collision for grass_start1 and grass_end2
			if(sym == 'a' || sym == 'h'){	
				objects_create(&obj_fg_deco_desc, pos, (void*)spr);
				objects_create(&obj_fall_trigger_desc, vec2_add(pos,vec2(0.0f,127.0f)), (void*)advance);
			} else {
				objects_create(&obj_ground_desc, pos, (void*)spr);

				//water speed trigger
				if( sym == 'j' || sym == 'k' || sym == 'l' ||
					sym == 'm' || sym == 'n' || sym == 'o'){
					ObjSpeedTrigger* t = (ObjSpeedTrigger*)objects_create(&obj_speed_trigger_desc, pos, (void*)spr);
					t->drag_coef = 1.9f;
				}
			}		
		} else {
			spr = empty_spr;
			if(sym == '_' || sym == '-' || sym == '='){
				// Fall trigger
				objects_create(&obj_fall_trigger_desc, pos, (void*)advance);

				// Coin arc over gap
				if(!tutorial_level){
					for(uint i = 0; i < 5; ++i) {
						float x = pos.x + advance * (-0.3f + i * 0.4f);
						int d = MIN(i, 4 - i);
						float y = v_height - 293.0f;
						if(d)
							y -= (d+1) * 25.0f;

						objects_create(
							&obj_powerup_desc, vec2(x, y), (void*)&coin_powerup
						);
					}
				}
			}
		}
		placement_interval(vec2(pos.x,pos.x + advance),spr);

		ground_x += (float)advance;
	}
	ground_x -= page_width;	
}

static void _gen_mushrooms(void){
	SprHandle spr;	
	int advance = 0;
	static float fg_x = 0;


	while(fg_x < page_width) {

		char sym = mchains_next(fg_chain, &rnd);
		mchains_symbol_info(fg_chain, sym, &advance, &spr);
		Vector2 pos = vec2(fg_page_cursor + fg_x, v_height-128.0f);
		if(spr) advance = (int) sprsheet_get_size_h(spr).x;
		else if (!tutorial_level && coins > 0){
			coins_cd = 2;
			Vector2 p = vec2(pos.x + advance / 2.0f, v_height-189.0f);
			SprHandle spr = sprsheet_get_handle(coin_powerup.spr);
			float width = sprsheet_get_size_h(spr).x;

			// checks if a token can be placed on the ground
			// (does not apply for air tokens elsewhere)
			if(placement_allowed(vec2(p.x,p.x+width),spr) &&
				fg_x + advance / 2.0f < page_width){
				objects_create(&obj_powerup_desc, p, (void*)&coin_powerup);
				coins--;					
			}
			
		}
				
		if(spr && placement_allowed(vec2(pos.x,pos.x + advance), spr)) {
			if(sym == 'x'){
				objects_create(&obj_cactus_desc, pos, (void*)spr);

				// placing bomb powerup after cactuses
				Vector2 p = vec2(pos.x + advance / 2.0f + 100.0f, v_height-189.0f);
				place_powerup(&obj_powerup_desc, p, ROCKET);
				
			} else {
				objects_create(&obj_mushroom_desc, pos, (void*)spr);
				
				// Placing coins/powerups on big shrooms
				if(!tutorial_level){
					Vector2 size = sprsheet_get_size_h(spr);
					float width = size.x;
					float height = size.y;
					Vector2 p = vec2_add(pos, vec2(width/2.0f,-height - 50.0f));	
					if(sym == 'j'){
						// Place rocket or a coin on top of mushroom
						if(!place_powerup(&obj_powerup_desc, p, BOMB))
							objects_create(&obj_powerup_desc, p, (void*)&coin_powerup);

					} else if(sym == 'h'){
						// Place shield or a coin on top of mushroom
						if(!place_powerup(&obj_powerup_desc, p, SHIELD))
							objects_create(&obj_powerup_desc, p, (void*)&coin_powerup);
					} else if(sym == 'c'){
						// placing trampoline powerup after certain mushroom
						Vector2 p = vec2(pos.x + advance / 2.0f + 200.0f, v_height-189.0f);
						place_powerup(&obj_powerup_desc, p, TRAMPOLINE);

					} else if(height >= 265.0f)
						objects_create(&obj_powerup_desc, p, (void*)&coin_powerup);
				}
			}	
		}

		fg_x += (float)advance;
	}
	fg_x -= page_width;	
}

static void _gen_fg_page(void) {
	placement_reset();
	
	// Add ground
	_gen_ground();

	// Add foreground mushrooms
	_gen_mushrooms();		

	if(coins_cd > 0) coins_cd--;
	if(coins_cd == 0) coins = 3;

	fg_page_cursor += page_width;
}

void worldgen_reset(uint seed, const LevelDesc* desc) {
	if(!rnd) {
		// First time
		rand_init_ex(&rnd, seed);
		page_width = v_width;
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

	bg_chain = mchains_new(desc->bg_chain);
	fg_chain = mchains_new(desc->fg_chain);
	ground_chain = mchains_new(desc->ground_chain);

	_gen_bg_page();
	_gen_fg_page();

	// reset coin counters (3 coins every 2nd page)
	coins = 3;
	coins_cd = 2;

	// reset powerup counters
	for(int i = 0; i < POWERUP_COUNT;i++){
		powerups[i] = levels_current_desc()->powerup_num[i];
	}

	objects_create(&obj_eraser_desc, vec2(-256.0f,0.0f), (void*)NULL);
	objects_create(&obj_eraser_desc, vec2(-256.0f,400.0f), (void*)NULL);
}

void worldgen_close(void) {
	mchains_del(fg_chain);
	mchains_del(bg_chain);
	mchains_del(ground_chain);
	rand_free_ex(&rnd);
}

void worldgen_update(float fg_camera_extent_max, float bg_camera_extent_max) {
	while(fg_camera_extent_max >= fg_page_cursor) {
		_gen_fg_page();
	}
	while(bg_camera_extent_max >= bg_page_cursor) {
		_gen_bg_page();
	}
}
