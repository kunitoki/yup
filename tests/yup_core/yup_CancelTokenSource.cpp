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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

#include <atomic>
#include <thread>
#include <type_traits>

using namespace yup;

TEST (CancelTokenSourceTests, DefaultConstructedSourceOwnsActiveToken)
{
    CancelTokenSource source;

    EXPECT_TRUE (source.isCancellable());
    EXPECT_FALSE (source.wasCancelled());
    EXPECT_FALSE (source.getToken().wasCancelled());
}

TEST (CancelTokenSourceTests, SourceCancelsTokenOnDestruction)
{
    CancelToken token;

    {
        CancelTokenSource source;
        token = source.getToken();

        EXPECT_FALSE (token.wasCancelled());
    }

    EXPECT_TRUE (token.wasCancelled());
}

TEST (CancelTokenSourceTests, ExplicitCancelMarksTokenAndRemainsCancellable)
{
    CancelTokenSource source;
    auto token = source.getToken();

    source.cancel();

    EXPECT_TRUE (token.wasCancelled());
    EXPECT_TRUE (token.isCancellable());
    EXPECT_TRUE (source.wasCancelled());
    EXPECT_TRUE (source.isCancellable());

    source.cancel(); // idempotent

    EXPECT_TRUE (source.wasCancelled());
}

TEST (CancelTokenSourceTests, MovedFromSourceDoesNotCancelOnDestruction)
{
    CancelToken token;
    CancelTokenSource destination;

    {
        CancelTokenSource source;
        token = source.getToken();

        destination = std::move (source); // transfer ownership

        EXPECT_FALSE (source.isCancellable());
        EXPECT_FALSE (source.wasCancelled());
    } // moved-from `source` destroyed here — must not cancel

    EXPECT_FALSE (token.wasCancelled());

    destination.cancel(); // the owning source still controls the token

    EXPECT_TRUE (token.wasCancelled());
}

TEST (CancelTokenSourceTests, MoveConstructionTransfersOwnershipAndCancelsOnDestruction)
{
    CancelToken token;

    {
        CancelTokenSource source;
        token = source.getToken();

        CancelTokenSource destination (std::move (source));

        EXPECT_FALSE (source.isCancellable());
        EXPECT_FALSE (token.wasCancelled());
    } // `destination` destroyed here — cancels the token

    EXPECT_TRUE (token.wasCancelled());
}

TEST (CancelTokenSourceTests, MoveAssignmentCancelsPreviouslyOwnedToken)
{
    CancelToken token;
    CancelTokenSource source;
    token = source.getToken();

    CancelTokenSource other;
    source = std::move (other);

    // `source` was reassigned: the token it previously owned is cancelled
    EXPECT_TRUE (token.wasCancelled());
}

TEST (CancelTokenSourceTests, SourceIsMoveOnly)
{
    static_assert (! std::is_copy_constructible_v<CancelTokenSource>);
    static_assert (! std::is_copy_assignable_v<CancelTokenSource>);
    static_assert (std::is_move_constructible_v<CancelTokenSource>);
    static_assert (std::is_move_assignable_v<CancelTokenSource>);
    static_assert (std::is_nothrow_move_constructible_v<CancelTokenSource>);
    static_assert (std::is_nothrow_move_assignable_v<CancelTokenSource>);
}

TEST (CancelTokenSourceTests, GetTokenCopiesNeverTriggerCancellation)
{
    CancelTokenSource source;
    auto token = source.getToken();

    {
        auto copy = token; // observer copy
    } // destroyed — must not cancel anything

    EXPECT_FALSE (token.wasCancelled());
    EXPECT_FALSE (source.wasCancelled());
}

TEST (CancelTokenSourceTests, SourceDestructionWakesWaitersAndFiresCallbacks)
{
    CancelToken token;
    std::atomic<int> callbackCount { 0 };
    std::atomic<bool> woken { false };

    std::thread waiter;
    CancelToken::Registration registration;

    {
        CancelTokenSource source;
        token = source.getToken();

        registration = token.registerCallback ([&]
        {
            ++callbackCount;
        });

        waiter = std::thread ([&]
        {
            if (token.waitForCancellation (2000))
                woken = true;
        });

        Thread::sleep (50);
    } // source destroyed here — cancels, waking the waiter and firing the callback

    waiter.join();

    EXPECT_TRUE (woken.load());
    EXPECT_EQ (1, callbackCount.load());
    EXPECT_TRUE (token.wasCancelled());
}
