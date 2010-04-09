#ifndef SYSTEM_H
#define SYSTEM_H

#include "utils.h"

/*
----------------
--- Graphics ---
----------------
*/

// Initializes video
void video_init(uint width, uint height, const char* name); 
// Initializes video, sets virtual resolution
void video_init_ex(uint width, uint height, uint v_width, uint v_height, const
	char* name, bool fullscreen);
// Deinitializes video
void video_close(void);
// Flips backbuffer and frontbuffer, sleeps to hit 60fps.
// Returns false if app is closed, true otherwise.
void video_present(void);
// Returns number of frame which is currently being rendered
uint video_get_frame(void);

typedef uint TexHandle;

// Loads texture from .png file, it must have pow2 dimensions
TexHandle tex_load(const char* filename);
// Returns widht and height of texture in pixels
void tex_size(TexHandle tex, uint* width, uint* height); 
// Frees texture which was loaded with tex_load
void tex_free(TexHandle tex);

// Draws textured rectangle. Source can be null rectangle,
// destination can have 0 for right and bottom.
void video_draw_rect(TexHandle tex, uint layer, 
	const RectF* source, const RectF* dest, Color tint);
// Draws rotated textured rectangle. 
// Same rules for source/dest applies as for video_draw_rect.
void video_draw_rect_rotated(TexHandle tex, uint layer,
	const RectF* source, const RectF* dest, float rotation, Color tint);
// Draws a line 
void video_draw_line(uint layer,
	const Vector2* start, const Vector2* end, Color color);

/*
-------------
--- Sound ---
-------------
*/

// Initializes sound
void sound_init(void);
// Deinitializes sound
void sound_close(void);
// Must be called once each frame
void sound_update(void);

typedef uint SoundHandle;

// Loads sound sample from .wav file
SoundHandle sound_load_sample(const char* filename);
// Prepares to steam sound from .ogg file
SoundHandle sound_load_stream(const char* filename);

// Frees resources used by sound
void sound_free(SoundHandle handle);

// Plays stream or sample
void sound_play(SoundHandle handle);
// Stops sounds stream which is being played. Does nothing for samples.
void sound_stop(SoundHandle handle);

// Sets volume of sound. For samples, it takes effect next time sample is played.
void sound_set_volume(SoundHandle handle, float volume);
// Returns volume of sound
float sound_get_volume(SoundHandle handle);

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
	MBTN_LEFT = 0,
	MBTN_RIGHT,
	MBTN_MIDDLE,
	MBTN_ELSE
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

/*
-----------
--- Time --
-----------
*/

float time_ms(void);
float time_delta(void);
uint time_fps(void);

/*
------------
--- Misc ---
------------
*/

bool system_update(void);

#endif

