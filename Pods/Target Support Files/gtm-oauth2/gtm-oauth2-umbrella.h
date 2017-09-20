#ifdef __OBJC__
#import <UIKit/UIKit.h>
#else
#ifndef FOUNDATION_EXPORT
#if defined(__cplusplus)
#define FOUNDATION_EXPORT extern "C"
#else
#define FOUNDATION_EXPORT extern
#endif
#endif
#endif

#import "GTMOAuth2Authentication.h"
#import "GTMOAuth2SignIn.h"
#import "GTMOAuth2ViewControllerTouch.h"

FOUNDATION_EXPORT double gtm_oauth2VersionNumber;
FOUNDATION_EXPORT const unsigned char gtm_oauth2VersionString[];

