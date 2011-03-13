#include "menus.h"

#include <font.h>
#include <gui.h>
#include <gfx_utils.h>
#include <touch_ctrls.h>

#include "arena.h"
#include "game.h"
#include "ai.h"
#include "controls.h"
#include "state.h"

MenuState menu_state;
MenuState menu_transition;
float menu_transition_t;

#define MENU_BACKGROUND_LAYER 5
#define MENU_PANEL_LAYER  7
#define MENU_SHADOW_LAYER 8
#define MENU_TEXT_LAYER 9
#define MENU_POPUP_LAYER 8 

#define BACKGROUND_IMG "greed_assets/back_chapter_1.png"
#define MENU_ATLAS_IMG "greed_assets/menu_atlas.png"
#define CHAPTERS_ATLAS "greed_assets/chapter_atlas.png"
#define ARENAS_ATLAS "greed_assets/atlas_c%d.png"

TexHandle background;
TexHandle menu_atlas;
TexHandle chapters_atlas;
TexHandle arenas_atlas[MAX_CHAPTERS];

static RectF background_source = {0.0f, 0.0f, 480.0f, 320.0f};
static RectF panel_source = {0.0f, 0.0f, 338.0f, 225.0f};
static RectF title_source = {362.0f, 0.0f, 362.0f + 70.0f, 256.0f};
static RectF separator_source = {0.0f, 246.0f, 322.0f, 246.0f + 8.0f};
static RectF arena_list_panel_source[6];
static RectF chapter_list_panel_source[6];

static Color menu_text_color = COLOR_RGBA(214, 214, 215, 255);
static Color menu_sel_text_color = COLOR_RGBA(166, 166, 168, 255);

extern FontHandle huge_font;
extern FontHandle big_font;

// Tweakables
float menu_transition_length = 0.5f;
float menu_animation_depth = 0.5f;

void menus_init(void) {
	background = tex_load(BACKGROUND_IMG);
	menu_atlas = tex_load(MENU_ATLAS_IMG);
	chapters_atlas = tex_load(CHAPTERS_ATLAS);
	char file[128];
	for(uint i = 0; i < MAX_CHAPTERS; ++i) {
		sprintf(file, ARENAS_ATLAS, i+1);
		arenas_atlas[i] = tex_load(file);
	}	

	for(uint i = 0; i < 6; ++i) {
		float fx = 1.0f + (float)(i % 3) * 339.0f;
		float fy = 1.0f + (float)(i / 3) * 226.0f;
		arena_list_panel_source[i] = rectf(fx, fy, fx + 338.0f, fy + 225.0f);
		
		fx = 1.0f + (float)(i % 3) * 340.0f;
		fy = 1.0f + (float)(i / 3) * 227.0f;
		chapter_list_panel_source[i] = rectf(fx, fy, fx + 338.0f, fy + 225.0f);
	}

	menu_state = menu_transition = MENU_MAIN;
}

void menus_close(void) {
	tex_free(background);
	tex_free(menu_atlas);
	tex_free(chapters_atlas);
	for(uint i = 0; i < MAX_CHAPTERS; ++i) 
		tex_free(arenas_atlas[i]);
}

void menus_update(void) {
}

Vector2 _adjust_scale_pos(Vector2 p, Vector2 center, float scale) {
	Vector2 c_to_p = vec2_sub(p, center);
	c_to_p = vec2_scale(c_to_p, scale);
	return vec2_add(center, c_to_p);
}

float _t_to_scale(float t) {
	float max = 1.0f + menu_animation_depth;
	float min = 1.0f - menu_animation_depth;
	t = smoothstep(0.0f, 1.0f, (t + 1.0f) / 2.0f);
	return 1.0f / lerp(max, min, t);
}

void menus_draw_rect(TexHandle tex, uint layer, const RectF* src, 
	const Vector2* pos, float t) {
	assert(pos);

	float scale = _t_to_scale(t);
	Vector2 center = {240.0f, 168.0f};
	Vector2 dest = _adjust_scale_pos(*pos, center, scale);

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));
	gfx_draw_textured_rect(tex, layer, src, &dest, 0.0f, scale, c);
}

