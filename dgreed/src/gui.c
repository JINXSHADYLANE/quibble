#include "gui.h"
#include "system.h"
#include "font.h"
#include "gfx_utils.h"

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

// Tweakables
float gui_widget_fadein_speed = 5.0f;
float gui_widget_fadeout_speed = 2.0f;

WidgetState gui_state[HASH_MAP_SIZE];
GuiDesc gui_style;
uint unique_widgets = 0;
bool default_style_loaded = false;

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
	uint* p = (uint*)&pos->x;
	result ^= _hash(*p);
	p = (uint*)&pos->y;
	result ^= _hash(*p);
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

GuiDesc gui_default_style(const char* assets_prefix) {
	const char* default_texture = "gui.png";
	const char* default_font = "nova.bft";

	GuiDesc style;

	// Construct paths
	char texture_path[256];
	char font_path[256];
	if(assets_prefix) {
		strcpy(texture_path, assets_prefix);
		strcpy(font_path, assets_prefix);
	}	
	else {
		texture_path[0] = font_path[0] = '\0';
	}
	strcat(texture_path, default_texture);
	strcat(font_path, default_font);

	// Load resources, fill in data
	style.texture = tex_load(texture_path);
	style.font = font_load(font_path);

	style.text_color = COLOR_WHITE;

	style.first_layer = 1;
	style.second_layer = 2;
	style.text_layer = 3;

	style.src_button_up = rectf(1.0f, 1.0f, 129.0f, 25.0f);
	style.src_button_down = rectf(1.0f, 26.0f, 129.0f, 50.0f);
	style.src_switch_off_up = rectf(1.0f, 1.0f, 129.0f, 25.0f);
	style.src_switch_off_down = rectf(1.0f, 139.0f, 129.0f, 163.0f);
	style.src_switch_on_up = rectf(1.0f, 26.0f, 129.0f, 50.0f);
	style.src_switch_on_down = rectf(1.0f, 164.0f, 129.0f, 188.0f);
	style.src_slider = rectf(1.0f, 51.0f, 197.0f, 67.0f);
	style.src_slider_knob_up = rectf(1.0f, 68.0f, 33.0f, 84.0f);
	style.src_slider_knob_down = rectf(34.0f, 68.0f, 66.0f, 84.0f);

	default_style_loaded = true;

	return style;
}

bool gui_validate_desc(const GuiDesc* desc) {
	assert(desc);

	// TODO: No way to validate texture/font handles yet, fix that

	if(desc->first_layer >= 16 || desc->second_layer >= 16 
		|| desc->text_layer >= 16)
		return false;
	if(!(desc->first_layer < desc->second_layer && 
		desc->second_layer < desc->text_layer))
		return false;

	// Assure button up&down sizes are same
	if(!feql(rectf_width(&desc->src_button_down), rectf_width(&desc->src_button_up)))
		return false;
	if(!feql(rectf_height(&desc->src_button_down), rectf_height(&desc->src_button_up)))
		return false;
	
	// Assure all switch img sizes are same
	float switch_on_up_w = rectf_width(&desc->src_switch_on_up);
	float switch_on_down_w = rectf_width(&desc->src_switch_on_down);
	float switch_off_up_w = rectf_width(&desc->src_switch_off_up);
	float switch_off_down_w = rectf_width(&desc->src_switch_off_down);
	float switch_on_up_h = rectf_height(&desc->src_switch_on_up);
	float switch_on_down_h = rectf_height(&desc->src_switch_on_down);
	float switch_off_up_h = rectf_height(&desc->src_switch_off_up);
	float switch_off_down_h = rectf_height(&desc->src_switch_off_down);
	if(!feql(switch_on_up_w, switch_on_down_w)
		|| !feql(switch_on_up_w, switch_off_up_w)
		|| !feql(switch_on_up_w, switch_off_down_w)
		|| !feql(switch_on_up_h, switch_on_down_h)
		|| !feql(switch_on_up_h, switch_off_up_h)
		|| !feql(switch_on_up_h, switch_off_down_h))
		return false;

	// Assure slider knob sizes are same
	float knob_up_w = rectf_width(&desc->src_slider_knob_up);
	float knob_down_w = rectf_width(&desc->src_slider_knob_down);
	float knob_up_h = rectf_height(&desc->src_slider_knob_up);
	float knob_down_h = rectf_height(&desc->src_slider_knob_down);
	if(!feql(knob_up_w, knob_down_w) || !feql(knob_up_h, knob_down_h))
		return false;
	
	// Assure slider & knob heights are same
	if(!feql(rectf_height(&desc->src_slider), knob_up_h))
		return false;
	
	return true;
}

void gui_init(const GuiDesc* desc) {
	assert(desc);
	assert(gui_validate_desc(desc));

	memcpy(&gui_style, desc, sizeof(GuiDesc));

	uint i;
	for(i = 0; i < HASH_MAP_SIZE; ++i)
		gui_state[i].hash = 0;
}	

