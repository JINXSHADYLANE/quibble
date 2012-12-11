#include "system.h"

#ifndef OSX_XCODE
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

/*
--------------------------
--- Device orientation ---
--------------------------
*/

DevOrient orientation_current(void) {
	return ORIENT_LANDSCAPE_RIGHT;
}

bool orientation_change(DevOrient* new, float* anim_start, float* anim_len) {
	return false;
}

/*
---------------------
--- Running state ---
---------------------
*/

RunStateCallback enter_background_cb = NULL;
RunStateCallback enter_foreground_cb = NULL;

void runstate_background_cb(RunStateCallback cb) {
	enter_background_cb = cb;
}

void runstate_foreground_cb(RunStateCallback cb) {
	enter_foreground_cb = cb;
}

/*
--------------------
--- Acceleration ---
--------------------
*/

void acc_shake_cb(ShakeCallback cb) {
}

/*
-------------
--- Input ---
-------------
*/

uint keybindings[8] = {
	SDLK_UP,
	SDLK_DOWN,
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_z,
	SDLK_x,
	SDLK_p,
	SDLK_ESCAPE
};

// Totaly arbitrary. Only thing known - 256 is not enough.
#define N_KEYS 400
#define N_MOUSE_BUTTONS 4

byte old_keystate[N_KEYS];
byte keystate[N_KEYS];

byte old_mousestate[N_MOUSE_BUTTONS];
byte mousestate[N_MOUSE_BUTTONS];
uint mouse_x, mouse_y;

bool key_pressed(Key key) {
	assert(key < KEY_COUNT);
	return keystate[keybindings[key]];
}

bool char_pressed(char c) {
	return keystate[(size_t)c];
}

bool key_down(Key key) {
	assert(key < KEY_COUNT);
	return keystate[keybindings[key]] && !old_keystate[keybindings[key]];
}

bool char_down(char c) {
	return keystate[(size_t)c] && !old_keystate[(size_t)c];
}

bool key_up(Key key) {
	assert(key < KEY_COUNT);
	return !keystate[keybindings[key]] && old_keystate[keybindings[key]];
}

bool char_up(char c) {
	return !keystate[(size_t)c] && old_keystate[(size_t)c];
}

bool mouse_pressed(MouseButton button) {
	assert(button < N_MOUSE_BUTTONS);
	return mousestate[button];
}

bool mouse_down(MouseButton button) {
	assert(button < N_MOUSE_BUTTONS);
	return mousestate[button] && !old_mousestate[button];
}

bool mouse_up(MouseButton button) {
	assert(button < N_MOUSE_BUTTONS);
	return !mousestate[button] && old_mousestate[button];
}

void mouse_pos(uint* x, uint* y) {
	assert(x);
	assert(y);

	*x = (uint)((float)mouse_x * x_size_factor);
	*y = (uint)((float)mouse_y * y_size_factor);
}

Vector2 mouse_vec(void) {
	uint x, y;
	mouse_pos(&x, &y);
	return vec2((float)x, (float)y);
}

static Vector2 touch_hitpos;
static float touch_hittime;
static Touch touch;

uint touches_count(void) {
	return mouse_pressed(MBTN_PRIMARY) ? 1 : 0;
}

Touch* touches_get(void) {
	if(mouse_pressed(MBTN_PRIMARY)) {
		touch.hit_pos = touch_hitpos;
		touch.hit_time = touch_hittime;
		touch.pos = mouse_vec();
		return &touch;
	}
	return NULL;
}

/*
------------------
--- Text Input ---
------------------
*/

// Stub implementation

void txtinput_start(void) {
}

const char* txtinput_get(void) {
	return NULL;
}

const char* txtinput_did_end(void) {
	return NULL;
}

const char* txtinput_end(void) {
	return NULL;
}

void txtinput_clear(void) {
}

/*
-------------------
--- Time & Misc ---
-------------------
*/

float t_ms = 0.0f, t_d = 0.0f;
float t_scale = 1.0f;
uint last_frame_time = 0, last_fps_update = 0;
uint fps_count = 0;
uint fps = 0;
uint last_time = 0;

float time_s(void) {
	return t_ms / 1000.0f;
}

float time_ms(void) {
	return t_ms;
}

float time_delta(void) {
	//return t_d;
	return (float)MS_PER_FRAME;
}

uint time_fps(void) {
	return fps;
}

void time_scale(float s) {
	t_scale = s;
}

uint time_ms_current(void) {
	return SDL_GetTicks();
}

void _time_update(uint current_time) {
	t_ms += (float)(current_time - last_frame_time) * t_scale;
	t_d = (float)(current_time - last_frame_time) * t_scale;
	if(last_fps_update + 1000 < current_time) {
		fps = fps_count;
		fps_count = 0;
		last_fps_update = current_time;
	}
	fps_count++;
	last_frame_time = current_time;
}

uint _sdl_to_greed_mbtn(uint mbtn_id) {
	switch (mbtn_id) {
		case SDL_BUTTON_LEFT:
			return MBTN_LEFT;
		case SDL_BUTTON_RIGHT:
			return MBTN_RIGHT;
		case SDL_BUTTON_MIDDLE:
			return MBTN_MIDDLE;
		default:
			break;
	}
	return MBTN_COUNT;
}

extern void async_process_schedule(void);

bool system_update(void) {
	SDL_Event evt;
	int n_keys;
	byte* curr_keystate;

	memcpy(old_mousestate, mousestate, sizeof(mousestate));
	memcpy(old_keystate, keystate, sizeof(keystate));
	while(SDL_PollEvent(&evt)) {
		if(evt.type == SDL_MOUSEMOTION) {
			mouse_x = evt.motion.x;
			mouse_y = evt.motion.y;
		}
		if(evt.type == SDL_MOUSEBUTTONUP) {
			mousestate[_sdl_to_greed_mbtn(evt.button.button)] = 0;
		}
		if(evt.type == SDL_MOUSEBUTTONDOWN) {
			mousestate[_sdl_to_greed_mbtn(evt.button.button)] = 1;

			touch_hitpos = mouse_vec();
			touch_hittime = time_s();
		}
		if(evt.type == SDL_QUIT)
			return false;
	}
	curr_keystate = SDL_GetKeyState(&n_keys);
	memcpy(keystate, curr_keystate, n_keys);

	async_process_schedule();

	uint curr_time;
	do {
		curr_time = SDL_GetTicks();
		if(MS_PER_FRAME - (curr_time - last_time) > 15)
			SDL_Delay(5);
	} while(curr_time - last_time < MS_PER_FRAME);

	last_time = curr_time;

	_time_update(curr_time);

	return true;
}
