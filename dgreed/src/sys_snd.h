#ifndef SYS_SND_H
#define SYS_SND_H

#include "utils.h"

/*
-------------
--- Sound ---
-------------
*/

#ifndef NO_DEVMODE
typedef struct {
	uint sample_count;
	uint stream_count;
	uint playing_samples;
	uint playing_streams;
} SoundStats;

const SoundStats* sound_stats(void);
#endif

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
// Returns length of sound in seconds
float sound_get_length(SoundHandle handle);

// Extented API for controlling specific sound sources

typedef uint SourceHandle;

// Plays stream or sample, optionally looping it, returns source.
// If result is null - sound was skipped.
SourceHandle sound_play_ex(SoundHandle handle, bool loop);
// Pauses a specific source, if it is playing
void sound_pause_ex(SourceHandle source);
// Resumes a paused source
void sound_resume_ex(SourceHandle source);
// Stops a source, SourceHandle becomes invalid
void sound_stop_ex(SourceHandle source);
// Sets a volume for a source
void sound_set_volume_ex(SourceHandle source, float volume);
// Returns current volume of a source
float sound_get_volume_ex(SourceHandle source);
// Returns play cursor position in seconds
float sound_get_pos_ex(SourceHandle source);
// Sets play cursor position
void sound_set_pos_ex(SourceHandle source, float pos);

#endif

