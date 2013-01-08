#include "system.h"

#include <SDL.h>
#include <unistd.h>

#define MS_PER_FRAME (1000.0f/60.0f)

// Main

extern void _async_init(void);
extern void _async_close(void);
extern int dgreed_main(int, char**);

void _sys_video_init(void);

int SDL_main(int argc, char** argv) {
	
	// Construct log file name
	assert(argc >= 1);
	const char* prog_name = path_get_file(argv[0]);
	const char* postfix = ".log";
	char logfile[128];
	assert(strlen(prog_name) + strlen(postfix) < 128);
	strcpy(logfile, prog_name);
	strcat(logfile, postfix);

	_async_init();
	log_init(logfile, LOG_LEVEL_INFO);

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

void acc_shake_cb(ShakeCallback cb) {
}

/*
-------------
--- Input ---
-------------
*/

bool key_pressed(Key key) {
	return false;
}

bool char_pressed(char c) {
	return false;
}

bool key_down(Key key) {
	return false;
}

bool char_down(char c) {
	return false;
}

bool key_up(Key key) {
	return false;
}

bool char_up(char c) {
	return false;
}

bool mouse_pressed(MouseButton button) {
	return false;
}

bool mouse_down(MouseButton button) {
	return false;
}

bool mouse_up(MouseButton button) {
	return false;
}

void mouse_pos(uint* x, uint* y) {
	x = 0;
	y = 0;
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
	//int n_keys;
	//byte* curr_keystate;
	
	//memcpy(old_mousestate, mousestate, sizeof(mousestate));
	//memcpy(old_keystate, keystate, sizeof(keystate));
	while(SDL_PollEvent(&evt)) {
		/*
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
		*/
		if(evt.type == SDL_QUIT)
			return false;
	}
	//curr_keystate = SDL_GetKeyState(&n_keys);
	//memcpy(keystate, curr_keystate, n_keys);

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

// non-OpenGL video stuff

uint _sys_native_width = 0;
uint _sys_native_height = 0;
bool _sys_video_initialized = false;
SDL_Window* _sys_window = NULL;
SDL_GLContext _sys_glcontext;

void _sys_video_init(void) {
	assert(!_sys_video_initialized);
	_sys_video_initialized = true;

	if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0)
		LOG_ERROR("Unable to initialize SDL");

	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(0, &mode);
	_sys_native_width = mode.w;
	_sys_native_height = mode.h;

	SDL_Window* window = SDL_CreateWindow("dgreed", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			_sys_native_width, _sys_native_height,
			SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS
	);

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
	SDL_Quit();
}

void _sys_video_get_native_resolution(uint* width, uint* height) {
	assert(width && height);

	if(!_sys_video_initialized)
		_sys_video_init();

	*width = _sys_native_width;
	*height = _sys_native_height;
}

