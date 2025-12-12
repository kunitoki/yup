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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

TEST (ChildProcessTests, ReadAllProcesOutput)
{
#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD
    ChildProcess p;

#if YUP_WINDOWS
    EXPECT_TRUE (p.start ("tasklist"));
#else
    EXPECT_TRUE (p.start ("ls /"));
#endif

    auto output = p.readAllProcessOutput();
    EXPECT_TRUE (output.isNotEmpty());
#endif
}

TEST (ChildProcessTests, StartWithEnvironment)
{
#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD
    ChildProcess p;
    StringPairArray env;
    env.set ("YUP_TEST_VAR", "test_value");
    env.set ("PATH", SystemStats::getEnvironmentVariable ("PATH", ""));

#if YUP_WINDOWS
    EXPECT_TRUE (p.start ("cmd /c echo %YUP_TEST_VAR%", env));
#else
    EXPECT_TRUE (p.start ("printenv YUP_TEST_VAR", env));
#endif

    auto output = p.readAllProcessOutput().trim();
    EXPECT_TRUE (output.contains ("test_value"));
#endif
}

TEST (ChildProcessTests, IsRunning)
{
#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD
    ChildProcess p;

#if YUP_WINDOWS
    EXPECT_TRUE (p.start ("cmd /c timeout /t 1"));
#else
    EXPECT_TRUE (p.start ("sleep 1"));
#endif

    // Should be running initially
    EXPECT_TRUE (p.isRunning());

    // Wait for completion
    p.waitForProcessToFinish (2000);

    // Should not be running after completion
    EXPECT_FALSE (p.isRunning());
#endif
}

TEST (ChildProcessTests, Kill)
{
#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD
    ChildProcess p;

#if YUP_WINDOWS
    EXPECT_TRUE (p.start ("cmd /c timeout /t 30"));
#else
    EXPECT_TRUE (p.start ("sleep 30"));
#endif

    EXPECT_TRUE (p.isRunning());

    // Kill the process
    EXPECT_TRUE (p.kill());

    // Give it a moment to terminate
    Thread::sleep (100);

    // Should not be running after kill
    EXPECT_FALSE (p.isRunning());
#endif
}

TEST (ChildProcessTests, GetExitCode)
{
#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD
    ChildProcess p;

#if YUP_WINDOWS
    EXPECT_TRUE (p.start ("cmd /c exit 42"));
    p.waitForProcessToFinish (1000);
    auto exitCode = p.getExitCode();
    EXPECT_EQ (exitCode, 42);
#else
    // On POSIX, use 'true' command which exits with 0
    EXPECT_TRUE (p.start ("true"));
    p.waitForProcessToFinish (1000);
    auto exitCode = p.getExitCode();
    EXPECT_EQ (exitCode, 0);

    // Test non-zero exit using 'false' command which exits with 1
    ChildProcess p2;
    EXPECT_TRUE (p2.start ("false"));
    p2.waitForProcessToFinish (1000);
    EXPECT_EQ (p2.getExitCode(), 1);
#endif
#endif
}

TEST (ChildProcessTests, StartWithStringArray)
{
#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD
    ChildProcess p;
    StringArray args;

#if YUP_WINDOWS
    args.add ("cmd");
    args.add ("/c");
    args.add ("echo");
    args.add ("test");
#else
    args.add ("echo");
    args.add ("test");
#endif

    EXPECT_TRUE (p.start (args));

    auto output = p.readAllProcessOutput().trim();
    EXPECT_TRUE (output.contains ("test"));
#endif
}

// Note: yup_runSystemCommand and yup_getOutputFromCommand are internal POSIX functions
// not exposed in the public API. They are tested indirectly through ChildProcess and File operations.
