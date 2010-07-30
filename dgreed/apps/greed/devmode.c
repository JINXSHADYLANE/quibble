#include "devmode.h"

#include <system.h>
#include <memory.h>
#include <gui.h>
#include <font.h>
#include <tweakables.h>
#include <particles.h>

#include "physics.h"
#include "game.h"
#include "arena.h"
#include "ai.h"

#define TEXT_LAYER 12

FontHandle small_font;

extern FontHandle small_font;
extern FontHandle big_font;

Tweaks* tweaks;

void devmode_init(void) {
	tweaks = tweaks_init("greed_assets/vars.mml", 
		rectf(10.0f, 60.0f, 410.0f, 310.0f), TEXT_LAYER, big_font);

	game_register_tweaks(tweaks);	
	physics_register_tweaks(tweaks);
	ai_register_tweaks(tweaks);
}

void devmode_close(void) {
	tweaks_close(tweaks);
}

void devmode_update(void) {
}

extern bool draw_physics_debug;
extern bool draw_ai_debug;
extern bool draw_gfx_debug;

#define DISPLAY_TEXT(t) \
	font_draw(small_font, t, TEXT_LAYER, &stats_cursor, COLOR_WHITE); \
	stats_cursor.y += y_adv

#define DISPLAY_TEXT2(t, param) \
	sprintf(text, t, param); \
	font_draw(small_font, text, TEXT_LAYER, &stats_cursor, COLOR_WHITE); \
	stats_cursor.y += y_adv

void _arena_select(void) {
	Vector2 cursor = vec2(10.0f, 76.0f);

	cursor.x += 200.0f;
	uint arena = (uint)(gui_slider(&cursor) * (float)total_arenas);
	arena = MIN(arena, total_arenas-1);

	const char* name = NULL;
	uint max_players = 0;

	uint curr_arena = 0;
	uint curr_chapter = 0;
	for(;curr_chapter < MAX_CHAPTERS; ++curr_chapter) {
		for(uint i = 0; 
			i < chapters[curr_chapter].n_arenas; 
			++i, ++curr_arena) {
			
			if(arena == curr_arena) {
				name = chapters[curr_chapter].arena_file[i];
				max_players = chapters[curr_chapter].arena_players[i];
			}
		}
	}

	char label[256];
	sprintf(label, "Arena: %s", name);
	cursor.x -= 200.0f;
	gui_label(&cursor, label);
	cursor.y += 22.0f;
	

	// Number of players
	cursor.x += 200.0f;
	uint n_players = 2 + (uint)(gui_slider(&cursor) * (float)(max_players-1));
	n_players = MIN(n_players, max_players);
	cursor.x -= 200.0f;
	sprintf(label, "Players: %u", n_players);
	gui_label(&cursor, label);
	cursor.y += 22.0f;

	// AI Personalities
	uint ai[3];
	for(uint i = 0; i < n_players-1; ++i) {
		cursor.x += 200.0f;
		uint ai_personality = 
			(uint)(gui_slider(&cursor) * (float)ai_personality_count);
		ai_personality = MIN(ai_personality, ai_personality_count-1);	

		ai[i] = ai_personality;	
		
		cursor.x -= 200.0f;
		sprintf(label, "A.I. %u: %s", i, ai_personality_names[ai_personality]);
		gui_label(&cursor, label);
		cursor.y += 22.0f;
	}

	cursor.y = 76.0f + 22.0f * 6.0f;
	cursor.x = 114.0f;
	if(gui_button(&cursor, "Go")) {
		game_reset(name, n_players);

		for(uint i = 0; i < n_players-1; ++i)
			ai_init_agent(1+i, ai[i]);	
	}

}

