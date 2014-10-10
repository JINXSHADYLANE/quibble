#import "DGreedAppDelegate.h"
#import "NSFileManager+DirectoryLocations.h"

#include "utils.h"
#include "memory.h"
#include "system.h"

#ifdef USE_FLURRY
#import "flurry/Flurry.h"
extern const char* flurryKey;
#endif

extern void dgreed_preinit(void);
extern bool dgreed_init(int argc, const char** argv);
extern void dgreed_close(void);
extern bool dgreed_render(void);
extern void dgreed_open_url(const char* url);
extern uint time_ms_current(void);
extern void keyval_app_suspend(void);
extern void gamecenter_app_suspend(void);
extern void malka_states_app_suspend(void);
extern void _set_gamecenter_app_delegate(DGreedAppDelegate* _app_delegate);
extern void _set_iap_app_delegate(DGreedAppDelegate* _app_delegate);
extern void _set_camera_app_delegate(DGreedAppDelegate* _app_delegate);
extern void _iap_received_products_response(SKProductsResponse* response);
extern void _iap_updated_transaction(SKPaymentQueue* queue, SKPaymentTransaction* transaction);
extern void _camera_did_cancel(UIImagePickerController* picker); 
extern void _camera_did_finish_picking(UIImagePickerController* picker, NSDictionary* info);
extern void _ios_mail_did_finish(MFMailComposeViewController* controller, MFMailComposeResult result);
extern void _ios_mail_set_delegate(DGreedAppDelegate* delegate);
extern void _loc_write_base(void);
extern double malka_call(const char* func_name, const char* fmt, ...);
extern void malka_full_gc(void);

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
@synthesize is_ipad;

#pragma mark -
#pragma mark Application lifecycle

#ifdef USE_FLURRY
void report_error(const char* msg) {
    NSString* err = [NSString stringWithUTF8String:msg];
    [Flurry logError:@"dgreed" message:err error:nil];
}

void report_event(const char* key, ...) {
    NSString* ns_key = [NSString stringWithUTF8String:key];
    NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];

    va_list v;
	va_start(v, key);
	const char* name;
    const char* val;
    do {
        name = va_arg(v, const char*);
        val = va_arg(v, const char*);
        if(name && val) {
            NSString* ns_name = [NSString stringWithUTF8String:name];
            NSString* ns_val = [NSString stringWithUTF8String:val];
            [dict setValue:ns_val forKey:ns_name];
        }
        else {
            break;
        }
    } while(1);
	va_end(v);
    
    if([dict count] > 0)
        [Flurry logEvent:ns_key withParameters:dict];
    else
        [Flurry logEvent:ns_key];
    
    [dict release];
}
#else
void report_error(const char* msg) {
}
void report_event(const char* key, ...) {
}
#endif

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
#ifdef USE_FLURRY
    NSString* ns_key = [NSString stringWithUTF8String:flurryKey];
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setSecureTransportEnabled:NO];
    [Flurry setSessionReportsOnPauseEnabled:YES];
    [Flurry setSessionReportsOnCloseEnabled:NO];
    [Flurry startSession:ns_key];
#endif
    
    CGRect screen_bounds = [[UIScreen mainScreen] bounds];
	
	window = [[UIWindow alloc] initWithFrame:screen_bounds];
    gl_controller = [[GLESViewController alloc] init];
    controller = [[AutoRotateViewController alloc] init];
    
    // Use different autorotate policies for iOS6 and pre-iOS6
    NSString *reqSysVer = @"6.0";
    NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending) {
        [window setRootViewController:controller];
    }
    else {
         [window addSubview:controller.view];
    }
    
    [window addSubview:gl_controller.view];
    [window makeKeyAndVisible];
    
    gl_view = [gl_controller.gl_view retain];
	
    if([application respondsToSelector:@selector(setStatusBarHidden:withAnimation:)]) {
        [application setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
    }
    else {
        [application setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
    }
    
    /* Determine if we're on iPad */
    self.is_ipad = false;
    if ([[UIDevice currentDevice] respondsToSelector: @selector(userInterfaceIdiom)])
            self.is_ipad = ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad);
	
	/* Get home directory for utils.c */
	NSString* home = NSHomeDirectory();
	g_home_dir = strclone([home UTF8String]);
	
	/* Get application support directory */
	NSFileManager* fileManager = [[NSFileManager alloc] init];
	NSString* storage = [fileManager applicationSupportDirectory];
	g_storage_dir = strclone([storage UTF8String]);
	[fileManager release];
    
    _set_gamecenter_app_delegate(self);
    _set_iap_app_delegate(self);
    _set_camera_app_delegate(self);
    _ios_mail_set_delegate(self);
    
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
    
    // Prerender first frame to cache everything in
    [gl_controller.gl_view setRunning:true];
    dgreed_render();
    [gl_controller.gl_view setRunning:false];
    
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
	LOG_INFO("iOS: applicationWillResignActive");
	[gl_view stopAnimation];
    [[GLESView singleton] setRunning:NO];
    
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
    
    gamecenter_app_suspend();
    malka_states_app_suspend();
    keyval_app_suspend();
    _loc_write_base();
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
    [[GLESView singleton] setRunning:YES];
        
    // Update time counters
    if(did_resign_active) {
        did_resign_active = false;
        inactive_time += time_ms_current() - resign_active_t;
    }
}


- (void)applicationWillTerminate:(UIApplication *)application {
	LOG_INFO("iOS: applicationWillTerminte");
    gamecenter_app_suspend();
    malka_states_app_suspend();
    keyval_app_suspend();
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

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    _iap_received_products_response(response);
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
    for(SKPaymentTransaction* transaction in transactions) {
        _iap_updated_transaction(queue, transaction);
    }
}

- (BOOL)popoverControllerShouldDismissPopover:(UIPopoverController *)popoverController {
    _camera_did_cancel(NULL);
    [[GLESView singleton] setRunning:YES];
    return YES;
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
    _camera_did_cancel(picker);
    [[GLESView singleton] setRunning:YES];
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
    _camera_did_finish_picking(picker, info);
    [[GLESView singleton] setRunning:YES];
}

- (void)mailComposeController:(MFMailComposeViewController *)mail_controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
    _ios_mail_did_finish(mail_controller, result);
}

@end
