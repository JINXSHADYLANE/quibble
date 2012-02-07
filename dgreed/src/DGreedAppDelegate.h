#import <UIKit/UIKit.h>
#import "GLESView.h"
#import "GLESViewController.h"
#import "AutoRotateViewController.h"

@interface DGreedAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow* window;
    AutoRotateViewController* controller;
    GLESViewController* gl_controller;
	GLESView* gl_view;
}

@property(nonatomic,readonly) AutoRotateViewController* controller;

@end

