#include "level_select.h"
#include "game.h"
#include "levels.h"
#include "common.h"
#include "hud.h"
#include <uidesc.h>
#include <vfont.h>

static SeasonType current_season = AUTUMN;

void level_select_set_season(SeasonType season){
	current_season = season;
}

static void level_select_init(void) {

}

static void level_select_close(void) {

}

static void level_select_preenter(void){
	if(levels_current_desc()->season != current_season){
		int offset = levels_start_of_season(current_season);
		LevelDesc* desc= (LevelDesc*) darray_get(&levels_descs,offset);		
		levels_reset(desc->name);
		game_force_reset();		
	}	
}

static void level_select_enter(void) {
	game_pause();
}

static void level_select_leave(void) {

}

static bool level_select_update(void) {
	game_update_empty();
	return true;
}

static bool level_select_render(float t) {
	if(t != 0.0f) game_update_empty();

	vfont_select(FONT_NAME, 48.0f);

	UIElement* element = uidesc_get("level_select");

	UIElement* buttons_pos = uidesc_get_child(element, "buttons_pos");
	UIElement* button_quit = uidesc_get_child(element, "quit");
	SprHandle place_spr;
	SprHandle button_spr;
	SprHandle lock_spr;	

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, WIDTH, HEIGHT), col); 

	int level_count = levels_count(current_season);
	int offset = levels_start_of_season(current_season);

	switch(current_season){
		case WINTER:
			button_spr = sprsheet_get_handle("ball_winter");
			lock_spr = sprsheet_get_handle("lock_winter");
		break;

		default:
			button_spr = sprsheet_get_handle("ball_autumn");
			lock_spr = sprsheet_get_handle("lock_autumn");
		break;
	}

	Vector2 size = sprsheet_get_size_h(button_spr);
	uint columns = 6;
	float result = (float) level_count / columns;
	uint rows = ceil(result);

	if(level_count < columns) columns = level_count;
	uint c = 0;

	Vector2 spacing = vec2(size.x / 2.0f,size.y / 2.0f);

	Vector2 pos = buttons_pos->vec2;

	pos.x -= ( size.x * (columns-1) +  (spacing.x * (columns-1) ) ) / 2.0f; 
	pos.y -= ( size.y * (rows-1) +  (spacing.y * (rows-1) ) ) / 2.0f;

	for(int i = offset; i < offset + level_count;i++){

		LevelDesc* desc= (LevelDesc*) darray_get(&levels_descs,i);

		// Draw level icon
		spr_draw_cntr_h(button_spr, hud_layer, pos, 0.0f, 1.0f, col);

		if(!level_is_unlocked(i)){
			// Draw lock
			spr_draw_cntr_h(lock_spr, hud_layer, pos, 0.0f, 1.0f, col);			
		} else {

			uint place = levels_get_place(i);
			char place_txt[16];


			switch(place){
				case 1:
					place_spr = sprsheet_get_handle("ribbon_first");
					sprintf(place_txt, "First");
				break;

				case 2:
					place_spr = sprsheet_get_handle("ribbon_second");
					sprintf(place_txt, "Second");
				break;	

				case 3:
					place_spr = sprsheet_get_handle("ribbon_third");
					sprintf(place_txt, "Third");
				break;

				case 4:
					place_spr = sprsheet_get_handle("ribbon_last");
					sprintf(place_txt, "Last");
				break;	

				default:
					place_spr = empty_spr;
				break;						
			}
			if(place_spr != empty_spr){

				Vector2 ribbon_pos = vec2_add(pos,vec2(0.0f,40.0f));

				spr_draw_cntr_h(place_spr, hud_layer, ribbon_pos, 0.0f, 1.0f, col);

				vfont_select(FONT_NAME, 13.0f);

				Vector2 half_size = vec2_scale(vfont_size(place_txt), 0.5f);
				vfont_draw(place_txt, hud_layer, vec2_sub(ribbon_pos,half_size), col);				
			}

			// Button action
			if(touches_down() && t == 0.0f) {
				Touch* t = touches_get();

				if(!t)
					continue;

				Vector2 hit_pos = t[0].hit_pos;

				float r_sqr = 20.0f * 20.0f;
				if(vec2_length_sq(vec2_sub(hit_pos, pos)) < r_sqr) {
					levels_reset(desc->name);
					malka_states_push("shop");
				}
			}
			vfont_select(FONT_NAME, 48.0f);
			// Draw number
			char n[4];
			sprintf(n, "%d",i-offset+1);
			Vector2 half_size = vec2_scale(vfont_size(n), 0.5f);
			vfont_draw(n, hud_layer, vec2_sub(pos,half_size), col);
		}

		// newline at end of column
		if(++c >= columns){
			c = 0;
			pos.x -= (size.x + spacing.x) * (columns-1);
			pos.y += size.y + spacing.y;
		} else {
			pos.x += size.x + spacing.x;
		}			

	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	return true;
}

StateDesc level_select_state = {
	.init = level_select_init,
	.close = level_select_close,
	.enter = level_select_enter,
	.preenter = level_select_preenter,
	.leave = level_select_leave,
	.update = level_select_update,
	.render = level_select_render
};
