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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

TEST (ApplicationTests, MoreThanOneInstanceAllowedReturnsTrue)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);
    EXPECT_TRUE (app->moreThanOneInstanceAllowed());
}

TEST (ApplicationTests, AnotherInstanceStartedDoesNotCrash)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);
    EXPECT_NO_THROW (app->anotherInstanceStarted ("test command line"));
}

TEST (ApplicationTests, AnotherInstanceStartedWithEmptyString)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);
    EXPECT_NO_THROW (app->anotherInstanceStarted (String()));
}

TEST (ApplicationTests, SuspendedDoesNotCrash)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);
    EXPECT_NO_THROW (app->suspended());
}

TEST (ApplicationTests, ResumedDoesNotCrash)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);
    EXPECT_NO_THROW (app->resumed());
}

TEST (ApplicationTests, UnhandledExceptionDoesNotCrash)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);
    EXPECT_NO_THROW (app->unhandledException (nullptr, "test_file.cpp", 42));
}

TEST (ApplicationTests, UnhandledExceptionCannotBeAbortedIgnored)
{
    auto* app = dynamic_cast<YUPApplication*> (YUPApplicationBase::getInstance());
    ASSERT_NE (nullptr, app);

    try
    {
        app->unhandledException (nullptr, "test_file.cpp", 42);
        SUCCEED();
    }
    catch (...)
    {
        FAIL() << "unhandledException should not throw";
    }
}
