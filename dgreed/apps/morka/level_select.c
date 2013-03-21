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
		game_request_reset();		
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

	UIElement* element = uidesc_get("level_select");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* button_quit = uidesc_get_child(element, "quit");
	SprHandle button_spr;
	SprHandle lock_spr;	

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Pick a level:";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(text->vec2, half_size), col);

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
	if(level_count < columns) columns = level_count;
	uint c = 0;

	Vector2 spacing = vec2(size.x / 2.0f,size.y / 2.0f);

	Vector2 button_pos = vec2(WIDTH / 2.0f, 256.0f);

	button_pos.x -= ( size.x * (columns-1) +  (spacing.x * (columns-1) ) ) / 2.0f; 

	for(int i = offset; i < offset + level_count;i++){

		LevelDesc* desc= (LevelDesc*) darray_get(&levels_descs,i);

		// Draw button
		spr_draw_cntr_h(button_spr, layer, button_pos, 0.0f, 1.0f, col);

		if(desc->locked){
			// Draw lock
			spr_draw_cntr_h(lock_spr, layer, button_pos, 0.0f, 1.0f, col);			
		} else {

			// Button action
			if(touches_down() && t == 0.0f) {
				Touch* t = touches_get();

				if(!t)
					continue;

				Vector2 hit_pos = t[0].hit_pos;

				float r_sqr = 20.0f * 20.0f;
				if(vec2_length_sq(vec2_sub(hit_pos, button_pos)) < r_sqr) {
					levels_reset(desc->name);
					game_request_reset();
					malka_states_push("game");
				}
			}

			// Draw number
			char n[4];
			sprintf(n, "%d",i-offset+1);
			Vector2 half_size = vec2_scale(vfont_size(n), 0.5f);
			vfont_draw(n, layer, vec2_sub(button_pos,half_size), col);
		}

		// newline at end of column
		if(++c >= columns){
			c = 0;
			button_pos.x -= (size.x + spacing.x) * (columns-1);
			button_pos.y += size.y + spacing.y;
		} else {
			button_pos.x += size.x + spacing.x;
		}			

	}

	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_replace("season_select");
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
