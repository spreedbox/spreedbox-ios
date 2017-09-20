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

#import "GTMGatherInputStream.h"
#import "GTMHTTPFetcher.h"
#import "GTMHTTPFetcherLogging.h"
#import "GTMHTTPFetcherLogViewController.h"
#import "GTMHTTPFetcherService.h"
#import "GTMHTTPFetchHistory.h"
#import "GTMHTTPUploadFetcher.h"
#import "GTMMIMEDocument.h"
#import "GTMReadMonitorInputStream.h"

FOUNDATION_EXPORT double gtm_http_fetcherVersionNumber;
FOUNDATION_EXPORT const unsigned char gtm_http_fetcherVersionString[];

