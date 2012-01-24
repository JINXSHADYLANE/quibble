#import <UIKit/UIKit.h>
#import "GLESView.h"
#import "GLESViewController.h"

@interface DGreedAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow* window;
    GLESViewController* controller;
	GLESView* gl_view;
}

@end

