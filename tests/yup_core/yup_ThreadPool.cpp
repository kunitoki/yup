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
// Test job that can be controlled for timing-sensitive tests
class TestJob : public ThreadPoolJob
{
public:
    TestJob (const String& name)
        : ThreadPoolJob (name)
    {
    }

    JobStatus runJob() override
    {
        ++runCount;

        while (! shouldExit() && ! shouldFinish.load())
            Thread::sleep (10);

        return jobHasFinished;
    }

    void finish()
    {
        shouldFinish = true;
    }

    int getRunCount() const { return runCount.load(); }

private:
    std::atomic<int> runCount { 0 };
    std::atomic<bool> shouldFinish { false };
};

// Job that runs multiple times
class RepeatingJob : public ThreadPoolJob
{
public:
    RepeatingJob (const String& name, int maxRuns = 3)
        : ThreadPoolJob (name)
        , maxRunCount (maxRuns)
    {
    }

    JobStatus runJob() override
    {
        ++runCount;
        Thread::sleep (10); // Small delay to simulate work

        if (runCount >= maxRunCount || shouldExit())
            return jobHasFinished;

        return jobNeedsRunningAgain;
    }

    int getRunCount() const { return runCount.load(); }

private:
    std::atomic<int> runCount { 0 };
    int maxRunCount;
};

// Quick job that finishes immediately
class QuickJob : public ThreadPoolJob
{
public:
    QuickJob (const String& name)
        : ThreadPoolJob (name)
    {
    }

    JobStatus runJob() override
    {
        hasRun = true;
        return jobHasFinished;
    }

    bool hasRunJob() const { return hasRun.load(); }

private:
    std::atomic<bool> hasRun { false };
};

// Listener for testing job exit signals
class TestListener : public Thread::Listener
{
public:
    void exitSignalSent() override
    {
        ++callCount;
    }

    int getCallCount() const { return callCount.load(); }

private:
    std::atomic<int> callCount { 0 };
};
} // namespace

class ThreadPoolTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F (ThreadPoolTests, CreatePool)
{
    ThreadPool pool (2);
    EXPECT_EQ (pool.getNumThreads(), 2);
    EXPECT_EQ (pool.getNumJobs(), 0);
}

TEST_F (ThreadPoolTests, CreatePoolWithOptions)
{
    ThreadPoolOptions options = ThreadPoolOptions {}
                                    .withNumberOfThreads (3)
                                    .withThreadName ("TestPool");

    ThreadPool pool (options);
    EXPECT_EQ (pool.getNumThreads(), 3);
}

TEST_F (ThreadPoolTests, AddJobAndWaitForCompletion)
{
    ThreadPool pool (2);
    QuickJob job ("QuickJob");

    pool.addJob (&job, false);

    // Wait for job to finish
    bool finished = pool.waitForJobToFinish (&job, 2000);
    EXPECT_TRUE (finished);
    EXPECT_TRUE (job.hasRunJob());
    EXPECT_FALSE (pool.contains (&job));
}

TEST_F (ThreadPoolTests, AddMultipleJobs)
{
    ThreadPool pool (2);
    QuickJob job1 ("Job1");
    QuickJob job2 ("Job2");
    QuickJob job3 ("Job3");

    pool.addJob (&job1, false);
    pool.addJob (&job2, false);
    pool.addJob (&job3, false);

    // Wait for all jobs to finish
    pool.waitForJobToFinish (&job1, 2000);
    pool.waitForJobToFinish (&job2, 2000);
    pool.waitForJobToFinish (&job3, 2000);

    EXPECT_TRUE (job1.hasRunJob());
    EXPECT_TRUE (job2.hasRunJob());
    EXPECT_TRUE (job3.hasRunJob());
}

TEST_F (ThreadPoolTests, GetNumJobs)
{
    ThreadPool pool (1);
    TestJob job1 ("Job1");
    TestJob job2 ("Job2");

    pool.addJob (&job1, false);

    // Give it time to start running
    Thread::sleep (50);

    pool.addJob (&job2, false);

    // Should have at least 1 job (might be 2 if first hasn't finished)
    int numJobs = pool.getNumJobs();
    EXPECT_GE (numJobs, 1);
    EXPECT_LE (numJobs, 2);

    // Finish jobs
    job1.finish();
    job2.finish();
    pool.waitForJobToFinish (&job1, 2000);
    pool.waitForJobToFinish (&job2, 2000);

    EXPECT_EQ (pool.getNumJobs(), 0);
}

