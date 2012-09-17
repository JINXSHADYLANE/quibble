#import "AutoRotateViewController.h"

#import "GLESView.h"

@implementation AutoRotateViewController

extern void _orientation_set_current(UIInterfaceOrientation orient);
extern void _orientation_start_transition(UIInterfaceOrientation next, float len);

static AutoRotateViewController* global_auto_rotate_view_controller = NULL;

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
        
        [self.view setMultipleTouchEnabled:YES];
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle


// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
    self.view = [[UIView alloc] initWithFrame:[UIScreen mainScreen].bounds];
}

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}
*/

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotate
{
    return YES;
}

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskAll;
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return YES;
}


- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    float seconds = (float)duration;
    _orientation_start_transition(toInterfaceOrientation, seconds);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    UIInterfaceOrientation current = [UIApplication sharedApplication].statusBarOrientation;
    _orientation_set_current(current);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [[GLESView singleton] touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [[GLESView singleton] touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [[GLESView singleton] touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [[GLESView singleton] touchesCancelled:touches withEvent:event];
}

@end
