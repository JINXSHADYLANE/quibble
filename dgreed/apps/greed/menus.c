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
#include "sounds.h"
#include "background.h"

MenuState menu_state;
MenuState menu_transition;
float menu_transition_t;

#define MENU_ICONS_LAYER  7
#define MENU_SHADOW_LAYER 8
#define MENU_TEXT_LAYER 9
#define MENU_POPUP_LAYER 8 

#define MENU_ATLAS_IMG "greed_assets/gui_icons_2.png"
#define ICONS_IMG "greed_assets/gui_icons.png"
#define CHAPTERS_ATLAS "greed_assets/chapter_atlas.png"
#define ARENAS_ATLAS "greed_assets/atlas_c%d.png"

TexHandle menu_atlas;
TexHandle chapters_atlas;
TexHandle arenas_atlas[MAX_CHAPTERS];
TexHandle gui_icons;

static RectF icon_rect = {0.0f, 0.0f, 128.0f, 128.0f};
static RectF panel_source = {0.0f, 0.0f, 338.0f, 225.0f};
static RectF title_source = {16.0f, 0.0f, 496.0f, 128.0f};
static RectF arena_list_panel_source[6];

extern FontHandle huge_font;
extern FontHandle big_font;

// Tweakables
float menu_transition_length = 0.5f;
float menu_animation_depth = 0.5f;

void menus_init(void) {
	background_init();
	menu_atlas = tex_load(MENU_ATLAS_IMG);
	gui_icons = tex_load(ICONS_IMG);
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
	}	
	menu_state = menu_transition = MENU_MAIN;
}

void menus_close(void) {
	background_close();
	tex_free(menu_atlas);
	tex_free(gui_icons);
	tex_free(chapters_atlas);
	for(uint i = 0; i < MAX_CHAPTERS; ++i) 
		tex_free(arenas_atlas[i]);
}

void menus_update(void) {
	// Update background only if we're in a state which needs update
	bool update = false;
	const MenuState bkg_update[] = {
		MENU_MAIN, MENU_SETTINGS, MENU_CHAPTER, MENU_ARENA
	};

	for(uint i = 0; i < ARRAY_SIZE(bkg_update); ++i) {
		MenuState s = bkg_update[i];
		if(menu_state == s || menu_transition == s) {
			update = true;
			break;
		}
	}

	if(update)
		background_update();
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
	Vector2 center = {240.0f, 160.0f};
	Vector2 dest = _adjust_scale_pos(*pos, center, scale);

	Color c = color_lerp(COLOR_WHITE, COLOR_TRANSPARENT, fabs(t));
	gfx_draw_textured_rect(tex, layer, src, &dest, 0.0f, scale, c);
}

RectF _icon_src(uint id) {
	float x = 128.0f * (float) (id % 4);
	float y = 128.0f * (float) (id / 4);
	RectF result = icon_rect;
	result.left += x; result.right += x;
	result.top += y; result.bottom += y;
	return result;
}

bool _menu_icon(uint id, Vector2 pos, float t) {
	assert(id < 16);

	Color c = color_lerp(COLOR_WHITE, COLOR_WHITE & 0xFFFFFF, fabs(t));	
	float scale = _t_to_scale(t);
	pos = _adjust_scale_pos(pos, vec2(240.0f, 160.0f), scale);
	RectF src = _icon_src(id);

	if(t == 0.0f) {
		return touch_button(gui_icons, MENU_ICONS_LAYER, &src, &pos);
	}
	else {
		float s = 64.0f * scale;
		RectF dest = {pos.x - s, pos.y - s, pos.x + s, pos.y + s};
		video_draw_rect(gui_icons, MENU_ICONS_LAYER, &src, &dest, c);
		return false;
	}
}

void _render_main(float t) {
	float time = time_ms() / 1000.0f;
	Vector2 pos = {240.0f, 88.0f};
	menus_draw_rect(menu_atlas, MENU_ICONS_LAYER, &title_source, &pos, t);	

	// Play
	pos = vec2(240.0f, 230.0f);
	if(_menu_icon(0, pos, t)) {
		menu_transition = MENU_CHAPTER;
		menu_transition_t = time;
	}

	// Settings
	pos = vec2(90.0f, 230.0f);
	_menu_icon(1, pos, t);

	// Info
	pos = vec2(390.0f, 230.0f);
	_menu_icon(2, pos, t);
}

void _render_settings(float t) {
}

void _render_gameover(float t) {
}

void _render_pause(float t) {
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
			gfx_draw_textured_rect(tex, MENU_ICONS_LAYER,
				source, &vdest, 0.0f, scale, c);
			
			// Render text
			//vdest =	
			//	vec2(x_start + *camera_lookat_x + (float)i * x_spacing - 165.0f,
			//	y_coord + 70.0f);

			//_menu_text(&vdest, text[i], &center, t, false, false);
		}	
	}

	//Vector2 vdest = vec2(center.x, 295.0f);
	//if(_menu_button(&vdest, "Back", &center, t) && t==0.0f) {
	//	menu_transition = back;
	//	menu_transition_t = -time_ms() / 1000.0f;
	//}

	return result;
}

void _render_chapters(float t) {	
	const char* text[5];
	for(uint i = 0; i < 5; ++i) {
		text[i] = chapters[i].name;
	}

	if(_render_slidemenu(t, &chp_camera_lookat_x, &chp_old_camera_lookat_x,
		MENU_MAIN, text, chapters_atlas, arena_list_panel_source, MAX_CHAPTERS)) {

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
	static const char* arena_name;
	static bool load = false;
	if(load) {
			game_reset(arena_name, 2);
			ai_init_agent(1, 0);

			menu_transition = MENU_GAME;
			menu_transition_t = time_ms() / 1000.0f;

			load = false;
	}
	if(_render_slidemenu(t, &arn_camera_lookat_x, &arn_old_camera_lookat_x,
		MENU_CHAPTER, chapters[selected_chapter].arena_name,
		arenas_atlas[selected_chapter], arena_list_panel_source, 
		n_arenas)) {

		uint selected_arena = (int)(arn_camera_lookat_x / -x_spacing + 0.5f);
		selected_arena = MIN(selected_arena, MAX_ARENAS_IN_CHAPTER-1);
		selected_arena = MAX(selected_arena, 0);

		arena_name = chapters[selected_chapter].arena_file[selected_arena];
		if(arena_name != NULL) {
			Vector2 center = {240.0f, 160.0f};
			font_draw_ex(huge_font, "Loading...", MENU_TEXT_LAYER, &center, 1.0f,
			COLOR_WHITE); 	
			load = true;
		}	
	}
	else {
		arena_name = NULL;
		load = false;
	}
}

bool menu_last_game_did_win = false;

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
		background_render(color);
	}	
	if((menu_state == MENU_GAMEOVER || menu_state == MENU_PAUSE) 
		&& menu_transition != MENU_GAME && menu_transition != menu_state) {
		Color color = color_lerp(COLOR_TRANSPARENT, COLOR_WHITE, t);
		background_render(color);
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

