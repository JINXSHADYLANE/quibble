#include "level_select.h"
#include "game.h"
#include "levels.h"
#include "common.h"
#include "shop.h"
#include "hud.h"
#include <keyval.h>
#include <uidesc.h>
#include <vfont.h>
#include <mfx.h>

static bool sound_enabled;
static bool music_enabled;

static SprHandle sound_on;
static SprHandle sound_off;
static SprHandle music_on;
static SprHandle music_off;

extern SoundHandle music;
extern SourceHandle music_source;

static void level_select_init(void) {
	sound_on = sprsheet_get_handle("sound");
	sound_off = sprsheet_get_handle("sound_off");
	music_on = sprsheet_get_handle("music");
	music_off = sprsheet_get_handle("music_off");
	sound_enabled = keyval_get_bool("sound", true);
	music_enabled = keyval_get_bool("music", true);

	if(!sound_enabled)
		mfx_snd_set_volume(0.0f);

	if(!music_enabled)
		sound_pause_ex(music_source);

	vfont_select(FONT_NAME, 48.0f);
	for(int i = 0; i <= 9; ++i) {
		char str[2];
		sprintf(str, "%d", i);
		vfont_precache(str);
	}
}

static void level_select_close(void) {
	keyval_set_bool("sound", sound_enabled);
	keyval_set_bool("music", music_enabled);
}

static void level_select_enter(void) {
	game_pause();
}

static void level_select_leave(void) {

}

static bool level_select_update(void) {
	game_update_empty();
	return true;
}

static bool level_select_render(float t) {
	if(t != 0.0f) game_update_empty();

	vfont_select(FONT_NAME, 48.0f);

	UIElement* element = uidesc_get("level_select");

	UIElement* buttons_pos = uidesc_get_child(element, "buttons_pos");
	UIElement* button_sound = uidesc_get_child(element, "button_sound");
	UIElement* button_music = uidesc_get_child(element, "button_music");	

	SprHandle place_spr;
	SprHandle sound;
	SprHandle music;

	float alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer, rectf(0.0f, 0.0f, v_width, v_height), col); 

	int level_count = levels_count();

	SprHandle button_spr = sprsheet_get_handle("ball_autumn");
	SprHandle spec_button_spr = sprsheet_get_handle("ball_special");
	SprHandle lock_spr = sprsheet_get_handle("lock_autumn");

	Vector2 size = sprsheet_get_size_h(button_spr);
	uint columns = 6;
	float result = (float) level_count / columns;
	uint rows = ceil(result);

	if(level_count < columns) columns = level_count;
	uint c = 0;

	Vector2 spacing = vec2(size.x / 2.0f,size.y / 2.0f);

	Vector2 pos = buttons_pos->vec2;

	pos.x -= ( size.x * (columns-1) +  (spacing.x * (columns-1) ) ) / 2.0f; 
	pos.y -= ( size.y * (rows-1) +  (spacing.y * (rows-1) ) ) / 2.0f;

	for(int i = 0; i < level_count; i++) {

		LevelDesc* desc= (LevelDesc*) darray_get(&levels_descs,i);

		// Draw level icon
		spr_draw_cntr_h(
			i < 12 ? button_spr : spec_button_spr
			, hud_layer, pos, 0.0f, 1.0f, col
		);

		if(!level_is_unlocked(i)) {
			// Draw lock
			spr_draw_cntr_h(lock_spr, hud_layer, pos, 0.0f, 1.0f, col);			

			// Show help message when special level is pressed
			if(i >= 12 && hud_button_ex(empty_spr,pos,50.0f,col,t)) {
				malka_states_push("special_help");				
			}
		} 
		else {

			uint place = levels_get_place(i);
			char place_txt[16];

			switch(place) {
				case 1:
					place_spr = sprsheet_get_handle("ribbon_first");
					sprintf(place_txt, "First");
				break;

				case 2:
					place_spr = sprsheet_get_handle("ribbon_second");
					sprintf(place_txt, "Second");
				break;	

				case 3:
					place_spr = sprsheet_get_handle("ribbon_third");
					sprintf(place_txt, "Third");
				break;

				case 4:
					place_spr = sprsheet_get_handle("ribbon_last");
					sprintf(place_txt, "Last");
				break;	

				case 5:
					place_spr = sprsheet_get_handle("ribbon_tutorial");
					sprintf(place_txt, "Done");
				break;	

				default:
					place_spr = empty_spr;
				break;						
			}
			if(place_spr != empty_spr) {

				Vector2 ribbon_pos = vec2_add(pos,vec2(0.0f,40.0f));

				spr_draw_cntr_h(place_spr, hud_layer, ribbon_pos, 0.0f, 1.0f, col);

				vfont_select(FONT_NAME, 13.0f);

				Vector2 half_size = vec2_scale(vfont_size(place_txt), 0.5f);
				vfont_draw(place_txt, hud_layer+1, vec2_sub(ribbon_pos,half_size), col);				
			}

			// Button action
			if(hud_button_ex(empty_spr, pos, 50.0f, col, t)){
				levels_reset(desc->name);
				shop_reset();
				malka_states_push("shop");				
			}

			vfont_select(FONT_NAME, 48.0f);
			// Draw number
			Vector2 half_size = vec2_scale(vfont_number_size(i+1), 0.5f);
			vfont_draw_number(i+1, NULL, hud_layer+1, vec2_sub(pos,half_size), col);
		}

		// newline at end of column
		if(++c >= columns){
			c = 0;
			pos.x -= (size.x + spacing.x) * (columns-1);
			pos.y += size.y + spacing.y;
		} else {
			pos.x += size.x + spacing.x;
		}			

	}

	sound = sound_enabled ? sound_on : sound_off;
	music = music_enabled ? music_on : music_off; 

	// Sound toggle
	if(hud_button_ex(sound,button_sound->vec2,50.0f,col,t)){
		sound_enabled = !sound_enabled;
		keyval_set_bool("sound", sound_enabled);

		mfx_snd_set_volume(sound_enabled ? 1.0f : 0.0f);
	}

	// Music toggle
	if(hud_button_ex(music,button_music->vec2,50.0f,col,t)){
		music_enabled = !music_enabled;
		keyval_set_bool("music", music_enabled);

		if(music_enabled) 
			sound_resume_ex(music_source);	
		else
			sound_pause_ex(music_source);
	}


	return true;
}

StateDesc level_select_state = {
	.init = level_select_init,
	.close = level_select_close,
	.enter = level_select_enter,
	.preenter = NULL,
	.leave = level_select_leave,
	.update = level_select_update,
	.render = level_select_render
};
