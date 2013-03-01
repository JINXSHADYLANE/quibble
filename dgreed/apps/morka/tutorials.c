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
	 NULL,
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 true,
	 false,
	 AUTO,
	 AUTO
	},

	{1.0f,
	 "Touch to jump",
	 NULL,
	 NULL,
	 {WIDTH/2.0f, 600.0f},
	 true,
	 true,
	 true,
	 false,
	 MUSHROOM_IN_FRONT,
	 AUTO
	},

	{1.0f,
	 "Touch and hold to plunge down",
	 NULL,
	 NULL,
	 {WIDTH/2.0f, 600.0f},
	 true,
	 true,
	 false,
	 false,
	 MUSHROOM_BELOW,
	 AUTO
	},

	{0.0f,
	 "Now do a triple bounce",
	 NULL,
	 NULL,
	 {WIDTH/2.0f, 600.0f},
	 false,
	 false,
	 false,
	 true,
	 BOUNCE_PERFORMED,
	 COMBO_X3
	},

	{2.5f,
	 "You are ready for the race!",
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

TutorialScenario scenarios[] = {
	{"level1", level1_steps},
	{NULL, NULL}
};

static TutorialScenario* current_scenario = NULL;
static TutorialStep* current_step = NULL;
static uint step_i = 0;
static SprHandle finger;
static SprHandle star;

static float wait_t = 0.0f;
static bool show_tutorials = true;		// are tutorials enabled overall
static bool tutorial_active = false;	// is a tutorial step active and being displayed
static bool tutorial_paused = false;	// should the current step be displayed when game is unpaused
static bool step_done = false;
static bool hint_press = false;

static bool state_changed = false;

static ObjRabbit* rabbit = NULL;

void tutorials_reset(ObjRabbit* r){
	state_changed = false;
	rabbit = r;
	step_i = 0;
	if(current_scenario != NULL) current_step = &current_scenario->steps[step_i];
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

void tutorials_init(void){
	finger = sprsheet_get_handle("tut_finger");
	star = sprsheet_get_handle("tut_star");
}

void tutorials_set_level(const char* level){
	current_scenario = NULL;
	int i = 0;
	while(scenarios[i].level_name != NULL && current_scenario == NULL){
		if(strcmp(scenarios[i].level_name, level) == 0) current_scenario = &scenarios[i];
		i++;
	}
}

void tutorial_event(EventType e){
	if(current_step != NULL){
		if(e == current_step->show_on && !tutorial_active){
			rabbit->data->input_disabled = current_step->disable_input;
			wait_t = time_s() + current_step->delay;
			if(current_step->pause_game){
				tutorial_paused = true;
				game_pause();
				malka_states_push("tutorial_pause");
			} else {
				tutorial_paused = false;
			}	

			tutorial_active = true;

		}
		if(e == current_step->dismiss_on && step_done){
			step_done = false;

			current_step = &current_scenario->steps[++step_i];
			tutorial_active = false;

			// if the event dismissed this step, it might start the next one too
			tutorial_event(e);
		}
	}

}

void tutorials_close(void){

}

bool tutorials_render(float t){

	if(tutorial_active){

		float alpha = 1.0f-fabsf(t);
		byte a = lrintf(255.0f * alpha);
		Color col = COLOR_RGBA(255, 255, 255, a);

		float ts = time_s();

		// Img
		if(current_step->img != NULL){

			Vector2 pos = vec2(WIDTH/2.0f,HEIGHT/2.0f);
			SprHandle handle = sprsheet_get_handle(current_step->img);
			Vector2 size = sprsheet_get_size_h(handle);

			RectF rec = {
				.left = pos.x - size.x / 2.0f, 
				.top = pos.y - size.y / 2.0f,
				.right = 0,
				.bottom = 0
			};

			spr_draw_h(handle, hud_layer,rec,col);
		}

		// Text
		if(current_step->text != NULL){

			Vector2 text_pos = vec2(WIDTH/2.0f,700.0f);
			static TutorialStep* step = NULL;

			char str[32];
			sprintf(str, "%s",current_step->text);

			vfont_select(FONT_NAME, 40.0f); 
			static Vector2 half_size = {0.0f, 0.0f};
			if(half_size.x == 0.0f || step != current_step) {
				half_size = vec2_scale(vfont_size(str), 0.5f);
			}
			vfont_draw(str, hud_layer, vec2_sub(text_pos, half_size), COLOR_RGBA(70, 49, 27, a));
			step = current_step;
		}


		// Finger animation
		Vector2 size = sprsheet_get_size_h(finger);
		Vector2 finger_pos = vec2_sub(current_step->finger_pos,vec2_scale(size,0.5f));

		float d = 0.0f;

		if(finger_pos.x >= 0.0f && finger_pos.y >= 0.0f){

			// animate finger according to hints
			if(current_step->show_hint_press){
				if(hint_press) d = 1.0f;
			} else {
				d = sinf(ts*6.0f);
			}	

			if(d > 0.0f){
				RectF rec_star = {
					.left = finger_pos.x - 25.0f, 
					.top = finger_pos.y - d*3.0f - 35.0f,
					.right = 0,
					.bottom = 0
				};
				spr_draw_h(star, hud_layer,rec_star,col);
			}

			RectF rec_finger = {
				.left = finger_pos.x, 
				.top = finger_pos.y - d*5.0f,
				.right = 0,
				.bottom = 0
			};
			spr_draw_h(finger, hud_layer,rec_finger,col);
		}
		
		if(tutorial_paused){
			if(ts > wait_t && key_pressed(KEY_A)){

				if(current_step->force_input){
					if(rabbit->data->touching_ground)
						rabbit->data->force_jump = true;
					else
						rabbit->data->force_dive = true;
				}

				game_unpause();
				malka_states_pop();
				step_done = false;
				if(current_step->dismiss_on != FINAL_STEP)
					current_step = &current_scenario->steps[++step_i];
				tutorial_active = false;
			}	
		}
			
		if(current_step->state != NULL && !state_changed){
			game_end();
			game_over_set_screen(TUTORIAL_SCREEN);
			malka_states_push(current_step->state);
			tutorial_active = false;
			tutorial_paused = true;
			step_done = true;
			state_changed = true;
		} 

		if(!tutorial_paused && ts > wait_t) step_done = true;
	}
	return true;
}

void tutorials_hint_press(bool p){
	hint_press = p;
}

bool tutorials_show_paused(void){
	return tutorial_paused;
}