#import <UIKit/UIKit.h>
#import "GLESView.h"

@interface GLESViewController : UIViewController
{
    GLESView* gl_view;
}
@property(nonatomic, retain) GLESView* gl_view;

+ (GLESViewController*) singleton;

@end