void gui_close(void) {
	if(default_style_loaded) {
		tex_free(gui_style.texture);
		font_free(gui_style.font);
	}
}	

void gui_label(const Vector2* pos, const char* text) {
	assert(pos);
	assert(text);

	// Fonts look nicer at integer coordinates
	Vector2 p = vec2(floorf(pos->x), floorf(pos->y));
	font_draw(gui_style.font, text, gui_style.text_layer, &p, 
		gui_style.text_color);		
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
	float width = rectf_width(&gui_style.src_button_up);
	float height = rectf_height(&gui_style.src_button_up);
	RectF widget_rect = rectf(pos->x, pos->y, pos->x + width, pos->y + height);
	bool mouse_over = rectf_contains_point(&widget_rect, &mouse_pos);
	if(mouse_over) {
		state->state.button.blend += time_delta() * gui_widget_fadein_speed/1000.0f;
		if(state->state.button.blend > 1.0f)
			state->state.button.blend = 1.0f;
	}
	else {
		state->state.button.blend -= time_delta() * gui_widget_fadeout_speed/1000.0f;
		if(state->state.button.blend < 0.0f)
			state->state.button.blend = 0.0f;
	}		

	// Draw
	byte alpha = (byte)(state->state.button.blend * 255.0f);
	Color upper_color = COLOR_RGBA(255, 255, 255, alpha); 
	RectF dest = rectf(pos->x, pos->y, 0.0f, 0.0f);
	if(alpha < 255)
		video_draw_rect(gui_style.texture, gui_style.first_layer, 
			&gui_style.src_button_up, &dest, COLOR_WHITE); 
	if(alpha > 0)
		video_draw_rect(gui_style.texture, gui_style.second_layer, 
			&gui_style.src_button_down, &dest, upper_color);
	float text_width = font_width(gui_style.font, text);
	float text_offset_x = (width - text_width) / 2.0f;		
	float text_offset_y = (height - font_height(gui_style.font)) / 2.0f;
	Vector2 text_dest = vec2(floorf(pos->x + text_offset_x), 
		floorf(pos->y + text_offset_y));
	font_draw(gui_style.font, text, gui_style.text_layer, &text_dest, 
		gui_style.text_color);

	return mouse_over && mouse_down(MBTN_LEFT);
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
	float width = rectf_width(&gui_style.src_switch_on_up);
	float height = rectf_height(&gui_style.src_switch_on_up);
	
	bool text_inside = width > 80.0f;
	float text_width = font_width(gui_style.font, text);

	RectF widget_rect = rectf(pos->x, pos->y, pos->x + width, pos->y + height);
	if(!text_inside)
		widget_rect.right += text_width;

	bool mouse_over = rectf_contains_point(&widget_rect, &mouse_pos);
	if(mouse_over) {
		state->state._switch.blend += time_delta() * gui_widget_fadein_speed/1000.0f;
		if(state->state._switch.blend > 1.0f)
			state->state._switch.blend = 1.0f;
	}
	else {
		state->state._switch.blend -= time_delta() * gui_widget_fadeout_speed/1000.0f;
		if(state->state._switch.blend < 0.0f)
			state->state._switch.blend = 0.0f;
	}		

	// Draw
	float t = state->state._switch.blend;
	byte alpha = (byte)(t * 255.0f);
	Color upper_color = COLOR_RGBA(255, 255, 255, alpha); 
	RectF dest = rectf(pos->x, pos->y, 0.0f, 0.0f);
	if(alpha < 255) {
		RectF* source = state->state._switch.state ?
			&gui_style.src_switch_on_up : &gui_style.src_switch_off_up;
		video_draw_rect(gui_style.texture, gui_style.first_layer, source, 
			&dest, COLOR_WHITE); 
	}
	if(alpha > 0) {
		RectF* source = state->state._switch.state ?
			&gui_style.src_switch_on_down : &gui_style.src_switch_off_down;
		video_draw_rect(gui_style.texture, gui_style.second_layer, source, 
			&dest, upper_color);
	}		
	Vector2 text_offset;
	text_offset.y = (height - font_height(gui_style.font)) / 2.0f;
	if(text_inside) 
		text_offset.x = (width - text_width) / 2.0f;		
	else 
		text_offset.x = width - 2.0f;		

	Vector2 text_dest = vec2_add(*pos, text_offset);
	text_dest = vec2(floorf(text_dest.x), floorf(text_dest.y));
	font_draw(gui_style.font, text, gui_style.text_layer, &text_dest, 
		gui_style.text_color);

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
	float width = rectf_width(&gui_style.src_slider);
	float height = rectf_height(&gui_style.src_slider);
	RectF widget_rect = rectf(pos->x, pos->y, pos->x + width, pos->y + height);
	bool mouse_over = rectf_contains_point(&widget_rect, &mouse_pos);
	if(mouse_over) {
		state->state.slider.blend += time_delta() * gui_widget_fadein_speed/1000.0f;
		if(state->state.slider.blend > 1.0f)
			state->state.slider.blend = 1.0f;
	}
	else {
		state->state.slider.blend -= time_delta() * gui_widget_fadeout_speed/1000.0f;
		if(state->state.slider.blend < 0.0f)
			state->state.slider.blend = 0.0f;
	}		
	if(mouse_over && mouse_pressed(MBTN_LEFT)) 
		state->state.slider.state = (mouse_pos.x - pos->x) / width;

	// Draw
	float t = state->state.slider.blend;
	byte alpha = (byte)(t * 255.0f);
	Color upper_color = COLOR_RGBA(255, 255, 255, alpha); 
	RectF dest = rectf(pos->x, pos->y, 0.0f, 0.0f);
	video_draw_rect(gui_style.texture, gui_style.first_layer, 
		&gui_style.src_slider, &dest, COLOR_WHITE);
	float knob_width = rectf_width(&gui_style.src_slider_knob_up);
	float knob_height = rectf_height(&gui_style.src_slider_knob_up);
	float knob_offset_x = (width - knob_width) * state->state.slider.state;
	dest.left += knob_offset_x;
	dest.right = dest.left + knob_width;
	dest.bottom = dest.top + knob_height;
	if(alpha < 255)
		video_draw_rect(gui_style.texture, gui_style.first_layer, 
			&gui_style.src_slider_knob_up, &dest, COLOR_WHITE); 
	if(alpha > 0)
		video_draw_rect(gui_style.texture, gui_style.second_layer, 
			&gui_style.src_slider_knob_down, &dest, upper_color);

	return state->state.slider.state;
}

