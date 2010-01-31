#include "layouts.h"
#include "common.h"
#include <mml.h>
#include <memory.h>

char current_layout[CHAR_COUNT];
uint layouts_count = 0;
MMLObject layouts;

char interesting_nonletters[] = 
	{',', '.', '/', ';', '\'', '[', ']', ']', '-', '='}; 

bool _is_char_interesting(char c) {
	if('a' <= c && 'z' >= c)
		return true;

	for(uint i = 0; i < sizeof(interesting_nonletters); ++i) {
		if(c == interesting_nonletters[i])
			return true;
	}
	return false;

}

void layouts_init(void) {
	char* layouts_text = txtfile_read(LAYOUTS_FILE);
	if(!layouts_text)
		LOG_ERROR("Unable to read layouts file");

	if(!mml_deserialize(&layouts, layouts_text))
		LOG_ERROR("Unable to deserialize layouts file");

	MEM_FREE(layouts_text);	

	NodeIdx root = mml_root(&layouts);

	if(strcmp(mml_get_name(&layouts, root), "layouts") != 0)
		LOG_ERROR("Invalid layouts file");

	layouts_count = mml_count_children(&layouts, root);

	// Set default layout
	layouts_set("default");
}

void layouts_close(void) {
	mml_free(&layouts);
}

void layouts_set(const char* name) {
	assert(name);

	NodeIdx root = mml_root(&layouts);
	NodeIdx layout = mml_get_first_child(&layouts, root);
	while(layout) {
		if(strcmp(mml_get_name(&layouts, layout), "layout") != 0)
			LOG_ERROR("Bad layout");
		
		if(strcmp(mml_getval_str(&layouts, layout), name) != 0)
			layout = mml_get_next(&layouts, layout);
		
		// Found right layout
		memset(current_layout, 0, sizeof(current_layout));
		NodeIdx pair = mml_get_first_child(&layouts, layout);
		while(pair) {
			char first = *mml_get_name(&layouts, pair);
			char second = *mml_getval_str(&layouts, pair);

			if(!_is_char_interesting(first) || !_is_char_interesting(second))
				pair = mml_get_next(&layouts, pair);

			current_layout[(size_t)first] = second;
			pair = mml_get_next(&layouts, pair);
		}
		return;
	}

	LOG_ERROR("No layout %s found!", name);
}

char layouts_map(char keyb_char) {
	return current_layout[(size_t)keyb_char];
}

