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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifdef YUP_EVENTS_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#define YUP_CORE_INCLUDE_OBJC_HELPERS 1
#define YUP_CORE_INCLUDE_JNI_HELPERS 1
#define YUP_CORE_INCLUDE_NATIVE_HEADERS 1
#define YUP_CORE_INCLUDE_COM_SMART_PTR 1
#define YUP_EVENTS_INCLUDE_WIN32_MESSAGE_WINDOW 1

#if YUP_USE_WINRT_MIDI
#define YUP_EVENTS_INCLUDE_WINRT_WRAPPER 1
#endif

#include "yup_events.h"

//==============================================================================
#if YUP_MAC
#import <IOKit/IOKitLib.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <UserNotifications/UserNotifications.h>

#elif YUP_IOS
#import <UserNotifications/UserNotifications.h>

#elif YUP_WINDOWS
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4471 4710 4711 4514 4820 4668 4505)
#include <sdkddkver.h>
#include <ShObjIdl.h>
#include <windows.ui.notifications.h>
#include <roapi.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>
#include <winstring.h>
YUP_END_IGNORE_WARNINGS_MSVC

#include <cwchar>
#include <map>
#include <optional>

#if ! YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")
#endif

#elif YUP_LINUX || YUP_BSD
#include <unistd.h>

#elif YUP_EMSCRIPTEN
#include <emscripten.h>
#include <deque>
#include <mutex>

#elif YUP_ANDROID
#include <jni.h>
#include <SDL3/SDL_system.h>

#endif

//==============================================================================
#include "messages/yup_ApplicationBase.cpp"
#include "messages/yup_DeletedAtShutdown.cpp"
#include "messages/yup_MessageListener.cpp"
#include "messages/yup_MessageManager.cpp"
#include "broadcasters/yup_ActionBroadcaster.cpp"
#include "broadcasters/yup_AsyncUpdater.cpp"
#include "broadcasters/yup_LockingAsyncUpdater.cpp"
#include "broadcasters/yup_ChangeBroadcaster.cpp"
#include "timers/yup_MultiTimer.cpp"
#include "timers/yup_Timer.cpp"
#include "interprocess/yup_ChildProcessManager.cpp"
#include "interprocess/yup_InterprocessConnection.cpp"
#include "interprocess/yup_InterprocessConnectionServer.cpp"
#include "interprocess/yup_ConnectedChildProcess.cpp"
#include "interprocess/yup_NetworkServiceDiscovery.cpp"
#include "native/yup_ScopedLowPowerModeDisabler.cpp"
#include "notifications/yup_ToastNotification.cpp"

//==============================================================================
#if YUP_MAC || YUP_IOS
#include "native/yup_MessageQueue_apple.h"
#if YUP_MAC
#include "native/yup_MessageManager_mac.mm"
#else
#include "native/yup_MessageManager_ios.mm"
#endif
#include "native/yup_ToastNotification_apple.mm"

#elif YUP_WINDOWS
#include "native/yup_RunningInUnity.h"
#include "native/yup_Messaging_windows.cpp"
#if YUP_EVENTS_INCLUDE_WINRT_WRAPPER
#include "native/yup_WinRTWrapper_windows.cpp"
#endif
#include "native/yup_ToastNotification_windows.cpp"

#elif YUP_LINUX || YUP_BSD
#include "native/yup_EventLoopInternal_linux.h"
#include "native/yup_Messaging_linux.cpp"
#include "native/yup_ToastNotification_linux.cpp"

#elif YUP_EMSCRIPTEN
#include "native/yup_Messaging_emscripten.cpp"
#include "native/yup_ToastNotification_wasm.cpp"

#elif YUP_ANDROID
#include "native/yup_Messaging_android.cpp"
#include "native/yup_ToastNotification_android.cpp"

#endif
