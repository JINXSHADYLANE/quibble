#include "camera.h"
#include "system.h"
#include "image.h"

#import "DGreedAppDelegate.h"
#import "UIImage+Resize.h"

static int width = 0;
static int height = 0;
static DGreedAppDelegate* app_delegate = nil;
static UIPopoverController* popover = nil;
static CameraCallback camera_cb = NULL;
static UIImagePickerController* picker = nil;
static bool got_image = false;
static bool taking_picture = false;
static bool importing = false;
CGRect pop_snap;

void _set_camera_app_delegate(DGreedAppDelegate* _app_delegate) {
    app_delegate = _app_delegate;
}

void _camera_did_finish_picking(UIImagePickerController* picker, NSDictionary* info) {
    got_image = true;
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    UIImage* img = [info objectForKey:UIImagePickerControllerOriginalImage];
    if(taking_picture)
        UIImageWriteToSavedPhotosAlbum(img, nil, nil, NULL);
    UIImage* photo = [img resizedImageWithContentMode:UIViewContentModeScaleAspectFill
                                               bounds:CGSizeMake(width, height)
                                 interpolationQuality:kCGInterpolationLow];
    
    if(camera_cb) {
        (*camera_cb)(NULL, width, height, (void*)photo);
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
    
    [pool release];
}

void _camera_did_cancel(UIImagePickerController* picker) {    
    if(camera_cb) {
        (*camera_cb)(NULL, 0, 0, NULL);
        camera_cb = NULL;
    }
    
    if(app_delegate.is_ipad && !got_image) {
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

static void _present_image_picker(CameraCallback cb, UIImagePickerControllerSourceType type) {
    picker = [[UIImagePickerController alloc] init];
    picker.sourceType = type;
    picker.allowsEditing = NO;
    picker.delegate = app_delegate;
    
    camera_cb = cb;
    
    if(app_delegate.is_ipad) {
        popover = [[UIPopoverController alloc] initWithContentViewController:picker];
        popover.delegate = app_delegate;
        camera_pop_up();
    }
    else {
        // Use deprecated method - prefered one is available only in iOS 5.0 and later
        [app_delegate.gl_controller presentModalViewController:picker animated:YES];
    }
    [picker release];

    [[GLESView singleton] setRunning:NO];
}

void camera_popup_origin(const RectF* rect) {
    if (app_delegate.is_ipad) {
        pop_snap = CGRectMake(rect->left, rect->top, rectf_width(rect), rectf_height(rect));
    } else {
        pop_snap = CGRectMake(0, 0, 0, 0);
    }
}

void camera_take_picture(CameraCallback cb, uint w, uint h) {
    assert(camera_is_available());
    width = w;
    height = h;
    taking_picture = true;
    importing = false;
    _present_image_picker(cb, UIImagePickerControllerSourceTypeCamera);
}

void camera_import(CameraCallback cb, uint w, uint h) {
    assert(camera_can_import());
    width = w;
    height = h;
    taking_picture = false;
    importing = true;
    _present_image_picker(cb, UIImagePickerControllerSourceTypePhotoLibrary);
}

void camera_pop_up(void) {
    UIInterfaceOrientation current = [UIApplication sharedApplication].statusBarOrientation;
    if (current == UIInterfaceOrientationPortrait || current == UIInterfaceOrientationPortraitUpsideDown) {
        [popover presentPopoverFromRect:pop_snap 
                                 inView:app_delegate.gl_controller.view 
               permittedArrowDirections:UIPopoverArrowDirectionDown
                               animated:YES];
                got_image = false;
    }
}

void camera_export(void* uiimage, const char* path, bool lossless) {
    uint start_t = time_ms_current();
    
    UIImage* image = (UIImage*)uiimage;
    
    // Compress to png/jpeg
    NSData* img_data = nil;
    if(lossless)
        img_data = UIImagePNGRepresentation(image);
    else
        img_data = UIImageJPEGRepresentation(image, 0.85f);
        
    // Save to file
    FileHandle f = file_create(path);
    file_write(f, [img_data bytes], [img_data length]);
    file_close(f);
    
    LOG_INFO("Image exported in %ums", time_ms_current() - start_t);
}

void camera_export_dig(void* uiimage, const char* path) {
    uint start_t = time_ms_current();
    
    UIImage* image = (UIImage*)uiimage;
    
    CGImageRef cg_image = image.CGImage;
	uint w = CGImageGetWidth(cg_image);
	uint h = CGImageGetHeight(cg_image);
    CGColorSpaceRef color_space = CGImageGetColorSpace(cg_image);
	CGColorSpaceModel color_model = CGColorSpaceGetModel(color_space);
	uint bits_per_component = CGImageGetBitsPerComponent(cg_image);
	if(color_model != kCGColorSpaceModelRGB || bits_per_component != 8)
		LOG_ERROR("Bad image color space - please use 24 or 32 bit rgb/rgba");
	CFDataRef image_data = CGDataProviderCopyData(CGImageGetDataProvider(cg_image));
    size_t bytes_per_row = CGImageGetBytesPerRow(cg_image);
	void* data = (void*)CFDataGetBytePtr(image_data);
    
    Color* src = data;
    uint dig_w = next_pow2(w);
    uint dig_h = next_pow2(h);
    uint16* img16bit = malloc(dig_w*dig_h*2);
    for(uint y = 0; y < dig_h; ++y) {
        for(uint x = 0; x < dig_w; ++x) {
            uint dst_i = y * dig_w + x;
            if(x < w && y < h) {
                uint src_i = (y * bytes_per_row)/sizeof(Color) + x;
                byte r, g, b, a __attribute__ ((unused));
                COLOR_DECONSTRUCT(src[src_i], r, g, b, a);
                img16bit[dst_i] = RGB565_ENCODE8(r, g, b);
            }
            else {
                img16bit[dst_i] = 0xFFFF;
            }
        }
    }

    CFRelease(image_data);
    
    image_write_dig_quick(path, dig_w, dig_h, PF_RGB565, img16bit);
    free(img16bit);
    
    LOG_INFO("dig image exported in %ums", time_ms_current() - start_t);
}

