#include "sys_snd.h"

#ifndef NO_DEVMODE
const SoundStats* sound_stats(void) {
	return NULL;
}
#endif

void sound_init(void) {
}

void sound_close(void) {
}

void sound_update(void) {
}

SoundHandle sound_load_sample(const char* filename) {
	return 1;
}

SoundHandle sound_load_stream(const char* filename) {
	return 1;
}

void sound_free(SoundHandle handle) {
}

void sound_play(SoundHandle handle) {
}

void sound_stop(SoundHandle handle) {
}

void sound_set_volume(SoundHandle handle, float volume) {
}

float sound_get_volume(SoundHandle handle) {
	return 1.0f;
}

float sound_get_length(SoundHandle handle) {
}

SourceHandle sound_play_ex(SoundHandle handle, bool loop) {
	return 1;
}

void sound_pause_ex(SourceHandle source) {
}

void sound_resume_ex(SourceHandle source) {
}

void sound_stop_ex(SourceHandle source) {
}

void sound_set_volume_ex(SourceHandle source, float volume) {
}

float sound_get_volume_ex(SourceHandle source) {
	return 1.0f;
}

float sound_get_pos_ex(SourceHandle source) {
	return 1.0f;
}

void sound_set_pos_ex(SourceHandle source, float pos) {
}

