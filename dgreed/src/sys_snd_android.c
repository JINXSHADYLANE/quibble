#include "sys_snd.h"

#include <SDL.h>
#include <jni.h>

static JNIEnv* env;

static jobject sound;

static jmethodID init;
static jmethodID close;
static jmethodID update;

static jmethodID load_sample;
static jmethodID load_stream;
//static jmethodID free;

#ifndef NO_DEVMODE
const SoundStats* sound_stats(void) {
	return NULL;
}
#endif

void sound_init(void) {
	env = SDL_AndroidGetJNIEnv();	
	jclass class = (*env)->FindClass(env, "com/quibble/dgreed/Sound");
	jmethodID constr = (*env)->GetMethodID(env, class, "<init>", "()V");

	sound = (*env)->NewObject(env, class, constr);
	sound = (*env)->NewGlobalRef(env, sound);

	init = (*env)->GetMethodID(env, class, "Init", "()V");
	close = (*env)->GetMethodID(env, class, "Close", "()V");
	update = (*env)->GetMethodID(env, class, "Update", "()V");

	load_sample = (*env)->GetMethodID(env, class, "LoadSample", 
		"(Ljava/lang/String;)Lcom/quibble/dgreed/IPlayable;"
	);

	load_stream = (*env)->GetMethodID(env, class, "LoadStream",
		"(Ljava/lang/String;)Lcom/quibble/dgreed/IPlayable;"
	);

	(*env)->CallVoidMethod(env, sound, init);
}

void sound_close(void) {
	(*env)->CallVoidMethod(env, sound, close);
	(*env)->DeleteGlobalRef(env, sound);
}

void sound_update(void) {
	(*env)->CallVoidMethod(env, sound, update);
}

SoundHandle sound_load_sample(const char* filename) {
	jstring str = (*env)->NewStringUTF(env, filename);
	jobject playable = (*env)->CallObjectMethod(env, sound, load_sample, str);
	(*env)->DeleteLocalRef(env, str);

	return 1;
}

SoundHandle sound_load_stream(const char* filename) {
	jstring str = (*env)->NewStringUTF(env, filename);
	jobject playable = (*env)->CallObjectMethod(env, sound, load_stream, str);
	(*env)->DeleteLocalRef(env, str);

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
	return 1.0f;
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

