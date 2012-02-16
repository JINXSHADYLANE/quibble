#import "GLESView.h"


@implementation GLESView

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
		
		// Init OpenGL ES 1.1 context
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		// Set context as current
		if(!context || ![EAGLContext setCurrentContext:context]) {
			[self release];
			return nil;
		}	
        
        CGFloat screen_scale = [[UIScreen mainScreen] scale];
        //self.contentScaleFactor = screen_scale;
		
		// Create framebuffer and renderbuffer
		GLuint framebuffer, renderbuffer;
		glGenFramebuffersOES(1, &framebuffer);
		glGenRenderbuffersOES(1, &renderbuffer);
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderbuffer);
		[context renderbufferStorage:GL_RENDERBUFFER_OES 
						fromDrawable:eagl_layer];
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, renderbuffer);
        
		float w = CGRectGetWidth(frame) * screen_scale;
        float h = CGRectGetHeight(frame) * screen_scale;
        glViewport(0, 0, w, h);
		
		// See if CADisplayLink is supported
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			display_link_supported = TRUE;
		
		global_gles_view = self;
		
		[self setMultipleTouchEnabled: YES];
	}
    return self;
}

- (void) startAnimation {
	if(!running) {
		if(display_link_supported) {
			display_link = [NSClassFromString(@"CADisplayLink") 
							displayLinkWithTarget:self selector:@selector(drawView)];
			[display_link addToRunLoop:[NSRunLoop currentRunLoop] 
							   forMode:NSDefaultRunLoopMode];
		}
		else {
			animation_timer = [NSTimer 
							   scheduledTimerWithTimeInterval:(NSTimeInterval)(1.0f / 60.0f)
							   target:self selector:@selector(drawView)
							   userInfo:nil repeats:YES];
		}
		running = YES;
	}
}

- (void) stopAnimation {
	if(running) {
		if(display_link_supported) {
			[display_link invalidate];
			display_link = nil;
		}
		else {
			[animation_timer invalidate];
			animation_timer = nil;
		}
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
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
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
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView: self];
	cmouse_x = (uint)(location.y);
	cmouse_y = (uint)(CGRectGetWidth(view_frame) - location.x);
	cmouse_down = cmouse_pressed = true;
	
	for(id t in [touches allObjects]) {
		UITouch* touch = t;
		CGPoint pos = [touch locationInView: self];
		float x = pos.y;
		float y = CGRectGetWidth(view_frame) - pos.x;
		_touch_down(x, y);
	}
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView: self];
	cmouse_x = (uint)(location.y);
	cmouse_y = (uint)(CGRectGetWidth(view_frame) - location.x);
	cmouse_up = true;
	cmouse_pressed = false;
	
	for(id t in [touches allObjects]) {
		UITouch* touch = t;
		CGPoint pos = [touch previousLocationInView: self];
		float x = pos.y;
		float y = CGRectGetWidth(view_frame) - pos.x;
		_touch_up(x, y);
	}
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView: self];
	cmouse_x = (uint)(location.y);
	cmouse_y = (uint)(CGRectGetWidth(view_frame) - location.x);
	cmouse_pressed = true;
	
	for(id t in [touches allObjects]) {
		UITouch* touch = t;
		CGPoint pos = [touch previousLocationInView: self];
		float x = pos.y;
		float y = CGRectGetWidth(view_frame) - pos.x;
		
		pos = [touch locationInView: self];
		float new_x = pos.y;
		float new_y = CGRectGetWidth(view_frame) - pos.x;
		_touch_move(x, y, new_x, new_y);
	}	
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}
@end