bool _menu_button(const Vector2* center, const char* text,
	const Vector2* ref, float t) {

	uint mx, my;
	mouse_pos(&mx, &my);
	Vector2 vmouse = vec2((float)mx, (float)my);

	Color text_col = color_lerp(menu_text_color, menu_text_color && 0xFFFFFF, 
		fabs(t));
	Color selected_text_col = color_lerp(menu_sel_text_color, 
		menu_sel_text_color && 0xFFFFFF, fabs(t));	

	float scale = _t_to_scale(t);
	Vector2 adj_vdest = _adjust_scale_pos(*center, *ref, scale);

	RectF rect = font_rect_ex(huge_font, text,
		&adj_vdest, scale);

	// Make rect smaller in y direction to prevent multiple
	// items being selected at once
	rect.top += 8.0f;
	rect.bottom -= 8.0f;

	bool mouse_inside = rectf_contains_point(&rect, &vmouse);
	Color curr_text_col = mouse_inside ? selected_text_col : text_col;

	font_draw_ex(huge_font, text, MENU_TEXT_LAYER, 
		&adj_vdest, scale, curr_text_col);

	return mouse_inside && mouse_down(MBTN_LEFT); 	
}

void _menu_text(const Vector2* topleft, const char* text,
	const Vector2* ref, float t, bool centered, bool small) {

	if(!text)
		return;

	Color text_col = color_lerp(menu_text_color, menu_text_color && 0xFFFFFF, 
		fabs(t));


	FontHandle font = huge_font;
	if(small)
		font = big_font;

	float scale = _t_to_scale(t);
	Vector2 adj_vdest = _adjust_scale_pos(*topleft, *ref, scale);
	RectF rect = font_rect_ex(font, text,
		&adj_vdest, scale);

	if(!centered) {
		adj_vdest.x += rectf_width(&rect) / 2.0f;
		adj_vdest.y += rectf_height(&rect) / 2.0f;
	}	
	font_draw_ex(font, text, MENU_TEXT_LAYER, 
		&adj_vdest, scale, text_col);
		
}

void _render_main(float t) {
	const char* items[] = {
		"Play",
		"Settings",
		"Info"
	};

	float scale = _t_to_scale(t);

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));

	// Panel
	Vector2 center = vec2(240.0f, 168.0f);
	gfx_draw_textured_rect(menu_atlas, MENU_PANEL_LAYER, &panel_source,
		&center, 0.0f, scale, c);

	// Title
	Vector2 vdest = _adjust_scale_pos(vec2(240.0f, 46.0f), center, scale);
	gfx_draw_textured_rect(menu_atlas, MENU_TEXT_LAYER, &title_source,
		&vdest, -PI/2.0f, scale, c);
	
	// Text
	vdest = vec2(240.0f, 139.0f);
	for(uint i = 0; i < ARRAY_SIZE(items); ++i) {
		if(_menu_button(&vdest, items[i], &center, t) && t==0.0f) {
			if(i == 0)
				menu_transition = MENU_CHAPTER;
			if(i == 1)
				menu_transition = MENU_SETTINGS;
			
			menu_transition_t = time_ms() / 1000.0f;
		}

		if(i < ARRAY_SIZE(items)-1) {
			vdest.y += 19.0f;
			Vector2 adj_vdest = _adjust_scale_pos(vdest, center, scale);
			gfx_draw_textured_rect(menu_atlas, MENU_SHADOW_LAYER,
				&separator_source, &adj_vdest, 0.0f, scale, c);	
		}

		vdest.y += 17.0f;	
	}
}

