#ifndef COMMON_H
#define COMMON_H 

#define ASSETS_DIR "morka_assets/"
#define SCRIPTS_DIR "morka_scripts/"
extern float v_width;
extern float v_height;

//#ifndef __APPLE__
	#define FONT_NAME ASSETS_DIR "Chango-Regular.ttf"
//#else
//	#define FONT_NAME "Baskerville-Bold"
//#endif

#define background_layer 0
#define dust_layer 1 // needs to be changed in particles.mml
#define bg_mushrooms_layer 2
#define finish_line_layer 3
#define trunk_layer 5
#define branch_layer 6
#define foreground_layer 7
#define ai_rabbit_layer 8
#define rabbit_layer 9
#define ground_layer 10
#define particle_layer 11 // needs to be changed in particles.mml
#define hud_layer 12

#endif
