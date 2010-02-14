#include "gui.h"
#include "system.h"
#include "font.h"

typedef enum {
	WIDGET_BUTTON,
	WIDGET_SWITCH,
	WIDGET_SLIDER
} WidgetType;	

typedef struct {
	float blend;
} ButtonState;

typedef struct {
	float blend;
	bool state;
} SwitchState;

typedef struct {
	float blend;
	float state;
} SliderState;	

typedef struct {
	uint hash;
	WidgetType type;
	union {
		ButtonState button;
		SwitchState _switch;
		SliderState slider;
	} state;	
} WidgetState;	

// Works better when prime
#define HASH_MAP_SIZE 131

WidgetState gui_state[HASH_MAP_SIZE];
uint unique_widgets = 0;

// Hash function from http://www.concentric.net/~Ttwang/tech/inthash.htm
uint _hash(uint key)
{
	uint c2=0x27d4eb2d; // a prime or an odd constant
	key = (key ^ 61) ^ (key >> 16);
	key = key + (key << 3);
	key = key * c2;
	key = key ^ (key >> 15);
	return key;
}

// Returns hash code for widget
uint _hash_widget(WidgetType type, const Vector2* pos) {
	uint result = 0x4A55F113;
	result ^= _hash(type);
	result ^= _hash((uint)(size_t)&(pos->x));
	result ^= _hash((uint)(size_t)&(pos->y));
	assert(result);
	return result;
}	

// Returns pointer to widget state, or null if it does not exist
WidgetState* _get_widget(uint hash) {
	assert(hash);

	uint i;
	for(i = 0; i < HASH_MAP_SIZE; ++i) {
		uint idx = (hash + i) % HASH_MAP_SIZE;
		if(gui_state[idx].hash == hash)
			return &gui_state[idx];
	}
	return NULL;
}	

// Allocates space for widget in hash map.
// Make sure that widget with same hash value is not already in map!
WidgetState* _alloc_widget(uint hash) {
	assert(hash);

	if(unique_widgets > HASH_MAP_SIZE * 3 / 4)
		LOG_WARNING("More than 75\% of gui_state is being used");

	uint i;
	for(i = 0; i < HASH_MAP_SIZE; ++i) {
		uint idx = (hash + i) % HASH_MAP_SIZE;
		assert(gui_state[idx].hash != hash);
		if(gui_state[idx].hash == 0) {
			unique_widgets++;
			gui_state[idx].hash = hash;
			return &gui_state[idx];
		}	
	}
	LOG_ERROR("GUI hash map overflow");
	return NULL;
}	

#define GUI_LAYER1 1
#define GUI_LAYER2 2
#define GUI_LAYER_TEXT 3

#define GUI_TEXTURE "greed_assets/gui.png"
#define GUI_FONT "greed_assets/nova.bft"

const RectF src_button_down = {1.0f, 1.0f, 129.0f, 25.0f};
const RectF src_button_up = {1.0f, 26.0f, 129.0f, 50.0f};
const RectF src_slider = {1.0f, 51.0f, 197.0f, 67.0f};
const RectF src_knob_down = {1.0f, 68.0f, 33.0f, 84.0f};
const RectF src_knob_up = {34.0f, 68.0f, 66.0f, 84.0f};
TexHandle gui_texture;
FontHandle gui_font;

void gui_init(void) {
	uint i;
	for(i = 0; i < HASH_MAP_SIZE; ++i)
		gui_state[i].hash = 0;

	gui_texture = tex_load(GUI_TEXTURE);
	gui_font = font_load(GUI_FONT);
}	

void gui_close(void) {
	tex_free(gui_texture);
	font_free(gui_font);
}	

void gui_label(const Vector2* pos, const char* text) {
	assert(pos);
	assert(text);

	font_draw(gui_font, text, GUI_LAYER_TEXT, pos, COLOR_WHITE);		
}	

