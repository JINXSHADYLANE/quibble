#include "system.h"

#include "memory.h"
#include "darray.h"
#include "wav.h"
#include "gfx_utils.h"

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <MessageUI/MFMailComposeViewController.h>
#import <Twitter/TWTweetComposeViewController.h>
#import <MediaPlayer/MPMusicPlayerController.h>
#import <Social/Social.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#import "DGreedAppDelegate.h"
#import "GLESView.h"
#import "GLESViewController.h"
#import "AutoRotateViewController.h"

// Main function

extern void _async_init(void);
extern void _async_close(void);

int main(int argc, char *argv[]) {

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	_async_init();
    log_init(NULL, LOG_LEVEL_INFO);

    int retVal = UIApplicationMain(argc, argv, nil, @"DGreedAppDelegate");

    log_close();
    _async_close();

    [pool release];
    return retVal;
}

/*
--------------------------
--- Device orientation ---
--------------------------
*/

static DevOrient current_orientation;
static bool orientation_transition_start = false;
static DevOrient orientation_next;
static float orientation_transition_start_t = 0.0f;
static float orientation_transition_len = 0.0f;
uint fps_count;

DevOrient _uikit_to_dev_orient(UIInterfaceOrientation orient) {
    switch(orient) {
        case UIInterfaceOrientationLandscapeLeft:
            return ORIENT_LANDSCAPE_LEFT;
        case UIInterfaceOrientationLandscapeRight:
            return ORIENT_LANDSCAPE_RIGHT;
        case UIInterfaceOrientationPortrait:
            return ORIENT_PORTRAIT;
        case UIInterfaceOrientationPortraitUpsideDown:
            return ORIENT_PORTRAIT_UPSIDE_DOWN;
    }
    assert(0 && "Bad device orientation");
    return ORIENT_LANDSCAPE_LEFT;
}

void _orientation_set_current(UIInterfaceOrientation orient) {
    current_orientation = _uikit_to_dev_orient(orient);
}

void _orientation_start_transition(UIInterfaceOrientation next, float len) {
    orientation_transition_start = true;
    orientation_next = _uikit_to_dev_orient(next);
    orientation_transition_len = len;
    orientation_transition_start_t = time_ms_current() / 1000.0f;
}


DevOrient orientation_current(void) {
    return current_orientation;
}

bool orientation_change(DevOrient* new, float* anim_start, float* anim_len) {
    if(orientation_transition_start) {
        if(new)
            *new = orientation_next;
        if(anim_start)
            *anim_start = orientation_transition_start_t;
        if(anim_len)
            *anim_len = orientation_transition_len;

        orientation_transition_start = false;
        return true;
    }
    return orientation_transition_start;
}

/*
---------------------
--- Running state ---
---------------------
*/

RunStateCallback enter_background_cb = NULL;
RunStateCallback enter_foreground_cb = NULL;

void runstate_background_cb(RunStateCallback cb) {
	enter_background_cb = cb;
}

void runstate_foreground_cb(RunStateCallback cb) {
	enter_foreground_cb = cb;
}

/*
---------------------
--- Accelerometer ---
---------------------
*/

ShakeCallback shake_cb = NULL;

void acc_shake_cb(ShakeCallback cb) {
    shake_cb = cb;
}

/*
-------------
--- Input ---
-------------
*/

bool key_pressed(Key key) {
	return false;
}

bool char_pressed(char c) {
	return false;
}

bool key_down(Key key) {
	return false;
}

bool char_down(char c) {
	return false;
}

bool key_up(Key key) {
	return false;
}

bool char_up(char c) {
	return false;
}

// Current state
uint cmouse_x, cmouse_y;
bool cmouse_pressed, cmouse_up, cmouse_down;

// Cached last frame state
static uint lmouse_x, lmouse_y;
static bool lmouse_pressed, lmouse_up, lmouse_down;

bool mouse_pressed(MouseButton button) {
	return lmouse_pressed;
}

bool mouse_down(MouseButton button) {
	return lmouse_down;
}

bool mouse_up(MouseButton button) {
	return lmouse_up;
}

void mouse_pos(uint* x, uint* y) {
	*x = lmouse_x;
	*y = lmouse_y;
}

