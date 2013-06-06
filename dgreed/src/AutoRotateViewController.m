#import "AutoRotateViewController.h"
#import "GLESViewController.h"
#import "camera.h"

@implementation AutoRotateViewController

extern void _orientation_set_current(UIInterfaceOrientation orient);
extern void _orientation_start_transition(UIInterfaceOrientation next, float len);

float rotation_duration;

static AutoRotateViewController* global_auto_rotate_view_controller = NULL;
static int orientations = 0;
+ (AutoRotateViewController*) singleton
{
    assert(global_auto_rotate_view_controller);
    return global_auto_rotate_view_controller;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (id)init
{
    self = [super init];
    if(self) {
        UIInterfaceOrientation current = [UIApplication sharedApplication].statusBarOrientation;
        _orientation_set_current(current);
        global_auto_rotate_view_controller = self;
        self.view.userInteractionEnabled = NO;
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    //[super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle


// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
    self.view = [[[UIView alloc] initWithFrame:[UIScreen mainScreen].bounds] autorelease];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void)viewDidAppear:(BOOL)animated
{
    [self shouldAutorotate];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    self.view = nil;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return UIInterfaceOrientationLandscapeLeft;
}

- (NSUInteger)supportedInterfaceOrientations{
    return UIInterfaceOrientationMaskLandscape;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return [self is_rotation_allowed:interfaceOrientation];
}

- (BOOL)shouldAutorotate
{
    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    UIInterfaceOrientation interface_orientation = orientation;
    if (orientation != UIDeviceOrientationPortrait && orientation != UIDeviceOrientationPortraitUpsideDown) {
        interface_orientation = [UIApplication sharedApplication].statusBarOrientation;
    }
    BOOL result = [self shouldAutorotateToInterfaceOrientation:interface_orientation];
    if(result)
        [self willRotateToInterfaceOrientation:interface_orientation duration:0.8f];
    return result;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    rotation_duration = (float) duration;
    _orientation_start_transition(toInterfaceOrientation, rotation_duration);
    if ([self is_rotation_allowed:toInterfaceOrientation]) {
        GLESViewController *glview = [GLESViewController singleton];
        [UIImageView animateWithDuration:rotation_duration delay:0.0 options: UIViewAnimationOptionOverrideInheritedDuration |
         UIViewAnimationOptionAllowUserInteraction animations:^{
             if (toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown) {
                 [glview.view setTransform:CGAffineTransformMake(-1, 0, 0, -1, 0, 0)];
             }
             if (toInterfaceOrientation == UIInterfaceOrientationPortrait) {
                 [glview.view setTransform:CGAffineTransformMake(1, 0, 0, 1, 0, 0)];
             }
             if (toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft) {
                 [glview.view setTransform:CGAffineTransformMake(-1, 0, 0, -1, 0, 0)];
             }
             if (toInterfaceOrientation == UIInterfaceOrientationLandscapeRight) {
                 [glview.view setTransform:CGAffineTransformMake(1, 0, 0, 1, 0, 0)];
             }

             //camera_pop_up();
         } completion:^(BOOL finished) {}];
    }
}
/*
- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskAll;
}
*/

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    UIInterfaceOrientation current = [UIApplication sharedApplication].statusBarOrientation;
    _orientation_set_current(current);
}

- (void)allow_rotation:(int)allowed {
    orientations = allowed;
}

- (bool)is_rotation_allowed:(UIInterfaceOrientation) requested {
    if (requested == UIInterfaceOrientationLandscapeLeft) {
        return orientations & 1;
    } else if (requested == UIInterfaceOrientationLandscapeRight) {
        return orientations & 2;
    } else if (requested == UIInterfaceOrientationPortrait) {
        return orientations & 4;
    } else if (requested == UIInterfaceOrientationPortraitUpsideDown) {
        return orientations & 8;        
    }
    return false;
}

@end
