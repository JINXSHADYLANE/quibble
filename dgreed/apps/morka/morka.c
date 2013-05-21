#include <malka/malka.h>
#include <malka/ml_states.h>
#include <system.h>
#include <sprsheet.h>
#include <particles.h>
#include <anim.h>
#include <uidesc.h>
#include <mfx.h>
#include <keyval.h>
#include <vfont.h>

#include <time.h>

#include "level_select.h"
#include "game.h"
#include "pause.h"
#include "game_over.h"
#include "shop.h"
#include "in_app.h"
#include "item_unlock.h"
#include "fadeout.h"
#include "powerup_help.h"
#include "splash.h"

#include "mchains.h"
#include "devmode.h"
#include "common.h"

const char* flurryKey = "WD543CKD93XRJ7XV8B77";

float v_width = 1024.0f;
float v_height = 768.0f;

SoundHandle music;
SourceHandle music_source;

void dgreed_preinit(void) {
}

bool dgreed_init(int argc, const char** argv) {
	params_init(argc, argv);

	rand_init(time(NULL));

	const char* sprsheet = ASSETS_DIR "spritesheet.mml";
    
#if !defined(TARGET_IOS) && !defined(ANDROID)
    uint r = params_find("-r");
    if(r != ~0 && params_count() > r+1){
		sscanf(params_get(r+1), "%fx%f", &v_width, &v_height);

	    if(v_width == 0) 
	    	v_width = 1024;
	    if(v_height == 0)
	    	v_height = 768; 

	   	assert(v_width >= 480 && v_height >= 320 && v_width <= 2560 && v_height <= 1600);
    }
#else
	uint w, h;
	video_get_native_resolution(&w, &h);
	v_width = w;
	v_height = h;
#endif

    if(v_width > 1280.0f || v_height > 800.0f)
    	sprsheet = ASSETS_DIR "r_spritesheet.mml";

	float n_width = v_width;
	float n_height = v_height;

	// Resolution hacks
	if(n_width < 500.0f && n_height < 360.0f) {
		v_width = n_width * 2.0f;
		v_height = n_height * 2.0f;
		sprsheet = ASSETS_DIR "s_spritesheet.mml";
	}
	else if(n_width < 960.0f && n_height < 640.0f) {
		v_width = n_width * 1.5f;
		v_height = n_height * 1.5f;
	}
	else if(n_width > 1280.0f && n_height > 800.0f) {
		v_width = n_width / 2.0f;
		v_height = n_height / 2.0f;
	}

	if(params_find("-s") != ~0) {
		n_width /= 2;
		n_height /= 2;
	}

	video_init_ex(n_width, n_height, v_width, v_height, "morka", false);
	sound_init();

	// Play music
	music = sound_load_stream(ASSETS_DIR "Radio Martini.ogg");
	music_source = sound_play_ex(music, true);

	keyval_init("morka.db");

	const char* particles = "particles.mml";
	particles_init_ex(ASSETS_DIR, particles, particle_layer);
	mfx_init(ASSETS_DIR "effects.mml");
	
	anim_init(ASSETS_DIR "animations.mml");

	devmode_init();

	malka_init();
	malka_params(argc, argv);

	malka_states_register("game", &game_state);
	malka_states_register("level_select", &level_select_state);
	malka_states_register("pause", &pause_state);
	malka_states_register("game_over", &game_over_state);
	malka_states_register("shop", &shop_state);
	malka_states_register("in_app", &in_app_state);
	malka_states_register("item_unlock", &item_unlock_state);	
	malka_states_register("fadeout", &fadeout_state);
	malka_states_register("powerup_help", &powerup_help_state);
	malka_states_register("splash", &splash_state);

	malka_states_set_transition_len(0.5f);

	malka_states_push("splash");

	sprsheet_init(sprsheet);
	mchains_init(ASSETS_DIR "mchains.mml");
	uidesc_init(ASSETS_DIR "uidesc.mml", vec2((float)v_width, (float)v_height));

	levels_init(ASSETS_DIR "levels.mml");
	levels_reset("level1");

	malka_states_init(SCRIPTS_DIR "main.lua");
	malka_states_start();
    
    return true;
}

void dgreed_close(void) {
	malka_states_end();
	malka_states_close();
	levels_close();
	uidesc_close();
	mchains_close();
	sprsheet_close();

	malka_close();

	devmode_close();

	anim_close();
	mfx_close();
	particles_close();

	keyval_close();

	sound_stop_ex(music_source);
	sound_free(music);

	sound_close();
	video_close();
}

bool dgreed_update(void) {
	if(char_up('q'))
		malka_states_pop();

	mfx_update();
	sound_update();

	return true;
}

bool dgreed_render(void) {
	devmode_render();
	particles_draw();
	//vfont_draw_cache(15, vec2(0.0f, 0.0f));
	return malka_states_step();
}

#ifndef TARGET_IOS
int dgreed_main(int argc, const char** argv) {
	params_init(argc, argv);

	dgreed_preinit();
	dgreed_init(argc, argv);
	while(true) {
		if(!dgreed_update())
			break;
		if(!dgreed_render())
			break;
	}
	dgreed_close();

	return 0;
}
#endif

