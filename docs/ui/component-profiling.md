# Measuring Component Paint Speed

This guide explains how to use YUP's built-in paint profiling system to measure
the rendering cost of individual components and identify bottlenecks in your UI.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Overview

`PaintProfiler` is a process-wide singleton that records per-component paint
timings broken into four categories:

| Category | Meaning |
|---|---|
| **self** | Time inside the component's own `paint()` callback |
| **children** | Time spent painting all direct and indirect children |
| **framework** | Framework bookkeeping (clip setup, transform, etc.) |
| **total** | Full elapsed time for the complete paint pass |

Samples are stored in a per-component ring buffer. Statistical summaries — min,
max, mean, p50, p95, p99 — are computed on demand from the stored samples.

Profiling is always available; no compile-time flag is required. The profiler
works by attaching as a `ComponentListener` so it receives
`componentPaintCompleted` callbacks with raw tick counts from
`ComponentPaintMetrics`. It converts ticks to microseconds automatically.

---

## Step 1 — Start a Session

The recommended API is `PaintProfiler::startSession()`, which returns a
`ScopedSession`. The session enables profiling on the entire component subtree
rooted at the component you pass in, and disables it automatically when the
handle is destroyed.

```cpp
class MyApp : public Component
{
public:
    MyApp()
    {
        auto& profiler = PaintProfiler::getInstance();
        profileSession = profiler.startSession (*this);
    }

    void dumpStats()
    {
        if (profileSession != nullptr && ! profileSession->isPaused())
        {
            auto snap = profileSession->createSnapshot();
            // ... inspect snap ...
        }
    }

private:
    std::unique_ptr<PaintProfiler::ScopedSession> profileSession;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyApp)
};
```

### Session options

Pass a `PaintProfileOptions` struct to control the session's behaviour:

```cpp
PaintProfileOptions options;
options.sampleCapacity             = 600;     // retain 600 frames of history
options.minimumSampleMicros        = 50.0;    // discard samples shorter than 50 µs
options.includeBounds              = true;    // record component bounds per sample
options.includeRepaintArea         = true;    // record dirty rect per sample
options.includeInvisibleComponents = false;   // skip invisible components
options.recordSkippedSelfPaint     = true;    // track child-only cost with no own paint()

profileSession = PaintProfiler::getInstance().startSession (*this, options);
```

---

## Step 2 — Take Snapshots

A `Snapshot` is an immutable, point-in-time view of every registered component's
statistics. Call `createSnapshot()` at whatever rate you need — a 10 Hz timer is
typical for a dashboard display.

```cpp
void onTimer()
{
    if (profileSession == nullptr || profileSession->isPaused())
        return;

    auto snap = profileSession->createSnapshot (PaintProfileTimeKind::total, 32);
    // use snap ...
}
```

`createSnapshot` accepts:

- **sortBy** — which time kind determines the descending sort order (default
  `total`).
- **histogramBuckets** — bucket count for the global frame histogram (default
  32).

### Snapshot contents

```cpp
struct Snapshot
{
    uint64 frameIndex;                      // frame counter at snapshot time
    std::vector<ComponentEntry> components; // one entry per profiled component
    PaintProfileSummary globalFrameTotal;   // per-frame total across all components
    PaintProfileHistogram globalFrameHistogram;
};
```

Each `ComponentEntry` gives you:

```cpp
struct ComponentEntry
{
    String              name;       // component title at snapshot time
    PaintProfileStats*  stats;      // live pointer — may be stale after destruction
    PaintProfileSummary self;
    PaintProfileSummary children;
    PaintProfileSummary framework;
    PaintProfileSummary total;
};
```

A `PaintProfileSummary` exposes: `lastMicros`, `minMicros`, `maxMicros`,
`meanMicros`, `p50Micros`, `p95Micros`, `p99Micros`, and `sampleCount`.

---

## Step 3 — Interpret the Data

### Performance thresholds

| Range | Meaning |
|---|---|
| < 500 µs | Normal — no action needed |
| 500 µs – 2 ms | Warm — worth investigating if sustained |
| > 2 ms | Hot — likely causing dropped frames at 60 Hz |

At 60 Hz, the full frame budget is ~16.7 ms. A single component that
consistently takes > 2 ms for its own paint is a significant contributor.

### Reading a summary

```cpp
const auto& entry = snap.components[0];

// Is the component itself expensive, or is it due to children?
double selfCost     = entry.self.p95Micros;
double childrenCost = entry.children.p95Micros;

// p95 is the most useful signal: it captures spikes while ignoring outliers
double worstNormal  = entry.total.p95Micros;
```

Use **p95** as the primary signal. `maxMicros` is useful for catching spikes,
but a single GC or OS event can inflate it. `meanMicros` smoothes over spikes
that matter.

