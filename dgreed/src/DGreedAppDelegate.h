#import <UIKit/UIKit.h>
#import <StoreKit/StoreKit.h>
#import "GLESView.h"
#import "GLESViewController.h"
#import "AutoRotateViewController.h"

@interface DGreedAppDelegate : NSObject <
    UIApplicationDelegate, 
    GKLeaderboardViewControllerDelegate, 
    GKAchievementViewControllerDelegate,
    UIAccelerometerDelegate,
    SKProductsRequestDelegate,
    SKPaymentTransactionObserver
    > {
    
    UIWindow* window;
    AutoRotateViewController* controller;
    GLESViewController* gl_controller;
	GLESView* gl_view;
    UIAcceleration* last_acceleration;
    BOOL is_shaking;
}

@property(nonatomic,retain) UIWindow* window;
@property(nonatomic,retain) AutoRotateViewController* controller;
@property(nonatomic,retain) GLESViewController* gl_controller;
@property(nonatomic,retain) UIAcceleration* last_acceleration;

@end

