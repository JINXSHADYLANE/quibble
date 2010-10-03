#include "ego.hpp"

extern "C" {
		#include "utils.h"
		#include "system.h"
		#include "font.h"
		#include "gui.h"
		#include "gfx_utils.h"
}

// --- Log ---

void ego::Log::init(const ego::string& filename) {
	c::log_init(filename.c_str(), LOG_LEVEL_INFO);		
}

void ego::Log::close() {
	c::log_close();
}

void ego::Log::info(const char* format, ...) {
	va_list args;
	va_start(args, format);
	c::log_send(LOG_LEVEL_INFO, format, args);
	va_end(args);
}

void ego::Log::info(const ego::string& msg) {
	info("%s", msg.c_str());
}

void ego::Log::warning(const char* format, ...) {
	va_list args;
	va_start(args, format);
	c::log_send(LOG_LEVEL_WARNING, format, args);
	va_end(args);
}

void ego::Log::warning(const ego::string& msg) {
	warning("%s", msg.c_str());
}

void ego::Log::error(const char* format, ...) {
	va_list args;
	va_start(args, format);
	c::log_send(LOG_LEVEL_WARNING, format, args);
	va_end(args);

	close();
	exit(0);
}

void ego::Log::error(const ego::string& msg) {
	error("%s", msg.c_str());
}

// --- Vector2 ---

ego::Vector2::Vector2()
	: x(0.0f), y(0.0f) {
}

ego::Vector2::Vector2(const ego::Vector2& v)
	: x(v.x), y(v.y) {
}

ego::Vector2::Vector2(float x, float y)
	: x(x), y(y) {
}

ego::Vector2& ego::Vector2::operator =(const ego::Vector2& v) {
	x = v.x;
	y = v.y;
	return *this;
}

ego::Vector2 ego::Vector2::operator +(const ego::Vector2& v) const {
	ego::Vector2 result(*this);
	result += v;
	return result;
}

ego::Vector2& ego::Vector2::operator +=(const ego::Vector2& v) {
	x += v.x;
	y += v.y;
	return *this;
}

ego::Vector2 ego::Vector2::operator -(const ego::Vector2& v) const {
	ego::Vector2 result(*this);
	result -= v;
	return result;
}

ego::Vector2& ego::Vector2::operator -=(const ego::Vector2& v) {
	x += v.x;
	y += v.y;
	return *this;
}

ego::Vector2 ego::Vector2::operator *(float s) const {
	ego::Vector2 result(*this);
	result *= s;
	return result;
}

ego::Vector2& ego::Vector2::operator *=(float s) {
	x *= s;
	y *= s;
	return *this;
}

ego::Vector2 ego::Vector2::operator /(float s) const {
	ego::Vector2 result(*this);
	result /= s;
	return result;
}

ego::Vector2& ego::Vector2::operator /=(float s) {
	x /= s;
	y /= s;
	return *this;
}

ego::Vector2 ego::Vector2::operator -() const {
	ego::Vector2 result(*this);
	result *= -1.0f;
	return result;
}

float ego::Vector2::dot(const Vector2& v1, const Vector2& v2) {
	return v1.x*v2.x + v1.y*v2.y;
}

float ego::Vector2::lengthSquared() const {
	return x*x + y*y;
}

float ego::Vector2::length() const {
	return sqrtf(lengthSquared());
}

void ego::Vector2::normalize() {
	float s = 1.0f / length();
	*this *= s;
}

ego::Vector2 ego::Vector2::normalized() const {
	ego::Vector2 result(*this);
	result.normalize();
	return result;
}

float* ego::Vector2::getPtr() {
	return &x;
}

const float* ego::Vector2::getConstPtr() const {
	return &x;
}

// --- RectF ---

ego::RectF::RectF()
	: left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {
}	

ego::RectF::RectF(const RectF& r)
	: left(r.left), top(r.top), right(r.right), bottom(r.bottom) {
}	

ego::RectF::RectF(float l, float t, float r, float b) 
	: left(l), top(t), right(r), bottom(b) {
}	

ego::RectF& ego::RectF::operator =(const RectF& r) {
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
	return *this;
}

float ego::RectF::width() {
	return right - left;
}

float ego::RectF::height() {
	return bottom - top;
}

bool ego::RectF::collidePoint(const ego::Vector2& point) {
	c::RectF c_rectf = c::rectf(left, top, right, bottom);
	c::Vector2 c_vector2 = c::vec2(point.x, point.y);
	return c::rectf_contains_point(&c_rectf, &c_vector2);
}

bool ego::RectF::collideCircle(const ego::Vector2& center, float radius) {
	c::RectF c_rectf = c::rectf(left, top, right, bottom);
	c::Vector2 c_vector2 = c::vec2(center.x, center.y);
	return rectf_circle_collision(&c_rectf, &c_vector2, radius);
}

// --- Color ---

ego::Color::Color() 
	: value(0) {
}

ego::Color::Color(const ego::Color& c)
	: value(c.value) {
}	

