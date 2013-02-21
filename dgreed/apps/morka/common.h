#ifndef COMMON_H
#define COMMON_H 

#define ASSETS_DIR "morka_assets/"
#define SCRIPTS_DIR "morka_scripts/"
#define WIDTH 1024
#define HEIGHT 768

#ifndef __APPLE__
	#define FONT_NAME ASSETS_DIR "Chango-Regular.ttf"
#else
	#define FONT_NAME "Baskerville-Bold"
#endif
#define background_layer 0
#define dust_layer 1 // needs to be changed in particles.mml
#define bg_mushrooms_layer 2
#define finish_line_layer 3
#define foreground_layer 4
#define ai_rabbit_layer 5
#define rabbit_layer 6
#define ground_layer 7
#define particle_layer 8 // needs to be changed in particles.mml
#define hud_layer 9

#endif