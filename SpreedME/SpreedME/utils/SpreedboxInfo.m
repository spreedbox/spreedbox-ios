//
//  SpreedboxInfo.m
//  Nextcloud
//
//  Created by conceiva apple on 3/11/17.
//  Copyright © 2017 TWS. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SpreedboxInfo.h"
#import "Nextcloud-swift.h"

@implementation SpreedboxInfo

+ (NSString *)getServerName
{
    NCManageDatabase* dbInstance = [NCManageDatabase sharedInstance];
    tableAccount* account = [dbInstance getAccountActive];
    if (account != nil) {
        /*NSRange start = [account.url rangeOfString:@"://"];
        NSRange range = NSMakeRange(start.location + 3,
                                          account.url.length - (start.location + 3));
        return [account.url substringWithRange:range];*/
        return account.url;
    }
    return @"";
}
@end
