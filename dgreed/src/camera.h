#ifndef CAMERA_H
#define CAMERA_H

// iOS Camera/Import image wrapper
// Does not work on non-iOS builds!

#ifndef TARGET_IOS
#error Camera works only on iOS
#endif

#include "utils.h"
#import <UIKit/UIKit.h>

typedef void (*CameraCallback)(Color* data, uint w, uint h, void* uiimage);

bool camera_is_available(void);
bool camera_can_import(void);
void camera_take_picture(CameraCallback cb, uint w, uint h);
void camera_import(CameraCallback cb, uint w, uint h);
void camera_popup_origin(const RectF* rect);
void camera_pop_up(void);

// Exports image to a jpg (lossy) or png (lossless) file. 
// Automatically scales and crops to get desired width/height.
// Also, pads to nearest power-of-two size.
void camera_export(void* uiimage, const char* path, bool lossless);

void camera_export_dig(void* uiimage, const char* path);

#endif
