#include "level_select.h"
#include "game.h"
#include "common.h"
#include <uidesc.h>
#include <vfont.h>

extern uint level_id;

static void level_select_init(void) {

}

static void level_select_close(void) {

}

static void level_select_enter(void) {
	printf("entering level select\n");
	//game_reset_empty();
}

static void level_select_leave(void) {
}

static bool level_select_update(void) {
	game_update_empty();
	return true;
}

static bool level_select_render(float t) {
	// Game scene
	game_render(t);

	UIElement* element = uidesc_get("level_select");
	uint layer = hud_layer+1;

	UIElement* text = uidesc_get_child(element, "text");
	UIElement* level1 = uidesc_get_child(element, "level1");
	UIElement* level2 = uidesc_get_child(element, "level2");
	UIElement* level3 = uidesc_get_child(element, "level3");
	UIElement* level4 = uidesc_get_child(element, "level4");
	UIElement* level5 = uidesc_get_child(element, "level5");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	spr_draw("blue_shade", layer, rectf(0.0f, 0.0f, 1024.0f, 768.0f), COLOR_WHITE); 

	// Text
	vfont_select(FONT_NAME, 48.0f); 
	const char* str = "Pick a level:";
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	vfont_draw(str, layer, vec2_sub(text->vec2, half_size), COLOR_WHITE);

	// Level 1
	spr_draw_cntr_h(level1->spr, layer, level1->vec2, 0.0f, 1.0f, COLOR_WHITE);	
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, level1->vec2)) < 40.0f * 40.0f) {
			level_id = 1;
			game_reset();
			//game_was_reset = true;
			game_unpause();
			time_scale(1.0f);
			malka_states_push("game");
		}
	}

	// Level 2
	spr_draw_cntr_h(level2->spr, layer, level2->vec2, 0.0f, 1.0f, COLOR_WHITE);	
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, level2->vec2)) < 40.0f * 40.0f) {
			level_id = 2;	
			game_reset();
			//game_was_reset = true;
			game_unpause();
			time_scale(1.0f);
			malka_states_push("game");
		}
	}

	// Level 3
	spr_draw_cntr_h(level3->spr, layer, level3->vec2, 0.0f, 1.0f, COLOR_WHITE);	
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, level3->vec2)) < 40.0f * 40.0f) {
			level_id = 3;			
			game_reset();
			//game_was_reset = true;
			game_unpause();
			time_scale(1.0f);
			malka_states_push("game");
		}
	}

	// Level 4
	spr_draw_cntr_h(level4->spr, layer, level4->vec2, 0.0f, 1.0f, COLOR_WHITE);	
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, level4->vec2)) < 40.0f * 40.0f) {
			level_id = 4;			
			game_reset();
			//game_was_reset = true;
			game_unpause();
			time_scale(1.0f);
			malka_states_push("game");
		}
	}

	// Level 5
	spr_draw_cntr_h(level5->spr, layer, level5->vec2, 0.0f, 1.0f, COLOR_WHITE);	
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, level5->vec2)) < 40.0f * 40.0f) {
			level_id = 5;
			game_reset();
			//game_was_reset = true;
			game_unpause();
			time_scale(1.0f);
			malka_states_push("game");
		}
	}

	// Quit Button
	spr_draw_cntr_h(button_quit->spr, layer, button_quit->vec2, 0.0f, 1.0f, COLOR_WHITE);	
	if(touches_count() > 0) {
		Touch* t = touches_get();
		if(vec2_length_sq(vec2_sub(t[0].hit_pos, button_quit->vec2)) < 40.0f * 40.0f) {
			// Close program
		}
	}
	
	return true;
}

StateDesc level_select_state = {
	.init = level_select_init,
	.close = level_select_close,
	.enter = level_select_enter,
	.leave = level_select_leave,
	.update = level_select_update,
	.render = level_select_render
};
