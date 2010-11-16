#import "GLESView.h"


@implementation GLESView

static GLESView* global_gles_view = NULL;

+ (Class) layerClass {
	return [CAEAGLLayer class];
}	

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
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
		
		// Create framebuffer and renderbuffer
		GLuint framebuffer, renderbuffer;
		glGenFramebuffersOES(1, &framebuffer);
		glGenRenderbuffersOES(1, &renderbuffer);
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, renderbuffer);
		[context renderbufferStorage:GL_RENDERBUFFER_OES 
						fromDrawable:eagl_layer];
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, renderbuffer);
		glViewport(0, 0, CGRectGetWidth(frame), CGRectGetHeight(frame));
		
		// See if CADisplayLink is supported
		NSString *reqSysVer = @"3.1";
		NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
		if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
			display_link_supported = TRUE;
		
		global_gles_view = self;
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

- (void) drawView {
	system_update();
	
	bool res = dgreed_update();
	res = res && dgreed_render();
	
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


- (void)dealloc {
	if([EAGLContext currentContext] == context)
		[EAGLContext setCurrentContext:nil];
	[context release];
    [super dealloc];
}

@end