bool gui_getstate_switch(const Vector2* pos) {
	assert(pos);
	uint hash = _hash_widget(WIDGET_SWITCH, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_SWITCH;
		state->state._switch.blend = 0.0f;
		state->state._switch.state = false;
	}	
	assert(state->type == WIDGET_SWITCH);
	return state->state._switch.state;
}	

float gui_getstate_slider(const Vector2* pos) {
	assert(pos);
	uint hash = _hash_widget(WIDGET_SLIDER, pos);
	WidgetState* state = _get_widget(hash);
	if(state == NULL) {
		state = _alloc_widget(hash);
		state->type = WIDGET_SLIDER;
		state->state.slider.blend = 0.0f;
		state->state.slider.state = 0.5f;
	}
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

void gui_draw_label(const Vector2* center, float scale,
	const char* text, Color tint) {
	assert(center);
	assert(text);

	font_draw_ex(gui_style.font, text, gui_style.text_layer, center,
		scale, tint);
}

void gui_draw_button(const Vector2* center, float scale,
	const char* text, bool state, Color tint) {
	assert(center);
	assert(text);

	RectF* src = state ? 
		&gui_style.src_button_down : &gui_style.src_button_up;

	gfx_draw_textured_rect(gui_style.texture, gui_style.first_layer,
		src, center, 0.0f, scale, tint);
	
	font_draw_ex(gui_style.font, text, gui_style.text_layer, center,
		scale, tint);
}

void gui_draw_switch(const Vector2* center, float scale,
	const char* text, bool state, Color tint) {
	assert(center);
	assert(text);

	float width = rectf_width(&gui_style.src_switch_on_up);
	bool text_inside = width > 80.0f;

	RectF* src = state ?
		&gui_style.src_switch_on_up : &gui_style.src_switch_off_up;

	Vector2 dest = *center;
	Vector2 text_dest = *center;

	if(!text_inside) {
		float text_width = font_width(gui_style.font, text) * scale;
		dest.x -= (text_width) / 2.0f;
		text_dest.x += (width - 2.0f) * scale / 2.0f;
	}

	gfx_draw_textured_rect(gui_style.texture, gui_style.first_layer,
		src, &dest, 0.0f, scale, tint);

	font_draw_ex(gui_style.font, text, gui_style.text_layer, &text_dest,
		scale, tint);
}

void gui_draw_slider(const Vector2* center, float scale,
	float state, Color tint) {
	assert(center);

	float width = rectf_width(&gui_style.src_slider);
	float knob_width = rectf_width(&gui_style.src_slider_knob_up);
	float knob_offset_x = (width - knob_width) * state + knob_width/2.0f;
	knob_offset_x -= width / 2.0f;
	knob_offset_x *= scale;

	gfx_draw_textured_rect(gui_style.texture, gui_style.first_layer,
		&gui_style.src_slider, center, 0.0f, scale, tint);

	Vector2 knob_center = *center;
	knob_center.x += knob_offset_x;
	
	gfx_draw_textured_rect(gui_style.texture, gui_style.first_layer,
		&gui_style.src_slider_knob_up, &knob_center, 0.0f, scale, tint);

}
