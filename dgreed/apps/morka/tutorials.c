#include "tutorials.h"
#include "hud.h"
#include "common.h"
#include "game.h"
#include "game_over.h"
#include <vfont.h>

TutorialStep level1_steps[] = {

	/*
	{0.0f,					// delay before finishing a step
	"Example step",			// text
	 40.0f,					// text font size
	 {0.5f,0.5f},			// text position ( 0.5f * width, 0.5f * height )
	 "spikeshroom_1",		// image
	 "level_select",		// state to push on event
	 {50.0f, 50.0f},		// finger touch animation (negative values to hide)
	 true,					// pause game on event
	 true,					// force jump/dive after event
	 false,					// disable input from player
	 false,					// show touch hints for player
	 MUSHROOM_IN_FRONT,		// event show trigger
	 COMBO_X3				// event dismiss trigger or (NONE, FINAL_STEP)
	},
	*/

	{0.0f,
	 "Welcome to the training",
	 40.0f,
	 {0.5f,0.178385416f},
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 true,
	 false,
	 AUTO,
	 MUSHROOM_IN_FRONT
	},

	{0.0f,
	 "Touch to jump",
	 40.0f,
	 {0.5f,0.178385416f},
	 NULL,
	 NULL,
	 {0.5, 0.390625},
	 true,
	 true,
	 true,
	 false,
	 MUSHROOM_IN_FRONT,
	 AUTO
	},

	{0.0f,
	 "Touch and hold to plunge down",
	 40.0f,
	 {0.5f,0.178385416f}, 
	 NULL,
	 NULL,
	 {0.5f, 0.390625},
	 true,
	 true,
	 true,
	 false,
	 MUSHROOM_BELOW,
	 AUTO
	},
	{0.5f,
	 NULL,
	 0.0f,
	 {0.0f,0.0f},
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 true,
	 false,
	 AUTO,
	 MUSHROOM_BELOW
	},	
	{0.0f,
	 "You're doing great!",
	 40.0f,
	 {0.5f,0.178385416f},
	 NULL,
	 NULL,
	 {0.5f, 0.390625},
	 true,
	 true,
	 true,
	 false,
	 MUSHROOM_BELOW,
	 AUTO
	},
	{0.5f,
	 NULL,
	 0.0f,
	 {0.0f,0.0f},
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 true,
	 false,
	 AUTO,
	 MUSHROOM_BELOW
	},			
	{0.0f,
	 "One more time",
	 40.0f,
	 {0.5f,0.178385416f},	 
	 NULL,
	 NULL,
	 {0.5f, 0.390625},
	 true,
	 true,
	 true,
	 false,
	 MUSHROOM_BELOW,
	 AUTO
	},	
	{0.0f,
	 "Now try it all by yourself",
	 40.0f,
	 {0.5f,0.178385416f},	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 BOUNCE_PERFORMED,
	 COMBO_X3
	},

	{2.5f,
	 "You are ready for the race!",
	 40.0f,
	 {0.5f,0.178385416f},	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 COMBO_X3,
	 AUTO
	},

	{1.0f,
	 NULL,
	 0.0f,
	 {0.0f,0.0f},
	 NULL,
	 "game_over",
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 FINAL_STEP
	}

};

TutorialStep default_steps[] = {
	{0.5f,
	 NULL,
	 0.0f,
	 {0.0f,0.0f},	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 AUTO
	},

	{0.625f,
	 "3",
	 150.0f,
	 {0.5f,0.5f},	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 AUTO
	},

	{0.625f,
	 "2",
	 150.0f,
	 {0.5f,0.5f},	 	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 AUTO
	},

	{0.625f,
	 "1",
	 150.0f,
	 {0.5f,0.5f},	 	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 AUTO
	},

	{0.625f,
	 "Go!",
	 150.0f,
	 {0.5f,0.5f},	 	 
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 AUTO
	},

	{0.0f,
	 NULL,
	 0.0f,
	 {0.0f,0.0f},
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 false,
	 false,
	 AUTO,
	 FINAL_STEP
	}

};

