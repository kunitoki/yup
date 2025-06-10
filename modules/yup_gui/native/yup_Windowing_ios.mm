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

/** This class is used to handle the iOS-specific parts of the YupWindowing class. */
@interface YupApplicationDelegate : NSObject <UIApplicationDelegate>

@end

@implementation YupApplicationDelegate

- (id)init
{
    self = [super init];
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(UIApplication* __unused)application
{
    if (auto* app = yup::YUPApplicationBase::createInstance())
    {
        if (!app->initialiseApp())
            exit(app->shutdownApp());
    }
    else
    {
        jassertfalse;
    }
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

- (void)applicationDidBecomeActive:(UIApplication* __unused)application
{
}

- (void)applicationWillResignActive:(UIApplication* __unused)application
{
}

@end

namespace yup
{

int yup_iOSMain(int argc, const char* argv[], void* customDelegatePtr);
int yup_iOSMain(int argc, const char* argv[], void* customDelegatePtr)
{
    Class delegateClass = (customDelegatePtr != nullptr ? reinterpret_cast<Class>(customDelegatePtr) : [YupApplicationDelegate class]);

    return UIApplicationMain(argc, const_cast<char**>(argv), nil, NSStringFromClass(delegateClass));
}

} // namespace yup
