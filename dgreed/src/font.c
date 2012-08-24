#include "font.h"

#include "memory.h"
#include "lib9-utf/utf.h"
#include "gfx_utils.h"

#pragma pack(1)
typedef struct {
	//byte id, x, y, width, height, xoffset, yoffset, xadvance;
	signed short id, x, y, width, height, xoffset, yoffset, xadvance;
} CharDesc;	
#pragma pack()

typedef struct {
	Rune codepoint;
	RectF source;
	float x_offset, y_offset;
	float x_advance;
} Char;	

typedef struct {
	TexHandle tex;
	float height;
	float y_gap;
	uint n_chars;
	Char* chars;
	bool active;
	float scale;
} FontDesc;

#define MAX_FONTS 4
FontDesc fonts[MAX_FONTS];
uint font_count = 0;

static int _cmp_char(const void *a, const void *b) {
	Char* ca = (Char*)a;
	Char* cb = (Char*)b;
	return ca->codepoint - cb->codepoint;
}

static Char* _get_char(FontDesc* desc, Rune codepoint) {
	// Binary search
	uint idx = ~0;
	uint low = 0, high = desc->n_chars;
	while(high > low) {
		uint mid = (low + high) / 2;
		if(codepoint < desc->chars[mid].codepoint)
			high = mid;
		else if(codepoint > desc->chars[mid].codepoint)
			low = mid;
		else {
			idx = mid;
			break;
		}
	}

	if(idx == ~0) 
		return NULL;

	assert(idx < desc->n_chars);
	return &desc->chars[idx];
}

FontHandle font_load(const char* filename) {
	return font_load_ex(filename, 1.0f);
}

FontHandle font_load_ex(const char* filename, float scale) {
	return font_load_exp(filename, scale, NULL);
}

FontHandle font_load_exp(const char* filename, float scale, const char* prefix) {
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
	if(id != FOURCC('F', 'O', 'N', 'T')) {
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
	char tex_path[256];
	if(prefix) {
		assert(strlen(tex_filename) + strlen(prefix) < 255);
		strcpy(tex_path, prefix);
	}
	else {
		tex_path[0] = '\0';
	}
	strcat(tex_path, tex_filename);
	fonts[result].tex = tex_load(tex_path);

	// Read number of chars
	uint16 chars = file_read_uint16(font_file);
	fonts[result].n_chars = chars;

	// Alloc space for chars
	fonts[result].chars = MEM_ALLOC((chars+1) * sizeof(Char));
	
	// Read char descs one by one
	bool found_space = false;
	bool needs_sort = false;
	CharDesc char_desc;
	Char* chr = fonts[result].chars;
	uint n = 0;
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

		if(char_desc.id == ' ')
			found_space = true;

		chr[n].codepoint = char_desc.id;
		chr[n].source = rectf(
			(float)char_desc.x, (float)char_desc.y,
			(float)(char_desc.x + char_desc.width),
			(float)(char_desc.y + char_desc.height)
		);
		chr[n].x_offset = (float)char_desc.xoffset;
		chr[n].y_offset = (float)char_desc.yoffset;
		chr[n].x_advance = (float)char_desc.xadvance;

		if(n > 0 && chr[n-1].codepoint > chr[n].codepoint)
			needs_sort = true;
		n++;
	}
	assert(n == fonts[result].n_chars);

	if(!found_space) {
		chr[n].codepoint = ' ';
		chr[n].x_advance = 0.0f;
		n++; fonts[result].n_chars++;
		needs_sort = true;
	}

	if(needs_sort)
		qsort(chr, n, sizeof(Char), _cmp_char);

	// If space glyph has 0 x_advance, use 'a' glyph x_advance
	Char* space = _get_char(&fonts[result], ' ');
	assert(space);
	if(space->x_advance == 0.0f) {
		Char* a = _get_char(&fonts[result], 'a');
		space->x_advance = a->x_advance;
	}

	file_close(font_file);
	
	fonts[result].scale = scale;

	return result;
}

void font_free(FontHandle font) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	MEM_FREE(fonts[font].chars);

	fonts[font].active = false;
	tex_free(fonts[font].tex);
	font_count--;
}	