void _render_settings(float t) {
	controls_draw(t == 0.0f ? 0.01f : t);

	float scale = _t_to_scale(t);

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));

	// Panel
	Vector2 center = vec2(240.0f, 168.0f);
	gfx_draw_textured_rect(menu_atlas, MENU_PANEL_LAYER, &panel_source,
		&center, 0.0f, scale, c);

	// Title text
	Vector2 dest = center;
	dest.y = 74.0f;
	_menu_text(&dest, "Settings", &center, t, true, false);

	// Volume sliders
	dest = vec2(75.0f, 105.0f);
	_menu_text(&dest, "SFX volume:", &center, t, false, true);
	dest.x += 80.0f;
	if(t == 0.0f) {
		gui_slider(&dest);	
	}	
	else {
		float state = gui_getstate_slider(&dest);
		Vector2 pos = vec2_add(dest, vec2(254.0f/2.0f, 18.0f/2.0f));
		pos = _adjust_scale_pos(pos, center, scale);
		gui_draw_slider(&pos, scale, state, c);
	}

	dest.x = 75.0f;	
	dest.y += 20.0f; 
	_menu_text(&dest, "Music volume:", &center, t, false, true);
	dest.x += 80.0f;
	if(t == 0.0f) {
		gui_slider(&dest);
	}
	else {
		float state = gui_getstate_slider(&dest);
		Vector2 pos = vec2_add(dest, vec2(254.0f/2.0f, 18.0f/2.0f));
		pos = _adjust_scale_pos(pos, center, scale);
		gui_draw_slider(&pos, scale, state, c);
	}

	dest = vec2(75.0f, 155.0f);
	_menu_text(&dest, "Controls:", &center, t, false, true);

	const char* ctrl_names[] = {
		"joystick + 2 buttons",
		"joystick + button",
		"4 buttons"
	};	

	const Vector2 ctrl_pos[] = {
		{75.0f, 170.0f},
		{205.0f, 170.0f},
		{325.0f, 170.0f}
	};

	if(t == 0.0f) {
		controls_adjust();
		for(uint i = 0; i < ARRAY_SIZE(ctrl_pos); ++i) {
			gui_setstate_switch(&ctrl_pos[i], pstate.control_type == i);
			bool new_state = gui_switch(&ctrl_pos[i], ctrl_names[i]);
			if(new_state && pstate.control_type != i)
				pstate.control_type = i;
		}
	}	
	else {
		for(uint i = 0; i < ARRAY_SIZE(ctrl_pos); ++i) {
			bool state = gui_getstate_switch(&ctrl_pos[i]);
			float height = 30.0f;
			float width = 22.0f + font_width(big_font, ctrl_names[i]);
			Vector2 pos = vec2_add(ctrl_pos[i], vec2(width/2.0f, height/2.0f));
			pos = _adjust_scale_pos(pos, center, scale);
			gui_draw_switch(&pos, scale, ctrl_names[i], state, c);
		}
	}

	// Back button	
	Vector2 vdest = vec2(center.x, 295.0f);
	if(_menu_button(&vdest, "Back", &center, t) && t==0.0f) {
		menu_transition = MENU_MAIN;
		menu_transition_t = -time_ms() / 1000.0f;
	}
}

// Sliding menu handling
bool _mouse_acrobatics(float* camera_lookat_x, float* old_camera_lookat_x,
	uint item_count, float x_spacing, Vector2 center, RectF sliding_area,
	float width, float height, float t) {

	if(t != 0.0f)
		return false;

	uint mx, my;
	mouse_pos(&mx, &my);
	Vector2 vmouse = vec2((float)mx, (float)my);

	static Vector2 down_vmouse = {0.0f, 0.0f};
	static bool sliding_action = false;

	if(rectf_contains_point(&sliding_area, &vmouse)) {
		if(mouse_down(MBTN_LEFT)) {
			down_vmouse = vmouse;
			sliding_action = false;
		}

		if(mouse_pressed(MBTN_LEFT)) {
			float dist_sq = vec2_length_sq(vec2_sub(vmouse, down_vmouse));
			if(!sliding_action && dist_sq > 100.0f) {
				sliding_action = true;
				*old_camera_lookat_x = *camera_lookat_x;
			}	
			
			if(sliding_action) {
				float dx = vmouse.x - down_vmouse.x;
				*camera_lookat_x = lerp(*camera_lookat_x,
					*old_camera_lookat_x + dx*2.0f, 0.3f);
			}
		}
	}
	else {
		if(sliding_action)
			goto end_sliding;
	}

	if(mouse_up(MBTN_LEFT)) {
		end_sliding:
		if(sliding_action) {
			float frac, intgr;
			frac = modff(-*camera_lookat_x/x_spacing, &intgr);
			if(frac > 0.5f)
				*old_camera_lookat_x = intgr + 1.0f;
			else
				*old_camera_lookat_x = intgr;

			if(*old_camera_lookat_x < 0.0f)
				*old_camera_lookat_x = 0.0f;
			if(*old_camera_lookat_x > (float)(item_count-1))
				*old_camera_lookat_x = (float)(item_count-1);

			*old_camera_lookat_x *= -x_spacing;	
			
			sliding_action = false;
		}
		else {
			if(vmouse.y > center.y - height/2.0f &&
				vmouse.y < center.y + height/2.0f) {
				
				if(vmouse.x > center.x + width/2.0f) {
					*old_camera_lookat_x /= -x_spacing;
					*old_camera_lookat_x += 1.0f;
					*old_camera_lookat_x = 
						MIN(*old_camera_lookat_x, (float)(item_count-1));
					*old_camera_lookat_x *= -x_spacing;	
				}
				else if(vmouse.x < center.x - width/2.0f) {
					*old_camera_lookat_x /= -x_spacing;
					*old_camera_lookat_x -= 1.0f;
					*old_camera_lookat_x = MAX(*old_camera_lookat_x, 0.0f);
					*old_camera_lookat_x *= -x_spacing;	
				}	
				else
					return true;
			}	
		}
	}

	*camera_lookat_x = lerp(*camera_lookat_x, *old_camera_lookat_x, 0.2f);

	return false;
}	

