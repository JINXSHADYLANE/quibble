#include "localization.h"

#include "darray.h"
#include "datastruct.h"
#include "mml.h"
#include "memory.h"

const char* base_path = "base.loc";
const char* loc_files = NULL;
const char* current_lang = NULL;

// If true, do not translate, just return passed strings
static bool passthrough = true;

// Translation mml. Keep this around as string storage.
static MMLObject loc_mml;

// Dictionary used for translation
static Dict loc_dict;

// If true, generate base.loc while running
static bool genbase = false;

// This is used as set - keys are encountered strings, values - use count
static Dict base_dict;

// Key storage for base_dict
static DArray base_strs;

void loc_init(const char* _loc_files, bool production) {
	if(loc_files)
		loc_files = strclone(_loc_files);

	if(!production) {
		genbase = true;
		dict_init(&base_dict);
		base_strs = darray_create(sizeof(char), 0);
	}
	else {
		genbase = false;
	}
}

static void _merge_base(MMLObject* mml) {
	assert(mml);

	// Iterate over all strings in mml and delete them
	// from our dict
	
	NodeIdx root = mml_root(mml);
	assert(strcmp(mml_get_name(mml, root), "lang") == 0);
	assert(strcmp(mml_getval_str(mml, root), "base") == 0);

	for(NodeIdx str_node = mml_get_first_child(mml, root);
		0 != str_node;
		str_node = mml_get_next(mml, str_node)) {

		const char* str = mml_get_name(mml, str_node);
		dict_delete(&base_dict, str);
	}

	// Iterate over remaining strs in dict and insert them
	// into mml. TODO: Sort them by use count
	
	for(uint i = 0; i < base_dict.mask + 1; ++i) {
		DictEntry* e = &base_dict.map[i];
    
        if(e->key && e->data) {
        
			NodeIdx new = mml_node(mml, e->key, e->key);
			mml_append(mml, root, new);
		}
	}
}

void _loc_write_base(void) {
	if(genbase) {
		MMLObject mml;

		if(file_exists(base_path)) {
			// Read old base.loc
			char* base_text = txtfile_read(base_path);
			assert(base_text);
			if(!mml_deserialize(&mml, base_text))
				LOG_ERROR("Can't parse %s", base_path);
			MEM_FREE(base_text);
		}
		else {
			// No base.loc, make empty mml
			mml_empty(&mml);

			NodeIdx root = mml_root(&mml);
			mml_set_name(&mml, root, "lang");
			mml_setval_str(&mml, root, "base");
		}

		// Merge encountered strings with old ones
		_merge_base(&mml);

		// Write new base.loc
		char* base_text = mml_serialize(&mml);
		txtfile_write(base_path, base_text);
		printf("%s", base_text);
		MEM_FREE(base_text);

		mml_free(&mml);
	}
}

void loc_close(void) {
	_loc_write_base();

	if(loc_files)
		MEM_FREE(loc_files);

	if(current_lang) {
		MEM_FREE(current_lang);
		mml_free(&loc_mml);
		dict_free(&loc_dict);
	}

	passthrough = true;
	genbase = false;
}

void loc_select(const char* lang) {
	// Back out of redundant select
	if(current_lang && strcmp(current_lang, lang) == 0)
		return;

	// Unload current lang
	if(current_lang) {
		assert(!passthrough);
		passthrough = true;
		MEM_FREE(current_lang);
		current_lang = NULL;
		mml_free(&loc_mml);
		dict_free(&loc_dict);
	}

	if(strcmp(lang, "base") == 0)
		return;

	// Parse mml
	current_lang = strclone(lang);
	const char* lang_text = txtfile_read(lang);
	if(!mml_deserialize(&loc_mml, lang_text))
		LOG_ERROR("Unable to parse lang %s", lang);
	MEM_FREE(lang_text);

	// Populate dict with translation strings
	dict_init(&loc_dict);
	NodeIdx root = mml_root(&loc_mml);
	for(NodeIdx child = mml_get_first_child(&loc_mml, root);
		child != 0;
		child = mml_get_next(&loc_mml, child)) {

		dict_insert(
			&loc_dict, 
			mml_get_name(&loc_mml, child),
			mml_getval_str(&loc_mml, child)
		);
	}

	passthrough = false;
}

static void _rebase_strs(const void* old_base, const void* new_base, size_t old_size) {
	const void* minp = old_base;
	const void* maxp = minp + old_size;
	ptrdiff_t delta = new_base - old_base;
	for(uint i = 0; i < base_dict.mask + 1; ++i) {
		const void** ptr = (const void**)&base_dict.map[i].key;
		if(minp <= *ptr && *ptr < maxp)
			*ptr += delta;

		ptr = &base_dict.map[i].data;
		if(minp <= *ptr && *ptr < maxp)
			*ptr += delta;
	}
}

const char* loc_str(const char* str) {
	assert(str);

	if(genbase) {
		DictEntry* entry = dict_entry(&base_dict, str);

		if(entry) {
			// Old string, increase use count
			entry->data += 1;
		}
		else {
			// New string, copy to local store 
            void* old_data = base_strs.data;
			uint old_size = base_strs.size;
			darray_append_multi(&base_strs, str, strlen(str)+1);
			const char* local_str = base_strs.data + old_size;
            if(old_data != base_strs.data)
                _rebase_strs(old_data, base_strs.data, old_size);

			// Insert to dict, set count to 1
			dict_insert(&base_dict, local_str, (void*)1);
		}
	}

	if(passthrough) {
		return str;
	}
	else {
		const char* translated_str = dict_get(&loc_dict, str);
		if(!translated_str) {
			LOG_WARNING("Can't translate string \"%s\"", str);
			return str;
		}
		return translated_str;
	}
}