float font_width(FontHandle font, const char* string) {
    // All invalid fonts simply return 0
	if(font >= MAX_FONTS || !fonts[font].active)
        return 0.0f;

	Rune codepoint;
	float width = 0.0f;
	uint n = strlen(string);
	uint i = 0;
	
	while(i < n) {
		i += chartorune(&codepoint, string + i);  
		if(codepoint != 0) {
			Char* c = _get_char(&fonts[font], codepoint);
			if(c)
				width += c->x_advance;
		}
	}

	return width * fonts[font].scale;
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

	Rune codepoint;
	float cursor_x = 0.0f;
	uint n = strlen(string);
	uint i = 0;
	float s = fonts[font].scale;

	while(i < n) {
		i += chartorune(&codepoint, string + i);
		if(codepoint != 0) {
			const Char* chr = _get_char(&fonts[font], codepoint);
			if(!chr)
				continue;

			RectF dest = rectf(topleft->x + cursor_x, topleft->y, 0.0f, 0.0f);
			dest.left += chr->x_offset * s;
			dest.top += chr->y_offset * s;
			dest.right = dest.left + rectf_width(&chr->source) * s;
			dest.bottom = dest.top + rectf_height(&chr->source) * s;

			if(chr->codepoint != ' ')
				video_draw_rect(fonts[font].tex, layer, &chr->source, &dest, tint);

			cursor_x += chr->x_advance * s;
		}
	}
}	

RectF font_rect_ex(FontHandle font, const char* string, 
	const Vector2* center, float scale) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	float width = font_width(font, string) * scale;
	float height = font_height(font) * scale;
	Vector2 topleft = vec2(center->x - width/2.0f, center->y - height/2.0f);

	return rectf(topleft.x, topleft.y, topleft.x + width, topleft.y + height);
}
	
void font_draw_ex(FontHandle font, const char* string, uint layer,
	const Vector2* center, float scale, Color tint) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	float s = fonts[font].scale * scale;
	float width = font_width(font, string) * scale;
	float height = font_height(font) * scale;

	Vector2 topleft = vec2(
		floorf(center->x - width/2.0f), 
		floorf(center->y - height/2.0f)
	);
	float cursor_x = 0.0f;
	Rune codepoint;
	uint n = strlen(string);
	uint i = 0;
	while(i < n) {
		i += chartorune(&codepoint, string + i);
		if(codepoint != 0) {
			const Char* chr = _get_char(&fonts[font], codepoint);
			if(!chr)
				continue;

			RectF dest = rectf(topleft.x + cursor_x, topleft.y, 0.0f, 0.0f);
			dest.left += chr->x_offset * s;
			dest.top += chr->y_offset * s;
			dest.right = dest.left + rectf_width(&chr->source) * s;
			dest.bottom = dest.top + rectf_height(&chr->source) * s;

			if(chr->codepoint != ' ')
				video_draw_rect(fonts[font].tex, layer, &chr->source, &dest, tint);

			cursor_x += chr->x_advance * s;
		}
	}	
}	

void font_draw_rot(FontHandle font, const char* string, uint layer,
	const Vector2* center, float scale, float rot, Color tint) {
	assert(font < MAX_FONTS);
	assert(fonts[font].active);

	float s = fonts[font].scale * scale;
	float width = font_width(font, string) * scale;
	float height = font_height(font) * scale;

	Vector2 topleft = vec2_add(*center, 
		vec2_rotate(vec2(-width/2.0f, -height/2.0f), rot)
	);

	Vector2 cursor = vec2(0.0f, 0.0f);

	Rune codepoint;
	uint n = strlen(string);
	uint i = 0;
	while(i < n) {
		i += chartorune(&codepoint, string + i);
		if(codepoint != 0) {
			const Char* chr = _get_char(&fonts[font], codepoint);
			if(!chr)
				continue;

			Vector2 dest = vec2_add(cursor, topleft); 
			dest = vec2_add(dest, vec2_rotate(vec2(
				(chr->x_offset + rectf_width(&chr->source)/2.0f) * s,
				(chr->y_offset + rectf_height(&chr->source)/2.0f) * s
			), rot));

			/*
			RectF dest = rectf(topleft.x + cursor_x, topleft.y, 0.0f, 0.0f);
			dest.left += chr->x_offset * s;
			dest.top += chr->y_offset * s;
			dest.right = dest.left + rectf_width(&chr->source) * s;
			dest.bottom = dest.top + rectf_height(&chr->source) * s;
			*/

			if(chr->codepoint != ' ')
				gfx_draw_textured_rect(fonts[font].tex, layer, &chr->source, 
						&dest, rot, s, tint);

			cursor = vec2_add(cursor, 
				vec2_rotate(vec2(chr->x_advance * s, 0.0f), rot)
			);
		}
	}	
}	

