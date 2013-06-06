#include "vfont.h"
#include "darray.h"
#include "memory.h"

#import <QuartzCore/QuartzCore.h>

#ifdef TARGET_IOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

DArray fonts;
static DArray render_scratchpad;

extern float resolution_factor;

uint vfont_selected_font = 0;

void _vfont_init(void) {
	fonts = darray_create(sizeof(Font), 0);
	render_scratchpad = darray_create(1, 1024);
}

void _vfont_close(void) {
	darray_free(&render_scratchpad);

	// Free fonts
	for(uint i = 0; i < fonts.size; ++i) {
		Font* font = darray_get(&fonts, i);
		[(id)font->uifont release];
		MEM_FREE(font->name);
	}
	darray_free(&fonts);
}

RectF _vfont_bbox(const char* string) {
    Font* font = darray_get(&fonts, vfont_selected_font);
    NSString* ns_string = [NSString stringWithUTF8String:string];

#ifdef TARGET_IOS
    CGSize size = [ns_string sizeWithFont:(id)font->uifont];
    // Add additional padding since iOS often returns too small rect
    size.width += 4.0f;
    size.height += 2.0f;
#else
	NSDictionary* attr = [NSDictionary dictionaryWithObject:(id)font->uifont forKey: NSFontAttributeName];
	NSSize size = [ns_string sizeWithAttributes:attr];
#endif

    return rectf(0.0f, 0.0f,
		size.width / resolution_factor,
		size.height / resolution_factor
	);
}

void _vfont_render_text(const char* string, CachePage* page, RectF* dest) {
    uint x = (uint)ceilf(dest->left);
    uint y = (uint)ceilf(dest->top);
    uint w = (uint)ceilf(rectf_width(dest));
    uint h = (uint)ceilf(rectf_height(dest));
    
	uint s = w * h * sizeof(Color);
	darray_reserve(&render_scratchpad, s);
    Color* data = render_scratchpad.data;
    memset(data, 0, s);
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        data, w, h, 8, w*4,
        color_space, kCGImageAlphaPremultipliedLast
    );

#ifdef TARGET_IOS
    UIGraphicsPushContext(ctx);
#else
	NSGraphicsContext *old_nsctx = [NSGraphicsContext currentContext];
	NSGraphicsContext *nsctx = [NSGraphicsContext graphicsContextWithGraphicsPort:ctx flipped:NO];
	[NSGraphicsContext setCurrentContext:nsctx];
#endif
    
    CGContextSaveGState(ctx); 

    CGContextTranslateCTM(ctx, 0, h);
    CGContextScaleCTM(ctx, resolution_factor, -resolution_factor);
        
    CGFloat c_rgba[] = {1.0f, 1.0f, 1.0f, 1.0f};
    CGColorRef c = CGColorCreate(color_space, c_rgba);
    CGContextSetFillColorWithColor(ctx, c);
    CGColorRelease(c);
    
    Font* font = darray_get(&fonts, vfont_selected_font);
    NSString* ns_str = [NSString stringWithUTF8String:string];

#ifdef TARGET_IOS
    [ns_str drawAtPoint:CGPointMake(2.0f, 1.0f) withFont:font->uifont];
//    assert(s.width <= w);
//    assert(s.height <= h);
#else
	NSPoint pt = {0.0f, 0.0f};
	NSDictionary* attr = [NSDictionary dictionaryWithObjectsAndKeys:
		font->uifont, NSFontAttributeName,
		[NSColor whiteColor], NSForegroundColorAttributeName,
		nil];
	[ns_str drawAtPoint:pt withAttributes:attr];
#endif
    
    /*
    // Premultiply alpha
    for(uint i = 0; i < w * h; ++i) {
        byte r, g, b, a;
        COLOR_DECONSTRUCT(data[i], r, g, b, a);
        r = (r * a) >> 8;
        g = (g * a) >> 8;
        b = (b * a) >> 8;
        data[i] = COLOR_RGBA(r, g, b, a);
    }
    */
    
    /*
    // Draw a border to see where overdraw happens
    for(uint y = 0; y < h; ++y) {
        for(uint x = 0; x < w; ++x) {
            if(y == 0 || y == (h-1) || x == 0 || x == (w-1))
                data[y * w + x] = COLOR_RGBA(0, 0, 0, 255);
        }
    }
    */
    
    tex_blit(page->tex, CGBitmapContextGetData(ctx), x, y, w, h);
    CGContextRestoreGState(ctx);

#ifdef TARGET_IOS
    UIGraphicsPopContext();
#else
	[NSGraphicsContext setCurrentContext:old_nsctx];
#endif

    CGContextRelease(ctx);
    CGColorSpaceRelease(color_space);
}

void vfont_select(const char* font_name, float size) {
    assert(font_name && size);
    
    // Look for existing font
    Font* f = DARRAY_DATA_PTR(fonts, Font);
    for(uint i = 0; i < fonts.size; ++i) {
        if(size == f[i].size && strcmp(font_name, f[i].name) == 0) {
            vfont_selected_font = i;
            return;
        }
    }
    
    NSString* ns_font_name = [NSString stringWithUTF8String:font_name];
    
    // Make new font
    Font new = {
        .name = strclone(font_name),
        .size = size,
#ifdef TARGET_IOS
        .uifont = [UIFont fontWithName:ns_font_name size:size]
#else
		.uifont = [NSFont fontWithName:ns_font_name size:size]
#endif
    };
    
    if(new.uifont == nil)
        LOG_ERROR("Unable to select font %s", font_name);
        
    darray_append(&fonts, &new);
    vfont_selected_font = fonts.size - 1;
}

