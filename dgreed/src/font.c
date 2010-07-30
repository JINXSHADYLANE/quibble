#include "font.h"

#pragma pack(1)
typedef struct {
	//byte id, x, y, width, height, xoffset, yoffset, xadvance;
	signed short id, x, y, width, height, xoffset, yoffset, xadvance;
} CharDesc;	
#pragma pack()

typedef struct {
	RectF source;
	float x_offset, y_offset;
	float x_advance;
} Char;	

// TODO: Find a way to store less than 256 Chars
typedef struct {
	TexHandle tex;
	float height;
	Char chars[256];
	bool active;
	float scale;
} FontDesc;

#define MAX_FONTS 4
FontDesc fonts[MAX_FONTS];
uint font_count = 0;

FontHandle font_load(const char* filename) {
	return font_load_ex(filename, 1.0f);
}

FontHandle font_load_ex(const char* filename, float scale) {
	assert(filename);
	assert(sizeof(CharDesc) == 16);

	LOG_INFO("Loading font from file %s", filename);

	uint i;

	// Mark all fonts unactive if this is first font
	if(font_count == 0) {
		for(i = 0; i < MAX_FONTS; ++i)
			fonts[i].active = false;
	}		

	// Find empty space in font pool
	FontHandle result = 0;		
	while(fonts[result].active && result < MAX_FONTS) {
		result++;
	}	
	font_count++;
	if(font_count > MAX_FONTS || result == MAX_FONTS)
		LOG_ERROR("Font pool overflow");
	fonts[result].active = true;	

	FileHandle font_file = file_open(filename);

	// Confirm file id
	uint id = file_read_uint32(font_file);
	if(id != ('F' + ('O' << 8) + ('N' << 16) + ('T' << 24))) {
		file_close(font_file);
		LOG_ERROR("Trying to load invalid font file");
	}	

	// Read font height
	fonts[result].height = (float)file_read_uint16(font_file);

	// Read texture filename
	uint16 str_len = file_read_uint16(font_file);
	assert(str_len < 64 && str_len > 0);
	char tex_filename[64];
	file_read(font_file, tex_filename, str_len);
	tex_filename[str_len] = '\0';

	// Load texture
	fonts[result].tex = tex_load(tex_filename);

	// Read number of chars
	uint16 chars = file_read_uint16(font_file);
	
	// Null previous chars
	memset(fonts[result].chars, 0, sizeof(fonts[result].chars));

	// Read char descs one by one
	CharDesc char_desc;
	for(i = 0; i < chars; ++i) {
		file_read(font_file, &char_desc, sizeof(char_desc));

		char_desc.id = endian_swap2(char_desc.id);
		char_desc.x = endian_swap2(char_desc.x);
		char_desc.y = endian_swap2(char_desc.y);
		char_desc.width = endian_swap2(char_desc.width);
		char_desc.height = endian_swap2(char_desc.height);
		char_desc.xoffset = endian_swap2(char_desc.xoffset);
		char_desc.yoffset = endian_swap2(char_desc.yoffset);
		char_desc.xadvance = endian_swap2(char_desc.xadvance);
		
		fonts[result].chars[char_desc.id].source = 
			rectf((float)char_desc.x, (float)char_desc.y,
				(float)(char_desc.x + char_desc.width),
				(float)(char_desc.y + char_desc.height));
		fonts[result].chars[char_desc.id].x_offset = (float)char_desc.xoffset;
		fonts[result].chars[char_desc.id].y_offset = (float)char_desc.yoffset;
		fonts[result].chars[char_desc.id].x_advance = 
			(float)char_desc.xadvance;
	}

	file_close(font_file);
	
	fonts[result].scale = scale;

	return result;
}

void font_free(FontHandle font) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	fonts[font].active = false;
	tex_free(fonts[font].tex);
	font_count--;
}	

float font_width(FontHandle font, const char* string) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	float width = 0.0f;
	uint i = 0;
	float s = fonts[font].scale;
	while(string[i]) {
		width += fonts[font].chars[(size_t)string[i]].x_advance;
		i++;
	}
	return width * s;
}	

float font_height(FontHandle font) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	return fonts[font].height * fonts[font].scale;
}

void font_draw(FontHandle font, const char* string, uint layer,
	const Vector2* topleft, Color tint) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	float cursor_x = 0.0f;
	uint i = 0;
	float s = fonts[font].scale;
	while(string[i]) {
		const Char* chr = &fonts[font].chars[(size_t)string[i]];

		RectF dest = rectf(topleft->x + cursor_x, topleft->y, 0.0f, 0.0f);
		dest.left += chr->x_offset * s;
		dest.top += chr->y_offset * s;
		dest.right = dest.left + rectf_width(&chr->source) * s;
		dest.bottom = dest.top + rectf_height(&chr->source) * s;

		if(string[i] != ' ')
			video_draw_rect(fonts[font].tex, layer,
				&(fonts[font].chars[(size_t)string[i]].source), &dest, tint);

		cursor_x += chr->x_advance * s;
		i++;
	}	
}	
	
void font_draw_ex(FontHandle font, const char* string, uint layer,
	const Vector2* center, float scale, Color tint) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	float s = fonts[font].scale * scale;
	float width = font_width(font, string) * scale;
	float height = font_height(font) * scale;

	Vector2 topleft = vec2(center->x - width/2.0f, center->y - height/2.0f);
	float cursor_x = 0.0f;
	uint i = 0;
	while(string[i]) {
		const Char* chr = &fonts[font].chars[(size_t)string[i]];

		RectF dest = rectf(topleft.x + cursor_x, topleft.y, 0.0f, 0.0f);
		dest.left += chr->x_offset * s;
		dest.top += chr->y_offset * s;
		dest.right = dest.left + rectf_width(&chr->source) * s;
		dest.bottom = dest.top + rectf_height(&chr->source) * s;

		if(string[i] != ' ')
			video_draw_rect(fonts[font].tex, layer,
				&(fonts[font].chars[(size_t)string[i]].source), &dest, tint);

		cursor_x += chr->x_advance * s;
		i++;
	}	
}	
	
