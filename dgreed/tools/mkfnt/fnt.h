#include <utils.h>

// A wraper for font renderers

typedef void* FntHandle;

typedef struct {
	uint w, h, x_off, y_off;
	float x_bearing, x_advance;
} GlyphMetrics;

typedef struct {
	float ascent, descent, gap;
} FntMetrics;

// Load ttf font from file of specified size in pixels
FntHandle fnt_init(const char* filename, uint px_size);
// Release internal resources
void fnt_close(FntHandle fnt);

// Returns vertical font metrics
FntMetrics fnt_metrics(FntHandle fnt);

// Allocates large-enough 8bpp bitmap and renders glyph into it
byte* fnt_get_glyph(FntHandle fnt, int codepoint, GlyphMetrics* metrics);



