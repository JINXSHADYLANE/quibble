#import "GLESViewController.h"

@implementation GLESViewController

@synthesize gl_view = _gl_view;
@synthesize text_view = _text_view;

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
    
    // Create gl view
    self.gl_view = [[GLESView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    [self.view addSubview:self.gl_view];
    
    // Create invisible text capture view
    self.text_view = [[TextCapturerView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 10.0f, 10.0f)];
    self.text_view.hidden = YES;
    self.text_view.autocorrectionType = UITextAutocorrectionTypeNo;
    self.text_view.autocapitalizationType = UITextAutocapitalizationTypeNone;
    self.text_view.delegate = self;
    [self.view addSubview:self.text_view];
    
    global_gles_view_controller = self;
}

extern bool txtinput_return;

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    txtinput_return = true;
    return NO;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self.gl_view startAnimation];
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
