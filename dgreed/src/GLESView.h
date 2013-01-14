#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface GLESView : UIView {
	EAGLContext* context;
	NSTimer* animation_timer;
	BOOL running;
    BOOL rotate_touches;
	id display_link;
	CGRect view_frame;
}

@property(nonatomic) BOOL rotate_touches;

- (BOOL) isRunning;
- (void) setRunning:(BOOL)r;
- (void) startAnimation;
- (void) stopAnimation;
- (void) drawView;
- (void) present;
+ (GLESView*) singleton;

@end