TutorialScenario scenarios[] = {
	{"level1", level1_steps},
	{"default", default_steps},
	{NULL, NULL}
};

const static float transition = 1.0f;
static float start_time = 0.0f;
static float end_time = 0.0f;

static TutorialScenario* current_scenario = NULL;
static TutorialStep* current_step = NULL;
static TutorialStep* prev_step = NULL;	// for text fadeout
static uint step_i = 0;
static SprHandle finger;
static SprHandle star;

static float delay = 0.0f;
static bool show_tutorials = true;		// are tutorials enabled overall
static bool tutorial_active = false;	// is a tutorial step being displayed
static bool step_done = false;
static bool hint_press = false;

static bool state_changed = false;

static ObjRabbit* rabbit = NULL;



void tutorials_init(void){
	finger = sprsheet_get_handle("tut_finger");
	star = sprsheet_get_handle("tut_star");
}

void tutorials_reset(ObjRabbit* r){
	state_changed = false;
	rabbit = r;
	step_i = 0;
	if(current_scenario) current_step = &current_scenario->steps[step_i];
	else current_step = NULL;
	tutorial_active = false;	
}

void tutorials_enable(void){
	show_tutorials = true;
}
void tutorials_disable(void){
	show_tutorials = false;
}
bool tutorials_are_enabled(void){
	return show_tutorials;
}

void tutorials_hint_press(bool p){
	hint_press = p;
}

bool tutorials_paused(void){
	if(current_step) return current_step->pause_game;
	return false;
}

void tutorials_close(void){
}

void tutorials_set_level(const char* level){
	current_scenario = NULL;
	int i = 0;
	while(scenarios[i].level_name != NULL && current_scenario == NULL){
		if(strcmp(scenarios[i].level_name, level) == 0)
			current_scenario = &scenarios[i];
		i++;
	}
	if(!current_scenario) tutorials_set_level("default");
}

void tutorial_event(EventType e){
	if(current_step != NULL){
		if(e == current_step->show_on && !tutorial_active){
			rabbit->data->input_disabled = current_step->disable_input;
			delay = time_s() + current_step->delay;
			if(current_step->pause_game){
				game_pause();
			}
			tutorial_active = true;
			step_done = false;
			start_time = time_s();
		}
		if(e == current_step->dismiss_on && step_done && tutorial_active){
			prev_step = current_step;
			current_step = &current_scenario->steps[++step_i];
			tutorial_active = false;
			step_done = false;
			end_time = time_s();

			// if the event dismissed this step, it might start the next one too
			tutorial_event(e);
		}
	}

}

static void _tutorial_image(byte a){
	Vector2 pos = vec2(v_width/2.0f,v_height/2.0f);
	SprHandle handle = sprsheet_get_handle(current_step->img);
	Vector2 size = sprsheet_get_size_h(handle);

	RectF rec = {
		.left = pos.x - size.x / 2.0f, 
		.top = pos.y - size.y / 2.0f,
		.right = 0,
		.bottom = 0
	};

	spr_draw_h(handle, hud_layer,rec,COLOR_RGBA(255, 255, 255, a));
}

static void _tutorial_text(TutorialStep* step,byte a){
	static TutorialStep* old = NULL;

	char str[32];
	sprintf(str, "%s",step->text);

	vfont_select(FONT_NAME, step->font_size); 
	static Vector2 half_size = {0.0f, 0.0f};
	if(half_size.x == 0.0f || old != step) {
		half_size = vec2_scale(vfont_size(str), 0.5f);
	}
	Color col = COLOR_RGBA(255, 255, 255, a);
	Vector2 pos = vec2(v_width * step->text_pos.x,v_height * step->text_pos.y);
	vfont_draw(str, hud_layer, vec2_sub(pos, half_size),col);
	old = step;
}