TEST_F (ThreadPoolTests, GetJob)
{
    ThreadPool pool (1);
    TestJob job1 ("Job1");
    TestJob job2 ("Job2");

    pool.addJob (&job1, false);
    pool.addJob (&job2, false);

    // Give jobs time to be queued
    Thread::sleep (50);

    // Get jobs from pool
    ThreadPoolJob* retrievedJob = pool.getJob (0);
    EXPECT_NE (retrievedJob, nullptr);

    // Finish jobs
    job1.finish();
    job2.finish();
    pool.waitForJobToFinish (&job1, 2000);
    pool.waitForJobToFinish (&job2, 2000);
}

TEST_F (ThreadPoolTests, GetJobOutOfRange)
{
    ThreadPool pool (2);

    ThreadPoolJob* job = pool.getJob (10);
    EXPECT_EQ (job, nullptr);
}

TEST_F (ThreadPoolTests, ContainsJob)
{
    ThreadPool pool (1);
    TestJob job1 ("Job1");
    TestJob job2 ("Job2");

    EXPECT_FALSE (pool.contains (&job1));

    pool.addJob (&job1, false);

    // Give it time to be added
    Thread::sleep (50);

    EXPECT_TRUE (pool.contains (&job1));
    EXPECT_FALSE (pool.contains (&job2));

    job1.finish();
    pool.waitForJobToFinish (&job1, 2000);

    EXPECT_FALSE (pool.contains (&job1));
}

TEST_F (ThreadPoolTests, RemoveJob)
{
    ThreadPool pool (1);
    TestJob job ("Job");

    pool.addJob (&job, false);

    // Give job time to start
    Thread::sleep (50);

    EXPECT_TRUE (pool.contains (&job));

    // Remove with interrupt
    bool removed = pool.removeJob (&job, true, 2000);
    EXPECT_TRUE (removed);
    EXPECT_FALSE (pool.contains (&job));
}

TEST_F (ThreadPoolTests, RemoveJobNotInPool)
{
    ThreadPool pool (2);
    TestJob job ("Job");

    bool removed = pool.removeJob (&job, false, 1000);
    EXPECT_TRUE (removed); // Returns true because job isn't in pool
}

TEST_F (ThreadPoolTests, WaitForJobToFinishTimeout)
{
    ThreadPool pool (1);
    TestJob job ("Job");

    pool.addJob (&job, false);

    // Wait with very short timeout - should timeout
    bool finished = pool.waitForJobToFinish (&job, 50);
    EXPECT_FALSE (finished);

    // Now finish the job properly
    job.finish();
    finished = pool.waitForJobToFinish (&job, 2000);
    EXPECT_TRUE (finished);
}

TEST_F (ThreadPoolTests, SetJobName)
{
    TestJob job ("OriginalName");

    EXPECT_EQ (job.getJobName(), "OriginalName");

    job.setJobName ("NewName");
    EXPECT_EQ (job.getJobName(), "NewName");
}

TEST_F (ThreadPoolTests, SignalJobShouldExit)
{
    ThreadPool pool (1);
    TestJob job ("Job");

    pool.addJob (&job, false);

    // Give job time to start
    Thread::sleep (50);

    EXPECT_FALSE (job.shouldExit());

    job.signalJobShouldExit();
    EXPECT_TRUE (job.shouldExit());

    // Job should finish quickly now
    bool finished = pool.waitForJobToFinish (&job, 2000);
    EXPECT_TRUE (finished);
}

TEST_F (ThreadPoolTests, AddAndRemoveListener)
{
    TestJob job ("Job");
    TestListener listener;

    job.addListener (&listener);

    EXPECT_EQ (listener.getCallCount(), 0);

    job.signalJobShouldExit();

    // Give listener time to be notified
    Thread::sleep (50);

    EXPECT_GT (listener.getCallCount(), 0);

    job.removeListener (&listener);
}

TEST_F (ThreadPoolTests, MultipleListeners)
{
    TestJob job ("Job");
    TestListener listener1;
    TestListener listener2;

    job.addListener (&listener1);
    job.addListener (&listener2);

    job.signalJobShouldExit();

    // Give listeners time to be notified
    Thread::sleep (50);

    EXPECT_GT (listener1.getCallCount(), 0);
    EXPECT_GT (listener2.getCallCount(), 0);

    job.removeListener (&listener1);
    job.removeListener (&listener2);
}

