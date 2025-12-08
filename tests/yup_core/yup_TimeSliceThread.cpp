/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace
{
// Mock TimeSliceClient for testing
class TestTimeSliceClient : public TimeSliceClient
{
public:
    TestTimeSliceClient (int returnValue = 100)
        : returnValueMs (returnValue)
    {
    }

    int useTimeSlice() override
    {
        ++callCount;
        lastCallTime = Time::getCurrentTime();
        return returnValueMs;
    }

    void setReturnValue (int value)
    {
        returnValueMs = value;
    }

    int getCallCount() const { return callCount; }

    Time getLastCallTime() const { return lastCallTime; }

    void resetCallCount() { callCount = 0; }

private:
    int returnValueMs;
    std::atomic<int> callCount { 0 };
    Time lastCallTime;
};

// Client that returns negative to trigger auto-removal
class SelfRemovingClient : public TimeSliceClient
{
public:
    SelfRemovingClient (int callsBeforeRemoval = 3)
        : maxCalls (callsBeforeRemoval)
    {
    }

    int useTimeSlice() override
    {
        ++callCount;
        if (callCount >= maxCalls)
            return -1; // Signal removal
        return 50;
    }

    int getCallCount() const { return callCount; }

private:
    int maxCalls;
    std::atomic<int> callCount { 0 };
};
} // namespace

class TimeSliceThreadTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F (TimeSliceThreadTests, ConstructorCreatesThread)
{
    TimeSliceThread thread ("TestThread");
    EXPECT_FALSE (thread.isThreadRunning());
}

TEST_F (TimeSliceThreadTests, AddClient)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client;

    thread.addTimeSliceClient (&client);
    EXPECT_EQ (thread.getNumClients(), 1);
    EXPECT_TRUE (thread.contains (&client));
}

TEST_F (TimeSliceThreadTests, AddMultipleClients)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1;
    TestTimeSliceClient client2;
    TestTimeSliceClient client3;

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);
    thread.addTimeSliceClient (&client3);

    EXPECT_EQ (thread.getNumClients(), 3);
    EXPECT_TRUE (thread.contains (&client1));
    EXPECT_TRUE (thread.contains (&client2));
    EXPECT_TRUE (thread.contains (&client3));
}

TEST_F (TimeSliceThreadTests, AddNullptrDoesNothing)
{
    TimeSliceThread thread ("TestThread");
    thread.addTimeSliceClient (nullptr);
    EXPECT_EQ (thread.getNumClients(), 0);
}

TEST_F (TimeSliceThreadTests, AddSameClientTwiceDoesNotDuplicate)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client;

    thread.addTimeSliceClient (&client);
    thread.addTimeSliceClient (&client);

    EXPECT_EQ (thread.getNumClients(), 1);
}

TEST_F (TimeSliceThreadTests, RemoveClient)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client;

    thread.addTimeSliceClient (&client);
    EXPECT_EQ (thread.getNumClients(), 1);

    thread.removeTimeSliceClient (&client);
    EXPECT_EQ (thread.getNumClients(), 0);
    EXPECT_FALSE (thread.contains (&client));
}

TEST_F (TimeSliceThreadTests, RemoveClientThatWasNotAdded)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client;

    thread.removeTimeSliceClient (&client);
    EXPECT_EQ (thread.getNumClients(), 0);
}

TEST_F (TimeSliceThreadTests, RemoveAllClients)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1;
    TestTimeSliceClient client2;
    TestTimeSliceClient client3;

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);
    thread.addTimeSliceClient (&client3);
    EXPECT_EQ (thread.getNumClients(), 3);

    thread.removeAllClients();
    EXPECT_EQ (thread.getNumClients(), 0);
}

TEST_F (TimeSliceThreadTests, GetClient)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1;
    TestTimeSliceClient client2;

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);

    EXPECT_EQ (thread.getClient (0), &client1);
    EXPECT_EQ (thread.getClient (1), &client2);
}

TEST_F (TimeSliceThreadTests, GetClientOutOfRange)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client;

    thread.addTimeSliceClient (&client);
    EXPECT_EQ (thread.getClient (10), nullptr);
}

TEST_F (TimeSliceThreadTests, ContainsReturnsFalseForNonAddedClient)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1;
    TestTimeSliceClient client2;

    thread.addTimeSliceClient (&client1);
    EXPECT_TRUE (thread.contains (&client1));
    EXPECT_FALSE (thread.contains (&client2));
}

TEST_F (TimeSliceThreadTests, ThreadCallsUseTimeSlice)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client (50); // Return 50ms

    thread.addTimeSliceClient (&client);
    thread.startThread();

    // Wait for some calls to happen
    Thread::sleep (300);

    thread.stopThread (1000);

    EXPECT_GT (client.getCallCount(), 0);
}

TEST_F (TimeSliceThreadTests, ThreadCallsMultipleClients)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1 (50);
    TestTimeSliceClient client2 (50);
    TestTimeSliceClient client3 (50);

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);
    thread.addTimeSliceClient (&client3);
    thread.startThread();

    // Wait for calls
    Thread::sleep (300);

    thread.stopThread (1000);

    EXPECT_GT (client1.getCallCount(), 0);
    EXPECT_GT (client2.getCallCount(), 0);
    EXPECT_GT (client3.getCallCount(), 0);
}

