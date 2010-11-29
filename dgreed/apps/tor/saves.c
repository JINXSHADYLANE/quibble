#include "saves.h"
#include "common.h"
#include <utils.h>
#include <darray.h>
#include <memory.h>

#pragma pack(1)
typedef struct {
	uint hash;
	bool solved;
} PuzzleState;	
#pragma pack()

FileHandle saves_file = 0;
DArray saves;

void _reset_state(void) {
	saves = darray_create(sizeof(PuzzleState), 0);	
}

#define MAGIC_NUMBER1 0x75F3A951
#define MAGIC_NUMBER2 0x32165498
#define MAGIC_NUMBER3 0xA78BC211

void _read_state(void) {
	if(!file_exists(SAVE_FILE)) {
	error:
		_reset_state();
		return;
	}

	saves_file = file_open(SAVE_FILE);
	uint hash = file_read_uint32(saves_file);
	uint len = file_read_uint32(saves_file) ^ MAGIC_NUMBER1;
	if(len > 1024 || len % sizeof(PuzzleState) != 0)
		goto error;
	
	void* data = MEM_ALLOC(len);
	file_read(saves_file, data, len);
	file_close(saves_file);
	saves_file = 0;
	uint real_hash = hash_murmur(data, len, MAGIC_NUMBER2);
	if(real_hash != hash)
		goto error;

	uint n_elements = len / sizeof(PuzzleState);
	saves = darray_create(sizeof(PuzzleState), n_elements);
	PuzzleState* array_ptr = DARRAY_DATA_PTR(saves, PuzzleState);
	memcpy((void*)array_ptr, data, len);
	saves.size = n_elements;
	MEM_FREE(data);
}

void _write_state(void) {
	assert(saves.data);
	assert(saves.size > 0);

	FileHandle saves_file = file_create(SAVE_FILE);
	uint size = saves.size * sizeof(PuzzleState);
	file_write_uint32(saves_file, hash_murmur(saves.data, size, MAGIC_NUMBER2));
	file_write_uint32(saves_file, size ^ MAGIC_NUMBER1);
	file_write(saves_file, saves.data, size);
	file_close(saves_file);
	saves_file = 0;
}

void _set_state(uint hash, bool state) {
	assert(saves.data);

	PuzzleState* states_ptr = DARRAY_DATA_PTR(saves, PuzzleState);
	for(uint i = 0; i < saves.size; ++i) {
		if(hash == states_ptr[i].hash) {
			states_ptr[i].solved = state;
			return;
		}
	}

	PuzzleState new_state = {hash, state};
	darray_append(&saves, (void*)&new_state);
}

bool _get_state(uint hash) {
	assert(saves.data);

	PuzzleState* states_ptr = DARRAY_DATA_PTR(saves, PuzzleState);
	for(uint i = 0; i < saves.size; ++i) {
		if(hash == states_ptr[i].hash) 
			return states_ptr[i].solved;
	}

	PuzzleState new_state = {hash, false};
	darray_append(&saves, (void*)&new_state);
	return false;
}

void saves_init(void) {
	saves.data = 0;
	_read_state();
}

void saves_close(void) {
	assert(saves.data);
	darray_free(&saves);
}

bool saves_get_state(const char* puzzle_name) {
	// Lets assume no hash collisions will happen... 
	uint hash = hash_murmur((void*)puzzle_name, strlen(puzzle_name), MAGIC_NUMBER3);
	return _get_state(hash);
}

void saves_set_state(const char* puzzle_name, bool solved) {
	uint hash = hash_murmur((void*)puzzle_name, strlen(puzzle_name), MAGIC_NUMBER3);
	_set_state(hash, solved);
	_write_state();
}

