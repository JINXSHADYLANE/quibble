#import <UIKit/UIKit.h>
#import "GLESView.h"
#import "GLESViewController.h"
#import "AutoRotateViewController.h"

@interface DGreedAppDelegate : NSObject <UIApplicationDelegate, GKLeaderboardViewControllerDelegate, GKAchievementViewControllerDelegate> {
    UIWindow* window;
    AutoRotateViewController* controller;
    GLESViewController* gl_controller;
	GLESView* gl_view;
}

@property(nonatomic,retain) UIWindow* window;
@property(nonatomic,retain) AutoRotateViewController* controller;
@property(nonatomic,retain) GLESViewController* gl_controller;

@end

