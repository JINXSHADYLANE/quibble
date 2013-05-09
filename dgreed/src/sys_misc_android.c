#include "system.h"

#include "keyval.h"

#include <jni.h>
#include <SDL.h>
#include <unistd.h>

#define MS_PER_FRAME (1000.0f/60.0f)

// Main

extern void _async_init(void);
extern void _async_close(void);
extern int dgreed_main(int, char**);

void _sys_video_init(void);

int SDL_main(int argc, char** argv) {
	
	_async_init();
	log_init(NULL, LOG_LEVEL_INFO);

	_sys_video_init();

	int res = dgreed_main(argc, argv);

	log_close();
	_async_close();

#ifdef TRACK_MEMORY
	MemoryStats stats;
	mem_stats(&stats);
	LOG_INFO("Memory usage stats:");
	LOG_INFO(" Total allocations: %u", stats.n_allocations);
	LOG_INFO(" Peak dynamic memory usage: %zuB", stats.peak_bytes_allocated);
	if(stats.bytes_allocated) {
		LOG_INFO(" Bytes still allocted: %zu", stats.bytes_allocated);
		LOG_INFO(" Dumping allocations info to memory.txt");
		mem_dump("memory.txt");
	}
#endif

	exit(0);

	return res;	
}

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

static ShakeCallback shake_cb = NULL;

void acc_shake_cb(ShakeCallback cb) {
	shake_cb = cb;
}

/*
-------------
--- Input ---
-------------
*/

static const int keybindings[8] = {
	SDLK_UP,
	SDLK_DOWN,
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_z,
	SDLK_x,
	SDLK_p,
	SDLK_AC_BACK
};

#define max_keys 16

static int keys_down[max_keys];
static int keys_down_n = 0;
static int keys_up[max_keys];
static int keys_up_n = 0;
static int keys_pressed[max_keys];
static int keys_pressed_n = 0;

static void _add_key(int* list, int* count, int key) {
	if(*count < max_keys) {
		list[*count] = key;
		(*count)++;
	}
}

static void _rem_key(int* list, int* count, int key) {
	for(uint i = 0; i < *count; ++i) {
		if(list[i] == key) {
			list[i] = list[*count - 1];
			(*count)--;
			return;
		}
	}
}

static bool _check_key(int* list, int* count, int key) {
	for(uint i = 0; i < *count; ++i) {
		if(list[i] == key)
			return true;
	}
	return false;
}

bool key_pressed(Key key) {
	if(key == KEY_A)
		return mouse_pressed(MBTN_PRIMARY);
	int k = keybindings[key];
	return _check_key(keys_pressed, &keys_pressed_n, k);
}

bool char_pressed(char c) {
	return _check_key(keys_pressed, &keys_pressed_n,
		(int)c
	);
}

bool key_down(Key key) {
	if(key == KEY_A)
		return mouse_down(MBTN_PRIMARY);
	int k = keybindings[key];
	return _check_key(keys_down, &keys_down_n, k);
}

bool char_down(char c) {
	return _check_key(keys_down, &keys_down_n,
		(int)c
	);
}

bool key_up(Key key) {
	if(key == KEY_A)
		return mouse_up(MBTN_PRIMARY);
	int k = keybindings[key];
	return _check_key(keys_up, &keys_up_n, k);
}

bool char_up(char c) {
	return _check_key(keys_up, &keys_up_n, 
		(int)c
	);
}

// Current state
static uint cmouse_x, cmouse_y;
static bool cmouse_pressed, cmouse_up, cmouse_down;

// Cached last frame state
static uint lmouse_x, lmouse_y;
static bool lmouse_pressed, lmouse_up, lmouse_down;

bool mouse_pressed(MouseButton button) {
	return lmouse_pressed;
}

bool mouse_down(MouseButton button) {
	return lmouse_down;
}

bool mouse_up(MouseButton button) {
	return lmouse_up;
}

void mouse_pos(uint* x, uint* y) {
	*x = lmouse_x;
	*y = lmouse_y;
}

Vector2 mouse_vec(void) {
	return vec2((float)lmouse_x, (float)lmouse_y);
}


#define max_touches 11
static uint touch_count = 0;
static Touch touches[max_touches];
extern float touch_scale_x, touch_scale_y;

static void _touch_down(float x, float y) {
	x *= touch_scale_x;
	y *= touch_scale_y;
	if(touch_count < max_touches) {
		uint i = touch_count;
		touches[i].hit_time = time_s();
		touches[i].hit_pos = vec2(x, y);
		touches[i].pos = vec2(x, y);
		touch_count++;
	}
}

