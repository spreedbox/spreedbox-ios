/**
 * @copyright Copyright (c) 2017 Struktur AG
 * @author Yuriy Shevchuk
 * @author Ivan Sein <ivan@nextcloud.com>
 *
 * @license GNU GPL version 3 or any later version
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#import "SMLoginManager.h"

#import "AFNetworking.h"
#import "ChannelingManager.h"
#import "SettingsController.h"
#import "SMAppIdentityController.h"
#import "SMConnectionController.h"
#import "SpreedMeTrustedSSLStore.h"
#import "SpreedMeStrictSSLSecurityPolicy.h"


typedef void (^GetUserComboCompletionBlock)(NSDictionary *jsonResponse, NSError *error);


@interface SMLoginManager ()
{
	NSMutableArray *_completeLoginCompletionBlocks;

	BOOL _isLoggingIn;
	
	AFHTTPSessionManager *_httpRequestOpManager;
}

@end


@implementation SMLoginManager

+ (instancetype)sharedInstance
{
	static dispatch_once_t once;
    static SMLoginManager *sharedInstance;
    dispatch_once(&once, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}


- (instancetype)init
{
	self = [super init];
	if (self) {
		_httpRequestOpManager = [[AFHTTPSessionManager alloc] init];
		_httpRequestOpManager.responseSerializer = [[AFJSONResponseSerializer alloc] init];
		_httpRequestOpManager.requestSerializer = [[AFHTTPRequestSerializer alloc] init];
		// Since we use this class only in SpreedMe mode we can set 'SpreedMeStrictSSLSecurityPolicy'
		_httpRequestOpManager.securityPolicy = [SpreedMeStrictSSLSecurityPolicy defaultPolicy];
	}
	return self;
}


#pragma mark - Public methods

- (BOOL)isLoggingIn
{
	return _isLoggingIn;
}


- (void)cancelLogin
{
	
}


- (LoginManagerOperation *)getUserComboUsername:(NSString *)userName
									   password:(NSString *)password
									   clientId:(NSString *)clientId
								   clientSecret:(NSString *)clientSecret
								completionBlock:(GetUserDataCompletionBlock)block
{
	if (_httpRequestOpManager) {
		
		NSDictionary *parameters = @{@"username" : userName,
									 @"password" : password,
									 @"client_id" : clientId,
									 @"client_secret": clientSecret};
		
		NSString *server = [SMConnectionController sharedInstance].currentRESTAPIEndpoint;
		server = [server stringByAppendingFormat:@"/auth/users/login"];
		
		GetUserComboCompletionBlock copiedBlock = [block copy];
		
		NSURLSessionTask *operation = [_httpRequestOpManager POST:server
															 parameters:parameters success:^(NSURLSessionTask *operation, id responseObject) {
			if (copiedBlock) {
				copiedBlock(responseObject, nil);
			}
		} failure:^(NSURLSessionTask *operation, NSError *error) {
			NSDictionary *responseDict = nil;
			
			if ([error isKindOfClass:[NSDictionary class]]) {
				//responseDict = error;
			} else if ([error isKindOfClass:[NSString class]]) {
				responseDict = @{@"error" : error};
			}
			
			if (copiedBlock) {
				copiedBlock(responseDict, error);
			}
		}];
		
		LoginManagerOperation *loginOperation = [[LoginManagerOperation alloc] init];
		loginOperation.requestOperation = operation;
		
		return loginOperation;
		
	} else {
		spreed_me_log("Error: no _httpRequestOpManager!");
	}
	return nil;
}


- (LoginManagerOperation *)getAppTokenWithAccessToken:(NSString *)accessToken
										applicationId:(NSString *)applicationId
									  applicationName:(NSString *)appName
											 clientId:(NSString *)clientId
										 clientSecret:(NSString *)clientSecret
									  completionBlock:(GetAppTokenCompletionBlock)block
{
	if (_httpRequestOpManager) {
		
		NSDictionary *parameters = @{@"access_token" : accessToken,
									 @"application_id" : applicationId,
									 @"application_name" : appName,
									 @"client_id" : clientId,
									 @"client_secret": clientSecret};
		
		NSString *server = [SMConnectionController sharedInstance].currentRESTAPIEndpoint;
		server = [server stringByAppendingFormat:@"/auth/users/apptoken/"];
		
		GetAppTokenCompletionBlock copiedBlock = [block copy];
		
		NSURLSessionTask *operation = [_httpRequestOpManager POST:server
															 parameters:parameters success:^(NSURLSessionTask *operation, id responseObject) {
			 if (copiedBlock) {
				 copiedBlock(responseObject, nil);
			 }
		 } failure:^(NSURLSessionTask *operation, NSError *error) {
			 NSDictionary *responseDict = nil;
			 
			 if ([error isKindOfClass:[NSDictionary class]]) {
				 //responseDict = error;
			 } else if ([error isKindOfClass:[NSString class]]) {
				 responseDict = @{@"error" : error};
			 }
			 
			 if (copiedBlock) {
				 copiedBlock(responseDict, error);
			 }
		 }];
		
		LoginManagerOperation *loginOperation = [[LoginManagerOperation alloc] init];
		loginOperation.requestOperation = operation;
		
		return loginOperation;
		
	} else {
		spreed_me_log("Error: no _httpRequestOpManager!");
	}
	return nil;
}


- (LoginManagerOperation *)refreshUserComboAndSecretWithAppToken:(NSString *)apptoken
														clientId:(NSString *)clientId
													clientSecret:(NSString *)clientSecret
												 completionBlock:(GetUserDataCompletionBlock)block
{
	if (_httpRequestOpManager) {
		
		NSDictionary *parameters = @{@"application_token" : apptoken,
									 @"client_id" : clientId,
									 @"client_secret": clientSecret};
		
		NSString *server = [SMConnectionController sharedInstance].currentRESTAPIEndpoint;
		server = [server stringByAppendingFormat:@"/auth/users/apptoken/refresh"];
		
		GetUserDataCompletionBlock copiedBlock = [block copy];
		
		NSURLSessionTask *operation = [_httpRequestOpManager POST:server
															 parameters:parameters success:^(NSURLSessionTask *operation, id responseObject) {
																 if (copiedBlock) {
																	 copiedBlock(responseObject, nil);
																 }
															 } failure:^(NSURLSessionTask *operation, NSError *error) {
																 NSDictionary *responseDict = nil;
																 
																 if ([error isKindOfClass:[NSDictionary class]]) {
																	 //responseDict = operation.responseObject;
																 } else if ([error isKindOfClass:[NSString class]]) {
                                                                     responseDict = @{@"error" : error};
																 }
																 
																 if (copiedBlock) {
																	 copiedBlock(responseDict, error);
																 }
															 }];
		
		LoginManagerOperation *loginOperation = [[LoginManagerOperation alloc] init];
		loginOperation.requestOperation = operation;
		
		return loginOperation;
		
	} else {
		spreed_me_log("Error: no _httpRequestOpManager!");
	}
	return nil;
}


@end
