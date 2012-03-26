#import "GLESViewController.h"

@implementation GLESViewController

@synthesize gl_view;

static GLESViewController* global_gles_view_controller = NULL;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        
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


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    gl_view = [[GLESView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    [self.view addSubview:gl_view];
    
    global_gles_view_controller = self;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [gl_view startAnimation];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

+ (GLESViewController*) singleton {
	assert(global_gles_view_controller);
	return global_gles_view_controller;
}

@end
