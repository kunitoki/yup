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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_events
    vendor:             yup
    version:            7.0.12
    name:               YUP message and event handling classes
    description:        Classes for running an application's main event loop and sending/receiving messages, timers, etc.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_EVENTS_H_INCLUDED

#include <yup_core/yup_core.h>

//==============================================================================
/** Config: YUP_EXECUTE_APP_SUSPEND_ON_BACKGROUND_TASK
    Will execute your application's suspend method on an iOS background task, giving
    you extra time to save your applications state.
*/
#ifndef YUP_EXECUTE_APP_SUSPEND_ON_BACKGROUND_TASK
#define YUP_EXECUTE_APP_SUSPEND_ON_BACKGROUND_TASK 0
#endif

#if YUP_WINDOWS && YUP_EVENTS_INCLUDE_WINRT_WRAPPER
// If this header file is missing then you are probably attempting to use WinRT
// functionality without the WinRT libraries installed on your system. Try installing
// the latest Windows Standalone SDK and maybe also adding the path to the WinRT
// headers to your build system. This path should have the form
// "C:\Program Files (x86)\Windows Kits\10\Include\10.0.14393.0\winrt".
#include <inspectable.h>
#include <hstring.h>
#endif

#include "messages/yup_MessageManager.h"
#include "messages/yup_Message.h"
#include "messages/yup_MessageListener.h"
#include "messages/yup_CallbackMessage.h"
#include "messages/yup_DeletedAtShutdown.h"
#include "messages/yup_NotificationType.h"
#include "messages/yup_ApplicationBase.h"
#include "messages/yup_Initialisation.h"
#include "messages/yup_MountedVolumeListChangeDetector.h"
#include "broadcasters/yup_ActionBroadcaster.h"
#include "broadcasters/yup_ActionListener.h"
#include "broadcasters/yup_AsyncUpdater.h"
#include "broadcasters/yup_LockingAsyncUpdater.h"
#include "broadcasters/yup_ChangeListener.h"
#include "broadcasters/yup_ChangeBroadcaster.h"
#include "timers/yup_Timer.h"
#include "timers/yup_TimedCallback.h"
#include "timers/yup_MultiTimer.h"
#include "interprocess/yup_ChildProcessManager.h"
#include "interprocess/yup_InterprocessConnection.h"
#include "interprocess/yup_InterprocessConnectionServer.h"
#include "interprocess/yup_ConnectedChildProcess.h"
#include "interprocess/yup_NetworkServiceDiscovery.h"
#include "native/yup_ScopedLowPowerModeDisabler.h"

#if YUP_LINUX || YUP_BSD
#include "native/yup_EventLoop_linux.h"
#endif

#if YUP_WINDOWS
#if YUP_EVENTS_INCLUDE_WIN32_MESSAGE_WINDOW
#include "native/yup_HiddenMessageWindow_windows.h"
#endif
#if YUP_EVENTS_INCLUDE_WINRT_WRAPPER
#include "native/yup_WinRTWrapper_windows.h"
#endif
#endif