ego::Color::Color(float r, float g, float b, float a) {
	int br = int(c::clamp(0.0f, 1.0f, r) * 255.0f);
	int bg = int(c::clamp(0.0f, 1.0f, g) * 255.0f);
	int bb = int(c::clamp(0.0f, 1.0f, b) * 255.0f);
	int ba = int(c::clamp(0.0f, 1.0f, a) * 255.0f);
	value = COLOR_RGBA(br, bg, bb, ba);
}

ego::Color ego::Color::hsv(float h, float s, float v, float a) {
	c::ColorHSV chsv = {h, s, v, a};
	ego::Color result;
	result.value = c::hsv_to_rgb(chsv);
	return result;
}

void ego::Color::toRgba(float& r, float& g, float& b, float& a) {
	int br, bg, bb, ba;
	COLOR_DECONSTRUCT(value, br, bg, bb, ba);
	r = float(br) / 255.0f;
	g = float(bg) / 255.0f;
	b = float(bb) / 255.0f;
	a = float(ba) / 255.0f;
}

void ego::Color::toHsva(float& h, float& s, float& v, float& a) {
	c::ColorHSV chsv = c::rgb_to_hsv(value);
	h = chsv.h;
	s = chsv.s;
	v = chsv.v;
	a = chsv.a;
}

// --- Texture ---

ego::Texture::~Texture() {
	c::tex_free((c::TexHandle)handle);
}

ego::uint ego::Texture::width() {
	return mwidth;
}

ego::uint ego::Texture::height() {
	return mheight;
}

ego::Texture::Texture()
	: handle(0), mwidth(0), mheight(0) {
}

// --- Font ---

ego::Font::~Font() {
	c::font_free((c::FontHandle)handle);
}

float ego::Font::width(const ego::string& text) {
	return c::font_width((c::FontHandle)handle, text.c_str());
}

float ego::Font::height() {
	return c::font_height((c::FontHandle)handle);
}

ego::RectF ego::Font::bbox(const ego::string& text, const ego::Vector2& center,
	float scale) {
	c::Vector2 c_vec2 = c::vec2(center.x, center.y);
	c::RectF c_rectf = c::font_rect_ex((c::FontHandle)handle, 
		text.c_str(), &c_vec2, scale);
	return ego::RectF(c_rectf.left, c_rectf.top, c_rectf.right,
		c_rectf.bottom);
}	

void ego::Font::draw(const ego::string& text, ego::uint layer,
	const ego::Vector2& topleft, ego::Color tint, float scale) {
	c::Vector2 c_vec2 = c::vec2(topleft.x, topleft.y);

	c_vec2.x += width(text) * scale / 2.0f;
	c_vec2.y += height() * scale / 2.0f;

	c::font_draw_ex((c::FontHandle)handle, text.c_str(), layer,
		&c_vec2, scale, (c::Color)tint.value);
}	

ego::Font::Font() 
	: handle(0) {
}

// --- GUI ---

ego::GUI::~GUI() {
	c::gui_close();
}	

ego::GUI* ego::GUI::init(const ego::string& assets_prefix) {
	c::GuiDesc gui_desc = c::gui_default_style(assets_prefix.c_str());
	c::gui_init(&gui_desc);

	return new GUI();
}

void ego::GUI::label(const ego::Vector2& pos, const ego::string& text) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	c::gui_label(&v, text.c_str());
}

bool ego::GUI::button(const ego::Vector2& pos, const ego::string& text) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	return c::gui_button(&v, text.c_str());
}

bool ego::GUI::_switch(const ego::Vector2& pos, const ego::string& text) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	return c::gui_switch(&v, text.c_str());
}

float ego::GUI::slider(const ego::Vector2& pos) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	return c::gui_slider(&v);
}

bool ego::GUI::getSwitchState(const ego::Vector2& pos) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	return c::gui_getstate_switch(&v);
}

float ego::GUI::getSliderState(const ego::Vector2& pos) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	return c::gui_getstate_slider(&v);
}	

void ego::GUI::setSwitchState(const ego::Vector2& pos, bool state) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	c::gui_setstate_switch(&v, state);
}

void ego::GUI::setSliderState(const ego::Vector2& pos, float state) {
	c::Vector2 v = c::vec2(pos.x, pos.y);
	c::gui_setstate_slider(&v, state);
}

ego::GUI::GUI() {
}

// --- Video ---

ego::Video::~Video() {
	c::video_close();
}

ego::Video* ego::Video::init(ego::uint width, ego::uint height,
	const ego::string& title, ego::uint v_width, 
	ego::uint v_height, bool fullscreen){

	c::video_init_ex(width, height, v_width, v_height, 
		title.c_str(), fullscreen);

	return new Video();	
}	

void ego::Video::present() {
	c::video_present();
}

ego::uint ego::Video::getFrame() {
	return c::video_get_frame();
}

ego::Texture* ego::Video::loadTexture(const ego::string& filename) {
	ego::Texture* result = new ego::Texture();
	result->handle = c::tex_load(filename.c_str());
	return result;
}

ego::Font* ego::Video::loadFont(const ego::string& filename, float scale) {
	ego::Font* result = new ego::Font();
	result->handle = c::font_load_ex(filename.c_str(), scale);
	return result;
}

