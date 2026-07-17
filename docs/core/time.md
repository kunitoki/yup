# Time

`yup_core` models absolute time, durations, and high-resolution measurement.

## Time

`Time` is an absolute point in time, stored as milliseconds since the Unix epoch.

```cpp
Time now = Time::getCurrentTime();

int64 ms  = now.toMilliseconds();
String s  = now.toString (/* includeDate */ true, /* includeTime */ true);

Time built  = Time (2026, 0, 1, 12, 0);        // year, month(0-11), day, hour, min
Time parsed = Time::fromISO8601 ("2026-01-01T12:00:00Z");
int64 epoch = Time::currentTimeMillis();        // static, no Time object
```

`Time` supports arithmetic with `RelativeTime`:

```cpp
Time later = now + RelativeTime::seconds (30);
```

## RelativeTime

`RelativeTime` is a duration (a difference between two `Time` points), stored as
seconds. It has fluent constructors and converters for each unit.

```cpp
RelativeTime d = RelativeTime::minutes (5);

double secs = d.inSeconds();
double mins = d.inMinutes();
String desc = d.getDescription();     // e.g. "5 mins"

RelativeTime elapsed = laterTime - earlierTime;
```

Factory helpers: `milliseconds`, `seconds`, `minutes`, `hours`, `days`, `weeks`.

## PerformanceCounter

`PerformanceCounter` measures and averages the time taken by a repeated block of
code - a quick way to profile hot paths.

```cpp
PerformanceCounter pc ("render", /* runsPerPrint */ 100);

for (;;)
{
    pc.start();
    renderFrame();
    pc.stop();          // prints stats every 100 runs
}
```

For coarse wall-clock measurement, `Time::getMillisecondCounterHiRes()` returns a
high-resolution millisecond timer.

```{seealso}
For frame-level GPU/paint profiling, see
[Component paint profiling](../ui/component-profiling.md). For real-time
timers on a dedicated thread, see `HighResolutionTimer` in the
[Multithreading](../multithreading/index.md) area.
```

## See also

- [Maths](maths.md) - numeric helpers.
- [Multithreading](../multithreading/index.md) - timers and scheduling.