bool gui_button(const Vector2* pos, const char* text) {
	assert(pos);
	assert(text);

	// Get pointer to widget state
	uint hash = _hash_widget(WIDGET_BUTTON, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_BUTTON;
		state->state.button.blend = 0.0f;
	}
	assert(state->type == WIDGET_BUTTON);

	// Update state
	uint mouse_x, mouse_y;
	mouse_pos(&mouse_x, &mouse_y);
	Vector2 mouse_pos = vec2((float)mouse_x, (float)mouse_y);
	float width = src_button_up.right - src_button_up.left;
	float height = src_button_up.bottom - src_button_up.top;
	RectF widget_rect = rectf(pos->x, pos->y, pos->x + width, pos->y + height);
	bool mouse_over = rectf_contains_point(&widget_rect, &mouse_pos);
	if(mouse_over) {
		state->state.button.blend += time_delta() * 0.005f;
		if(state->state.button.blend > 1.0f)
			state->state.button.blend = 1.0f;
	}
	else {
		state->state.button.blend -= time_delta() * 0.002f;
		if(state->state.button.blend < 0.0f)
			state->state.button.blend = 0.0f;
	}		

	// Draw
	byte alpha = 255 - (byte)(state->state.button.blend * 255.0f);
	Color upper_color = COLOR_RGBA(255, 255, 255, alpha); 
	RectF dest = rectf(pos->x, pos->y, 0.0f, 0.0f);
	video_draw_rect(gui_texture, GUI_LAYER1, &src_button_up, &dest,
		COLOR_WHITE); 
	if(alpha > 0)
		video_draw_rect(gui_texture, GUI_LAYER2, &src_button_down, &dest,
			upper_color);
	float text_width = font_width(gui_font, text);
	float text_offset_x = (width - text_width) / 2.0f;		
	float text_offset_y = (height - font_height(gui_font)) / 2.0f;
	Vector2 text_dest = vec2(pos->x + text_offset_x, pos->y + text_offset_y);
	font_draw(gui_font, text, GUI_LAYER_TEXT, &text_dest, COLOR_WHITE);

	return mouse_over && mouse_pressed(MBTN_LEFT);
}	

bool gui_switch(const Vector2* pos, const char* text) {
	assert(pos);
	assert(text);

	// Get pointer to widget state
	uint hash = _hash_widget(WIDGET_SWITCH, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_SWITCH;
		state->state._switch.blend = 0.0f;
		state->state._switch.state = false;
	}
	assert(state->type == WIDGET_SWITCH);

	// Update state
	uint mouse_x, mouse_y;
	mouse_pos(&mouse_x, &mouse_y);
	Vector2 mouse_pos = vec2((float)mouse_x, (float)mouse_y);
	float width = src_button_up.right - src_button_up.left;
	float height = src_button_up.bottom - src_button_up.top;
	RectF widget_rect = rectf(pos->x, pos->y, pos->x + width, pos->y + height);
	bool mouse_over = rectf_contains_point(&widget_rect, &mouse_pos);
	if(mouse_over) {
		state->state._switch.blend += time_delta() * 0.005f;
		if(state->state._switch.blend > 1.0f)
			state->state._switch.blend = 1.0f;
	}
	else {
		state->state._switch.blend -= time_delta() * 0.002f;
		if(state->state._switch.blend < 0.0f)
			state->state._switch.blend = 0.0f;
	}		

	// Draw
	float t = state->state._switch.blend;
	t = state->state._switch.state ? 1.0f - t / 4.0f : t / 4.0f;
	byte alpha = 255 - (byte)(t * 255.0f);
	Color upper_color = COLOR_RGBA(255, 255, 255, alpha); 
	RectF dest = rectf(pos->x, pos->y, 0.0f, 0.0f);
	if(alpha < 255)
		video_draw_rect(gui_texture, GUI_LAYER1, &src_button_up, &dest,
			COLOR_WHITE); 
	if(alpha > 0)
		video_draw_rect(gui_texture, GUI_LAYER2, &src_button_down, &dest,
			upper_color);
	float text_width = font_width(gui_font, text);
	float text_offset_x = (width - text_width) / 2.0f;		
	float text_offset_y = (height - font_height(gui_font)) / 2.0f;
	Vector2 text_dest = vec2(pos->x + text_offset_x, pos->y + text_offset_y);
	font_draw(gui_font, text, GUI_LAYER_TEXT, &text_dest, COLOR_WHITE);

	if(mouse_over && mouse_down(MBTN_LEFT))
		state->state._switch.state = !state->state._switch.state;
	return state->state._switch.state;
}	

