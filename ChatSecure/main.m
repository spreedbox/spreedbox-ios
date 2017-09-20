//
//  main.m
//  ChatSecure
//
//  Created by David Chiles on 11/14/14.
//  Copyright (c) 2014 Chris Ballinger. All rights reserved.
//

@import UIKit;
#import <ChatSecureCore/ChatSecureCore.h>

/*#import "../../ios/iOSClient/AppDelegate.h"

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}*/

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([OTRAppDelegate class]));
    }
}
