#include "keyval.h"

#include "memory.h"
#include "datastruct.h"
#include "darray.h"
#include "async.h"

#define WRITES_BEFORE_FLUSH 128 

static const char* keyval_file;
static Dict keyval_dict;
static DArray keyval_strs;
static uint n_writes;
static bool keyval_initialized = false;

static void _rebase_ptrs(const void* old_base, const void* new_base, size_t strs_size) {
	const void* minp = old_base;
	const void* maxp = minp + strs_size;
	ptrdiff_t delta = new_base - old_base;
	size_t size = keyval_dict.mask + 1;
	for(uint i = 0; i < size; ++i) {
		if(minp <= (void*)keyval_dict.map[i].key 
			&& (void*)keyval_dict.map[i].key < maxp)
			keyval_dict.map[i].key += delta;

		if(minp <= keyval_dict.map[i].data && keyval_dict.map[i].data < maxp)
			keyval_dict.map[i].data += delta;
	}
}

static void _read(void) {
	// File format:
	// "KVS0"
	// 4 sizeof Dict
	// 4 sizeof DArray
	// 4 sizeof DictEntry
	// x Dict
	// x DArray
	// x DictEntry[]
	// x char[]
	
	FileHandle f = file_open(keyval_file);
	uint magic = file_read_uint32(f);
	if(magic != FOURCC('K', 'V', 'S', '0'))
		LOG_ERROR("Unable to load keyval file");

	size_t sizeof_dict = file_read_uint32(f);
	size_t sizeof_darray = file_read_uint32(f);
	size_t sizeof_entry = file_read_uint32(f);

	if(sizeof(Dict) != sizeof_dict
		|| sizeof(DArray) != sizeof_darray
		|| sizeof(DictEntry) != sizeof_entry)
		LOG_ERROR("keyval struct size mismatch");

	assert(sizeof(char) == 1);

	file_read(f, &keyval_dict, sizeof(Dict));
	file_read(f, &keyval_strs, sizeof(DArray));

	// Will use this to recalc pointers
	void* old_strs_data_ptr = keyval_strs.data;

	// Read dict entries
	size_t entries_size = (keyval_dict.mask + 1) * sizeof(DictEntry);
	keyval_dict.map = MEM_ALLOC(entries_size); 
	file_read(f, keyval_dict.map, entries_size);

	// Read strs
	keyval_strs.data = MEM_ALLOC(keyval_strs.reserved);	
	file_read(f, keyval_strs.data, keyval_strs.size);
	
	file_close(f);

	// Recalc pointers int dict
	_rebase_ptrs(old_strs_data_ptr, keyval_strs.data, keyval_strs.size);
}

static void _flush(void) {
	// Flushing is done rather often, let's make this as simple as possible.

	FileHandle f = file_create(keyval_file);

	file_write_uint32(f, FOURCC('K', 'V', 'S', '0'));
	file_write_uint32(f, sizeof(Dict));
	file_write_uint32(f, sizeof(DArray));
	file_write_uint32(f, sizeof(DictEntry));
	file_write(f, &keyval_dict, sizeof(Dict));
	file_write(f, &keyval_strs, sizeof(DArray));
	file_write(f, keyval_dict.map, (keyval_dict.mask + 1) * sizeof(DictEntry));
	file_write(f, keyval_strs.data, keyval_strs.size);

	file_close(f);
}