### Log a snapshot to the console

```cpp
auto snap = profileSession->createSnapshot (PaintProfileTimeKind::total, 32);

Logger::outputDebugString ("Paint profile — frame " + String (snap.frameIndex));
Logger::outputDebugString (
    String::formatted ("%-30s %9s %9s %9s %9s", "Widget", "last", "mean", "p95", "max"));

for (const auto& entry : snap.components)
{
    Logger::outputDebugString (
        String::formatted ("%-30s %7.2f ms %7.2f ms %7.2f ms %7.2f ms",
            entry.name.toRawUTF8(),
            entry.total.lastMicros  / 1000.0,
            entry.total.meanMicros  / 1000.0,
            entry.total.p95Micros   / 1000.0,
            entry.total.maxMicros   / 1000.0));
}
```

---

## Step 4 — Inspect Raw Samples

When you need more detail than aggregated statistics, pull the ring buffer
directly via `PaintProfiler::getStatsForComponent()`:

```cpp
auto* stats = PaintProfiler::getInstance().getStatsForComponent (myComponent);
if (stats != nullptr)
{
    const auto samples = stats->copySamples(); // chronological order, oldest first

    for (const auto& s : samples)
    {
        // s.selfMicros, s.childrenMicros, s.frameworkMicros, s.totalMicros
        // s.frameIndex, s.paintIndex
        // s.componentBounds, s.repaintArea
        // s.renderContinuous, s.selfPaintSkipped
    }

    auto summary   = stats->summarize (PaintProfileTimeKind::total);
    auto histogram = stats->createHistogram (PaintProfileTimeKind::self, 32);
}
```

`copySamples()` returns samples in chronological order regardless of the
ring-buffer write position.

---

## Step 5 — Reset and Pause

Clear accumulated history after a layout change or before a timed benchmark:

```cpp
profileSession->reset(); // clears all ring buffers for this session's components
```

Temporarily suppress recording without destroying the session:

```cpp
profileSession->setPaused (true);
// ... do something that should not be measured ...
profileSession->setPaused (false);
```

---

## Disabling profiling on a component

By default, profiling is active on any component that has a
`ComponentListener` attached. To prevent a specific component from being
measured — even when a `ScopedSession` or `enableComponent()` would otherwise
include it — use `setPaintProfilingDisabled(true)`:

```cpp
myBackground.setPaintProfilingDisabled (true);  // never profiled
myBackground.setPaintProfilingDisabled (false); // allow profiling again

bool disabled = myBackground.isPaintProfilingDisabled();
```

This is useful for excluding trivial components that would add noise to the
profile data.

---

## Manual component management

If you don't want an RAII session, manage individual components directly:

```cpp
auto& profiler = PaintProfiler::getInstance();

// Enable a single component
profiler.enableComponent (myWidget, PaintProfileOptions {});

// Enable an entire subtree (component + all children)
profiler.enableSubtree (rootPanel, PaintProfileOptions {});

// Query state
bool enabled = profiler.isComponentEnabled (myWidget);
auto* stats  = profiler.getStatsForComponent (myWidget);

// Reset a single component's ring buffer
profiler.resetComponent (myWidget);

// Reset the whole subtree
profiler.resetSubtree (rootPanel);

// Disable
profiler.disableComponent (myWidget);
profiler.disableSubtree (rootPanel);

// Reset everything across the whole process
profiler.resetAll();

// Create a global snapshot (all enabled components in the process)
auto snap = profiler.createSnapshot (PaintProfileTimeKind::total, 32);

// Global enable/disable for the entire profiler (affects all components)
profiler.setEnabled (false);
```

---

## Tips for Accurate Measurements

- **Warm up first.** Discard the first second of data after a `reset()` — the
  JIT-equivalent effects (GPU shader compilation, OS scheduling) inflate early
  samples.
- **Isolate one change at a time.** Use `setPaused(true)` on the session while
  switching components so the history stays clean.
- **Prefer p95 over max.** A single OS preemption can produce a multi-millisecond
  outlier that distorts `maxMicros`.
- **Separate self from children.** A high `total` with a low `self` means the
  component's own paint is fine but its children are costly — recurse down the
  tree.
- **Check `renderContinuous`.** Samples with `renderContinuous = true` in the
  ring buffer indicate the component is requesting continuous repaints.
  Every such component adds baseline CPU pressure even when nothing is animating
  visually.
- **Use `setOpaque(true)`.** Opaque components allow the renderer to skip
  painting the background beneath them. It is one of the cheapest paint
  optimisations available.

---

## Related

- [Component basics](component-basics.md) — `ComponentListener` and
  `setPaintProfilingDisabled`
- [Component caching](component-caching.md) — reduce paint cost via GPU texture
  caching
