#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <stdlib.h>

#import "SDL_uikitappdelegate.h"

@interface RedAlertIOSDelegate : SDLUIKitDelegate
@end

@interface SDLUIKitDelegate (RedAlertPrivate)
- (void)postFinishLaunch;
@end

static NSString *RedAlertGetAppDelegateClassName(id self, SEL selector)
{
	return @"RedAlertIOSDelegate";
}

static void RedAlertOrientationDebug(char const *message)
{
	if (getenv("RA_IOS_ORIENTATION_DEBUG")) {
		fprintf(stderr, "RA_IOS_ORIENTATION: %s\n", message);
		NSLog(@"RA_IOS_ORIENTATION: %s", message);
	}
}

/*
 * Apps built with the iOS 27 SDK must use the UIScene life cycle or UIKit
 * refuses to launch them. SDL2 creates its windows with initWithFrame: and
 * never gives them a scene, so attach each window SDL shows to the app's
 * window scene. Info.plist.in declares RedAlertSceneDelegate as the scene
 * delegate.
 */
static UIWindowScene *RedAlertWindowScene = nil;
static NSMutableArray *RedAlertPendingWindows = nil;
static IMP RedAlertOriginalMakeKeyAndVisible = NULL;

static UIWindowScene *RedAlertFindWindowScene(void)
{
	if (RedAlertWindowScene) {
		return RedAlertWindowScene;
	}
	for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
		if ([scene isKindOfClass:[UIWindowScene class]]) {
			return (UIWindowScene *)scene;
		}
	}
	return nil;
}

static void RedAlertMakeKeyAndVisible(UIWindow *self, SEL selector)
{
	if (!self.windowScene) {
		UIWindowScene *scene = RedAlertFindWindowScene();
		if (scene) {
			self.windowScene = scene;
		} else {
			/* The scene has not connected yet; attach this window when it does. */
			if (!RedAlertPendingWindows) {
				RedAlertPendingWindows = [[NSMutableArray alloc] init];
			}
			[RedAlertPendingWindows addObject:self];
		}
	}
	((void (*)(id, SEL))RedAlertOriginalMakeKeyAndVisible)(self, selector);
}

@interface RedAlertSceneDelegate : UIResponder <UIWindowSceneDelegate>
@end

@implementation RedAlertSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
	if (![scene isKindOfClass:[UIWindowScene class]]) {
		return;
	}
	[RedAlertWindowScene release];
	RedAlertWindowScene = (UIWindowScene *)[scene retain];
	for (UIWindow *window in RedAlertPendingWindows) {
		window.windowScene = RedAlertWindowScene;
		[window makeKeyAndVisible];
	}
	[RedAlertPendingWindows removeAllObjects];
}

- (void)sceneDidDisconnect:(UIScene *)scene
{
	if (scene == RedAlertWindowScene) {
		[RedAlertWindowScene release];
		RedAlertWindowScene = nil;
	}
}

- (void)sceneDidBecomeActive:(UIScene *)scene
{
	/* With scenes, UIKit no longer calls applicationDidBecomeActive: itself. */
	id<UIApplicationDelegate> delegate = [UIApplication sharedApplication].delegate;
	if ([delegate respondsToSelector:@selector(applicationDidBecomeActive:)]) {
		[delegate applicationDidBecomeActive:[UIApplication sharedApplication]];
	}
}

@end

@implementation RedAlertIOSDelegate

+ (void)load
{
	Class meta_class = object_getClass([SDLUIKitDelegate class]);
	Method method = class_getClassMethod([SDLUIKitDelegate class], @selector(getAppDelegateClassName));
	char const *types = method ? method_getTypeEncoding(method) : "@@:";
	class_replaceMethod(meta_class, @selector(getAppDelegateClassName), (IMP)RedAlertGetAppDelegateClassName, types);
	RedAlertOrientationDebug("installed delegate class override");

	Method make_key = class_getInstanceMethod([UIWindow class], @selector(makeKeyAndVisible));
	RedAlertOriginalMakeKeyAndVisible = method_setImplementation(make_key, (IMP)RedAlertMakeKeyAndVisible);
}

- (void)redAlert_requestLandscapeGeometry
{
	if (@available(iOS 16.0, *)) {
		UIWindowScene *window_scene = nil;
		UIWindow *window = [self window];
		if ([window respondsToSelector:@selector(windowScene)]) {
			window_scene = window.windowScene;
		}
		if (!window_scene) {
			for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
				if ([scene isKindOfClass:[UIWindowScene class]] && scene.activationState != UISceneActivationStateUnattached) {
					window_scene = (UIWindowScene *)scene;
					break;
				}
			}
		}
		if (!window_scene) {
			RedAlertOrientationDebug("no window scene for landscape request");
			return;
		}

		UIWindowSceneGeometryPreferencesIOS *preferences =
			[[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:UIInterfaceOrientationMaskLandscape];
		[window_scene requestGeometryUpdateWithPreferences:preferences errorHandler:^(NSError *error) {
			(void)error;
			RedAlertOrientationDebug("landscape geometry request failed");
		}];
		[preferences release];

		for (UIWindow *scene_window in window_scene.windows) {
			[scene_window.rootViewController setNeedsUpdateOfSupportedInterfaceOrientations];
		}
	} else {
		[UIViewController attemptRotationToDeviceOrientation];
	}
}

- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
	return UIInterfaceOrientationMaskLandscape;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	RedAlertOrientationDebug("RedAlertIOSDelegate didFinishLaunching");
	BOOL launched = [super application:application didFinishLaunchingWithOptions:launchOptions];
	[self redAlert_requestLandscapeGeometry];
	return launched;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	RedAlertOrientationDebug("RedAlertIOSDelegate didBecomeActive");
	[self redAlert_requestLandscapeGeometry];
}

- (void)postFinishLaunch
{
	RedAlertOrientationDebug("RedAlertIOSDelegate postFinishLaunch");
	[self redAlert_requestLandscapeGeometry];
	[super postFinishLaunch];
}

@end
