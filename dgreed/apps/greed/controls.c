#include "controls.h"
#include "game.h"
#include "state.h"

#include <touch_ctrls.h>

#define CONTROLS_LAYER 14

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

void controls_init(void) {
}

void controls_close(void) {
}

void controls_draw(float fadein) {
	if(fadein == 0.0f) {
		switch(pstate.control_type) {
			case 0: {
				joystick = touch_joystick(gui_tex, CONTROLS_LAYER, 
					&joystick_back_src, &joystick_nub_src,
					&pstate.joystick_pos);

				shoot_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&shoot_btn_src, &pstate.shoot_btn_pos);

				acc_btn = touch_button(gui_tex, CONTROLS_LAYER,
					&acc_btn_src, &pstate.acc_btn_pos);

				break;
			}
		}
	}
}

