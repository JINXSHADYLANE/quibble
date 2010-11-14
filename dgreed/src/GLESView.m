#import "GLESView.h"


@implementation GLESView

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
		
		[self drawView];
    }
    return self;
}

- (void) drawView {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}


- (void)dealloc {
	if([EAGLContext currentContext] == context)
		[EAGLContext setCurrentContext:nil];
	[context release];
    [super dealloc];
}

@end