static void _tutorial_finger_animation(Vector2 finger_pos,byte a){
	Vector2 half_size = vec2_scale(sprsheet_get_size_h(finger),0.5f);
	finger_pos = vec2(v_width * finger_pos.x,v_height * finger_pos.y);
	finger_pos = vec2_sub(finger_pos,half_size);

	// animate finger according to hints, or loop pressing animation
	float d = 0.0f;
	if(current_step->show_hint_press){
		if(hint_press) d = 1.0f;
	} else {
		d = sinf(time_s()*6.0f);
	}	

	// draw star on finger touch
	if(d > 0.0f){
		RectF rec_star = {
			.left = finger_pos.x - 25.0f, 
			.top = finger_pos.y - d*3.0f - 35.0f,
			.right = 0,
			.bottom = 0
		};
		spr_draw_h(star, hud_layer,rec_star,COLOR_RGBA(255, 255, 255, a));
	}

	// animated finger
	RectF rec_finger = {
		.left = finger_pos.x, 
		.top = finger_pos.y - d*5.0f,
		.right = 0,
		.bottom = 0
	};
	spr_draw_h(finger, hud_layer,rec_finger,COLOR_RGBA(255, 255, 255, a));
}

bool tutorials_render(float t){
	if(tutorial_active){

		float transition_alpha = 1.0f-fabsf(t);
		float ts = time_s();
		float td = 0.0f;

		if(current_step->dismiss_on == AUTO && !current_step->pause_game){
			td = normalize(ts,start_time,start_time + current_step->delay);
			td = clamp(0.0f,1.0f,td);
		} else {
			td = normalize(ts,start_time,start_time + transition);
			td = clamp(0.0f,0.5f,td);
		}

		float alpha = sin(PI*td);
		alpha *= transition_alpha;
		byte a = lrintf(255.0f * alpha);		

		// Draw tutorial image
		if(current_step->img) _tutorial_image(a);

		// Draw tutorial text
		if(current_step->text) _tutorial_text(current_step,a);

		// Finger animation
		if(current_step->finger_pos.x >= 0.0f && current_step->finger_pos.y >= 0.0f)
			_tutorial_finger_animation(current_step->finger_pos, a);
		
		// If tutorial has paused gameplay, unpause on touch
		if(current_step->pause_game){
			if(ts > delay && key_down(KEY_A)){

				// Force input on unpause
				if(current_step->force_input){
					if(rabbit->data->touching_ground)
						rabbit->data->force_jump = true;
					else
						rabbit->data->force_dive = true;
				}
				prev_step = current_step;
				if(current_step->dismiss_on != FINAL_STEP){
					current_step = &current_scenario->steps[++step_i];
				}

				tutorial_active = false;
				step_done = false;
				end_time = time_s();

				game_unpause();
			}	
		}
			
		// If current step has to change gamestate, push that state once	
		if(current_step->state != NULL && !state_changed){
			game_end();
			game_over_set_screen(TUTORIAL_SCREEN);
			malka_states_push(current_step->state);
			tutorial_active = false;
			step_done = true;
			state_changed = true;
		} 

		// If tutorial delay time has ran out, this step is done
		if(!current_step->pause_game && ts > delay) step_done = true;
	}

	// fade out previous text if necessary
	if(prev_step != NULL && prev_step->text != NULL &&
		(prev_step->dismiss_on != AUTO || prev_step->pause_game) ){

		float transition_alpha = 1.0f-fabsf(t);
		float ts = time_s();

		float tt = normalize(ts + transition/2.0f,end_time,end_time + transition);
		tt = clamp(0.5f,1.0f,tt);
		float alpha2 = sin(PI * tt);

		alpha2 *= transition_alpha;
		byte a2 = lrintf(255.0f * alpha2);

		_tutorial_text(prev_step,a2);
	}


	return true;
}