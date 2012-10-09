#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>

@interface AutoRotateViewController : UIViewController 

+(AutoRotateViewController*) singleton;
- (void)allow_rotation:(int)orientations;
- (bool)is_rotation_allowed:(UIInterfaceOrientation)requested;
@end
