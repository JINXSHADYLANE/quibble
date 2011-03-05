#include "controls.h"
#include "game.h"
#include "state.h"
#include "physics.h"
#include "menus.h"

#include <touch_ctrls.h>

#define CONTROLS_LAYER 15

extern TexHandle gui_tex;

static RectF joystick_back_src = {110.0f, 157.0f, 110.f+90.0f, 157.0f+90.0f}; 
static RectF joystick_nub_src = {40.0f, 148.0f, 40.0f+28.0f, 148.0f+28.0f};
static RectF acc_btn_src = {73.0f, 148.0f, 73.0f+28.0f, 148.0f+28.0f};
static RectF shoot_btn_src = {73.0f, 183.0f, 73.0f+28.0f, 183.0f+28.0f};
static RectF left_btn_src = {42.0f, 218.0f, 42.0f+28.0f, 218.0f+28.0f};
static RectF right_btn_src = {72.0f, 218.0f, 72.0f+28.0f, 218.0f+28.0f};

static Vector2 joystick; 
static bool shoot_btn;
static bool acc_btn;
static bool left_btn;
static bool right_btn;

void controls_init(void) {
}

void controls_close(void) {
}

void controls_draw(float fadein) {
	// Draw interactive controls
	if(fadein == 0.0f) {
		switch(pstate.control_type) {
			case 0: {
				acc_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&acc_btn_src, &pstate.acc_btn_pos);
			}		
			case 1: {
				joystick = touch_joystick(gui_tex, CONTROLS_LAYER, 
					&joystick_back_src, &joystick_nub_src,
					&pstate.joystick_pos);
				shoot_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&shoot_btn_src, &pstate.shoot_btn_pos);

				break;
			}		
			case 2: {
				left_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&left_btn_src, &pstate.left_btn_pos);
				right_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&right_btn_src, &pstate.right_btn_pos);
				acc_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&acc_btn_src, &pstate.acc_btn_pos);
				shoot_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&shoot_btn_src, &pstate.shoot_btn_pos);
				break;
			}	
			default:
				LOG_ERROR("Unknown touch control scheme");
		}
	}
	// Draw animated fadein, inactive
	else {
		switch(pstate.control_type) {
			case 0: {
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&acc_btn_src, &pstate.acc_btn_pos, fadein);
			}
			case 1: {
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&joystick_back_src, &pstate.joystick_pos, fadein);
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&joystick_nub_src, &pstate.joystick_pos, fadein);
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&shoot_btn_src, &pstate.shoot_btn_pos, fadein);
				break;
			}
			case 2: {
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&left_btn_src, &pstate.left_btn_pos, fadein);
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&right_btn_src, &pstate.right_btn_pos, fadein);
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&acc_btn_src, &pstate.acc_btn_pos, fadein);
				menus_draw_rect(gui_tex, CONTROLS_LAYER,
					&shoot_btn_src, &pstate.shoot_btn_pos, fadein);
				break;
			}
			default:
				LOG_ERROR("Unknown touch control scheme");
		}
	}
}

void controls_update(uint ship) {
	switch(pstate.control_type) {
		case 0: 
		case 1: {
			float acc;
			float dir = -1.0f;
			if(pstate.control_type == 1) {
				acc = vec2_length(joystick);
				if(acc > 0.0f)
					dir = vec2_dir(vec2(joystick.x, -joystick.y));
			}		
			else {
				acc = acc_btn ? 1.0f : 0.0f;
				if(vec2_length(joystick) > 0.0f)
					dir = vec2_dir(vec2(joystick.x, -joystick.y));
			}	

			physics_control_ship_ex(ship, dir, acc);

			ship_states[ship].is_accelerating = 
				!ship_states[ship].is_accelerating ? acc > 0.3f : true;

			if(shoot_btn)
				game_shoot(ship);

			joystick = vec2(0.0f, 0.0f);
			shoot_btn = false;
			acc_btn = false;
			break;
		}
		case 2: {
			physics_control_ship(ship, left_btn, right_btn, acc_btn);
			if(shoot_btn)
				game_shoot(ship);

			left_btn = right_btn = acc_btn = shoot_btn = false;	
			break;
		}
		default:
			LOG_ERROR("Unknown touch control scheme");
	}
}

bool _good_pos(Vector2 pos) {
	if(feql(pos.x, pstate.joystick_pos.x) && feql(pos.y, pstate.joystick_pos.y))
		return false;
	if(feql(pos.x, pstate.shoot_btn_pos.x) && feql(pos.y, pstate.shoot_btn_pos.y))
		return false;
	if(feql(pos.x, pstate.acc_btn_pos.x) && feql(pos.y, pstate.acc_btn_pos.y))
		return false;
	if(feql(pos.x, pstate.left_btn_pos.x) && feql(pos.y, pstate.left_btn_pos.y))
		return false;
	if(feql(pos.x, pstate.right_btn_pos.x) && feql(pos.y, pstate.right_btn_pos.y))
		return false;
	return true;	
}

void controls_adjust(void) {
	switch(pstate.control_type) {
		case 0: {
			Vector2	new_acc = touch_movable(&pstate.acc_btn_pos, 20.0f);
			if(_good_pos(new_acc))
				pstate.acc_btn_pos = new_acc;
		}
		case 1: {
			Vector2 new_joy = touch_movable(&pstate.joystick_pos, 30.0f);
			if(_good_pos(new_joy))
				pstate.joystick_pos = new_joy;
					Vector2 new_shoot = touch_movable(&pstate.shoot_btn_pos, 20.0f);
			if(_good_pos(new_shoot))
				pstate.shoot_btn_pos = new_shoot;
			break;
		}
		case 2: {
			Vector2	new_left = touch_movable(&pstate.left_btn_pos, 20.0f);
			if(_good_pos(new_left))
				pstate.left_btn_pos = new_left;
			Vector2	new_right = touch_movable(&pstate.right_btn_pos, 20.0f);
			if(_good_pos(new_right))
				pstate.right_btn_pos = new_right;
			Vector2	new_acc = touch_movable(&pstate.acc_btn_pos, 20.0f);
			if(_good_pos(new_acc))
				pstate.acc_btn_pos = new_acc;
			Vector2	new_shoot = touch_movable(&pstate.shoot_btn_pos, 20.0f);
			if(_good_pos(new_shoot))
				pstate.shoot_btn_pos = new_shoot;
			break;	
		}
		default:
			LOG_ERROR("Unknown touch control scheme");
	}
}