TEST_F (ThreadPoolTests, GetCurrentThreadPoolJob)
{
    ThreadPool pool (1);
    ThreadPoolJob* currentJob = nullptr;
    std::atomic<bool> hasRun { false };

    auto lambda = [&currentJob, &hasRun]()
    {
        currentJob = ThreadPoolJob::getCurrentThreadPoolJob();
        hasRun = true;
    };

    pool.addJob (lambda);

    // Wait for job to run
    while (! hasRun.load())
        Thread::sleep (10);

    // Give it extra time to complete
    Thread::sleep (100);

    EXPECT_NE (currentJob, nullptr);
}

TEST_F (ThreadPoolTests, GetCurrentThreadPoolJobFromMainThread)
{
    // Should return nullptr when called from non-pool thread
    ThreadPoolJob* job = ThreadPoolJob::getCurrentThreadPoolJob();
    EXPECT_EQ (job, nullptr);
}

TEST_F (ThreadPoolTests, AddLambdaJobReturningStatus)
{
    ThreadPool pool (2);
    std::atomic<int> runCount { 0 };
    std::atomic<bool> shouldFinish { false };

    auto lambda = [&runCount, &shouldFinish]() -> ThreadPoolJob::JobStatus
    {
        ++runCount;
        Thread::sleep (10);

        if (shouldFinish.load() || runCount >= 3)
            return ThreadPoolJob::jobHasFinished;

        return ThreadPoolJob::jobNeedsRunningAgain;
    };

    pool.addJob (std::function<ThreadPoolJob::JobStatus()> (lambda));

    // Wait for multiple runs
    while (runCount.load() < 3)
        Thread::sleep (20);

    // Give it time to finish
    Thread::sleep (200);

    EXPECT_GE (runCount.load(), 3);
}

TEST_F (ThreadPoolTests, AddLambdaJobReturningVoid)
{
    ThreadPool pool (2);
    std::atomic<bool> hasRun { false };

    auto lambda = [&hasRun]()
    {
        hasRun = true;
    };

    pool.addJob (lambda);

    // Wait for job to run
    while (! hasRun.load())
        Thread::sleep (10);

    EXPECT_TRUE (hasRun.load());
}

TEST_F (ThreadPoolTests, RepeatingJob)
{
    ThreadPool pool (1);
    RepeatingJob job ("RepeatingJob", 3);

    pool.addJob (&job, false);

    // Wait for job to finish all runs
    bool finished = pool.waitForJobToFinish (&job, 3000);
    EXPECT_TRUE (finished);
    EXPECT_EQ (job.getRunCount(), 3);
}

TEST_F (ThreadPoolTests, ConcurrentJobsThreadSafety)
{
    ThreadPool pool (4);
    std::atomic<int> counter { 0 };

    // Add multiple jobs that increment a shared counter
    for (int i = 0; i < 20; ++i)
    {
        pool.addJob ([&counter]()
        {
            for (int j = 0; j < 100; ++j)
                ++counter;
        });
    }

    // Wait for all jobs to complete
    Thread::sleep (1000);

    // Should have incremented 2000 times total
    EXPECT_EQ (counter.load(), 2000);
}

TEST_F (ThreadPoolTests, RemoveJobWhileRunning)
{
    ThreadPool pool (1);
    TestJob job ("LongRunningJob");

    pool.addJob (&job, false);

    // Give job time to start running
    Thread::sleep (100);

    EXPECT_TRUE (job.isRunning());

    // Remove with interrupt and reasonable timeout
    bool removed = pool.removeJob (&job, true, 2000);
    EXPECT_TRUE (removed);
    EXPECT_FALSE (pool.contains (&job));
}

TEST_F (ThreadPoolTests, AddJobAfterRemoval)
{
    ThreadPool pool (1);
    QuickJob job1 ("Job1");
    QuickJob job2 ("Job2");

    pool.addJob (&job1, false);
    pool.waitForJobToFinish (&job1, 2000);

    EXPECT_FALSE (pool.contains (&job1));

    pool.addJob (&job2, false);
    pool.waitForJobToFinish (&job2, 2000);

    EXPECT_TRUE (job1.hasRunJob());
    EXPECT_TRUE (job2.hasRunJob());
}

TEST_F (ThreadPoolTests, StressTestManyJobs)
{
    ThreadPool pool (4);
    std::atomic<int> completedJobs { 0 };

    // Add many quick jobs
    for (int i = 0; i < 50; ++i)
    {
        pool.addJob ([&completedJobs]()
        {
            Thread::sleep (5);
            ++completedJobs;
        });
    }

    // Wait for all to complete
    int timeout = 5000;
    while (completedJobs.load() < 50 && timeout > 0)
    {
        Thread::sleep (50);
        timeout -= 50;
    }

    EXPECT_EQ (completedJobs.load(), 50);
}