static int selected_chapter = 0;
static float chp_camera_lookat_x = 0.0f;
static float chp_old_camera_lookat_x = 0.0f;
static float arn_camera_lookat_x = 0.0f;
static float arn_old_camera_lookat_x = 0.0f;
static const float x_spacing = 380.0f;

bool _render_slidemenu(float t, float* camera_lookat_x, 
	float* old_camera_lookat_x, MenuState back, const char** text,
	TexHandle texture, const RectF* sources, uint n_items) {

	const uint item_count = n_items;
	const float y_coord = 160.0f;
	const float x_start = 240.0f;
	const Vector2 center = {240.0f, 160.0f};

	float scale = _t_to_scale(t);

	float width = rectf_width(&panel_source) * scale;
	float height = rectf_height(&panel_source) * scale;
	RectF sliding_area = rectf(0.0f, y_coord - height/2.0f, 
		480.0f, y_coord + height/2.0f);

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));

	bool result = false;

	if(_mouse_acrobatics(camera_lookat_x, old_camera_lookat_x, 
		item_count, x_spacing, center, sliding_area, width, height, t)) {

		result = true;
	}

	for(uint i = 0; i < item_count; ++i) {
		Vector2 vdest = _adjust_scale_pos(
			vec2(x_start + *camera_lookat_x + (float)i * x_spacing, y_coord), 
			center, scale);

		if(vdest.x > -width/2.0f && vdest.x < 480.0f + width/2.0f) {
			uint src_idx = MIN(i, 5);
			TexHandle tex = src_idx < 5 ? texture : menu_atlas;
			const RectF* source = src_idx < 5 ? &sources[src_idx] : &panel_source;
			gfx_draw_textured_rect(tex, MENU_PANEL_LAYER,
				source, &vdest, 0.0f, scale, c);
			
			// Render text
			//vdest =	
			//	vec2(x_start + *camera_lookat_x + (float)i * x_spacing - 165.0f,
			//	y_coord + 70.0f);

			//_menu_text(&vdest, text[i], &center, t, false, false);
		}	
	}

	Vector2 vdest = vec2(center.x, 295.0f);
	if(_menu_button(&vdest, "Back", &center, t) && t==0.0f) {
		menu_transition = back;
		menu_transition_t = -time_ms() / 1000.0f;
	}

	return result;
}

void _render_chapters(float t) {	
	const char* text[5];
	for(uint i = 0; i < 5; ++i) {
		text[i] = chapters[i].name;
	}

	if(_render_slidemenu(t, &chp_camera_lookat_x, &chp_old_camera_lookat_x,
		MENU_MAIN, text, chapters_atlas, chapter_list_panel_source, MAX_CHAPTERS)) {

		selected_chapter = (int)(chp_camera_lookat_x / -x_spacing + 0.5f);
		selected_chapter = MIN(selected_chapter, MAX_CHAPTERS-1);
		selected_chapter = MAX(selected_chapter, 0);
	
		menu_transition = MENU_ARENA;
		menu_transition_t = time_ms() / 1000.0f;
		arn_camera_lookat_x = arn_old_camera_lookat_x = 0.0f;
	}	
}		

void _render_arenas(float t) {
	uint n_arenas = chapters[selected_chapter].n_arenas;
	if(_render_slidemenu(t, &arn_camera_lookat_x, &arn_old_camera_lookat_x,
		MENU_CHAPTER, chapters[selected_chapter].arena_name,
		arenas_atlas[selected_chapter], arena_list_panel_source, 
		n_arenas)) {

		uint selected_arena = (int)(arn_camera_lookat_x / -x_spacing + 0.5f);
		selected_arena = MIN(selected_arena, MAX_ARENAS_IN_CHAPTER-1);
		selected_arena = MAX(selected_arena, 0);

		const char* arena_name =
			chapters[selected_chapter].arena_file[selected_arena];
		if(arena_name != NULL) {
			menu_transition = MENU_GAME;
			menu_transition_t = time_ms() / 1000.0f;

			// TODO: Draw loading text here

			game_reset(arena_name, 2);
			ai_init_agent(1, 0);
		}	
	}
}

bool menu_last_game_did_win = false;

