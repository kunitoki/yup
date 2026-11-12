/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

#include <functional>

#include <yup_core/yup_core.h>

#if ! YUP_EMSCRIPTEN
#include <yup_events/yup_events.h>
#endif

namespace yup
{

//==============================================================================
/** Runs all registered GoogleTest cases inside a YUP application context.

    On desktop platforms the tests execute on the message thread (so GUI and
    audio features initialise correctly), one suite at a time, printing a compact
    report and optionally generating a JUnit XML file. On Emscripten the tests run
    synchronously with sharding support.

    Most projects never call this directly and instead use the
    @ref YUP_IMPLEMENT_GTEST_MAIN macro, which defines a @c main that forwards to
    this function:

    @code
    #include <yup_test_runner.h>

    YUP_IMPLEMENT_GTEST_MAIN()
    @endcode

    Projects that need a custom entry point can call it explicitly instead:

    @code
    int main (int argc, char** argv)
    {
        return yup::runGoogleTests (argc, argv);
    }
    @endcode

    @returns 0 when every test passed, non-zero otherwise.
*/
int runGoogleTests (int argc, char** argv);

//==============================================================================
/** Optional hook letting tests take over the process when it is relaunched as a
    child (for example to host out-of-process plug-in scanners).

    Assign a callback that inspects the command line and returns true to take over
    the process before any test runs. Leaving it unset (the default) means the
    runner always executes the tests. This is an advanced feature; typical test
    suites never touch it.
*/
std::function<bool (const String&)>& getGoogleTestChildProcessEntryPoint();

#if ! YUP_EMSCRIPTEN
//==============================================================================
/** Creates the YUP application that drives the GoogleTest runner on native
    platforms. Used by @ref YUP_IMPLEMENT_GTEST_MAIN; most projects never call it
    directly.
*/
YUPApplicationBase* createGoogleTestRunnerApplication();
#endif

} // namespace yup

//==============================================================================
/** Defines the program entry point for a GoogleTest runner built on YUP.

    Place this once in a translation unit of your test executable. On native
    platforms it expands through YUP's standard entry-point machinery (the same
    one @ref START_YUP_APPLICATION uses), so it produces the correct entry per
    platform - @c WinMain on Windows, @c SDL_main on Android, the iOS app delegate
    bootstrap and @c main elsewhere - and runs the tests inside a YUP application
    context. On Emscripten it expands to a @c main that forwards to
    yup::runGoogleTests().
*/
#if YUP_EMSCRIPTEN

#define YUP_IMPLEMENT_GTEST_MAIN()                 \
    int main (int argc, char** argv)               \
    {                                              \
        return ::yup::runGoogleTests (argc, argv); \
    }

#else

#if YUP_IOS
#define YUP_IMPLEMENT_GTEST_IOS_DELEGATE() \
    void* yup_GetIOSCustomDelegateClass(); \
    void* yup_GetIOSCustomDelegateClass()  \
    {                                      \
        return nullptr;                    \
    }
#else
#define YUP_IMPLEMENT_GTEST_IOS_DELEGATE()
#endif

#define YUP_IMPLEMENT_GTEST_MAIN()                               \
    yup::YUPApplicationBase* yup_CreateApplication();            \
    yup::YUPApplicationBase* yup_CreateApplication()             \
    {                                                            \
        return ::yup::createGoogleTestRunnerApplication();       \
    }                                                            \
    YUP_IMPLEMENT_GTEST_IOS_DELEGATE()                           \
    YUP_MAIN_FUNCTION_DEFINITION

#endif
