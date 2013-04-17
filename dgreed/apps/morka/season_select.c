#include "season_select.h"
#include "level_select.h"
#include "game.h"
#include "levels.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

static void season_select_init(void) {

}

static void season_select_close(void) {

}

static void season_select_enter(void) {
	game_pause();
}

static void season_select_leave(void) {

}

static bool season_select_update(void) {
	game_update_empty();
	return true;
}

static bool season_select_render(float t) {
	UIElement* element = uidesc_get("season_select");

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, v_width, v_height), col); 

	SprHandle lock_spr;
	char season_text[16];


	// Season buttons
	for(int i = 0; i < 2; i++){
		char season_name[16];
		sprintf(season_name, "season%d", i+1);
		UIElement* season = uidesc_get_child(element, season_name);
		spr_draw_cntr_h(season->spr, hud_layer, season->vec2, 0.0f, 1.0f, col);

		switch(i){
			case WINTER:
				lock_spr = sprsheet_get_handle("lock_winter");
				sprintf(season_text, "Winter");
			break;

			default:
				lock_spr = sprsheet_get_handle("lock_autumn");
				sprintf(season_text, "Autumn");
			break;
		}

		Vector2 button_pos = season->vec2;

		if(level_is_unlocked(levels_start_of_season(i))){

			if(touches_down() && t == 0.0f) {
				Touch* t = touches_get();

				if(!t)
					continue;

				Vector2 hit_pos = t[0].hit_pos;
				

				float r_sqr = 70.0f * 70.0f;
				if(vec2_length_sq(vec2_sub(hit_pos, button_pos)) < r_sqr) {
					level_select_set_season(i);
					malka_states_push("level_select");
				}
			}	

			vfont_select(FONT_NAME, 58.0f);
			char season_num[2];
			sprintf(season_num, "%d", i+1);
			Vector2 half_size = vec2_scale(vfont_size(season_num), 0.5f);	
			vfont_draw(season_num, hud_layer, vec2_sub(button_pos,half_size), col);

			vfont_select(FONT_NAME, 20.0f);
			Vector2 half_size2 = vec2_scale(vfont_size(season_text), 0.5f);
			button_pos.y += 30.0f;	
			vfont_draw(season_text, hud_layer, vec2_sub(button_pos,half_size2), col);						



		} else {
			spr_draw_cntr_h(lock_spr, hud_layer, button_pos, 0.0f, 1.0f, col);
		}
	}

	return true;
}

StateDesc season_select_state = {
	.init = season_select_init,
	.close = season_select_close,
	.enter = season_select_enter,
	.leave = season_select_leave,
	.update = season_select_update,
	.render = season_select_render
};