float gui_slider(const Vector2* pos) {
	assert(pos);

	// Get pointer to widget state
	uint hash = _hash_widget(WIDGET_SLIDER, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_SLIDER;
		state->state.slider.blend = 0.0f;
		state->state.slider.state = 0.5f;
	}
	assert(state->type == WIDGET_SLIDER);

	// Update state
	uint mouse_x, mouse_y;
	mouse_pos(&mouse_x, &mouse_y);
	Vector2 mouse_pos = vec2((float)mouse_x, (float)mouse_y);
	float width = src_slider.right - src_slider.left;
	float height = src_slider.bottom - src_slider.top;
	RectF widget_rect = rectf(pos->x, pos->y, pos->x + width, pos->y + height);
	bool mouse_over = rectf_contains_point(&widget_rect, &mouse_pos);
	if(mouse_over) {
		state->state.slider.blend += time_delta() * 0.005f;
		if(state->state.slider.blend > 1.0f)
			state->state.slider.blend = 1.0f;
	}
	else {
		state->state.slider.blend -= time_delta() * 0.002f;
		if(state->state.slider.blend < 0.0f)
			state->state.slider.blend = 0.0f;
	}		
	if(mouse_over && mouse_pressed(MBTN_LEFT)) 
		state->state.slider.state = (mouse_pos.x - pos->x) / width;

	// Draw
	float t = state->state.slider.blend;
	byte alpha = 255 - (byte)(t * 255.0f);
	Color upper_color = COLOR_RGBA(255, 255, 255, alpha); 
	RectF dest = rectf(pos->x, pos->y, 0.0f, 0.0f);
	video_draw_rect(gui_texture, GUI_LAYER1, &src_slider, &dest, COLOR_WHITE);
	float knob_width = src_knob_up.right - src_knob_up.left;
	float knob_height = src_knob_up.bottom - src_knob_up.top;
	float knob_offset_x = (width - knob_width) * state->state.slider.state;
	dest.left += knob_offset_x;
	dest.right = dest.left + knob_width;
	dest.bottom = dest.top + knob_height;
	if(alpha < 255)
		video_draw_rect(gui_texture, GUI_LAYER1, &src_knob_up, &dest,
			COLOR_WHITE); 
	if(alpha > 0)
		video_draw_rect(gui_texture, GUI_LAYER2, &src_knob_down, &dest,
			upper_color);

	return state->state.slider.state;
}

bool gui_getstate_switch(const Vector2* pos) {
	assert(pos);
	uint hash = _hash_widget(WIDGET_SWITCH, pos);
	WidgetState* state = _get_widget(hash);
	assert(state);
	assert(state->type == WIDGET_SWITCH);
	return state->state._switch.state;
}	

float gui_getstate_slider(const Vector2* pos) {
	assert(pos);
	uint hash = _hash_widget(WIDGET_SLIDER, pos);
	WidgetState* state = _get_widget(hash);
	assert(state);
	assert(state->type == WIDGET_SLIDER);
	return state->state.slider.state;
}	

void gui_setstate_switch(const Vector2* pos, bool val) {
	assert(pos);
	uint hash = _hash_widget(WIDGET_SWITCH, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_SWITCH;
		state->state._switch.blend = 0.0f;
	}
	assert(state);
	assert(state->type == WIDGET_SWITCH);
	state->state._switch.state = val;
}	

void gui_setstate_slider(const Vector2* pos, float val) {
	assert(pos);
	uint hash = _hash_widget(WIDGET_SLIDER, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_SLIDER;
		state->state.slider.blend = 0.0f;
	}
	assert(state);
	assert(state->type == WIDGET_SLIDER);
	state->state.slider.state = val;
}

