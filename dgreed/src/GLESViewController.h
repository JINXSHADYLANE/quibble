#import <UIKit/UIKit.h>
#import "GLESView.h"
#import "TextCapturerView.h"

@interface GLESViewController : UIViewController <UITextFieldDelegate>

@property(nonatomic, retain) GLESView* gl_view;
@property(nonatomic, retain) TextCapturerView* text_view;

+ (GLESViewController*) singleton;

@end
