# Thread pools

A `ThreadPool` runs queued jobs on a fixed set of worker threads. It is the
right tool for many independent, short-to-medium tasks - decoding thumbnails,
analysing files, rendering previews - where creating a [`Thread`](threads.md)
per task would cost more than the task itself.

## Creating a pool

```cpp
yup::ThreadPool pool;   // one thread per CPU core, named "Pool"

yup::ThreadPool workers (yup::ThreadPoolOptions()
                             .withThreadName ("Thumbnailer")
                             .withNumberOfThreads (4)
                             .withDesiredThreadPriority (yup::Thread::Priority::low));
```

| Option | Default |
| --- | --- |
| `withThreadName` | `"Pool"` |
| `withNumberOfThreads` | The number of CPU cores |
| `withThreadStackSizeBytes` | The platform default |
| `withDesiredThreadPriority` | `Thread::Priority::normal` |

`ThreadPool (numThreads, stackSize, priority)` is a shorter form of the same.

A pool is usually long-lived: create it once, alongside whatever owns the work,
and keep it rather than creating one per batch.

## Adding jobs as lambdas

The simplest job is a function run once:

```cpp
pool.addJob ([file, this]
{
    auto image = decodeThumbnail (file);
    // ...
});
```

A job that works in slices returns a `ThreadPoolJob::JobStatus` instead. It is
run again until it returns `jobHasFinished`, and between slices the pool can
run other jobs, which keeps one long task from starving the rest:

```cpp
pool.addJob (std::function<yup::ThreadPoolJob::JobStatus()> ([state]
{
    if (state->processNextChunk())
        return yup::ThreadPoolJob::jobNeedsRunningAgain;

    return yup::ThreadPoolJob::jobHasFinished;
}));
```

Jobs start in the order they were added, on whichever thread is free.

## Job classes

Subclass `ThreadPoolJob` when a job has state, a name, or must be stopped from
outside:

```cpp
class AnalyseJob : public yup::ThreadPoolJob
{
public:
    explicit AnalyseJob (yup::File fileToAnalyse)
        : ThreadPoolJob ("Analyse " + fileToAnalyse.getFileName())
        , file (std::move (fileToAnalyse))
    {
    }

    JobStatus runJob() override
    {
        for (int block = 0; block < numBlocks; ++block)
        {
            if (shouldExit())
                return jobHasFinished;

            analyseBlock (block);
        }

        return jobHasFinished;
    }

private:
    yup::File file;
    int numBlocks = 0;
};

pool.addJob (new AnalyseJob (file), true);
```

The second argument of `addJob` decides who owns the job:

- `true` - the pool takes ownership and deletes the job when it finishes or is
  removed.
- `false` - you own it, and it must stay alive until it has finished or been
  removed from the pool.

Inside `runJob()`, check `shouldExit()` regularly and return as soon as it is
true. `setJobName` renames the job while it runs, and `isRunning()` tells
whether it is currently executing.

## Removing and waiting for jobs

```cpp
// Remove one job: if it is running, ask it to exit and wait up to 1 s.
pool.removeJob (job, true, 1000);

// Remove everything, interrupting running jobs.
pool.removeAllJobs (true, 2000);

// Wait for a job without removing it.
pool.waitForJobToFinish (job, 5000);
```

A job that has not started is simply dropped. A running job cannot be
interrupted by force; with `interruptIfRunning` set it is asked to exit (its
`shouldExit()` becomes true) and the call waits for it to return. All three
return false if the timeout passed first.

`removeAllJobs` takes an optional `ThreadPool::JobSelector` to remove only the
jobs it selects - for example only the ones belonging to a closed document.

The pool's destructor removes all jobs and waits for the running ones. To
bound that wait, call `removeAllJobs` with a timeout yourself before the pool
is destroyed - typically first thing in the owner's destructor, so jobs holding
a pointer to the owner are gone before its members are.

## Inspecting the pool

| Call | Returns |
| --- | --- |
| `getNumJobs()` | Jobs queued or running. |
| `getNumThreads()` | Worker threads. |
| `getJob (index)` | A queued or running job, or nullptr. |
| `contains (job)` | Whether the job is in the pool. |
| `isJobRunning (job)` | Whether it is executing right now. |
| `moveJobToFront (job)` | Nothing - it moves a waiting job to the head of the queue. |

## Getting results back

Jobs run on pool threads, so results that touch the UI must be posted to the
message thread:

```cpp
pool.addJob ([file, weakThis = yup::WeakReference<yup::Component> (this)]
{
    auto image = decodeThumbnail (file);

    yup::MessageManager::callAsync ([weakThis, image]
    {
        if (auto* view = dynamic_cast<ThumbnailView*> (weakThis.get()))
            view->setImage (image);
    });
});
```

The `WeakReference` makes the callback harmless if the component was deleted
while the job was running. See [the message thread](message-thread.md) for the
other ways of getting back to it.

To wait for a batch of lambda jobs, count them down with a
[`CountDownLatch`](synchronization.md#waiting-for-a-batch-of-work):

```cpp
yup::CountDownLatch done (static_cast<int> (files.size()));

for (const auto& file : files)
    pool.addJob ([&done, file] { analyse (file); done.countDown(); });

done.wait();
```

## Cancelling a batch

To stop a group of jobs that are not `ThreadPoolJob` subclasses, give them a
shared [`CancelToken`](cancellation.md):

```cpp
yup::CancelTokenSource cancelSource;
auto token = cancelSource.getToken();

for (const auto& file : files)
{
    pool.addJob ([token, file]
    {
        if (token.wasCancelled())
            return;

        analyse (file, token);
    });
}

// later, from any thread
cancelSource.cancel();
```

## Related pages

- [Threads](threads.md) - a dedicated thread for long-lived work.
- [Synchronization](synchronization.md) - sharing data between jobs.
- [Cancellation](cancellation.md) - cancellation tokens.
