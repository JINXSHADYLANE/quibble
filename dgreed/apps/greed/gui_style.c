#include "gui_style.h"
#include <system.h>
#include <font.h>

FontHandle small_font;
FontHandle big_font;
FontHandle huge_font;
TexHandle gui_tex;

GuiDesc greed_gui_style(bool highres) {
	GuiDesc style;

	gui_tex = tex_load("greed_assets/gui_new.png");
	if(highres) {
		huge_font = font_load_ex("greed_assets/lucida_grande_48px.bft", 1.0f);
		big_font = font_load_ex("greed_assets/lucida_grande_28px.bft", 0.5f);
		small_font = font_load_ex("greed_assets/lucida_grande_20px.bft", 0.5f);
	}	
	else {
		huge_font = font_load("greed_assets/lucida_grande_48px.bft");
		big_font = font_load("greed_assets/lucida_grande_14px.bft");
		small_font = font_load("greed_assets/lucida_grande_10px.bft");
	}	

	style.texture = gui_tex;
	style.font = big_font;

	style.text_color = COLOR_WHITE;

	style.first_layer = 10;
	style.second_layer = 11;
	style.text_layer = 12;

	style.src_button_up = rectf(0.0f, 0.0f, 252.0f, 41.0f);
	style.src_button_down = rectf(0.0f, 42.0f, 252.0f, 42.0f + 41.0f);
	style.src_switch_off_up = rectf(0.0f, 93.0f, 0.0f + 24.0f, 93.0f + 30.0f);
	style.src_switch_off_down = rectf(23.0f, 93.0f, 23.0f + 24.0f, 93.0f + 30.0f);
	style.src_switch_on_up = rectf(48.0f, 93.0f, 48.0f + 24.0f, 93.0f + 30.0f);
	style.src_switch_on_down = rectf(74.0f, 93.0f, 74.0f + 24.0f, 93.0f + 30.0f);
	style.src_slider = rectf(0.0f, 126.0f, 254.0f, 126.0f + 18.0f);
	style.src_slider_knob_up = rectf(1.0f, 149.0f, 1.0f + 18.0f, 149.0f + 18.0f);
	style.src_slider_knob_down = rectf(20.0f, 149.0f, 20.0f + 18.0f, 149.0f + 18.0f);

	return style;
}

void greed_gui_free(void) {
	tex_free(gui_tex);
	font_free(huge_font);
	font_free(big_font);
	font_free(small_font);
}	

