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

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace yup;

TEST (ScopeGuardTests, ScopeGuardCallsFunctionAtScopeEnd)
{
    bool flag = false;

    {
        ScopeGuard guard { [&]
        {
            flag = true;
        } };
        EXPECT_FALSE (flag);
    }

    EXPECT_TRUE (flag);
}

TEST (ScopeGuardTests, ScopeGuardHandlesExceptions)
{
    bool flag = false;

    auto throwingFunction = [&]()
    {
        ScopeGuard guard { [&]
        {
            flag = true;
        } };
        throw std::runtime_error ("Test exception");
    };

    EXPECT_THROW (throwingFunction(), std::runtime_error);
    EXPECT_TRUE (flag);
}

TEST (ScopeGuardTests, ScopeGuardExecutesOnMultipleReturns)
{
    bool flag = false;

    auto functionWithMultipleReturns = [&] (bool condition) -> bool
    {
        ScopeGuard guard { [&]
        {
            flag = true;
        } };
        if (condition)
            return true;
        return false;
    };

    EXPECT_TRUE (functionWithMultipleReturns (true));
    EXPECT_TRUE (flag);

    flag = false;
    EXPECT_FALSE (functionWithMultipleReturns (false));
    EXPECT_TRUE (flag);
}

TEST (ErasedScopeGuardTests, CallsCallbackOnDestruction)
{
    bool flag = false;
    {
        ErasedScopeGuard guard ([&]
        {
            flag = true;
        });
        EXPECT_FALSE (flag);
    }
    EXPECT_TRUE (flag);
}

TEST (ErasedScopeGuardTests, CallbackNotCalledAfterRelease)
{
    bool flag = false;
    {
        ErasedScopeGuard guard ([&]
        {
            flag = true;
        });
        guard.release();
        EXPECT_FALSE (flag);
    }
    EXPECT_FALSE (flag);
}

TEST (ErasedScopeGuardTests, CallbackCalledAfterReset)
{
    bool flag = false;
    {
        ErasedScopeGuard guard ([&]
        {
            flag = true;
        });
        guard.reset();
        EXPECT_TRUE (flag);
    }
}

TEST (ErasedScopeGuardTests, CallbackNotCalledAfterMove)
{
    bool flag = false;
    {
        ErasedScopeGuard guard1 ([&]
        {
            flag = true;
        });
        ErasedScopeGuard guard2 (std::move (guard1));
        EXPECT_FALSE (flag);
    }
    EXPECT_TRUE (flag);
}

TEST (ErasedScopeGuardTests, CallbackCalledAfterMoveAssignment)
{
    bool flag1 = false;
    bool flag2 = false;

    {
        ErasedScopeGuard guard1 ([&]
        {
            flag1 = true;
        });
        ErasedScopeGuard guard2 ([&]
        {
            flag2 = true;
        });

        guard2 = std::move (guard1);

        EXPECT_FALSE (flag1);
        EXPECT_TRUE (flag2);
    }

    EXPECT_TRUE (flag1);
    EXPECT_TRUE (flag2);
}

TEST (ErasedScopeGuardTests, CallbackCalledOnDefaultConstructor)
{
    ErasedScopeGuard guard;
    guard.reset(); // Should not crash even if no callback is provided
}
