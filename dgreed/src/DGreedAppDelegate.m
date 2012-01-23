#import "DGreedAppDelegate.h"
#import "NSFileManager+DirectoryLocations.h"

#include "utils.h"
#include "memory.h"

extern bool dgreed_init(int argc, const char** argv);
extern void dgreed_close(void);

const char* g_home_dir = NULL;
const char* g_storage_dir = NULL;

@implementation DGreedAppDelegate

#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    CGRect screen_bounds = [[UIScreen mainScreen] bounds];
	
	window = [[UIWindow alloc] initWithFrame:screen_bounds];
	gl_view = [[GLESView alloc] initWithFrame:screen_bounds];
	if(gl_view == nil)
		return NO;
	
	[window addSubview:gl_view];
    [window makeKeyAndVisible];
	
	[application setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
	
	/* Get home directory for utils.c */
	NSString* home = NSHomeDirectory();
	g_home_dir = strclone([home UTF8String]);
	
	/* Get application support directory */
	NSFileManager* fileManager = [[NSFileManager alloc] init];
	NSString* storage = [fileManager applicationSupportDirectory];
	g_storage_dir = strclone([storage UTF8String]);
	[fileManager release];
	
	if(!dgreed_init(0, NULL))
		return NO;
    
    return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
	LOG_INFO("iOS: applicationWillResignActive");
	[gl_view stopAnimation];
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
     */
	LOG_INFO("iOS: applicationDidEnterBackground");
	[gl_view stopAnimation];
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    /*
     Called as part of  transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
     */
	LOG_INFO("iOS: applicationWillEnterForeground");
	[gl_view startAnimation];
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
	LOG_INFO("iOS: applicationDidBecomeActive");
	[gl_view startAnimation];
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
    [window release];
    [super dealloc];
}


@end
