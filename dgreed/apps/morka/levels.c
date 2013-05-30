#include "levels.h"
#include "mchains.h"
#include "tutorials.h"
#include "characters.h"
#include <keyval.h>
#include <mempool.h>
#include <memory.h>
#include <mml.h>

DArray levels_descs;
static int level_num = 0;

static LevelDesc* current_level = NULL;
static MMLObject mml;

extern int selected_char;

uint levels_count(void){
	return levels_descs.size;
}

uint levels_get_powerup_count(void){
	if(!current_level) return 0;
	uint count = 0;
	for(uint i = 0; i < POWERUP_COUNT;i++)
		if(current_level->powerup_num[i] > 0) count++;

	return count;		
}

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
		new.ai_rabbit_num = 0;

		for(int i = 0; i < POWERUP_COUNT;i++ )
			new.powerup_num[i] = 0;

		NodeIdx child = mml_get_first_child(&mml, rs_node);
		for(; child != 0; child = mml_get_next(&mml, child)) {
			const char* type = mml_get_name(&mml, child);

 			if(strcmp(type, "distance") == 0) {
				new.distance = mml_getval_int(&mml, child);
			}
			else if(strcmp(type, "rabbits") == 0) {

				NodeIdx r1 = mml_get_first_child(&mml, child);
				for(; r1 != 0; r1 = mml_get_next(&mml, r1)) {
					const char* txt = mml_get_name(&mml, r1);
					if(strcmp(txt, "max_combo") == 0) {
						new.ai_max_combo[new.ai_rabbit_num] = mml_getval_int(&mml, r1);
					}

					NodeIdx r = mml_get_first_child(&mml, r1);
					for(; r != 0; r = mml_get_next(&mml, r)) {

						const char* txt = mml_get_name(&mml, r);
						if(strcmp(txt, "speed") == 0){
							new.ai_rabbit_speeds[new.ai_rabbit_num] = mml_getval_float(&mml, r);
						} else if(strcmp(txt, "xjump") == 0){
							new.ai_rabbit_xjumps[new.ai_rabbit_num] = mml_getval_float(&mml, r);
						} else if(strcmp(txt, "yjump") == 0){
							new.ai_rabbit_yjumps[new.ai_rabbit_num] = mml_getval_float(&mml, r);
						}
					}
					new.ai_rabbit_num++;

				}

			}
			else if(strcmp(type, "powerups") == 0) {

				NodeIdx r1 = mml_get_first_child(&mml, child);
				for(; r1 != 0; r1 = mml_get_next(&mml, r1)) {
					const char* txt = mml_get_name(&mml, r1);

					PowerupType p = POWERUP_COUNT; 

					if(strcmp(txt, "trampoline") == 0) {
						p = TRAMPOLINE;
					} else if(strcmp(txt, "rocket") == 0) {
						p = ROCKET;
					} else if(strcmp(txt, "bomb") == 0) {
						p = BOMB;
					} else if(strcmp(txt, "shield") == 0) {
						p = SHIELD;
					}
					assert(p < POWERUP_COUNT);

					NodeIdx r = mml_get_first_child(&mml, r1);
					for(; r != 0; r = mml_get_next(&mml, r)) {

						const char* txt = mml_get_name(&mml, r);
						if(strcmp(txt, "num") == 0) {
							new.powerup_num[p] = mml_getval_int(&mml, r);
						} else if(strcmp(txt, "xmin") == 0){
							new.powerup_pos[p].x = mml_getval_float(&mml, r);
						} else if(strcmp(txt, "xmax") == 0){
							new.powerup_pos[p].y = mml_getval_float(&mml, r);
						}

					}

				}

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
		if(strcmp(desc->name, level_name) == 0){
			current_level = desc;
			level_num = i;
		}	
	}
	assert(current_level);
	tutorials_set_level(level_name);

	report_event("Level - Start",
		"name", level_name,
		"character", default_characters[selected_char].name,
		NULL, NULL
	);
}

bool levels_set_next(void){
	if(level_num+1 < levels_descs.size) {
		if(level_is_unlocked(level_num+1)) {
			level_num++;
			current_level = (LevelDesc*)darray_get(&levels_descs,level_num);
			tutorials_set_level(current_level->name);
			return true;
		}
	}
	return false;
}

bool levels_next_unlocked(void){
	return level_is_unlocked(level_num+1);
}

bool levels_is_final(void){
	return level_num == levels_descs.size-1;
}

LevelDesc* levels_current_desc(void){
	return current_level;
}

bool level_is_unlocked(uint i) {
	char key_name[32];
	sprintf(key_name, "ulck_l%d",i);

	return i == 0 || keyval_get_bool(key_name, false);
}

void levels_unlock_powerups(uint level_num){
	LevelDesc* desc= (LevelDesc*) darray_get(&levels_descs,level_num);
	for(int i = 0; i < POWERUP_COUNT;i++){

		char key_name[32];
		sprintf(key_name, "ulck_pwr_%d",i);	

		if(desc->powerup_num[i] > 0 && keyval_get_int(key_name,0) == 0)
			keyval_set_int(key_name,1);

	}
}

int levels_get_next_unlocked_powerup(){
	for(int i = 0; i < POWERUP_COUNT;i++){

		char key_name[32];
		sprintf(key_name, "ulck_pwr_%d",i);	

		if(keyval_get_int(key_name,0) == 1)
			return i;

	}
	return -1;
}

void levels_set_unlocked_powerup_seen(uint id){
	char key_name[32];
	sprintf(key_name, "ulck_pwr_%d",id);
	/* 
	0 - powerup is locked
	1 - unlocked but user not notified yet
	2 - unlocked and user has been notified
	*/
	assert(keyval_get_int(key_name,0) == 1);
	keyval_set_int(key_name,2);
}

void levels_unlock_next(){
	if(level_num+1 <= 11){
		char key_name[16];
		sprintf(key_name, "ulck_l%d",level_num+1);

		keyval_set_bool(key_name,true);

		levels_unlock_powerups(level_num+1);
	}

	// Unlock levels 13 - 18 only if 8 in 1-12 are perfected
	uint perfected = 0;
	for(uint i = 0; i < 12; ++i) {
		if(levels_get_place(i) == 1) {
			perfected++;
		}
	}

	if(perfected >= 8) {
		for(uint i = 12; i < 18; ++i) {
			char key_name[16];
			sprintf(key_name, "ulck_l%d", i);
			keyval_set_bool(key_name, true);
		}
	}
}

uint levels_get_place(uint level){
	assert(level < levels_descs.size);

	char key_name[32];
	sprintf(key_name, "place_l%d",level);	

	return keyval_get_int(key_name,0);
}

void levels_set_place(uint place){

	char key_name[32];
	sprintf(key_name, "place_l%d",level_num);

	uint prev = keyval_get_int(key_name,0);

	if(prev == 0 || place < prev)	
		keyval_set_int(key_name,place);	
}
