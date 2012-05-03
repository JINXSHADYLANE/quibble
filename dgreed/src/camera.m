#include "camera.h"

#import "DGreedAppDelegate.h"

static DGreedAppDelegate* app_delegate = nil;
static UIPopoverController* popover = nil;
static CameraCallback camera_cb = NULL;

void _set_camera_app_delegate(DGreedAppDelegate* _app_delegate) {
    app_delegate = _app_delegate;
}

void _camera_did_finish_picking(UIImagePickerController* picker, NSDictionary* info) {
    UIImage* img = [info objectForKey:UIImagePickerControllerOriginalImage];
    
    // Rotate image to its proper orientation
    UIGraphicsBeginImageContext(img.size);
        
    [img drawAtPoint:CGPointMake(0, 0)];
    UIImage* photo = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
	CGImageRef cg_image = photo.CGImage;
	uint width = CGImageGetWidth(cg_image);
	uint height = CGImageGetHeight(cg_image);
    
    CGColorSpaceRef color_space = CGImageGetColorSpace(cg_image);
	CGColorSpaceModel color_model = CGColorSpaceGetModel(color_space);
	
    uint bits_per_component = CGImageGetBitsPerComponent(cg_image);
	if(color_model != kCGColorSpaceModelRGB || bits_per_component != 8)
		LOG_ERROR("Bad image color space - please use 24 or 32 bit rgb/rgba");
	CFDataRef image_data = CGDataProviderCopyData(CGImageGetDataProvider(cg_image));
	void* data = (void*)CFDataGetBytePtr(image_data);

    if(camera_cb) {
        (*camera_cb)((Color*)data, width, height, (void*)photo);
        camera_cb = NULL;
    }
    
    CFRelease(image_data);
    
    if(app_delegate.is_ipad) {
        assert(popover);
        [popover dismissPopoverAnimated:YES];
        [popover release];
    }
    else {
        [picker dismissModalViewControllerAnimated:YES];
    }

}

void _camera_did_cancel(UIImagePickerController* picker) {    
    if(camera_cb) {
        (*camera_cb)(NULL, 0, 0, NULL);
        camera_cb = NULL;
    }
    
    if(app_delegate.is_ipad) {
        assert(popover);
        [popover dismissPopoverAnimated:YES];
        [popover release];
    }
    else {
        [picker dismissModalViewControllerAnimated:YES];
    }
}

bool camera_is_available(void) {
    return [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera];
}

bool camera_can_import(void) {
    return [UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary];
}

static void _present_image_picked(CameraCallback cb, UIImagePickerControllerSourceType type) {
    UIImagePickerController* picker = [[UIImagePickerController alloc] init];
    picker.sourceType = type;
    picker.allowsEditing = NO;
    picker.delegate = app_delegate;
    
    camera_cb = cb;
    
    if(app_delegate.is_ipad) {
        popover = [[UIPopoverController alloc] initWithContentViewController:picker];
        [popover presentPopoverFromRect:CGRectMake(0.0f, 0.0f, 100.0f, 100.0f) 
                                 inView:app_delegate.gl_controller.view 
               permittedArrowDirections:UIPopoverArrowDirectionUp
                               animated:YES];
    }
    else {
        // Use deprecated method - prefered one is available only in iOS 5.0 and later
        [app_delegate.gl_controller presentModalViewController:picker animated:YES];
    }
    [picker release];
}

void camera_take_picture(CameraCallback cb) {
    assert(camera_is_available());
    _present_image_picked(cb, UIImagePickerControllerSourceTypeCamera);
}

void camera_import(CameraCallback cb) {
    assert(camera_can_import());
    _present_image_picked(cb, UIImagePickerControllerSourceTypePhotoLibrary);
}

void camera_export(void* uiimage, const char* path, uint w, uint h, bool lossless) {
    UIImage* image = (UIImage*)uiimage;
    
    // Crop image
    float aspect = (float)w / (float)h;
    float cropped_width = image.size.width;
    float cropped_height = image.size.width / aspect;
    if(cropped_height > image.size.height) {
        cropped_height = image.size.height;
        cropped_width = cropped_height * aspect;
    }
    CGImageRef cg_cropped_image = CGImageCreateWithImageInRect(image.CGImage, CGRectMake(
            floorf((image.size.width - cropped_width) / 2.0f),
            floorf((image.size.height - cropped_height) / 2.0f),
            floorf(cropped_width),
            floorf(cropped_height)
        )
    );
    UIImage* cropped_image = [[UIImage alloc] initWithCGImage:cg_cropped_image];
    CGImageRelease(cg_cropped_image);
    
    // Create empty image context and blit cropped image there, with scaling
    uint width = next_pow2(w);
    uint height = next_pow2(h);
    UIGraphicsBeginImageContext(CGSizeMake((float)width, (float)height));
    CGContextRef cg = UIGraphicsGetCurrentContext();
    CGContextSetRGBFillColor(cg, 0.0f, 0.0f, 0.0f, 0.0f);
    UIRectFill(CGRectMake(0.0f, 0.0f, (float)width, (float)height));    
    [cropped_image drawInRect:CGRectMake(0.0f, 0.0f, (float)MIN(w+1, width), (float)MIN(h+1, height))];
    UIImage* new = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    [cropped_image release];
    
    // Compress to png/jpeg
    NSData* img_data = nil;
    if(lossless)
        img_data = UIImagePNGRepresentation(new);
    else
        img_data = UIImageJPEGRepresentation(new, 0.85f);
        
    // Save to file
    FileHandle f = file_create(path);
    file_write(f, [img_data bytes], [img_data length]);
    file_close(f);
}