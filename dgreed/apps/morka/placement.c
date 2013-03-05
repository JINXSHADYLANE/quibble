#include "placement.h"

DArray rules;
DArray intervals;

PlacementRuleReadable ruleset[] = {
	// No mushrooms near gaps.
	{

		0.0f,
		0.0f,

		"grass_end1, grass_end2, grass_start1, empty",

		"bg_mushroom_2, bg_mushroom_3, bg_mushroom_5, bg_mushroom_6, \
		mushroom_1,	mushroom_2, mushroom_3, mushroom_4, mushroom_5, \
		mushroom_6, spikeshroom_1"
	},

	// No cactus within 500.0px of water
	{
		500.0f,
		500.0f,

		"water1, water2, water3, water4, water_end1, water_end2, \
		water_start1, water_start2",

		"spikeshroom_1",	
	},

	// end of rules indicator
	{
		0.0f,
		0.0f,
		"",
		""
	}	
};

void placement_init(void){
	rules = darray_create(sizeof(PlacementRule), 0);
	intervals = darray_create(sizeof(PlacementInterval), 0);

	// Create SprHandle rules from txt rules
	uint i = 0;
	while(ruleset[i].tiles){

		PlacementRule rule;
		rule.range = vec2(ruleset[i].before,ruleset[i].after);
		rule.tiles = darray_create(sizeof(SprHandle), 0);
		rule.unwanted = darray_create(sizeof(SprHandle), 0);

		// TODO fix repeating code by moving tokens to seperate function

		// fill tile array
		uint size = strlen(ruleset[i].tiles) + 1;
		char s[size];
		strcpy(s, ruleset[i].tiles);

		char* token = strtok(s, "\t, ");
		while (token) {
    		SprHandle h = sprsheet_get_handle(token);
    		if(h == 0) h = empty_spr;
    		darray_append(&rule.tiles,&h);
    		token = strtok(NULL, "\t, ");
		}
		
		// fill unwanted array
		size = strlen(ruleset[i].unwanted) + 1;
		char s2[size];
		strcpy(s2, ruleset[i].unwanted);

		token = strtok(s2, "\t, ");
		while (token) {
    		SprHandle h = sprsheet_get_handle(token);
    		darray_append(&rule.unwanted,&h);
    		token = strtok(NULL, "\t, ");
		}

		darray_append(&rules,&rule);

		i++;
	}
}

void placement_close(void){
	darray_free(&intervals);
	darray_free(&rules);
}

void placement_reset(void){
	intervals.size = 0;
}

void placement_interval(Vector2 pos, SprHandle spr){
	// Iterate over rules
	for(uint i = 0; i < rules.size;i++){
		PlacementRule *pr = darray_get(&rules, i);

		// iterate over tiles in a rule
		for(uint j = 0; j < pr->tiles.size;j++){
			
			SprHandle* sp = darray_get(&pr->tiles, j);
			SprHandle s = *sp;
			// if there is a rule for current tile, add interval
			if(spr == s){

				PlacementInterval t;
				t.range = vec2(pos.x + pr->range.x, pos.y + pr->range.y);

				t.unwanted = &pr->unwanted;

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
		for(uint j = 0; j < p->unwanted->size;j++){
			SprHandle* sp = darray_get(p->unwanted, j);
			SprHandle s = *sp;

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