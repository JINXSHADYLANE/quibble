#include "in_app.h"

#include "characters.h"
#include "common.h"
#include "game.h"
#include "hud.h"
#include "item_unlock.h"
#include "level_select.h"
#include "minimap.h"
#include "obj_types.h"

#include <keyval.h>
#include <uidesc.h>
#include <vfont.h>
#include <mfx.h>

#ifdef TARGET_IOS
	#include "iap.h"
#else
 
#endif

extern uint coins;
extern uint coins_original;

static uint item_count = 3;

StoreItemParams store_items[] = {
	{
	"100 coins in a cup",
	"cup",
	0.99f,
	100
	},

	{
	"300 coins in a toaster",
	"toaster",
	1.99f,
	300
	},

	{
	"1000 coins in a fridge",
	"refrigerator",
	2.99f,
	1000
	}		
};

static void in_app_init(void) {

}

static void in_app_close(void) {

}

static void in_app_preenter(void) {

}

static void in_app_enter(void) {
	game_pause();
}

static void in_app_leave(void) {
	keyval_set_int("coins",coins_original);
}

static bool in_app_update(void) {
	game_update_empty();
	return true;
}

static bool in_app_render(float t) {
	if(t != 0.0f) game_update_empty();

	UIElement* element = uidesc_get("in_app");
	UIElement* coin_icon = uidesc_get_child(element, "coin_icon");
	UIElement* coin_text = uidesc_get_child(element, "coin_text");

	UIElement* item_name = uidesc_get_child(element, "name");
	UIElement* item_cost = uidesc_get_child(element, "cost");
	UIElement* item_icon = uidesc_get_child(element, "icon");		

	UIElement* button_buy = uidesc_get_child(element, "button_buy");
	UIElement* button_quit = uidesc_get_child(element, "button_quit");

	float state_alpha = 1.0f-fabsf(t);
	byte a = lrintf(255.0f * state_alpha);
	Color col = COLOR_RGBA(255, 255, 255, a);

	spr_draw("blue_shade", hud_layer-1, rectf(0.0f, 0.0f, v_width, v_height), col); 

	// Coin icon
	spr_draw_cntr_h(coin_icon->spr, hud_layer+1,coin_icon->vec2, 0.0f, 1.0f, col);
	// Coin txt
	vfont_select(FONT_NAME, 38.0f);
	char str[32];
	sprintf(str, "%u",coins);
	vfont_draw(str, hud_layer+1, coin_text->vec2, col);

	static float xpos = 0.0f;
	static float inc = 600.0f;

	xpos = hud_scroll(xpos,inc,item_count,t);

	for(uint i = 0; i < item_count;i++){

		Vector2 offset = vec2(xpos + i * inc,0.0f);
		float d = normalize(fabsf(offset.x),0.0f,inc * (item_count-1));

		float scroll_alpha = 1.0f / exp(PI*d);
		byte a2 = lrintf(255.0f * scroll_alpha * state_alpha);
		Color col2 = COLOR_RGBA(255, 255, 255, a2);

		// Character icon
		SprHandle icon = sprsheet_get_handle(store_items[i].icon);
		Vector2 icon_pos = item_icon->vec2;
		icon_pos = vec2_add(icon_pos,offset);
		spr_draw_cntr_h(icon, hud_layer,icon_pos, 0.0f, 1.0f, col2);

		// Item Name
		vfont_select(FONT_NAME, 38.0f);
		Vector2 txt_pos = vec2_add(item_name->vec2,offset);
		Vector2	half_size = vec2_scale(vfont_size(store_items[i].name), 0.5f);
		vfont_draw(store_items[i].name, hud_layer, vec2_sub(txt_pos,half_size), col2);

		// Cost
		vfont_select(FONT_NAME, 48.0f);
		char cost[32];
		sprintf(cost, "%.2f$",store_items[i].cost);
		Vector2 txt_pos2 = vec2_add(item_cost->vec2,offset);
		Vector2	half_size2 = vec2_scale(vfont_size(cost), 0.5f);		
		vfont_draw(cost, hud_layer, vec2_sub(txt_pos2,half_size2), col2);

	}

	uint selection = hud_scroll_get_selection(xpos,-inc,item_count);

	// Buy button
	if(hud_button(button_buy, col, t)) {

		// TODO: actual purchasing
		if(true){

			mfx_trigger("buy");
			coins += store_items[selection].coins; // placeholder

			ObjItemUnlockParams p = {
				.spr = store_items[selection].icon,
				.text1 = "Purchased:",
				.text2 = "Coins",
				.text3 = store_items[selection].name,
				.state_num = 0
			};
			item_unlock_set(&p);

			malka_states_push("item_unlock");
		}
	}


	// Quit button
	if(hud_button(button_quit, col, t)) {
		malka_states_pop();
	}

	return true;
}

StateDesc in_app_state = {
	.init = in_app_init,
	.close = in_app_close,
	.enter = in_app_enter,
	.preenter = in_app_preenter,
	.leave = in_app_leave,
	.update = in_app_update,
	.render = in_app_render
};