static void _touch_move(float old_x, float old_y, float new_x, float new_y) {
	if(!touch_count)
		return;
	new_x *= touch_scale_x;
	new_y *= touch_scale_y;
	old_x *= touch_scale_x;
	old_y *= touch_scale_y;
    uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count && i < max_touches; ++i) {
		float dx = touches[i].pos.x - old_x;
		float dy = touches[i].pos.y - old_y;
		float d = dx*dx + dy*dy;
		if(dx*dx + dy*dy < min_d) {
			min_d = d;
			min_i = i;
		}
	}
    touches[min_i].pos = vec2(new_x, new_y);
}

static void _touch_up(float old_x, float old_y) {
	if(!touch_count)
		return;
	old_x *= touch_scale_x;
	old_y *= touch_scale_y;
	uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count && i < max_touches; ++i) {
		float dx = touches[i].pos.x - old_x;
		float dy = touches[i].pos.y - old_y;
		float d = dx*dx + dy*dy;
		if(dx*dx + dy*dy < min_d) {
			min_d = d;
			min_i = i;
		}
	}
	touches[min_i] = touches[--touch_count];
}

uint touches_count(void) {
	return touch_count;
}

bool touches_up(void) {
	return lmouse_up;
}

bool touches_down(void) {
	return lmouse_down;
}

