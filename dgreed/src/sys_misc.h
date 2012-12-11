#ifndef SYS_MISC_H
#define SYS_MISC_H

/*
--------------------------
--- Device orientation ---
--------------------------
*/

typedef enum {
	ORIENT_LANDSCAPE_LEFT = 1,
	ORIENT_LANDSCAPE_RIGHT = 2,
	ORIENT_PORTRAIT = 4,
	ORIENT_PORTRAIT_UPSIDE_DOWN = 8
} DevOrient;

// Returns current device orientation
DevOrient orientation_current(void);

// Returns true if orientation did just change, outputs
// new orientation, animation start time and length.
bool orientation_change(DevOrient* _new, float* anim_start, float* anim_len);

/*
---------------------
--- Running state ---
---------------------
*/

typedef void (*RunStateCallback)(void);

// Sets a callback which will be invoked when
// app enters background state. Can be NULL
// if you want to disable previous callback.
void runstate_background_cb(RunStateCallback cb);

// Sets a callback which will be invoked when
// app enters foreground state. Can be NULL
// if you want to disable previous callback.
void runstate_foreground_cb(RunStateCallback cb);

/*
---------------------
--- Accelerometer ---
---------------------
*/

typedef void (*ShakeCallback)(void);

// Sets a callback which will be invoked when
// device is shaked. Can be NULL.
void acc_shake_cb(ShakeCallback cb);

/*
-------------
--- Input ---
-------------
*/

typedef enum {
	KEY_UP = 0,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_A,
	KEY_B,
	KEY_PAUSE,
	KEY_QUIT,
	KEY_COUNT
} Key;

typedef enum {

	MBTN_PRIMARY = 0,
	MBTN_SECONDARY = 1,
	MBTN_MIDDLE,

	// Left/right is deprecated
	MBTN_LEFT = 0,
	MBTN_RIGHT = 1,

	MBTN_COUNT = 3
} MouseButton;

// Returns true if key is being held down
bool key_pressed(Key key);
bool char_pressed(char c);

// Returns true when key was just pressed down
bool key_down(Key key);
bool char_down(char c);

// Returns true when key is released
bool key_up(Key key);
bool char_up(char c);

// Same as key_***, just for mouse
bool mouse_pressed(MouseButton button);
bool mouse_down(MouseButton button);
bool mouse_up(MouseButton button);

// Returns absolute mouse position
void mouse_pos(uint* x, uint* y);
Vector2 mouse_vec(void);

typedef struct {
	float hit_time;
	Vector2 hit_pos;
	Vector2 pos;
} Touch;

// Returns number of fingers currently touching the screen
uint touches_count(void);

// Returns array to all touches, NULL if no touches
Touch* touches_get(void);

/*
------------------
--- Text Input ---
------------------
*/

// Start capturing text. Shows on-screen keyboard if
// running on keyboardless devices.
void txtinput_start(void);
// Currently entered text in utf-8
const char* txtinput_get(void);
// Overrides entered text
void txtinput_set(const char* text);
// Returns string if user ended text capture, NULL otherwise
const char* txtinput_did_end(void);
// Stop capturing text, hide keyboard
const char* txtinput_end(void);
// Clear currently entered text
void txtinput_clear(void);

/*
-----------
--- Time --
-----------
*/

float time_s(void);
float time_ms(void);
float time_delta(void);
uint time_fps(void);

void time_scale(float s);

// This returns precise current time, updated continiously
uint time_ms_current(void);

/*
------------
--- Misc ---
------------
*/

bool system_update(void);

#endif