void ego::Video::drawRect(ego::Texture* tex, ego::uint layer, 
	const ego::Vector2& dest) {
	c::RectF d = c::rectf(dest.x, dest.y, 0.0f, 0.0f);
	c::TexHandle t = tex ? (c::TexHandle)tex->handle : 0;
	c::video_draw_rect(t, layer, NULL, &d, COLOR_WHITE);
}

void ego::Video::drawRect(ego::Texture* tex, ego::uint layer,
	const ego::RectF& source, const ego::Vector2& dest,
	float rotation, Color tint) {
	c::RectF d = c::rectf(dest.x, dest.y, 0.0f, 0.0f);
	c::RectF s = c::rectf(source.left, source.top, source.right, source.bottom);
	c::TexHandle t = tex ? (c::TexHandle)tex->handle : 0;
	c::video_draw_rect_rotated(t, layer, &s, &d, rotation, tint.value);
}	

void ego::Video::drawRect(ego::Texture* tex, ego::uint layer,
	const ego::RectF& source, const ego::RectF& dest,
	float rotation, ego::Color tint) {
	c::RectF d = c::rectf(dest.left, dest.top, dest.right, dest.bottom);
	c::RectF s = c::rectf(source.left, source.top, source.right, source.bottom);
	c::TexHandle t = tex ? (c::TexHandle)tex->handle : 0; 
	c::video_draw_rect_rotated(t, layer, &s, &d, rotation, tint.value);
}

void ego::Video::drawLine(ego::uint layer, const ego::Vector2& start,
	const ego::Vector2& end, ego::Color color) {
	c::Vector2 s = c::vec2(start.x, start.y);
	c::Vector2 e = c::vec2(end.x, end.y);
	c::video_draw_line(layer, &s, &e, color.value);
}	

void ego::Video::drawCenteredRect(ego::Texture* tex, ego::uint layer,
	const ego::RectF& source, const ego::Vector2& dest, float rotation,
	float scale, ego::Color tint) {
	c::RectF s = c::rectf(source.left, source.top, 
		source.right, source.bottom);
	c::Vector2 d = c::vec2(dest.x, dest.y);
	c::TexHandle t = tex ? (c::TexHandle)tex->handle : 0;
	c::gfx_draw_textured_rect(t, layer, &s, &d, rotation, scale, tint.value);
}	

ego::Video::Video() {
}

// --- Sound ---

ego::Sound::~Sound() {
	c::sound_free((c::SoundHandle)handle);
}

void ego::Sound::play() {
	c::sound_play((c::SoundHandle)handle);
}

void ego::Sound::stop() {
	c::sound_stop((c::SoundHandle)handle);
}

float ego::Sound::getVolume() {
	return c::sound_get_volume((c::SoundHandle)handle);
}

void ego::Sound::setVolume(float value) {
	c::sound_set_volume((c::SoundHandle)handle, value);
}

ego::Sound::Sound() {
}

// --- Audio ---

ego::Audio::~Audio() {
	c::sound_close();
}

ego::Audio* ego::Audio::init() {
	c::sound_init();
	return new ego::Audio();
}

void ego::Audio::update() {
	c::sound_update();
}

ego::Sound* ego::Audio::loadStream(const ego::string& filename) {
	ego::Sound* result = new ego::Sound();
	result->handle = c::sound_load_stream(filename.c_str());
	return result;
}

ego::Sound* ego::Audio::loadSample(const ego::string& filename) {
	ego::Sound* result = new ego::Sound();
	result->handle = c::sound_load_sample(filename.c_str());
	return result;
}

ego::Audio::Audio() {
}

// --- Input ---

ego::Input::~Input() {
}

ego::Input* ego::Input::init() {
	return new ego::Input();
}

bool ego::Input::process() {
	return c::system_update();
}

bool ego::Input::keyPressed(ego::Input::Key key) {
	return c::key_pressed((c::Key)key);
}

bool ego::Input::keyDown(ego::Input::Key key) {
	return c::key_down((c::Key)key);
}

bool ego::Input::keyUp(ego::Input::Key key) {
	return c::key_up((c::Key)key);
}

bool ego::Input::mousePressed(ego::Input::MouseButton button) {
	return c::mouse_pressed((c::MouseButton)button);
}

bool ego::Input::mouseDown(ego::Input::MouseButton button) {
	return c::mouse_down((c::MouseButton)button);
}

bool ego::Input::mouseUp(ego::Input::MouseButton button) {
	return c::mouse_up((c::MouseButton)button);
}

void ego::Input::mousePos(int& x, int& y) {
	uint mx, my;
	c::mouse_pos(&mx, &my);
	x = mx;
	y = my;
}

ego::Vector2 ego::Input::mousePos() {
	uint mx, my;
	c::mouse_pos(&mx, &my);
	return ego::Vector2((float)mx, (float)my);
}

ego::Input::Input() {
}

// --- Time ---

float ego::Time::ms() {
	return c::time_ms();
}

float ego::Time::delta() {
	return c::time_delta();
}

ego::uint ego::Time::fps() {
	return c::time_fps();
}