Touch* touches_get(void) {
	return touch_count ? &touches[0] : NULL;
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

extern void malka_states_app_suspend(void);
extern void _loc_write_base(void);

static float inactive_time = 0.0f;
static bool did_resign_active = false;
static float resign_active_t;

static float t_ms = 0.0f, t_d = 0.0f;
static float t_scale = 1.0f;
static int last_frame_time = 0, last_fps_update = 0;
static uint fps_count = 0;
static uint fps = 0;

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
	int t = current_time - inactive_time;
	t_d = (float)(t - last_frame_time) * t_scale;
	t_ms += t_d;
	if(last_fps_update + 1000 < current_time) {
		fps = fps_count;
		fps_count = 0;
		last_fps_update = current_time;
	}
	fps_count++;
	last_frame_time = t;
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

uint _sys_native_width = 0;
uint _sys_native_height = 0;

static Vector2 _conv_touch(float x, float y) {
	uint ix = x * _sys_native_width;
	uint iy = y * _sys_native_height;
	return vec2(ix, iy);
}

#define sdl_window_event(name) \
	case (name):	\
	LOG_INFO(#name); \
	break;

static void _invoke_sound(const char* f) {
	JNIEnv* env = SDL_AndroidGetJNIEnv();
	jclass sound_class = (*env)->FindClass(env, "com/quibble/dgreed/Sound");
	jfieldID singleton_id = (*env)->GetStaticFieldID(env, sound_class, "singleton", 
		"Lcom/quibble/dgreed/Sound;"
	);
	jobject singleton = (*env)->GetStaticObjectField(env, sound_class, singleton_id);
	jmethodID pause_id = (*env)->GetMethodID(env, sound_class, f, "()V");
	(*env)->CallVoidMethod(env, singleton, pause_id);
}

static int joystick_axes[3] = {0, 0, 0};
static int last_shake_t = 0;

extern bool dgreed_sleeping;
bool system_update(void) {
	SDL_Event evt;

	keys_down_n = 0;
	keys_up_n = 0;
	while(SDL_PollEvent(&evt)) {
		if(evt.type == SDL_FINGERDOWN) {
			Vector2 t = _conv_touch(evt.tfinger.x, evt.tfinger.y);
			cmouse_x = lrintf(t.x);
			cmouse_y = lrintf(t.y);
			cmouse_down = cmouse_pressed = true;
			_touch_down(t.x, t.y);
		}
		if(evt.type == SDL_FINGERUP) {
			Vector2 t = _conv_touch(evt.tfinger.x, evt.tfinger.y);
			cmouse_x = lrintf(t.x);
			cmouse_y = lrintf(t.y);
			cmouse_up = true;
			cmouse_pressed = false;
			_touch_up(t.x, t.y);
		}
		if(evt.type == SDL_FINGERMOTION) {
			Vector2 t = _conv_touch(evt.tfinger.x, evt.tfinger.y);
			Vector2 old = _conv_touch(
				evt.tfinger.x - evt.tfinger.dx,
				evt.tfinger.y - evt.tfinger.dy
			);
			cmouse_x = lrintf(t.x);
			cmouse_y = lrintf(t.y);
			cmouse_pressed = true;

			_touch_move(old.x, old.y, t.x, t.y);
		}

		if(evt.type == SDL_KEYDOWN) {
			int keycode = evt.key.keysym.sym;
			_add_key(keys_down, &keys_down_n, keycode);
			_add_key(keys_pressed, &keys_pressed_n, keycode);
		}
		if(evt.type == SDL_KEYUP) {
			int keycode = evt.key.keysym.sym;
			_add_key(keys_up, &keys_up_n, keycode);
			_rem_key(keys_pressed, &keys_pressed_n, keycode);
		}

		if(evt.type == SDL_WINDOWEVENT) {
			if(evt.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				// Focus lost
				did_resign_active = true;
				resign_active_t = time_ms_current();
			}

			if(evt.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				// Focus gained
				did_resign_active = false;
				inactive_time += (time_ms_current() - resign_active_t);
			}

			if(evt.window.event == SDL_WINDOWEVENT_MINIMIZED) {
				// Enter background
				_invoke_sound("Pause");
				if(enter_background_cb)
					(*enter_background_cb)();
				malka_states_app_suspend();
				keyval_app_suspend();
				_loc_write_base();
				dgreed_sleeping = true;
			}

			if(evt.window.event == SDL_WINDOWEVENT_RESTORED) {
				// Enter foreground
				if(enter_foreground_cb)
					(*enter_foreground_cb)();
				dgreed_sleeping = false;
				_invoke_sound("Resume");
			}

			/*
			switch(evt.window.event) {
				sdl_window_event(SDL_WINDOWEVENT_NONE);
				sdl_window_event(SDL_WINDOWEVENT_SHOWN);
				sdl_window_event(SDL_WINDOWEVENT_HIDDEN);
				sdl_window_event(SDL_WINDOWEVENT_EXPOSED);
				sdl_window_event(SDL_WINDOWEVENT_MOVED);
				sdl_window_event(SDL_WINDOWEVENT_RESIZED);
				sdl_window_event(SDL_WINDOWEVENT_SIZE_CHANGED);
				sdl_window_event(SDL_WINDOWEVENT_MINIMIZED);
				sdl_window_event(SDL_WINDOWEVENT_MAXIMIZED);
				sdl_window_event(SDL_WINDOWEVENT_RESTORED);
				sdl_window_event(SDL_WINDOWEVENT_ENTER);
				sdl_window_event(SDL_WINDOWEVENT_LEAVE);
				sdl_window_event(SDL_WINDOWEVENT_FOCUS_GAINED);
				sdl_window_event(SDL_WINDOWEVENT_FOCUS_LOST);
				sdl_window_event(SDL_WINDOWEVENT_CLOSE);
			}
			*/
		}
		if(evt.type == SDL_SYSWMEVENT) {
			LOG_INFO("SDL_SYSWMEVENT");
		}

		if(evt.type == SDL_JOYAXISMOTION) {
			int a = evt.jaxis.axis;
			if(evt.jaxis.axis < 3) {
				int d = evt.jaxis.value - joystick_axes[a];
				if(d > 10000) {
					int t = time_ms_current();
					if(shake_cb && t - last_shake_t > 1000) {
						last_shake_t = t;
						(*shake_cb)();
					}
				}
				joystick_axes[a] = evt.jaxis.value;
			}
		}

		if(evt.type == SDL_QUIT)
			return false;
	}

	if(dgreed_sleeping)
		SDL_Delay(16);

	async_process_schedule();

	uint curr_time = SDL_GetTicks();

	_time_update(curr_time);
	
	lmouse_x = cmouse_x;
	lmouse_y = cmouse_y;
	lmouse_up = cmouse_up;
	lmouse_down = cmouse_down;
	lmouse_pressed = cmouse_pressed;
	
	cmouse_up = false;
	cmouse_down = false;

	return true;
}

// non-OpenGL video stuff
bool _sys_video_initialized = false;
SDL_Window* _sys_window = NULL;
SDL_GLContext _sys_glcontext;
SDL_Joystick* _sys_joystick = NULL;

void _sys_video_init(void) {
	assert(!_sys_video_initialized);
	_sys_video_initialized = true;

	if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0)
		LOG_ERROR("Unable to initialize SDL");

	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0, &mode);
	_sys_native_width = mode.w;
	_sys_native_height = mode.h;

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

	SDL_Window* window = SDL_CreateWindow("dgreed", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			_sys_native_width, _sys_native_height,
			SDL_WINDOW_OPENGL
	);

	if(SDL_NumJoysticks()) {
		_sys_joystick = SDL_JoystickOpen(0);
		SDL_JoystickEventState(SDL_ENABLE);
	}

	_sys_glcontext = SDL_GL_CreateContext(window);
	_sys_window = window;
}

void _sys_set_title(const char* title) {
	assert(_sys_window);
	SDL_SetWindowTitle(_sys_window, title);
}

void _sys_video_close(void) {
	assert(_sys_video_initialized);
	_sys_video_initialized = false;
	SDL_GL_DeleteContext(_sys_glcontext);
	SDL_DestroyWindow(_sys_window);

	if(_sys_joystick) {
		SDL_JoystickClose(_sys_joystick);
	}

	SDL_Quit();
}

void _sys_video_get_native_resolution(uint* width, uint* height) {
	assert(width && height);

	if(!_sys_video_initialized)
		_sys_video_init();

	*width = _sys_native_width;
	*height = _sys_native_height;
}

void _sys_present(void) {
	if(!dgreed_sleeping)
		SDL_GL_SwapWindow(_sys_window);
}

