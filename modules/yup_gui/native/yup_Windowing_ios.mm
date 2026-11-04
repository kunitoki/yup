/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

static void startYupApplication()
{
    if (YUPApplicationBase::getInstance() != nullptr)
        return;

    if (auto* app = YUPApplicationBase::createInstance())
    {
        if (! app->initialiseApp())
            exit (app->shutdownApp());
    }
    else
    {
        jassertfalse;
    }
}

static bool usesSceneLifecycle()
{
    return [[NSBundle mainBundle] objectForInfoDictionaryKey:@"UIApplicationSceneManifest"] != nil;
}

} // namespace yup

//==============================================================================
/** Drives the YUP application from the UIScene life cycle, required by recent iOS SDKs.

    Used when the Info.plist declares a UIApplicationSceneManifest naming this class. The
    application starts when the first window scene connects, so its windows attach to that scene.
*/
@interface YupSceneDelegate : UIResponder <UIWindowSceneDelegate>

@end

@implementation YupSceneDelegate

- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession* __unused)session options:(UISceneConnectionOptions* __unused)connectionOptions
{
    if ([scene isKindOfClass:[UIWindowScene class]])
        yup::startYupApplication();
}

- (void)sceneDidEnterBackground:(UIScene* __unused)scene
{
    if (auto* app = yup::YUPApplicationBase::getInstance())
        app->suspended();
}

- (void)sceneWillEnterForeground:(UIScene* __unused)scene
{
    if (auto* app = yup::YUPApplicationBase::getInstance())
        app->resumed();
}

@end

//==============================================================================
/** This class is used to handle the iOS-specific parts of the YupWindowing class.

    Without a UIApplicationSceneManifest in the Info.plist the legacy application life cycle
    is used, and the application starts and is suspended or resumed from here.
*/
@interface YupApplicationDelegate : NSObject <UIApplicationDelegate>

@end

@implementation YupApplicationDelegate

- (void)applicationDidFinishLaunching:(UIApplication* __unused)application
{
    if (! yup::usesSceneLifecycle())
        yup::startYupApplication();
}

- (void)applicationWillTerminate:(UIApplication* __unused)application
{
    yup::YUPApplicationBase::appWillTerminateByForce();
}

- (void)applicationDidEnterBackground:(UIApplication* __unused)application
{
    if (auto* app = yup::YUPApplicationBase::getInstance())
        app->suspended();
}

- (void)applicationWillEnterForeground:(UIApplication* __unused)application
{
    if (auto* app = yup::YUPApplicationBase::getInstance())
        app->resumed();
}

@end

namespace yup
{

int yup_iOSMain(int argc, const char* argv[], void* customDelegatePtr);
int yup_iOSMain(int argc, const char* argv[], void* customDelegatePtr)
{
    Class delegateClass = (customDelegatePtr != nullptr ? (__bridge Class) customDelegatePtr : [YupApplicationDelegate class]);

    return UIApplicationMain(argc, const_cast<char**>(argv), nil, NSStringFromClass(delegateClass));
}

} // namespace yup
