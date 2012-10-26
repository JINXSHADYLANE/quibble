#include "worldgen.h"

#include <mempool.h>
#include <utils.h>

#include "mchains.h"
#include "obj_types.h"

static WorldPage current_page;
static WorldPage next_page;
static MemPool element_pool;
static RndContext rnd = NULL;

static const float page_width = 1024.0f;
static float page_cursor = 0.0f;

static Chain* fg_chain;
static Chain* bg_chain;

static void _gen_page(WorldPage* prev, WorldPage* new) {
	assert(list_empty(&new->background));
	assert(list_empty(&new->mushrooms));

	// Add ground
	for(uint i = 0; i < 4; ++i) {
		WorldElement* ground = mempool_alloc(&element_pool);
		ground->desc = &obj_ground_desc;
		ground->pos = vec2(page_cursor + 128.0f + i * 256.0f, 683.0f);
		ground->userdata = (void*)i;
		list_push_back(&new->background, &ground->list);	
	}

	SprHandle spr;	
	uint advance;

	// Add background mushrooms
	static float bg_x = page_width;
	bg_x -= page_width;	
	while(bg_x < page_width) {
		char sym = mchains_next(bg_chain, &rnd);
		mchains_symbol_info(bg_chain, sym, &advance, &spr);

		if(spr) {
			WorldElement* shroom = mempool_alloc(&element_pool);
			shroom->desc = &obj_deco_desc;
			shroom->pos = vec2(page_cursor + bg_x, 483.0f);
			shroom->userdata = (void*)spr;
			list_push_back(&new->mushrooms, &shroom->list);
		}

		bg_x += (float)advance;
	}

	// Add foreground mushrooms
	static float fg_x = page_width;
	fg_x -= page_width;
	while(fg_x < page_width) {
		char sym = mchains_next(fg_chain, &rnd);
		mchains_symbol_info(fg_chain, sym, &advance, &spr);

		if(spr) {
			WorldElement* shroom = mempool_alloc(&element_pool);
			shroom->desc = &obj_mushroom_desc;
			shroom->pos = vec2(page_cursor + fg_x, 583.0f);
			shroom->userdata = (void*)spr;
			list_push_back(&new->mushrooms, &shroom->list);
		}

		fg_x += (float)advance;
	}

	page_cursor += page_width;
}

static void _free_page(WorldPage* page) {
	while(!list_empty(&page->background)) {
		WorldElement* el = list_last_entry(&page->background, WorldElement, list);
		list_pop_back(&page->background);
		mempool_free(&element_pool, el);
	}

	while(!list_empty(&page->mushrooms)) {
		WorldElement* el = list_last_entry(&page->mushrooms, WorldElement, list);
		list_pop_back(&page->mushrooms);
		mempool_free(&element_pool, el);
	}
}

void worldgen_reset(uint seed) {
	if(!rnd) {
		// First time
		rand_init_ex(&rnd, seed);
		mempool_init_ex(&element_pool, sizeof(WorldElement), 2048);
	}
	else {
		// Not first time
		rand_seed_ex(&rnd, seed);
		mempool_free_all(&element_pool);
		page_cursor = 0.0f;
		mchains_del(fg_chain);
		mchains_del(bg_chain);
	}

	fg_chain = mchains_new("fg");
	bg_chain = mchains_new("bg");

	// Reset & generate current page
	list_init(&current_page.background);
	list_init(&current_page.mushrooms);
	_gen_page(NULL, &current_page);

	// Reset & generate next page
	list_init(&next_page.background);
	list_init(&next_page.mushrooms);
	_gen_page(&current_page, &next_page);
}

void worldgen_close(void) {
	mchains_del(fg_chain);
	mchains_del(bg_chain);
	mempool_drain(&element_pool);
	rand_free_ex(&rnd);
}

WorldPage* worldgen_current(void) {
	return &current_page;
}

WorldPage* worldgen_next(void) {
	return &next_page;
}

WorldPage* worldgen_new_page(void) {
	_free_page(&current_page);
	current_page = next_page;

	// Patch the lists
	if(!list_empty(&next_page.background)) {
		current_page.background.next->prev = &current_page.background;
		current_page.background.prev->next = &current_page.background;
	}
	else {
		list_init(&current_page.background);
	}
	if(!list_empty(&next_page.mushrooms)) { 
		current_page.mushrooms.next->prev = &current_page.mushrooms;
		current_page.mushrooms.prev->next = &current_page.mushrooms;
	}
	else {
		list_init(&current_page.mushrooms);
	}
	
	list_init(&next_page.background);
	list_init(&next_page.mushrooms);

	_gen_page(&current_page, &next_page);

	return &next_page;
}

void worldgen_show(WorldPage* page) {
	WorldElement* el;

	list_for_each_entry(el, &page->background, list) {
		objects_create(el->desc, el->pos, el->userdata);
	}

	list_for_each_entry(el, &page->mushrooms, list) {
		objects_create(el->desc, el->pos, el->userdata);
	}
}

