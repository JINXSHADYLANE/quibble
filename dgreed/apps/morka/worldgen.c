#include "worldgen.h"

#include <mempool.h>
#include <utils.h>

#include "obj_types.h"

static WorldPage current_page;
static WorldPage next_page;
static MemPool element_pool;
static RndContext rnd = NULL;

static const float page_width = 1024.0f;
static float page_cursor = 0.0f;

static char* bg_elements[] = {
	"bg_mushroom_1",
	"bg_mushroom_2",
	"bg_mushrooms_3",
	"bg_mushrooms_4"
};

static char* mushroom_sprites[] = {
	"mushroom_1",
	"mushroom_2",
	"mushroom_3",
	"mushroom_4"
};

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

	// Add background mushrooms
	uint n_back_shrooms = rand_int_ex(&rnd, 0, 4);
	float shroom_spacing = page_width / n_back_shrooms;
	for(uint i = 0; i < n_back_shrooms; ++i) {
		WorldElement* shroom = mempool_alloc(&element_pool);
		shroom->desc = &obj_deco_desc;
		shroom->pos = vec2(
				page_cursor + i * shroom_spacing + rand_float_range_ex(&rnd, -100.0f, 100.0f),
				483.0f + rand_int_ex(&rnd, -10, 10)
		);
		shroom->userdata = bg_elements[rand_int_ex(&rnd, 0, 4)];
		list_push_back(&new->background, &shroom->list);
	}

	// Add foreground mushrooms
	uint n_front_shrooms = rand_int_ex(&rnd, 0, 4);
	shroom_spacing = page_width / n_front_shrooms;
	for(uint i = 0; i < n_front_shrooms; ++i) {
		WorldElement* shroom = mempool_alloc(&element_pool);
		shroom->desc = &obj_mushroom_desc;
		shroom->pos = vec2(
				page_cursor + i * shroom_spacing + rand_float_range_ex(&rnd, -200.0f, 200.0f),
				583.0f + rand_int_ex(&rnd, -20, 20)
		);
		shroom->userdata = mushroom_sprites[rand_int_ex(&rnd, 0, 4)];
		list_push_back(&new->mushrooms, &shroom->list);
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
	}

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

