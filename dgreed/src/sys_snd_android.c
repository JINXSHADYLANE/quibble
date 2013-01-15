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

static jmethodID playable_free;
static jmethodID playable_set_volume;
static jmethodID playable_get_volume;
static jmethodID playable_length;
static jmethodID playable_play;
static jmethodID playable_play_ex;
static jmethodID playable_stop;

static jmethodID source_pause;
static jmethodID source_resume;
static jmethodID source_stop;
static jmethodID source_set_volume;
static jmethodID source_get_volume;
static jmethodID source_set_pos;
static jmethodID source_get_pos;

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

	jclass iplayable = (*env)->FindClass(env, "com/quibble/dgreed/IPlayable");
	playable_free = (*env)->GetMethodID(env, iplayable, "Free", "()V");
	playable_set_volume = (*env)->GetMethodID(env, iplayable, "SetVolume", "(F)V");
	playable_get_volume = (*env)->GetMethodID(env, iplayable, "GetVolume", "()F");
	playable_length = (*env)->GetMethodID(env, iplayable, "Length", "()F");
	playable_play = (*env)->GetMethodID(env, iplayable, "Play", "()V");
	playable_play_ex = (*env)->GetMethodID(env, iplayable, "Play",
		"(Z)Lcom/quibble/dgreed/ISource;"
	);
	playable_stop = (*env)->GetMethodID(env, iplayable, "Stop", "()V");

	
	jclass isource = (*env)->FindClass(env, "com/quibble/dgreed/ISource");
	source_pause = (*env)->GetMethodID(env, isource, "Pause", "()V");
	source_resume = (*env)->GetMethodID(env, isource, "Resume", "()V");
	source_stop = (*env)->GetMethodID(env, isource, "Stop", "()V");
	source_set_volume = (*env)->GetMethodID(env, isource, "SetVolume", "(F)V");
	source_get_volume = (*env)->GetMethodID(env, isource, "GetVolume", "()F");
	source_set_pos = (*env)->GetMethodID(env, isource, "SetPos", "(F)V");
	source_get_pos = (*env)->GetMethodID(env, isource, "GetPos", "()F");

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

	return (SoundHandle)playable;
}

SoundHandle sound_load_stream(const char* filename) {
	jstring str = (*env)->NewStringUTF(env, filename);
	jobject playable = (*env)->CallObjectMethod(env, sound, load_stream, str);
	(*env)->DeleteLocalRef(env, str);

	return (SoundHandle)playable;
}

void sound_free(SoundHandle handle) {
	jobject playable = (jobject)handle;
	(*env)->CallVoidMethod(env, playable, playable_free);
}

void sound_play(SoundHandle handle) {
	jobject playable = (jobject)handle;
	(*env)->CallVoidMethod(env, playable, playable_play);
}

void sound_stop(SoundHandle handle) {
	jobject playable = (jobject)handle;
	(*env)->CallVoidMethod(env, playable, playable_stop);
}

void sound_set_volume(SoundHandle handle, float volume) {	
	jobject playable = (jobject)handle;
	(*env)->CallVoidMethod(env, playable, playable_set_volume, volume);
}

float sound_get_volume(SoundHandle handle) {
	jobject playable = (jobject)handle;
	return (*env)->CallFloatMethod(env, playable, playable_get_volume);
}

float sound_get_length(SoundHandle handle) {
	jobject playable = (jobject)handle;
	return (*env)->CallFloatMethod(env, playable, playable_length);
}

SourceHandle sound_play_ex(SoundHandle handle, bool loop) {
	jobject playable = (jobject)handle;
	jobject source = (*env)->CallObjectMethod(env, playable, playable_play_ex, loop);
	return (SourceHandle)source;
}

void sound_pause_ex(SourceHandle handle) {
	jobject source = (jobject)handle;
	(*env)->CallVoidMethod(env, source, source_pause);
}

void sound_resume_ex(SourceHandle handle) {
	jobject source = (jobject)handle;
	(*env)->CallVoidMethod(env, source, source_resume);
}

void sound_stop_ex(SourceHandle handle) {
	jobject source = (jobject)handle;
	(*env)->CallVoidMethod(env, source, source_stop);
}

void sound_set_volume_ex(SourceHandle handle, float volume) {
	jobject source = (jobject)handle;
	(*env)->CallVoidMethod(env, source, source_set_volume, volume);
}

float sound_get_volume_ex(SourceHandle handle) {
	jobject source = (jobject)handle;
	return (*env)->CallFloatMethod(env, source, source_get_volume);
}

float sound_get_pos_ex(SourceHandle handle) {
	jobject source = (jobject)handle;
	return (*env)->CallFloatMethod(env, source, source_get_pos);
}

void sound_set_pos_ex(SourceHandle handle, float pos) {
	jobject source = (jobject)handle;
	(*env)->CallVoidMethod(env, source, source_set_pos, pos);
}