TEST_F (TimeSliceThreadTests, ClientReturningNegativeIsRemoved)
{
    TimeSliceThread thread ("TestThread");
    SelfRemovingClient client (2); // Remove after 2 calls

    thread.addTimeSliceClient (&client);
    thread.startThread();

    // Wait long enough for multiple calls
    Thread::sleep (500);

    thread.stopThread (1000);

    // Client should have been called at least twice and then removed
    EXPECT_GE (client.getCallCount(), 2);
    EXPECT_FALSE (thread.contains (&client));
}

TEST_F (TimeSliceThreadTests, MoveToFrontOfQueue)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1 (1000); // Long delay
    TestTimeSliceClient client2 (1000); // Long delay

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);
    thread.startThread();

    // Let first client get called
    Thread::sleep (50);

    // Reset counts and move client2 to front
    client1.resetCallCount();
    client2.resetCallCount();

    thread.moveToFrontOfQueue (&client2);

    // Give it time to be called
    Thread::sleep (100);

    thread.stopThread (1000);

    // Client2 should have been called due to moveToFrontOfQueue
    EXPECT_GT (client2.getCallCount(), 0);
}

TEST_F (TimeSliceThreadTests, MoveToFrontOfQueueForNonExistentClient)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1;
    TestTimeSliceClient client2;

    thread.addTimeSliceClient (&client1);
    thread.moveToFrontOfQueue (&client2); // Not added, should not crash

    EXPECT_EQ (thread.getNumClients(), 1);
}

TEST_F (TimeSliceThreadTests, RemoveClientWhileThreadRunning)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1 (50);
    TestTimeSliceClient client2 (50);

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);
    thread.startThread();

    Thread::sleep (100);

    // Remove client while thread is running
    thread.removeTimeSliceClient (&client1);

    EXPECT_EQ (thread.getNumClients(), 1);
    EXPECT_FALSE (thread.contains (&client1));
    EXPECT_TRUE (thread.contains (&client2));

    thread.stopThread (1000);
}

TEST_F (TimeSliceThreadTests, AddClientWithDelay)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client;

    thread.startThread();
    thread.addTimeSliceClient (&client, 200); // 200ms delay

    // Check immediately - might not have been called yet
    Thread::sleep (50);
    int earlyCount = client.getCallCount();

    // Wait for delay to pass
    Thread::sleep (200);
    int laterCount = client.getCallCount();

    thread.stopThread (1000);

    // Later count should be higher than early count
    EXPECT_GE (laterCount, earlyCount);
}

TEST_F (TimeSliceThreadTests, DestructorStopsThread)
{
    TestTimeSliceClient client;

    {
        TimeSliceThread thread ("TestThread");
        thread.addTimeSliceClient (&client);
        thread.startThread();
        Thread::sleep (50);
        // Thread destructor should stop the thread
    }

    // If we get here without hanging, the destructor worked
    EXPECT_TRUE (true);
}

TEST_F (TimeSliceThreadTests, ClientsWithDifferentIntervals)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient fastClient (10);  // 10ms
    TestTimeSliceClient slowClient (200); // 200ms

    thread.addTimeSliceClient (&fastClient);
    thread.addTimeSliceClient (&slowClient);
    thread.startThread();

    Thread::sleep (500);

    thread.stopThread (1000);

    // Fast client should have been called many more times
    EXPECT_GT (fastClient.getCallCount(), slowClient.getCallCount());
}

TEST_F (TimeSliceThreadTests, RemoveAllClientsWhileRunning)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1 (50);
    TestTimeSliceClient client2 (50);
    TestTimeSliceClient client3 (50);

    thread.addTimeSliceClient (&client1);
    thread.addTimeSliceClient (&client2);
    thread.addTimeSliceClient (&client3);
    thread.startThread();

    Thread::sleep (100);

    thread.removeAllClients();

    EXPECT_EQ (thread.getNumClients(), 0);

    thread.stopThread (1000);
}

TEST_F (TimeSliceThreadTests, AddAndRemoveClientsConcurrently)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient client1 (20);
    TestTimeSliceClient client2 (20);
    TestTimeSliceClient client3 (20);

    thread.startThread();

    // Add clients while running
    thread.addTimeSliceClient (&client1);
    Thread::sleep (50);
    thread.addTimeSliceClient (&client2);
    Thread::sleep (50);
    thread.addTimeSliceClient (&client3);

    EXPECT_EQ (thread.getNumClients(), 3);

    // Remove clients while running
    thread.removeTimeSliceClient (&client1);
    EXPECT_EQ (thread.getNumClients(), 2);

    thread.removeTimeSliceClient (&client2);
    EXPECT_EQ (thread.getNumClients(), 1);

    thread.stopThread (1000);
}

TEST_F (TimeSliceThreadTests, ClientReturningZeroIsCalledQuickly)
{
    TimeSliceThread thread ("TestThread");
    TestTimeSliceClient quickClient (0); // Return 0 = call again ASAP

    thread.addTimeSliceClient (&quickClient);
    thread.startThread();

    Thread::sleep (200);

    thread.stopThread (1000);

    // Should be called many times with 0ms interval
    EXPECT_GT (quickClient.getCallCount(), 10);
}

TEST_F (TimeSliceThreadTests, EmptyThreadRuns)
{
    TimeSliceThread thread ("TestThread");
    thread.startThread();
    Thread::sleep (100);
    thread.stopThread (1000);

    // If we get here, empty thread runs without crashing
    EXPECT_TRUE (true);
}
