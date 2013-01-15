#import "GLESView.h"

#include "utils.h"

@implementation GLESView

@synthesize rotate_touches;

static GLESView* global_gles_view = NULL;

+ (Class) layerClass {
	return [CAEAGLLayer class];
}	

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
		view_frame = frame;

        // Aquire EAGL layer
		CAEAGLLayer* eagl_layer = (CAEAGLLayer*)super.layer;
		eagl_layer.opaque = YES;
        // This makes backbuffer use less memory, but you can see visual quality
        // degradation in some cases
        //eagl_layer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
        //    [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
        //    kEAGLColorFormatRGB565, kEAGLDrawablePropertyColorFormat, nil];
        
        
		// Init OpenGL ES 2.0 context
		//context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        // Init OpenGL ES 1.1 context
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        
		// Set context as current
		if(!context || ![EAGLContext setCurrentContext:context]) {
			[self release];
			return nil;
		}
        
        CGFloat screen_scale;
        if([[UIScreen mainScreen] respondsToSelector:@selector(scale)]) {
            screen_scale = [[UIScreen mainScreen] scale];
            self.contentScaleFactor = screen_scale;
        }
        
        // Create framebuffer and renderbuffer
		GLuint framebuffer, renderbuffer;
		glGenFramebuffers(1, &framebuffer);
		glGenRenderbuffers(1, &renderbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		[context renderbufferStorage:GL_RENDERBUFFER 
						fromDrawable:eagl_layer];
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
		
		global_gles_view = self;
		
		[self setMultipleTouchEnabled: YES];
	}
    return self;
}

- (BOOL) isRunning {
    return running;
}

- (void) setRunning:(BOOL)r {
    running = r;
}

- (void) startAnimation {
	if(!running) {
        display_link = [NSClassFromString(@"CADisplayLink") 
							displayLinkWithTarget:self selector:@selector(drawView)];
        [display_link addToRunLoop:[NSRunLoop currentRunLoop] 
							   forMode:NSDefaultRunLoopMode];
		running = YES;
	}
}

- (void) stopAnimation {
	if(running) {
        [display_link invalidate];
        display_link = nil;
	}
	running = NO;
}	

extern bool dgreed_update(void);
extern bool dgreed_render(void);
extern void system_update(void);

extern void async_process_schedule(void);

- (void) drawView {
	system_update();
	
	bool res = dgreed_update();
	res = res && dgreed_render();
	async_process_schedule();
    
	if(!res) {
		// Quit, somehow..
	}
}

- (void) present {
	[context presentRenderbuffer:GL_RENDERBUFFER];
}	

+ (GLESView*) singleton {
	assert(global_gles_view);
	return global_gles_view;
}

- (void) dealloc {
	if([EAGLContext currentContext] == context)
		[EAGLContext setCurrentContext:nil];
	[context release];
    [super dealloc];
}

extern uint cmouse_x, cmouse_y;
extern bool cmouse_up, cmouse_down, cmouse_pressed;

extern void _touch_down(float x, float y);
extern void _touch_move(float old_x, float old_y, float new_x, float new_y);
extern void _touch_up(float old_x, float old_y);

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    float x, y;
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView: self];
	cmouse_x = (uint)(location.y);
	cmouse_y = (uint)(CGRectGetWidth(view_frame) - location.x);
	cmouse_down = cmouse_pressed = true;
	
	for(id t in [touches allObjects]) {
		UITouch* touch = t;
		CGPoint pos = [touch locationInView: self];
        if(rotate_touches) {
            x = pos.y;
            y = CGRectGetWidth(view_frame) - pos.x;
        }
        else {
            x = pos.x;
            y = pos.y;
        }
		_touch_down(x, y);
	}
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    float x, y;
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView: self];
	cmouse_x = (uint)(location.y);
	cmouse_y = (uint)(CGRectGetWidth(view_frame) - location.x);
	cmouse_up = true;
	cmouse_pressed = false;
	
	for(id t in [touches allObjects]) {
		UITouch* touch = t;
		CGPoint pos = [touch previousLocationInView: self];
		if(rotate_touches) {
            x = pos.y;
            y = CGRectGetWidth(view_frame) - pos.x;
        }
        else {
            x = pos.x;
            y = pos.y;
        }
		_touch_up(x, y);
	}
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    float x, y, new_x, new_y;
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView: self];
	cmouse_x = (uint)(location.y);
	cmouse_y = (uint)(CGRectGetWidth(view_frame) - location.x);
	cmouse_pressed = true;
	
	for(id t in [touches allObjects]) {
		UITouch* touch = t;
		CGPoint pos = [touch previousLocationInView: self];
		if(rotate_touches) {
            x = pos.y;
            y = CGRectGetWidth(view_frame) - pos.x;
        }
        else {
            x = pos.x;
            y = pos.y;
        }
		
		pos = [touch locationInView: self];
		if(rotate_touches) {
            new_x = pos.y;
            new_y = CGRectGetWidth(view_frame) - pos.x;
        }
        else {
            new_x = pos.x;
            new_y = pos.y;
        }
		_touch_move(x, y, new_x, new_y);
	}	
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}
@end
