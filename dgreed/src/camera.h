#ifndef CAMERA_H
#define CAMERA_H

// iOS Camera/Import image wrapper
// Does not work on non-iOS builds!

#ifndef TARGET_IOS
#error Camera works only on iOS
#endif

#include "utils.h"

typedef void (*CameraCallback)(Color* data, uint w, uint h, void* uiimage);

bool camera_is_available(void);
bool camera_can_import(void);
void camera_take_picture(CameraCallback cb);
void camera_import(CameraCallback cb);

// Exports image to a jpg (lossy) or png (lossless) file. 
// Automatically scales and crops to get desired width/height.
// Also, pads to nearest power-of-two size.
void camera_export(void* uiimage, const char* path, uint w, uint h, bool lossless);

#endif
