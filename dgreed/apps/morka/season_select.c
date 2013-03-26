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
	uint layer = hud_layer+1;

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), col); 

	// Season buttons
	for(int i = 0; i < 2; i++){
		char season_name[16];
		sprintf(season_name, "season%d", i+1);
		UIElement* season = uidesc_get_child(element, season_name);
		spr_draw_cntr_h(season->spr, layer, season->vec2, 0.0f, 1.0f, col);

		TexHandle tex;
		RectF src;
		Vector2 cntr_off;
		sprsheet_get_ex_h(season->spr, &tex, &src, &cntr_off);	

		if(touches_down() && t == 0.0f) {
			Touch* t = touches_get();

			if(!t)
				continue;

			Vector2 hit_pos = t[0].hit_pos;
			
			Vector2 button_pos = vec2_add(season->vec2, cntr_off);
			float r_sqr = 70.0f * 70.0f;
			if(vec2_length_sq(vec2_sub(hit_pos, button_pos)) < r_sqr) {
				level_select_set_season(i);
				malka_states_replace("level_select");
			}
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
