#include "touch_ctrls.h"

// Tweakables
float joystick_area_radius = 80.0f;
float joystick_deadzone_radius = 10.0f;
float button_area_radius = 30.0f;
Color pressed_color = COLOR_RGBA(196, 196, 196, 255);

bool _get_touch(const Vector2* pos, float radius, Vector2* touch) {
	assert(pos);

	if(!mouse_pressed(MBTN_LEFT))
		return false;

	uint x, y;
	mouse_pos(&x, &y);
	Vector2 m_pos = {(float)x, (float)y};

	if(vec2_length_sq(vec2_sub(*pos, m_pos)) > radius*radius)
		return false;

	if(touch)
		*touch = m_pos;

	return true;	
}

Vector2 touch_joystick(TexHandle tex, uint layer, const RectF* back_src,
	const RectF* nub_src, const Vector2* pos) {
	assert(back_src);
	assert(nub_src);
	assert(pos);

	Color nub_color = COLOR_WHITE;

	float back_width = rectf_width(back_src);
	float back_height = rectf_height(back_src);

	// Draw static back
	RectF dest = {
		pos->x - back_width / 2.0f,
		pos->y - back_height / 2.0f, 
		0.0f, 0.0f
	};
	video_draw_rect(tex, layer, back_src, &dest, COLOR_WHITE);

	Vector2 touch;
	if(!_get_touch(pos, joystick_area_radius, &touch)) {
		touch = vec2(0.0f, 0.0f);	
	}	
	else {	
		touch = vec2_sub(touch, *pos);	
		nub_color = pressed_color;
	}	

	float nub_width = rectf_width(nub_src);
	float nub_height = rectf_width(nub_src);

	assert(feql(back_width, back_height) && feql(nub_width, nub_height));

	float back_radius = back_width / 2.0f;
	float nub_radius = nub_width / 2.0f;
	
	float move_radius = back_radius - nub_radius;

	assert(move_radius > 4.0f);
	
	float offset_len = vec2_length(touch);
	if(offset_len > move_radius) {
		touch = vec2_scale(touch, move_radius / offset_len);
		offset_len = move_radius;
		nub_color = COLOR_WHITE;
	}	
	
	// Draw nub
	dest.left = pos->x + touch.x - nub_width / 2.0f;
	dest.top = pos->y + touch.y - nub_height / 2.0f;
	video_draw_rect(tex, layer, nub_src, &dest, nub_color);

	if(offset_len < joystick_deadzone_radius)
		return vec2(0.0f, 0.0f);

	float norm_len = normalize(offset_len, 
		joystick_deadzone_radius, move_radius);	
	
	return vec2_scale(touch, norm_len / offset_len);
}	

bool touch_button(TexHandle tex, uint layer, const RectF* src,
	const Vector2* pos) {
	assert(src);
	assert(pos);

	Color color;
	bool res;

	Vector2 touch;
	if(_get_touch(pos, button_area_radius, &touch)) {
		color = pressed_color;
		res = true;
	}
	else {
		color = COLOR_WHITE;
		res = false;
	}

	float width = rectf_width(src);
	float height = rectf_height(src);

	RectF dest = {
		pos->x - width / 2.0f,
		pos->y - height / 2.0f,
		0.0f, 0.0f
	};
	video_draw_rect(tex, layer, src, &dest, color);

	return res;
}	

