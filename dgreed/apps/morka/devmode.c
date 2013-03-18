#include "devmode.h"
#include "common.h"

#include <system.h>
#include <font.h>
#include <darray.h>
#include <particles.h>
#include <memory.h>

static FontHandle dev_font;

extern bool draw_gfx_debug;
extern bool draw_physics_debug;
extern bool draw_ai_debug;
bool draw_stats = false;

void devmode_init(void) {
#ifndef NO_DEVMODE
	dev_font = font_load(ASSETS_DIR "devmode_font.bft");
#endif
}

void devmode_close(void) {
#ifndef NO_DEVMODE
	font_free(dev_font);
#endif
}

#define dprint(t) \
	font_draw(dev_font, t, 15, &cur, COLOR_BLACK); \
	cur.y += 18;

#define dprint2(t, param) \
	sprintf(text, t, param); \
	font_draw(dev_font, text, 15, &cur, COLOR_BLACK); \
	cur.y += 18;


#ifndef NO_DEVMODE
extern DArray* physics_components;
extern DArray* render_components;
extern DArray* update_components;

static void _draw_stats(void) {
	char text[128];
	Vector2 cur = {10.0f, 90.0f};

#ifdef TRACK_MEMORY
	MemoryStats mstats;
	mem_stats(&mstats);
	dprint( "mem");
	dprint2(" current %uk", (uint)mstats.bytes_allocated/1024);
	dprint2(" peak %uk", (uint)mstats.peak_bytes_allocated/1024);
	dprint2(" live %u", (uint)mstats.n_allocations - mstats.n_deallocations);
	dprint2(" total %u", (uint)mstats.n_allocations);
	cur.y += 18;
#endif

	dprint( "objects");
	dprint2(" physics: %u", physics_components->size);  
	dprint2(" render: %u", render_components->size);  
	dprint2(" update: %u", update_components->size);  
	cur.y += 18;

	const VideoStats* vstats = video_stats();
	dprint( "renderer");
	dprint2(" textures: %u", vstats->active_textures);
	dprint2(" batches: %u", vstats->frame_batches);
	dprint2(" rects: %u", vstats->frame_rects);
	dprint2(" lines: %u", vstats->frame_lines);
	cur.y += 18;

	const ParticleStats* pstats = particle_stats();
	dprint( "particles");
	dprint2(" psystems: %u", pstats->active_psystems);
	dprint2(" particles: %u", pstats->total_particles);
	cur.y += 18;
}
#endif

void devmode_render(void) {
#ifndef NO_DEVMODE
	if(char_down('g'))
		draw_gfx_debug = !draw_gfx_debug;
	if(char_down('p'))
		draw_physics_debug = !draw_physics_debug;
	if(char_down('a'))
		draw_ai_debug = !draw_ai_debug;
	if(char_down('s'))
		draw_stats = !draw_stats;

	if(draw_stats)
		_draw_stats();
#endif
}