Vector2 mouse_vec(void) {
	return vec2((float)lmouse_x, (float)lmouse_y);
}

#define max_touches 11
uint touch_count = 0;
Touch touches[max_touches];

void _touch_down(float x, float y) {
	if(touch_count < max_touches) {
		uint i = touch_count;
		touches[i].hit_time = time_ms() / 1000.0f;
		touches[i].hit_pos = vec2(x, y);
		touches[i].pos = vec2(x, y);
		touch_count++;
	}
}

void _touch_move(float old_x, float old_y, float new_x, float new_y) {
	if(!touch_count)
		return;
    uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count && i < max_touches; ++i) {
		float dx = touches[i].pos.x - old_x;
		float dy = touches[i].pos.y - old_y;
		float d = dx*dx + dy*dy;
		if(dx*dx + dy*dy < min_d) {
			min_d = d;
			min_i = i;
		}
	}
    touches[min_i].pos = vec2(new_x, new_y);
}

void _touch_up(float old_x, float old_y) {
	if(!touch_count)
		return;
	uint count = touch_count;
	float min_d = 10000.0f;
	uint min_i = 0;
	for(uint i = 0; i < count && i < max_touches; ++i) {
		float dx = touches[i].pos.x - old_x;
		float dy = touches[i].pos.y - old_y;
		float d = dx*dx + dy*dy;
		if(dx*dx + dy*dy < min_d) {
			min_d = d;
			min_i = i;
		}
	}
	touches[min_i] = touches[--touch_count];
}

uint touches_count(void) {
	return touch_count;
}

Touch* touches_get(void) {
	return touch_count ? &touches[0] : NULL;
}

/*
------------------
--- Text Input ---
------------------
*/

static bool txtinput_capturing = false;
bool txtinput_return = false;

void txtinput_start(void) {
    assert(!txtinput_capturing);
    txtinput_capturing = true;
    txtinput_clear();
    [[GLESViewController singleton].text_view becomeFirstResponder];
}

const char* txtinput_get(void) {
    assert(txtinput_capturing);
    NSString* text = [GLESViewController singleton].text_view.text;
    return [text UTF8String];
}

void txtinput_set(const char* text) {
	assert(text);
	NSString* ns_text = [NSString stringWithUTF8String:text];
	[GLESViewController singleton].text_view.text = ns_text;
}

const char* txtinput_did_end(void) {
    assert(txtinput_capturing);
    if(txtinput_return) {
        txtinput_return = false;
        return txtinput_end();
    }
    else {
        return NULL;
    }
}

const char* txtinput_end(void) {
    assert(txtinput_capturing);
    [[GLESViewController singleton].text_view resignFirstResponder];
    NSString* text = [GLESViewController singleton].text_view.text;
    txtinput_capturing = false;
    return [text UTF8String];

}

void txtinput_clear(void) {
    assert(txtinput_capturing);
    [GLESViewController singleton].text_view.text = @"";
}

/*
-----------
--- Time --
-----------
*/

static float t_acc = 0.0f, t_scale = 1.0f;
static float t_s = 0.0f, t_d = 0.0f;
static float last_frame_time = 0.0f, last_fps_update = 0.0f;
static uint fps = 0;
float inactive_time = 0.0f;

float time_s(void) {
    return t_acc / 1000.0f;
}

float time_ms(void) {
	return t_acc;
}

float time_delta(void) {
	// TODO: fix timestep here?
	//return t_d * 1000.0f;
	return (1000.0f / 60.0f) * t_scale;
}

uint time_fps(void) {
	return fps;
}

void time_scale(float s) {
	t_scale = s;
}

uint time_ms_current(void) {
	static mach_timebase_info_data_t info;
	static bool first = true;
	static uint64_t start_t;
	if(first) {
		mach_timebase_info(&info);
		start_t = mach_absolute_time();
		first = false;
	}

    uint64_t t = mach_absolute_time();

	t = t - start_t;

	t *= info.numer;
	t /= info.denom;

	return t / 1000000;
}

static float _get_t(void) {

	return (float)time_ms_current();
}

void _time_update(float current_time) {
	t_s = current_time - inactive_time;
	t_d = t_s - last_frame_time;
	t_acc += t_d * t_scale;
	if(last_fps_update + 1000.0f < t_s) {
		fps = fps_count;
		fps_count = 0;
		last_fps_update = t_s;
	}
	last_frame_time = t_s;
}

