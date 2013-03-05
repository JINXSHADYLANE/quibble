#include "placement.h"

#include <memory.h>

#define n_rules 2
static PlacementRule rules[n_rules];
static DArray intervals;

static SprHandle* handles;

PlacementRuleReadable ruleset[] = {
	// No mushrooms over gaps.
	{
		"grass_end1, grass_end2, grass_start1, empty",

		"bg_mushroom_2, bg_mushroom_3, bg_mushroom_5, bg_mushroom_6, \
		mushroom_1,	mushroom_2, mushroom_3, mushroom_4, mushroom_5, \
		mushroom_6, spikeshroom_1, token"
	},

	// No cactus on water or near gap
	{
		"water1, water2, water3, water4, water_end1, water_end2, \
		water_start1, water_start2, \
		grass_start2, grass_end1",

		"spikeshroom_1",	
	},

};

void placement_init(void){
	intervals = darray_create(sizeof(PlacementInterval), 0);
	intervals.size = 0;

	uint array_size = 0;

	// Count the size of SprHandle array
	for(uint i = 0; i < n_rules;i++){
		// count tile array
		uint size = strlen(ruleset[i].tiles) + 1;
		char s[size];
		strcpy(s, ruleset[i].tiles);

		char* token = strtok(s, "\t, ");
		while (token) {
			array_size++;
	    	token = strtok(NULL, "\t, ");
		}
			
		// count unwanted array
		size = strlen(ruleset[i].unwanted) + 1;
		char s2[size];
		strcpy(s2, ruleset[i].unwanted);

		token = strtok(s2, "\t, ");
		while (token) {
			array_size++;
	    	token = strtok(NULL, "\t, ");
		}
	}
	handles = MEM_ALLOC(sizeof(SprHandle) * array_size);

	uint counter = 0;

	// Create SprHandle rules from txt rules
	for(uint i = 0; i < n_rules;i++){
		PlacementRule rule;

		// TODO fix repeating code by moving tokens to seperate function

		// fill tile array
		uint size = strlen(ruleset[i].tiles) + 1;
		char s[size];
		strcpy(s, ruleset[i].tiles);

		rule.start_tile = counter;

		char* token = strtok(s, "\t, ");
		while (token) {
    		SprHandle h = sprsheet_get_handle(token);
    		if(h == 0) h = empty_spr;
    		handles[counter++] = h; 		
    		token = strtok(NULL, "\t, ");
		}

		rule.n_tiles = counter - rule.start_tile;
		
		// fill unwanted array
		size = strlen(ruleset[i].unwanted) + 1;
		char s2[size];
		strcpy(s2, ruleset[i].unwanted);

		rule.start_unwanted = counter;

		token = strtok(s2, "\t, ");
		while (token) {
    		SprHandle h = sprsheet_get_handle(token);
    		handles[counter++] = h;
    		token = strtok(NULL, "\t, ");
		}

		rule.n_unwanted = counter - rule.start_unwanted;

		rules[i] = rule;

	}
}

void placement_close(void){
	MEM_FREE(handles);
	darray_free(&intervals);
}

void placement_reset(void){
	intervals.size = 0;
}

void placement_interval(Vector2 pos, SprHandle spr){
	// Iterate over rules
	for(uint i = 0; i < n_rules;i++){
		PlacementRule rule = rules[i];

		// iterate over tiles in a rule
		for(uint j = rule.start_tile; j < rule.start_tile+rule.n_tiles;j++){
			SprHandle s = handles[j];

			// if there is a rule for current tile, add interval
			if(spr == s){

				PlacementInterval t;
				t.range = vec2(pos.x, pos.y);

				t.start_unwanted = rule.start_unwanted;
				t.n_unwanted = rule.n_unwanted;

				darray_append(&intervals, &t);

			}

		}


	}

}

bool placement_allowed(Vector2 pos, SprHandle spr){
	
 	// iterate over intervals
	for(uint i = 0; i < intervals.size;i++){
		PlacementInterval *p = darray_get(&intervals, i);

		// iterate over unwanted sprites in interval
		for(uint j = p->start_unwanted; j < p->start_unwanted+p->n_unwanted;j++){
			SprHandle s = handles[j];

			// check if rule applies to current sprite
			if(spr == s){
				// check if sprite falls into interval range
				if( (pos.x > p->range.x && pos.x < p->range.y) ||
					(pos.y > p->range.x && pos.y < p->range.y) ||
					(pos.x < p->range.x && pos.y > p->range.y)
					) return false;
			} 

		}

	}
	return true;
}