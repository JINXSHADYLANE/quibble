#include "tutorials.h"
#include "hud.h"
#include "common.h"
#include "game.h"
#include <vfont.h>

TutorialStep level1_steps[] = {

	/*
	{"Example step",		// text
	 "spikeshroom_1",		// image
	 "level_select",		// state to push on event
	 {50.0f, 50.0f},		// finger touch animation (negative values to hide)
	 true,					// pause game on event
	 true,					// force jump/dive after event
	 MUSHROOM_IN_FRONT,		// event show trigger
	 COMBO_X3				// event dismiss trigger or (NONE, FINAL_STEP)
	},
	*/

	{"Touch to jump",
	 NULL,
	 NULL,
	 {WIDTH/2.0f, 600.0f},
	 true,
	 true,
	 MUSHROOM_IN_FRONT,
	 NONE
	},

	{"Touch and hold to plunge down",
	 NULL,
	 NULL,
	 {WIDTH/2.0f, 600.0f},
	 true,
	 true,
	 MUSHROOM_BELOW,
	 NONE
	},

	{"Now do a triple bounce",
	 NULL,
	 NULL,
	 {-1.0f, -1.0f},
	 false,
	 false,
	 BOUNCE_PERFORMED,
	 COMBO_X3
	},

	{NULL,
	 NULL,
	 "game_over",
	 {-1.0f, -1.0f},
	 false,
	 false,
	 COMBO_X3,
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
static bool tutorial_unpaused = false;	// should the current step be displayed when game is unpaused

static ObjRabbit* rabbit = NULL;

void tutorials_reset(ObjRabbit* r){
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

		if(e == current_step->show_on){

			if(current_step->pause_game){
				tutorial_unpaused = false;
				game_pause();
				malka_states_push("tutorial_pause");
				wait_t = time_s() + 1.0f;
			} else {
				tutorial_unpaused = true;
				wait_t = 0.0f;
			}	

			tutorial_active = true;

		} else if(e == current_step->dismiss_on){
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

			// (bug)
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

		if(finger_pos.x >= 0.0f && finger_pos.y >= 0.0f){

			float td = time_s();

			float d = sinf(td*6);

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

		// Unpause game and hide tutorial text on input
		if(wait_t > 0.0f){
			float ts = time_s();
			if(ts > wait_t && key_pressed(KEY_A)){

				if(current_step->force_input){
					if(rabbit->data->touching_ground)
						rabbit->data->force_jump = true;
					else
						rabbit->data->force_dive = true;
				}

				game_unpause();
				malka_states_pop();
				if(current_step->dismiss_on != FINAL_STEP)
					current_step = &current_scenario->steps[++step_i];
				tutorial_active = false;
			}	
		}

		if(current_step->state != NULL){
			game_end();
			malka_states_push(current_step->state);
			tutorial_active = false;
			tutorial_unpaused = false;
		} 

	}

	return true;
}

bool tutorials_show_unpaused(void){
	return tutorial_unpaused;
}