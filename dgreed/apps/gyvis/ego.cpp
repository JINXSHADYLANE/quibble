#include "ego.hpp"

extern "C" {
		#include <utils.h>
		#include <system.h>
		#include <font.h>
		#include <gui.h>
		#include <gfx_utils.h>
		#include <memory.h>
}

// --- Log ---

void ego::Log::init(const ego::string& filename) {
	log_init(filename.c_str(), LOG_LEVEL_INFO);		
}

void ego::Log::close() {
	log_close();
}

void ego::Log::info(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_send(LOG_LEVEL_INFO, format, args);
	va_end(args);
}

void ego::Log::info(const ego::string& msg) {
	info("%s", msg.c_str());
}

void ego::Log::warning(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_send(LOG_LEVEL_WARNING, format, args);
	va_end(args);
}

void ego::Log::warning(const ego::string& msg) {
	warning("%s", msg.c_str());
}

void ego::Log::error(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log_send(LOG_LEVEL_WARNING, format, args);
	va_end(args);

	close();
	exit(0);
}

void ego::Log::error(const ego::string& msg) {
	error("%s", msg.c_str());
}

// --- IFile ---

ego::IFile::~IFile() {
	file_close((FileHandle)handle);
}

ego::uint ego::IFile::size() {
	return file_size((FileHandle)handle);
}

void ego::IFile::seek(ego::uint pos) {
	file_seek((FileHandle)handle, pos);
}

unsigned char ego::IFile::readByte() {
	return file_read_byte((FileHandle)handle);
}

unsigned short ego::IFile::readUint16() {
	return file_read_uint16((FileHandle)handle);
}

unsigned int ego::IFile::readUint32() {
	return file_read_uint32((FileHandle)handle);
}

float ego::IFile::readFloat() {
	return file_read_float((FileHandle)handle);
}

void ego::IFile::read(void* dest, ego::uint size) {
	file_read((FileHandle)handle, dest, size);
}

ego::IFile::IFile()
	: handle(0) {
}

// --- OFile ---

ego::OFile::~OFile() {
	file_close((FileHandle)handle);
}

void ego::OFile::writeByte(unsigned char data) {
	file_write_byte((FileHandle)handle, data);
}

void ego::OFile::writeUint16(unsigned short data) {
	file_write_uint16((FileHandle)handle, data);
}

void ego::OFile::writeUint32(unsigned int data) {
	file_write_uint32((FileHandle)handle, data);
}

void ego::OFile::writeFloat(float data) {
	file_write_float((FileHandle)handle, data);
}

void ego::OFile::write(void* data, uint size) {
	file_write((FileHandle)handle, data, size);
}

ego::OFile::OFile()
	: handle(0) {
}

// --- Filesystem ---

bool ego::Filesystem::exists(const ego::string& name) {
	return file_exists(name.c_str());
}

ego::IFile* ego::Filesystem::open(const ego::string& name) {
	ego::IFile* result = new ego::IFile();	
	result->handle = (size_t)file_open(name.c_str());
	return result;
}

ego::OFile* ego::Filesystem::create(const ego::string& name) {
	ego::OFile* result = new ego::OFile();
	result->handle = (size_t)file_create(name.c_str());
	return result;
}

ego::string ego::Filesystem::readText(const ego::string& name) {
	char* c_str = txtfile_read(name.c_str());
	ego::string result(c_str);
	MEM_FREE(c_str);
	return result;
}

void ego::Filesystem::writeText(const ego::string& name, 
	const ego::string& text) {
	txtfile_write(name.c_str(), text.c_str());
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
	::RectF c_rectf = rectf(left, top, right, bottom);
	::Vector2 c_vector2 = vec2(point.x, point.y);
	return rectf_contains_point(&c_rectf, &c_vector2);
}

bool ego::RectF::collideCircle(const ego::Vector2& center, float radius) {
	::RectF c_rectf = rectf(left, top, right, bottom);
	::Vector2 c_vector2 = vec2(center.x, center.y);
	return rectf_circle_collision(&c_rectf, &c_vector2, radius);
}

// --- Color ---

const ego::Color ego::Color::white(1.0f, 1.0f, 1.0f, 1.0f);
const ego::Color ego::Color::black(0.0f, 0.0f, 0.0f, 1.0f);

ego::Color::Color() 
	: value(0) {
}

ego::Color::Color(const ego::Color& c)
	: value(c.value) {
}	

ego::Color::Color(float r, float g, float b, float a) {
	int br = int(clamp(0.0f, 1.0f, r) * 255.0f);
	int bg = int(clamp(0.0f, 1.0f, g) * 255.0f);
	int bb = int(clamp(0.0f, 1.0f, b) * 255.0f);
	int ba = int(clamp(0.0f, 1.0f, a) * 255.0f);
	value = COLOR_RGBA(br, bg, bb, ba);
}