static void _gc(void) {
	// Since all str data is stored in append-only format, we need to
	// collect garbage from time to time
	
	DArray old_strs = keyval_strs;
	keyval_strs = darray_create(sizeof(char), 0);	

	size_t dict_size = keyval_dict.mask + 1;

	const char* prev_strs_data = DARRAY_DATA_PTR(keyval_strs, char);

	// First, add all keys - they are read more often
	for(uint i = 0; i < dict_size; ++i) {
		DictEntry* e = &keyval_dict.map[i];
		if(!e->key)
			continue;

		size_t len = strlen(e->key);
		darray_append_multi(&keyval_strs, e->key, len+1);

		const char* strs = DARRAY_DATA_PTR(keyval_strs, char);
		if(strs != prev_strs_data) {
			_rebase_ptrs(prev_strs_data, strs, keyval_strs.size - (len+1));
			prev_strs_data = strs;
		}
		e->key = &strs[keyval_strs.size - (len+1)];
	}

	// Then, values
	for(uint i = 0; i < dict_size; ++i) {
		DictEntry* e = &keyval_dict.map[i];
		if(!e->key)
			continue;

		const char* val = e->data;
		size_t len = strlen(val);
		darray_append_multi(&keyval_strs, val, len+1);

		const char* strs = DARRAY_DATA_PTR(keyval_strs, char);
		if(strs != prev_strs_data) {
			_rebase_ptrs(prev_strs_data, strs, keyval_strs.size - (len+1));
			prev_strs_data = strs;
		}
		e->data = &strs[keyval_strs.size - (len+1)];
	}

	darray_free(&old_strs);
}

void keyval_init(const char* file) {
	assert(file);

	keyval_file = file;
	n_writes = 0;

	if(file_exists(keyval_file)) {
		_read();
	}
	else {
		dict_init(&keyval_dict);
		keyval_strs = darray_create(sizeof(char), 0);
	}

	keyval_initialized = true;
}

void keyval_close(void) {
	if(n_writes) {
		_gc();
		_flush();
	}

	dict_free(&keyval_dict);
	darray_free(&keyval_strs);

	keyval_initialized = true;
}

const char* keyval_get(const char* key, const char* def) {
	assert(key && def);

	const char* val = dict_get(&keyval_dict, key);
	return val ? val : def;
}

int keyval_get_int(const char* key, int def) {
	assert(key);

	const char* val = dict_get(&keyval_dict, key);
	if(!val)
		return def;

	int int_val;
	sscanf(val, "%d", &int_val);
	return int_val;
}

bool keyval_get_bool(const char* key, bool def) {
	assert(key);

	const char* val = dict_get(&keyval_dict, key);
	if(!val)
		return def;

	return *val != 0;
}

float keyval_get_float(const char* key, float def) {
	assert(key);

	const char* val = dict_get(&keyval_dict, key);
	if(!val)
		return def;

	float float_val;
	sscanf(val, "%f", &float_val);
	return float_val;
}

void keyval_set(const char* key, const char* val) {
	assert(key && val);

	size_t key_len = strlen(key)+1;
	size_t val_len = strlen(val)+1;
	size_t keyval_len = key_len + val_len;

	void* old_strs_data = keyval_strs.data;
	
	darray_append_multi(&keyval_strs, key, key_len);
	darray_append_multi(&keyval_strs, val, val_len);

	void* new_strs_data = keyval_strs.data;
	if(old_strs_data != new_strs_data)
		_rebase_ptrs(old_strs_data, new_strs_data, keyval_strs.size - keyval_len);

	const char* strs = DARRAY_DATA_PTR(keyval_strs, char);
	const char* key_str = &strs[keyval_strs.size - keyval_len]; 
	const char* val_str = &strs[keyval_strs.size - val_len];

	dict_set(&keyval_dict, key_str, val_str);

	n_writes++;
	if(n_writes >= WRITES_BEFORE_FLUSH) {
		_flush();
		n_writes = 0;
	}
}

void keyval_set_int(const char* key, int val) {
	char int_str[32];
	sprintf(int_str, "%d", val);
	keyval_set(key, int_str);
}

void keyval_set_bool(const char* key, bool val) {
	keyval_set(key, val ? "1" : "\0");
}

void keyval_set_float(const char* key, float val) {
	char float_str[64];
	sprintf(float_str, "%f", val);
	keyval_set(key, float_str);
}

void keyval_flush(void) {
	_flush();
	n_writes = 0;
}

void keyval_gc(void) {
	_gc();
}

void keyval_wipe(void) {
	memset(keyval_dict.map, 0, sizeof(DictEntry) * keyval_dict.mask-1);
	keyval_dict.items = 0;
	keyval_strs.size = 0;
}

void keyval_app_suspend(void) {
	if(keyval_initialized) {
		_gc();
		_flush();
	}
}

