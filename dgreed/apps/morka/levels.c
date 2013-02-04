#include "levels.h"
#include "mchains.h"
#include <mempool.h>
#include <memory.h>
#include <mml.h>

static LevelDesc* current_level = NULL;
static MMLObject mml;

void levels_init(const char* filename){

	levels_descs = darray_create(sizeof(LevelDesc), 0);

	const char* mml_text = txtfile_read(filename);

	if(!mml_deserialize(&mml, mml_text))
		LOG_ERROR("Unable to parse levels desc %s",filename);
	MEM_FREE(mml_text);

	NodeIdx root = mml_root(&mml);
	if(strcmp(mml_get_name(&mml, root), "levels") != 0)
		LOG_ERROR("Invalid levels desc %s", filename);


	// Iterate over levels
	NodeIdx rs_node = mml_get_first_child(&mml, root);
	for(uint i = 0; rs_node != 0; ++i, rs_node = mml_get_next(&mml, rs_node)) {

		if(strcmp(mml_get_name(&mml, rs_node), "level") != 0)
			LOG_ERROR("Bad level description");

		LevelDesc new = {0};

		new.name = mml_getval_str(&mml, rs_node);
		new.background = MAX_UINT32;

		NodeIdx child = mml_get_first_child(&mml, rs_node);
		for(; child != 0; child = mml_get_next(&mml, child)) {
			const char* type = mml_get_name(&mml, child);

 			if(strcmp(type, "distance") == 0) {
				new.distance = mml_getval_int(&mml, child);
			}
			else if(strcmp(type, "background") == 0) {
				const char* spr_name = mml_getval_str(&mml, child);
				SprHandle spr_handle = 0;
				if(strcmp(spr_name, ".empty") != 0)
					spr_handle = sprsheet_get_handle(spr_name);
				new.background = spr_handle;
			}
			else if(strcmp(type, "bg_chain") == 0) {
				new.bg_chain = mml_getval_str(&mml, child);
			}
			else if(strcmp(type, "fg_chain") == 0) {
				new.fg_chain = mml_getval_str(&mml, child);
			}
			else if(strcmp(type, "ground_chain") == 0) {
				new.ground_chain = mml_getval_str(&mml, child);
			}
		}
		assert(new.distance);
		assert(new.background != MAX_UINT32);
		assert(new.bg_chain);
		assert(new.fg_chain);
		assert(new.ground_chain);		

	    darray_append(&levels_descs, &new);

	}

}

void levels_close(void){
	mml_free(&mml);
	darray_free(&levels_descs);
}

void levels_reset(const char* level_name){
	current_level = NULL;
	for(int i = 0; i < levels_descs.size;i++){
		LevelDesc* desc= (LevelDesc*) darray_get(&levels_descs,i);
		if(strcmp(desc->name, level_name) == 0)
			current_level = desc;
	}
	assert(current_level);
}

LevelDesc* levels_current_desc(void){
	return current_level;
}