ego::Color ego::Color::hsv(float h, float s, float v, float a) {
	ColorHSV chsv = {h, s, v, a};
	ego::Color result;
	result.value = hsv_to_rgb(chsv);
	return result;
}

ego::Color ego::Color::lerp(ego::Color a, ego::Color b, float t) {
	ego::Color result;
	result.value = color_lerp(a.value, b.value, t);
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
	ColorHSV chsv = rgb_to_hsv(value);
	h = chsv.h;
	s = chsv.s;
	v = chsv.v;
	a = chsv.a;
}

// --- Texture ---

ego::Texture::~Texture() {
	tex_free((TexHandle)handle);
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
	font_free((FontHandle)handle);
}

float ego::Font::width(const ego::string& text) {
	return font_width((FontHandle)handle, text.c_str());
}

float ego::Font::height() {
	return font_height((FontHandle)handle);
}

ego::RectF ego::Font::bbox(const ego::string& text, const ego::Vector2& center,
	float scale) {
	::Vector2 c_vec2 = vec2(center.x, center.y);
	::RectF c_rectf = font_rect_ex((FontHandle)handle, 
		text.c_str(), &c_vec2, scale);
	return ego::RectF(c_rectf.left, c_rectf.top, c_rectf.right,
		c_rectf.bottom);
}	

void ego::Font::draw(const ego::string& text, ego::uint layer,
	const ego::Vector2& topleft, ego::Color tint, float scale) {
	::Vector2 c_vec2 = vec2(topleft.x, topleft.y);

	c_vec2.x += width(text) * scale / 2.0f;
	c_vec2.y += height() * scale / 2.0f;

	font_draw_ex((FontHandle)handle, text.c_str(), layer,
		&c_vec2, scale, (::Color)tint.value);
}	

ego::Font::Font() 
	: handle(0) {
}

// --- GUI ---

ego::GUI::~GUI() {
	gui_close();
}	

ego::GUI* ego::GUI::init(const ego::string& assets_prefix) {
	GuiDesc gui_desc = gui_default_style(assets_prefix.c_str());
	gui_init(&gui_desc);

	return new GUI();
}

void ego::GUI::label(const ego::Vector2& pos, const ego::string& text) {
	::Vector2 v = vec2(pos.x, pos.y);
	gui_label(&v, text.c_str());
}

bool ego::GUI::button(const ego::Vector2& pos, const ego::string& text) {
	::Vector2 v = vec2(pos.x, pos.y);
	return gui_button(&v, text.c_str());
}

bool ego::GUI::_switch(const ego::Vector2& pos, const ego::string& text) {
	::Vector2 v = vec2(pos.x, pos.y);
	return gui_switch(&v, text.c_str());
}

float ego::GUI::slider(const ego::Vector2& pos) {
	::Vector2 v = vec2(pos.x, pos.y);
	return gui_slider(&v);
}

bool ego::GUI::getSwitchState(const ego::Vector2& pos) {
	::Vector2 v = vec2(pos.x, pos.y);
	return gui_getstate_switch(&v);
}

float ego::GUI::getSliderState(const ego::Vector2& pos) {
	::Vector2 v = vec2(pos.x, pos.y);
	return gui_getstate_slider(&v);
}	

void ego::GUI::setSwitchState(const ego::Vector2& pos, bool state) {
	::Vector2 v = vec2(pos.x, pos.y);
	gui_setstate_switch(&v, state);
}

void ego::GUI::setSliderState(const ego::Vector2& pos, float state) {
	::Vector2 v = vec2(pos.x, pos.y);
	gui_setstate_slider(&v, state);
}

ego::GUI::GUI() {
}

// --- Video ---

ego::Video::~Video() {
	video_close();
}

ego::Video* ego::Video::init(ego::uint width, ego::uint height,
	const ego::string& title, ego::uint v_width, 
	ego::uint v_height, bool fullscreen){

	if(v_width == 0 && v_height == 0) {
		v_width = width;
		v_height = height;
	}	

	video_init_ex(width, height, v_width, v_height, 
		title.c_str(), fullscreen);

	return new Video();	
}	

void ego::Video::present() {
	video_present();
}

ego::uint ego::Video::getFrame() {
	return video_get_frame();
}

ego::Texture* ego::Video::loadTexture(const ego::string& filename) {
	ego::Texture* result = new ego::Texture();
	result->handle = tex_load(filename.c_str());
	uint width, height;
	tex_size((TexHandle)result->handle, &width, &height);
	result->mwidth = width;
	result->mheight = height;
	return result;
}

ego::Font* ego::Video::loadFont(const ego::string& filename, float scale) {
	ego::Font* result = new ego::Font();
	result->handle = font_load_ex(filename.c_str(), scale);
	return result;
}

