#include "state.h"

#include <memory.h>
#include <mml.h>

#include "sounds.h"

#define STATE_FILE_NAME "greed_assets/state.mml"

PersistentState pstate = 
	{1.0f, 1.0f, 0, 
	 {70.0f, 250.0f}, 
	 {420.0f, 230.0f},
	 {340.0f, 250.0f},
	 {30.0f, 230.0f},
	 {90.0f, 250.0f}
};	 

#define READ_STATE(name, type, dest) { \
	node = mml_get_next(&state_obj, node); \
	if(strcmp(mml_get_name(&state_obj, node), #name) != 0) \
		LOG_ERROR(#name "state node is missing or in wrong place"); \
	dest = mml_getval_##type(&state_obj, node); \
}	

#define WRITE_STATE(name, type, src) { \
	node = mml_node(&state_obj, #name, ""); \
	mml_setval_##type(&state_obj, node, src); \
	mml_append(&state_obj, root, node); \
}

void _load_state(const char* filename) {
	assert(filename);

	char* state_text = txtfile_read(filename);

	MMLObject state_obj;
	if(!mml_deserialize(&state_obj, state_text))
		LOG_ERROR("Unable to deserialize state");

	NodeIdx root = mml_root(&state_obj);	
	if(strcmp(mml_get_name(&state_obj, root), "state") != 0)
		LOG_ERROR("State file root node name is wrong");

	NodeIdx node = mml_get_first_child(&state_obj, root);
	if(strcmp(mml_get_name(&state_obj, node), "sound_volume") != 0)
		LOG_ERROR("sound_volume state node is missing or in wrong place");
	pstate.sound_volume = mml_getval_float(&state_obj, node);	
	
	READ_STATE(music_volume, float, pstate.music_volume);
	READ_STATE(control_type, uint, pstate.control_type);
	READ_STATE(joystick_pos, vec2, pstate.joystick_pos);
	READ_STATE(acc_btn_pos, vec2, pstate.acc_btn_pos);
	READ_STATE(shoot_btn_pos, vec2, pstate.shoot_btn_pos);
	READ_STATE(left_btn_pos, vec2, pstate.left_btn_pos);
	READ_STATE(right_btn_pos, vec2, pstate.right_btn_pos);

	mml_free(&state_obj);
	MEM_FREE(state_text);

	sounds_set_music_volume(pstate.music_volume);
	sounds_set_effect_volume(pstate.sound_volume);
}

void _save_state(const char* filename) {
	assert(filename);

	MMLObject state_obj;
	mml_empty(&state_obj);
	
	NodeIdx node, root = mml_root(&state_obj);
	mml_set_name(&state_obj, root, "state");

	WRITE_STATE(sound_volume, float, pstate.sound_volume);
	WRITE_STATE(music_volume, float, pstate.music_volume);
	WRITE_STATE(control_type, uint, pstate.control_type);
	WRITE_STATE(joystick_pos, vec2, pstate.joystick_pos);
	WRITE_STATE(acc_btn_pos, vec2, pstate.acc_btn_pos);
	WRITE_STATE(shoot_btn_pos, vec2, pstate.shoot_btn_pos);
	WRITE_STATE(left_btn_pos, vec2, pstate.left_btn_pos);
	WRITE_STATE(right_btn_pos, vec2, pstate.right_btn_pos);

	char* state_text = mml_serialize(&state_obj);
	txtfile_write(filename, state_text);

	MEM_FREE(state_text);
	mml_free(&state_obj);
}

void state_init(void) {
	if(file_exists(STATE_FILE_NAME))
		_load_state(STATE_FILE_NAME);
}

void state_close(void) {
	_save_state(STATE_FILE_NAME);
}

void state_commit(void) {
	_save_state(STATE_FILE_NAME);
}	