void _render_gameover(float t) {
	const char* items[] = {
		"Restart",
		"Back",
		"Next"
	};

	float scale = _t_to_scale(t);	
	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));

	bool did_win = menu_last_game_did_win;

	Vector2 center = {240.0f, 168.0f};

	// Panel
	gfx_draw_textured_rect(menu_atlas, MENU_POPUP_LAYER, &panel_source,
		&center, 0.0f, scale*0.7f, c);

	// Text
	Vector2 cursor = {center.x, 120.0f};
	_menu_text(&cursor, did_win ? "Well done!" : "Bad luck.",
		&center, t, true, false);

	// Buttons
	const char* next = arena_get_next();
	Vector2 pos[] = {{175.0f, 220.0f}, {240.0f, 295.0f}, {320.0f, 220.0f}};
	if(!did_win || next == NULL)
		pos[0].x = 240.0f;
	
	for(uint i = 0; i < ARRAY_SIZE(items); ++i) {
		if(i == 2 && (!did_win || next == NULL))
			break;

		Vector2 p = pos[i];
		if(_menu_button(&p, items[i], &center, t) && t == 0.0f) {
			if(i == 0) {
				menu_transition = MENU_GAME;
				game_reset(arena_get_current(), 2);
				ai_init_agent(1, 0);
				menu_transition_t = time_ms() / 1000.0f;
			}	
			if(i == 1) {
				menu_transition = MENU_ARENA;
				menu_transition_t = -time_ms() / 1000.0f;
			}	
			if(i == 2) {
				if(next) {
					menu_transition = MENU_GAME;
					game_reset(next, 2);
					ai_init_agent(1, 0);
					menu_transition_t = time_ms() / 1000.0f;
				}
			}
		}
		cursor.x += 120.0f;
	}
}

void _render_pause(float t) {
	float scale = _t_to_scale(t);
	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));

	Vector2 center = {240.0f, 168.0f};

	// Panel
	gfx_draw_textured_rect(menu_atlas, MENU_POPUP_LAYER, &panel_source,
		&center, 0.0f, scale*0.7f, c);

	// Text
	Vector2 cursor = {center.x, 120.0f};
	_menu_text(&cursor, "Paused", &center, t, true, false);

	float time = time_ms() / 1000.0f;

	// Continue
	cursor = vec2(center.x, 220.0f);
	if(_menu_button(&cursor, "Continue", &center, t) && t == 0.0f) {
		menu_transition = MENU_GAME;
		menu_transition_t = -time;
	}

	cursor = vec2(center.x, 295.0f);
	if(_menu_button(&cursor, "Back", &center, t) && t == 0.0f) {
		menu_transition = MENU_ARENA;
		menu_transition_t = -time;
	}
}

void _menus_switch(MenuState state, float t) {
	if(state == MENU_MAIN)
		_render_main(t);
	if(state == MENU_SETTINGS)
		_render_settings(t);
	if(state == MENU_CHAPTER)
		_render_chapters(t);
	if(state == MENU_ARENA)
		_render_arenas(t);
	if(state == MENU_GAMEOVER)
		_render_gameover(t);
	if(state == MENU_PAUSE)
		_render_pause(t);
	if(state == MENU_GAME)
		game_render_transition(t);
}

void menus_render(void) {	
	if(menu_transition == MENU_GAME && menu_state == MENU_GAME)
		return;

	float time = time_ms() / 1000.0f;
	float t = (time - fabs(menu_transition_t)) / menu_transition_length;

	// Background
	if(menu_state != MENU_GAME && menu_state != MENU_GAMEOVER
		&& menu_state != MENU_PAUSE) {
		Color color = COLOR_WHITE;
		if(menu_transition == MENU_GAME) {
			color = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, t);
		}
		RectF dest = rectf(0.0f, 0.0f, 0.0f, 0.0f);
		video_draw_rect(background, MENU_BACKGROUND_LAYER, &background_source,
			&dest, color);
	}	
	if((menu_state == MENU_GAMEOVER || menu_state == MENU_PAUSE) 
		&& menu_transition != MENU_GAME && menu_transition != menu_state) {
		Color color = color_lerp(COLOR_TRANSPARENT, COLOR_WHITE, t);
		RectF dest = rectf(0.0f, 0.0f, 0.0f, 0.0f);
		video_draw_rect(background, MENU_BACKGROUND_LAYER, &background_source,
			&dest, color);
	}

	if(menu_state == menu_transition) {
		_menus_switch(menu_state, 0.0f);
	}	
	else {

		if(menu_transition_t > 0.0f) {
			_menus_switch(menu_state, 0.0f + t);
			_menus_switch(menu_transition, -1.0f + t);
		}
		else {
			_menus_switch(menu_state, 0.0f - t);
			_menus_switch(menu_transition, 1.0f - t);
		}

		if(t >= 1.0f)
			menu_state = menu_transition;
	}
}

