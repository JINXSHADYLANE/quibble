#include "particles.h"
#include "gui.h"
#include "memory.h"

#define EDITOR_WIDTH 800
#define EDITOR_HEIGHT 600
const float screen_width = (float)EDITOR_WIDTH;
const float screen_height = (float)EDITOR_HEIGHT;

void draw_grid(uint layer, float spacing) {
	const Color grid_col1 = COLOR_RGBA(196, 196, 196, 128);
	const Color grid_col2 = COLOR_RGBA(128, 128, 128, 128);

	Vector2 start, end;
	uint n = 0;
	float x = screen_width / 2.0f; 
	while(x > 0.0f) {
		start = vec2(x, 0.0f);
		end = vec2(x, screen_height);
		video_draw_line(layer, &start, &end, (n++ % 5)==0 ? grid_col1 : grid_col2);
		x -= spacing;
	}	
	n = 0;
	x = screen_width / 2.0f; 
	while(x < screen_width) {
		start = vec2(x, 0.0f);
		end = vec2(x, screen_height);
		video_draw_line(layer, &start, &end, (n++ % 5)==0 ? grid_col1 : grid_col2);
		x += spacing;
	}
	n = 0;
	float y = screen_height / 2.0f; 
	while(y > 0.0f) {
		start = vec2(0.0f, y);
		end = vec2(screen_width, y);
		video_draw_line(layer, &start, &end, (n++ % 5)==0 ? grid_col1 : grid_col2);
		y -= spacing;
	}
	n = 0;
	y = screen_height / 2.0f; 
	while(y < screen_height) {
		start = vec2(0.0f, y);
		end = vec2(screen_width, y);
		video_draw_line(layer, &start, &end, (n++ % 5)==0 ? grid_col1 : grid_col2);
		y += spacing;
	}
}	

int dgreed_main(int argc, const char** argv) {
	log_init("pview.log", LOG_LEVEL_INFO);
	video_init(800, 600, "PView");

	GuiDesc style = gui_default_style("greed_assets/");
	gui_init(&style);
	particles_init();

	if(psystem_descs_count < 1) 
		LOG_ERROR("No particle systems described!");
	
	int active_desc = 0;
	const char* active_desc_name = psystem_descs[active_desc].name;

	RectF gui_area = rectf(0.0f, 0.0f, 280.0f, 60.0f);		
	Vector2 button_prev_pos = vec2(10.0f, 10.0f);
	Vector2 button_next_pos = vec2(140.0f, 10.0f);
	Vector2 label_name_pos = vec2(20.0f, 30.0f);
	char label_text[256];
	
	while(system_update()) {
		if(mouse_down(MBTN_LEFT)) {
			uint x, y;
			mouse_pos(&x, &y);
			Vector2 pos = vec2((float)x, (float)y);
			if(!rectf_contains_point(&gui_area, &pos))
				particles_spawn(active_desc_name, &pos, 0.0f);
		}	

		particles_update(time_ms() / 1000.0f);

		sprintf(label_text, "Current psystem: %s", active_desc_name);
		gui_label(&label_name_pos, label_text); 	
		if(gui_button(&button_prev_pos, "Previuos")) 
			active_desc = MAX(0, active_desc-1);
		if(gui_button(&button_next_pos, "Next"))
			active_desc = MIN(psystem_descs_count-1, active_desc+1);
		active_desc_name = psystem_descs[active_desc].name;	

		particles_draw();
		draw_grid(0, 12.0f);

		video_present();
	}	

	particles_close();
	gui_close();
	video_close();
	
	#ifdef TRACK_MEMORY
	MemoryStats stats;
	mem_stats(&stats);
	LOG_INFO("Memory usage stats:");
	LOG_INFO(" Total allocations: %u", stats.n_allocations);
	LOG_INFO(" Peak dynamic memory usage: %uB", stats.peak_bytes_allocated);
	if(stats.bytes_allocated) {
		LOG_INFO(" Bytes still allocted: %u", stats.bytes_allocated);
		LOG_INFO(" Dumping allocations info to memory.txt");
		mem_dump("memory.txt");
	}	
	#endif

	log_close();
	return 0;
}