void ego::Video::drawRect(ego::Texture* tex, ego::uint layer, 
	const ego::Vector2& dest) {
	::RectF d = rectf(dest.x, dest.y, 0.0f, 0.0f);
	TexHandle t = tex ? (TexHandle)tex->handle : 0;
	video_draw_rect(t, layer, NULL, &d, COLOR_WHITE);
}

void ego::Video::drawRect(ego::Texture* tex, ego::uint layer,
	const ego::RectF& source, const ego::Vector2& dest,
	float rotation, Color tint) {
	::RectF d = rectf(dest.x, dest.y, 0.0f, 0.0f);
	::RectF s = rectf(source.left, source.top, source.right, source.bottom);
	TexHandle t = tex ? (TexHandle)tex->handle : 0;
	video_draw_rect_rotated(t, layer, &s, &d, rotation, tint.value);
}	

void ego::Video::drawRect(ego::Texture* tex, ego::uint layer,
	const ego::RectF& source, const ego::RectF& dest,
	float rotation, ego::Color tint) {
	::RectF d = rectf(dest.left, dest.top, dest.right, dest.bottom);
	::RectF s = rectf(source.left, source.top, source.right, source.bottom);
	TexHandle t = tex ? (TexHandle)tex->handle : 0; 
	video_draw_rect_rotated(t, layer, &s, &d, rotation, tint.value);
}

void ego::Video::drawLine(ego::uint layer, const ego::Vector2& start,
	const ego::Vector2& end, ego::Color color) {
	::Vector2 s = vec2(start.x, start.y);
	::Vector2 e = vec2(end.x, end.y);
	video_draw_line(layer, &s, &e, color.value);
}	

void ego::Video::drawCenteredRect(ego::Texture* tex, ego::uint layer,
	const ego::RectF& source, const ego::Vector2& dest, float rotation,
	float scale, ego::Color tint) {
	::RectF s = rectf(source.left, source.top, 
		source.right, source.bottom);
	::Vector2 d = vec2(dest.x, dest.y);
	TexHandle t = tex ? (TexHandle)tex->handle : 0;
	gfx_draw_textured_rect(t, layer, &s, &d, rotation, scale, tint.value);
}	

ego::Video::Video() {
}

// --- Sound ---

ego::Sound::~Sound() {
	sound_free((SoundHandle)handle);
}

void ego::Sound::play() {
	sound_play((SoundHandle)handle);
}

void ego::Sound::stop() {
	//sound_stop((SoundHandle)handle);
}

float ego::Sound::getVolume() {
	return 1.0f;
	//return sound_get_volume((SoundHandle)handle);
}

void ego::Sound::setVolume(float value) {
	//sound_set_volume((SoundHandle)handle, value);
}

ego::Sound::Sound() {
}

// --- Audio ---

ego::Audio::~Audio() {
	sound_close();
}

ego::Audio* ego::Audio::init() {
	sound_init();
	return new ego::Audio();
}

void ego::Audio::update() {
	sound_update();
}

ego::Sound* ego::Audio::loadStream(const ego::string& filename) {
	ego::Sound* result = new ego::Sound();
	result->handle = sound_load_stream(filename.c_str());
	return result;
}

ego::Sound* ego::Audio::loadSample(const ego::string& filename) {
	ego::Sound* result = new ego::Sound();
	result->handle = sound_load_sample(filename.c_str());
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
	return system_update();
}

bool ego::Input::keyPressed(ego::Input::Key key) {
	return key_pressed((::Key)key);
}

bool ego::Input::keyDown(ego::Input::Key key) {
	return key_down((::Key)key);
}

bool ego::Input::keyUp(ego::Input::Key key) {
	return key_up((::Key)key);
}

bool ego::Input::mousePressed(ego::Input::MouseButton button) {
	return mouse_pressed((::MouseButton)button);
}

bool ego::Input::mouseDown(ego::Input::MouseButton button) {
	return mouse_down((::MouseButton)button);
}

bool ego::Input::mouseUp(ego::Input::MouseButton button) {
	return mouse_up((::MouseButton)button);
}

void ego::Input::mousePos(int& x, int& y) {
	uint mx, my;
	mouse_pos(&mx, &my);
	x = mx;
	y = my;
}

ego::Vector2 ego::Input::mousePos() {
	uint mx, my;
	mouse_pos(&mx, &my);
	return ego::Vector2((float)mx, (float)my);
}

ego::Input::Input() {
}

// --- Time ---

float ego::Time::ms() {
	return time_ms();
}

float ego::Time::delta() {
	return time_delta();
}

ego::uint ego::Time::fps() {
	return time_fps();
}

// --- Random ---

void ego::Random::init(ego::uint seed) {
	rand_init(seed);
}

ego::uint ego::Random::_uint() {
	return rand_uint();
}

int ego::Random::_int(int min, int max) {
	return rand_int(min, max);
}

float ego::Random::_float() {
	return rand_float();
}

float ego::Random::_float(float min, float max) {
	return rand_float_range(min, max);
}

