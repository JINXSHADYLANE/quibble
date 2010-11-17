#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

@interface GLESView : UIView {
	EAGLContext* context;
	NSTimer* animation_timer;
	BOOL running;
	BOOL display_link_supported;
	id display_link;
	CGRect view_frame;
}

- (void) startAnimation;
- (void) stopAnimation;
- (void) drawView;
- (void) present;
+ (GLESView*) singleton;

@end
