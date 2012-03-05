#import "DGreedAppDelegate.h"
#import "NSFileManager+DirectoryLocations.h"

#include "utils.h"
#include "memory.h"
#include "system.h"

extern void dgreed_preinit(void);
extern bool dgreed_init(int argc, const char** argv);
extern void dgreed_close(void);
extern uint time_ms_current(void);
extern void keyval_app_suspend(void);
extern void gamecenter_app_suspend(void);
extern void _set_gamecenter_app_delegate(DGreedAppDelegate* _app_delegate);
extern float inactive_time;
extern Color clear_color;
extern float screen_widthf, screen_heightf;

extern RunStateCallback enter_background_cb;
extern RunStateCallback enter_foreground_cb;
extern ShakeCallback shake_cb;

const char* g_home_dir = NULL;
const char* g_storage_dir = NULL;

bool did_resign_active = false;
float resign_active_t;

@implementation DGreedAppDelegate

@synthesize window;
@synthesize controller;
@synthesize gl_controller;
@synthesize last_acceleration;

#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    CGRect screen_bounds = [[UIScreen mainScreen] bounds];
	
	window = [[UIWindow alloc] initWithFrame:screen_bounds];
    gl_controller = [[GLESViewController alloc] init];
    controller = [[AutoRotateViewController alloc] init];
    
    [window addSubview:controller.view];
    [window addSubview:gl_controller.view];	
    [window makeKeyAndVisible];
    
    gl_view = [gl_controller.gl_view retain];
	
    if([application respondsToSelector:@selector(setStatusBarHidden:withAnimation:)]) {
        [application setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
    }
    else {
        [application setStatusBarHidden:YES animated:false];
    }
	
	/* Get home directory for utils.c */
	NSString* home = NSHomeDirectory();
	g_home_dir = strclone([home UTF8String]);
	
	/* Get application support directory */
	NSFileManager* fileManager = [[NSFileManager alloc] init];
	NSString* storage = [fileManager applicationSupportDirectory];
	g_storage_dir = strclone([storage UTF8String]);
	[fileManager release];
    
    _set_gamecenter_app_delegate(self);
    
    [UIAccelerometer sharedAccelerometer].delegate = self;
	
	dgreed_preinit();
    
    // Set view color to clear color
    byte r, g, b, a;
    COLOR_DECONSTRUCT(clear_color, r, g, b, a);
    gl_controller.view.backgroundColor = [UIColor colorWithRed:(float)r/255.0f 
                                                         green:(float)r/255.0f
                                                          blue:(float)b/255.0f 
                                                         alpha:1.0f];

    if(!dgreed_init(0, NULL))
		return NO;
    
    if(screen_widthf > screen_heightf)
        gl_view.rotate_touches = YES;
    
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
	LOG_INFO("iOS: applicationWillResignActive");
	[gl_view stopAnimation];
    
    // Starting from now, do not count time
    did_resign_active = true;
    resign_active_t = time_ms_current();
    
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
     */
	LOG_INFO("iOS: applicationDidEnterBackground");
	[gl_view stopAnimation];
    
    if(enter_background_cb)
        (*enter_background_cb)();
    
    keyval_app_suspend();
    gamecenter_app_suspend();
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    /*
     Called as part of  transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
     */
	LOG_INFO("iOS: applicationWillEnterForeground");
	[gl_view startAnimation];
    
    if(enter_foreground_cb)
        (*enter_foreground_cb)();
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
	LOG_INFO("iOS: applicationDidBecomeActive");
	[gl_view startAnimation];
    
    // Update time counters
    if(did_resign_active) {
        did_resign_active = false;
        inactive_time += time_ms_current() - resign_active_t;
    }
}


- (void)applicationWillTerminate:(UIApplication *)application {
	LOG_INFO("iOS: applicationWillTerminte");
	dgreed_close();
	MEM_FREE(g_storage_dir);
	MEM_FREE(g_home_dir);
}


#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    /*
     Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
     */
	LOG_INFO("iOS: applicationDidReceiveMemoryWarning");
}


- (void)dealloc {
	[gl_view release];
    [gl_controller release];
    [controller release];
    [window release];
    [super dealloc];
}

- (void)leaderboardViewControllerDidFinish:(GKLeaderboardViewController *)viewController
{
    [controller dismissModalViewControllerAnimated:YES];
    [self.window bringSubviewToFront:gl_controller.view];
}

- (void)achievementViewControllerDidFinish:(GKAchievementViewController *)viewController
{
    [controller dismissModalViewControllerAnimated:YES];
    [self.window bringSubviewToFront:gl_controller.view];
}

bool isAccelerating(UIAcceleration* last, UIAcceleration* current, double threshold) {
    return fabs(last.x - current.x) > threshold 
        || fabs(last.y - current.y) > threshold
        || fabs(last.z - current.z) > threshold;
}

- (void) accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
    if(self.last_acceleration) {
        if(!is_shaking && isAccelerating(self.last_acceleration, acceleration, 2.5)) {
            is_shaking = true;
            
            if(shake_cb)
                (*shake_cb)();
        }
        if(is_shaking && !isAccelerating(self.last_acceleration, acceleration, 0.2)) {
            is_shaking = false;
        }
    }
    self.last_acceleration = acceleration;
}

@end