void devmode_render(void) {
	char text[256];

	Vector2 cursor = vec2(400.0f, 31.0f);
	bool show_tweaks = gui_switch(&cursor, "tweaks");
	if(show_tweaks) {
		tweaks_render(tweaks);
		return;
	}	
	cursor.y += 20.0f;

	bool show_arenasel = gui_switch(&cursor, "arena sel.");
	if(show_arenasel) {
		_arena_select();
		return;
	}
	cursor.y += 30.0f;

	draw_gfx_debug = gui_switch(&cursor, "gfx dbg");
	cursor.y += 20.0f;
	draw_physics_debug = gui_switch(&cursor, "phys. dbg");
	cursor.y += 20.0f;
	draw_ai_debug = gui_switch(&cursor, "ai dbg");
	cursor.y += 30.0f;

	Vector2 stats_cursor = vec2(10.0f, 24.0f);
	float y_adv = 8.0f;
	float x_adv = 60.0f;

	if(gui_switch(&cursor, "gfx")) {
		const VideoStats* v_stats = video_stats();
		DISPLAY_TEXT("gfx stats:");
		DISPLAY_TEXT2("  fps: %u", time_fps());
		DISPLAY_TEXT2("  act. tex: %u", v_stats->active_textures);
		DISPLAY_TEXT2("  layer sorts: %u", v_stats->frame_layer_sorts);
		DISPLAY_TEXT2("  batches: %u", v_stats->frame_batches);
		DISPLAY_TEXT2("  rects: %u", v_stats->frame_rects);
		DISPLAY_TEXT2("  lines: %u", v_stats->frame_lines);
		DISPLAY_TEXT2("  tex switches: %u", v_stats->frame_texture_switches);
		DISPLAY_TEXT("gfx layers (r/ln):");
		for(uint i = 0; i < v_stats->n_layers; ++i) {
			sprintf(text, "  %02u: %u/%u", i, v_stats->layer_rects[i],
				v_stats->layer_lines[i]);
			DISPLAY_TEXT(text);	
		}
		stats_cursor.x += x_adv;	
		stats_cursor.y = 24.0f;
	}	
	cursor.y += 20.0f;

	if(gui_switch(&cursor, "snd")) {
		const SoundStats* s_stats = sound_stats();
		DISPLAY_TEXT("snd stats:");
		DISPLAY_TEXT2("  samples: %u", s_stats->sample_count);
		DISPLAY_TEXT2("  streams: %u", s_stats->stream_count);
		DISPLAY_TEXT2("  pl. samples: %u", s_stats->playing_samples);
		DISPLAY_TEXT2("  pl. streams: %u", s_stats->playing_streams);
		stats_cursor.y += 8.0f;
	}
	cursor.y += 20.0f;

	#ifdef TRACK_MEMORY
	if(gui_switch(&cursor, "mem")) {
		MemoryStats m_stats;
		mem_stats(&m_stats);
		DISPLAY_TEXT("dyn. mem stats:");
		DISPLAY_TEXT2("  allocs: %u", m_stats.n_allocations);
		DISPLAY_TEXT2("  act. allocs: %u", m_stats.n_allocations -
			m_stats.n_deallocations);
		DISPLAY_TEXT2("  used: %zuK", m_stats.bytes_allocated / 1024);
		DISPLAY_TEXT2("  peak used: %zuK", m_stats.peak_bytes_allocated / 1024);
		stats_cursor.y += 8.0f;
	}
	cursor.y += 20.0f;
	#endif

	if(gui_switch(&cursor, "particles")) {
		const ParticleStats* p_stats = particle_stats();
		DISPLAY_TEXT("particle stats:");
		DISPLAY_TEXT2("  psystem descs: %u", p_stats->psystems);
		DISPLAY_TEXT2("  psystems: %u", p_stats->active_psystems);
		DISPLAY_TEXT2("  particles: %u", p_stats->total_particles);
		DISPLAY_TEXT2("  born/s: %u", p_stats->born_in_last_second);
		DISPLAY_TEXT2("  died/s: %u", p_stats->dead_in_last_second);
	}

}