/*
------------
--- Misc ---
------------
*/

bool system_update(void) {
	_time_update(_get_t());

	lmouse_x = cmouse_x;
	lmouse_y = cmouse_y;
	lmouse_up = cmouse_up;
	lmouse_down = cmouse_down;
	lmouse_pressed = cmouse_pressed;

	cmouse_up = false;
	cmouse_down = false;

    //orientation_transition_start = false;

	return true;
}

/*
-----------
--- OS ---
-----------
*/

void ios_open_web_url(const char* url) {
    NSString* ns_url = [NSString stringWithUTF8String:url];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString: ns_url]];
}

void ios_alert(const char* title, const char* text) {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithUTF8String:title]
                                                    message:[NSString stringWithUTF8String:text]
                                                   delegate:nil
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
    [alert release];
}

bool ios_has_twitter(void) {
    NSString* reqSysVer = @"5.0";
    NSString* currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending) {
        return [TWTweetComposeViewController canSendTweet];
    }
    
    return false;
}

void ios_tweet(const char* msg, const char* url) {
    Class TW = NSClassFromString(@"TWTweetComposeViewController");
    id tw = [[TW alloc] init];
    
    [tw setInitialText:[NSString stringWithUTF8String:msg]];
    if(url)
        [tw addURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
    
    [[GLESViewController singleton] presentModalViewController:tw animated:YES];
    [tw release];
}

bool ios_has_facebook(void) {
    NSString* reqSysVer = @"6.0";
    NSString* currSysVer = [[UIDevice currentDevice] systemVersion];
    if([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending) {
        return [SLComposeViewController isAvailableForServiceType:SLServiceTypeFacebook];
    }
    return false;
}

void ios_fb_post(const char* msg, const char* url) {
    Class SL = NSClassFromString(@"SLComposeViewController");
    if([SL respondsToSelector:@selector(composeViewControllerForServiceType:)]) {
        id sl = [SL composeViewControllerForServiceType:SLServiceTypeFacebook];
        
        [sl setInitialText:[NSString stringWithUTF8String:msg]];
        if(url)
            [sl addURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
            
        [[GLESViewController singleton] presentViewController:sl animated:YES completion:nil];
    }
}

@interface MailCompose : MFMailComposeViewController {
}
@end

@implementation MailCompose

// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return (interfaceOrientation == UIInterfaceOrientationPortrait) ||
        (interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown);
}

@end

bool ios_has_mail(void) {
    return [MailCompose canSendMail];
}


typedef void (*MailCallback)(const char* result);


static DGreedAppDelegate* mail_delegate = nil;
static MailCallback mail_callback = NULL;

void _ios_mail_set_delegate(DGreedAppDelegate* delegate) {
    mail_delegate = delegate;
}

void _ios_mail_did_finish(MFMailComposeViewController* controller, MFMailComposeResult result) {
    switch (result) {
        case MFMailComposeResultCancelled:
            mail_callback("cancelled");
            break;
        case MFMailComposeResultSaved:
            mail_callback("saved");
            break;
        case MFMailComposeResultSent:
            mail_callback("sent");
            break;
        case MFMailComposeResultFailed:
        default:
            mail_callback("failed");
            break;
    }

	// Remove the mail view
    [[GLESViewController singleton] dismissModalViewControllerAnimated:YES];
}

void ios_mail(const char* subject, const char* body, MailCallback cb) {
    assert(subject && body && cb && mail_delegate);

    MFMailComposeViewController* mail_view = [[MFMailComposeViewController alloc] init];

    NSString* ns_subject = [NSString stringWithUTF8String:subject];
    [mail_view setSubject:ns_subject];
    NSString* ns_body = [NSString stringWithUTF8String:body];

    bool is_html = [ns_body rangeOfString:@"<html"].location != NSNotFound;
    [mail_view setMessageBody:ns_body isHTML:is_html];

    mail_view.mailComposeDelegate = mail_delegate;

    mail_callback = cb;

    [[GLESViewController singleton] presentModalViewController:mail_view animated:YES];
}

