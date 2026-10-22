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

using namespace yup;

class SignalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        previousDispatcher = SignalDispatcher::setAsyncDispatcher ([this] (std::function<void()> function)
        {
            queued.push_back (std::move (function));
            return true;
        });
    }

    void TearDown() override
    {
        SignalDispatcher::setAsyncDispatcher (std::move (previousDispatcher));
    }

    void deliverQueued()
    {
        auto toDeliver = std::exchange (queued, {});
        for (auto& function : toDeliver)
            function();
    }

    struct CopyCounter
    {
        CopyCounter() = default;
        CopyCounter (const CopyCounter& other) : copies (other.copies + 1), moves (other.moves) {}
        CopyCounter (CopyCounter&& other) noexcept : copies (other.copies), moves (other.moves + 1) {}

        int copies = 0;
        int moves = 0;
    };

    std::vector<std::function<void()>> queued;
    SignalDispatcher::Function previousDispatcher;
};

TEST_F (SignalTests, CallsCallbacksInConnectionOrder)
{
    Signal<int> signal;
    std::vector<int> calls;

    auto first = signal.connect ([&] (int value) { calls.push_back (value); });
    auto second = signal.connect ([&] (int value) { calls.push_back (value * 10); });

    signal.emit (3);

    EXPECT_EQ (calls, (std::vector<int> { 3, 30 }));
    EXPECT_EQ (signal.getNumConnections(), 2);
}

TEST_F (SignalTests, DestroyingTheConnectionDisconnects)
{
    Signal<> signal;
    int calls = 0;

    {
        auto connection = signal.connect ([&] { ++calls; });
        EXPECT_TRUE (connection.isConnected());
        signal.emit();
    }

    signal.emit();

    EXPECT_EQ (calls, 1);
    EXPECT_EQ (signal.getNumConnections(), 0);
}

TEST_F (SignalTests, MovedConnectionStaysConnected)
{
    Signal<> signal;
    int calls = 0;

    auto connection = signal.connect ([&] { ++calls; });
    SignalConnection moved (std::move (connection));

    EXPECT_FALSE (connection.isConnected());
    EXPECT_TRUE (moved.isConnected());

    signal.emit();
    EXPECT_EQ (calls, 1);
}

TEST_F (SignalTests, CallbackDisconnectedDuringEmitIsNotCalled)
{
    Signal<> signal;
    SignalConnection second;
    int secondCalls = 0;

    auto first = signal.connect ([&] { second.disconnect(); });
    second = signal.connect ([&] { ++secondCalls; });

    signal.emit();

    EXPECT_EQ (secondCalls, 0);
}

TEST_F (SignalTests, CallbackConnectedDuringEmitIsCalledFromTheNextEmit)
{
    Signal<> signal;
    SignalConnection late;
    int lateCalls = 0;

    auto first = signal.connect ([&]
    {
        if (! late.isConnected())
            late = signal.connect ([&] { ++lateCalls; });
    });

    signal.emit();
    EXPECT_EQ (lateCalls, 0);

    signal.emit();
    EXPECT_EQ (lateCalls, 1);
}

TEST_F (SignalTests, SignalCanBeDestroyedWhileEmitting)
{
    auto signal = std::make_unique<Signal<>>();
    int calls = 0;

    auto first = signal->connect ([&] { ++calls; signal.reset(); });
    auto second = signal->connect ([&] { ++calls; });

    signal->emit();

    EXPECT_EQ (signal, nullptr);
    EXPECT_EQ (calls, 1);
}

TEST_F (SignalTests, ConnectionCanOutliveItsSignal)
{
    SignalConnection connection;

    {
        Signal<> signal;
        connection = signal.connect ([] {});
    }

    EXPECT_FALSE (connection.isConnected());
    connection.disconnect();
}

