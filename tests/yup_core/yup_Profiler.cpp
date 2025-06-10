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

#include <yup_core/yup_core.h>

#include <gtest/gtest.h>

using namespace yup;

#if YUP_ENABLE_PROFILING

class ProfilerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clean up any existing instance
        Profiler::deleteInstance();
    }

    void TearDown() override
    {
        // Clean up after tests
        Profiler::deleteInstance();
    }
};

TEST_F (ProfilerTest, SingletonInstance)
{
    auto* profiler1 = Profiler::getInstance();
    auto* profiler2 = Profiler::getInstance();

    EXPECT_NE (profiler1, nullptr);
    EXPECT_EQ (profiler1, profiler2);
}

TEST_F (ProfilerTest, StartStopTracing)
{
    auto* profiler = Profiler::getInstance();

    // Should not throw when starting tracing
    EXPECT_NO_THROW (profiler->startTracing());

    // Should not throw when stopping tracing
    EXPECT_NO_THROW (profiler->stopTracing());
}

TEST_F (ProfilerTest, StartTracingWithCustomBufferSize)
{
    auto* profiler = Profiler::getInstance();

    // Should not throw when starting tracing with custom buffer size
    EXPECT_NO_THROW (profiler->startTracing (1024));

    // Should not throw when stopping tracing
    EXPECT_NO_THROW (profiler->stopTracing());
}

TEST_F (ProfilerTest, SetOutputFolder)
{
    auto* profiler = Profiler::getInstance();

    File tempDir = File::getSpecialLocation (File::tempDirectory);

    EXPECT_NO_THROW (profiler->setOutputFolder (tempDir));
}

TEST_F (ProfilerTest, CompileTimePrettierFunction)
{
    // Test the compile-time function prettifier
    constexpr auto result1 = Profiler::compileTimePrettierFunction ([]
    {
        return "int main";
    });
    EXPECT_STREQ (result1.data(), "main");

    constexpr auto result2 = Profiler::compileTimePrettierFunction ([]
    {
        return "void SomeClass::someMethod";
    });
    EXPECT_STREQ (result2.data(), "SomeClass::someMethod");
}

TEST_F (ProfilerTest, ProfileMacros)
{
    // Test that profile macros don't crash
    EXPECT_NO_THROW (YUP_PROFILE_START());
    EXPECT_NO_THROW (YUP_PROFILE_STOP());

    File tempDir = File::getSpecialLocation (File::tempDirectory);
    EXPECT_NO_THROW (YUP_PROFILE_SET_OUTPUT_FOLDER (tempDir));

    // Test trace macros (these should compile without error)
    YUP_PROFILE_TRACE ("test", "test_event");
    YUP_PROFILE_NAMED_TRACE ("test", TestEvent);
    YUP_PROFILE_INTERNAL_TRACE();
    YUP_PROFILE_NAMED_INTERNAL_TRACE (InternalTestEvent);
}

TEST_F (ProfilerTest, MultipleStartStop)
{
    auto* profiler = Profiler::getInstance();

    // Test multiple start/stop cycles
    EXPECT_NO_THROW (profiler->startTracing());
    EXPECT_NO_THROW (profiler->stopTracing());

    // The second cycle should create a new instance due to singleton deletion in stopTracing
    EXPECT_NO_THROW (YUP_PROFILE_START());
    EXPECT_NO_THROW (YUP_PROFILE_STOP());
}

#else

// When profiling is disabled, the macros should be no-ops
TEST (ProfilerDisabledTest, MacrosAreNoOps)
{
    // These should compile to nothing when profiling is disabled
    YUP_PROFILE_START();
    YUP_PROFILE_STOP();
    YUP_PROFILE_TRACE ("test", "test_event");
    YUP_PROFILE_NAMED_TRACE ("test", TestEvent);
    YUP_PROFILE_INTERNAL_TRACE();
    YUP_PROFILE_NAMED_INTERNAL_TRACE (InternalTestEvent);

    File tempDir = File::getSpecialLocation (File::tempDirectory);
    YUP_PROFILE_SET_OUTPUT_FOLDER (tempDir);

    // Test passes if it compiles and runs without error
    SUCCEED();
}

#endif