TEST_F (SignalTests, EmitPassesReferencesThrough)
{
    Signal<int&> signal;
    auto connection = signal.connect ([] (int& value) { value += 1; });
    auto another = signal.connect ([] (int& value) { value *= 10; });

    int value = 1;
    signal.emit (value);

    EXPECT_EQ (value, 20);
}

TEST_F (SignalTests, EmitMovesAnRvalueOnlyIntoTheLastCallback)
{
    Signal<CopyCounter> signal;
    std::vector<CopyCounter> received;

    auto first = signal.connect ([&] (CopyCounter value) { received.push_back (value); });
    auto second = signal.connect ([&] (CopyCounter value) { received.push_back (value); });

    signal.emit (CopyCounter {});

    ASSERT_EQ (received.size(), 2u);
    EXPECT_EQ (received[0].copies, 2); // into the parameter, then into the vector
    EXPECT_EQ (received[1].copies, 1); // only into the vector
}

TEST_F (SignalTests, EmitDoesNotCopyForConstReferenceParameters)
{
    Signal<const CopyCounter&> signal;
    int copies = -1;

    auto connection = signal.connect ([&] (const CopyCounter& value) { copies = value.copies; });

    const CopyCounter value;
    signal.emit (value);

    EXPECT_EQ (copies, 0);
}

TEST_F (SignalTests, ConnectAcceptsAnyCallable)
{
    Signal<int> signal;
    auto total = std::make_shared<int> (0);

    struct Accumulator
    {
        std::shared_ptr<int> total;
        void operator() (int value) const { *total += value; }
    };

    auto connection = signal.connect (Accumulator { total });
    signal.emit (5);

    EXPECT_EQ (*total, 5);
}

TEST_F (SignalTests, EmitAsyncDeliversLater)
{
    Signal<String> signal;
    std::vector<String> received;

    auto connection = signal.connect ([&] (String value) { received.push_back (value); });

    EXPECT_TRUE (signal.emitAsync (String ("hello")));
    EXPECT_TRUE (received.empty());

    deliverQueued();

    ASSERT_EQ (received.size(), 1u);
    EXPECT_EQ (received[0], "hello");
}

TEST_F (SignalTests, EmitAsyncStoresACopyOfConstReferenceArguments)
{
    Signal<const String&> signal;
    String received;

    auto connection = signal.connect ([&] (const String& value) { received = value; });

    {
        String temporary ("copied");
        signal.emitAsync (temporary);
    }

    deliverQueued();

    EXPECT_EQ (received, "copied");
}

TEST_F (SignalTests, EmitAsyncCallsCallbacksConnectedAtDelivery)
{
    Signal<> signal;
    int early = 0;
    int late = 0;

    auto earlyConnection = signal.connect ([&] { ++early; });
    signal.emitAsync();

    earlyConnection.disconnect();
    auto lateConnection = signal.connect ([&] { ++late; });

    deliverQueued();

    EXPECT_EQ (early, 0);
    EXPECT_EQ (late, 1);
}

TEST_F (SignalTests, EmitAsyncDoesNothingAfterTheSignalIsDestroyed)
{
    int calls = 0;
    SignalConnection connection;

    {
        Signal<> signal;
        connection = signal.connect ([&] { ++calls; });
        signal.emitAsync();
    }

    deliverQueued();

    EXPECT_EQ (calls, 0);
}

TEST_F (SignalTests, EmitAsyncFailsWithoutDispatcher)
{
    SignalDispatcher::setAsyncDispatcher (nullptr);

    Signal<int> signal;
    EXPECT_FALSE (signal.emitAsync (1));
}

TEST_F (SignalTests, SetAsyncDispatcherReturnsThePreviousOne)
{
    auto installed = SignalDispatcher::setAsyncDispatcher (nullptr);
    ASSERT_NE (installed, nullptr);

    EXPECT_EQ (SignalDispatcher::setAsyncDispatcher (std::move (installed)), nullptr);
    EXPECT_TRUE (SignalDispatcher::dispatchAsync ([] {}));
    EXPECT_EQ (queued.size(), 1u);
}
