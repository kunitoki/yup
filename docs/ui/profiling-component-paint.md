# Measuring Component Paint Speed

This guide explains how to use YUP's built-in paint profiling system to measure the rendering cost of individual components and identify bottlenecks in your UI.

## Overview

YUP's `PaintProfiler` is a process-wide singleton that records per-component paint timings broken into four categories:

| Category | Meaning |
|---|---|
| **self** | Time inside the component's own `paint()` callback |
| **children** | Time spent painting all direct and indirect children |
| **framework** | Time for framework bookkeeping (clip setup, transform, etc.) |
| **total** | Full elapsed time for the complete paint pass |

Samples are stored in a per-component ring buffer (default capacity: 300 frames). Statistical summaries - min, max, mean, p50, p95, p99 - are computed on demand from the stored samples.

---

## Step 1 - Enable the Build Flag

Paint profiling is compiled out by default. Add the preprocessor definition to your target in CMake:

```cmake
yup_standalone_app (
    TARGET_NAME MyApp
    DEFINITIONS
        YUP_ENABLE_COMPONENT_PAINT_PROFILING=1
    MODULES
        yup::yup_gui
        # ... other modules
)
```

Without this flag every profiling call is a no-op and produces no overhead in release builds.

---

## Step 2 - Start a Session

The recommended API is `PaintProfiler::startSession()`, which returns a `ScopedSession`. The session enables profiling on the entire component subtree rooted at the component you pass in, and disables it automatically when the handle is destroyed.

```cpp
#if YUP_ENABLE_COMPONENT_PAINT_PROFILING
    std::unique_ptr<yup::PaintProfiler::ScopedSession> profileSession;
#endif

// In your component constructor or initialisation:
#if YUP_ENABLE_COMPONENT_PAINT_PROFILING
    profileSession = yup::PaintProfiler::getInstance().startSession (*this);
#endif
```

Guard every profiling call with `#if YUP_ENABLE_COMPONENT_PAINT_PROFILING` so the code compiles and runs correctly in builds without the flag.

### Session options

Pass a `PaintProfileOptions` struct to control the session's behaviour:

```cpp
yup::PaintProfileOptions options;
options.sampleCapacity             = 600;     // retain 600 frames of history
options.minimumSampleMicros        = 50.0;    // discard samples shorter than 50 µs
options.includeBounds              = true;    // record component bounds per sample
options.includeRepaintArea         = true;    // record dirty rect per sample
options.includeInvisibleComponents = false;   // should invisible components be included
options.recordSkippedSelfPaint     = true;    // track child-only cost even with no paint()

profileSession = yup::PaintProfiler::getInstance().startSession (*this, options);
```

---

## Step 3 - Take Snapshots

A `Snapshot` is an immutable, point-in-time view of every registered component's statistics. Call `createSnapshot()` on the session at whatever rate you need - a 10 Hz timer is typical for a dashboard display.

```cpp
void timerCallback() override
{
#if YUP_ENABLE_COMPONENT_PAINT_PROFILING
    if (profileSession == nullptr || profileSession->isPaused())
        return;

    auto snap = profileSession->createSnapshot (yup::PaintProfileTimeKind::total, 32);
    // use snap ...
#endif
}
```

`createSnapshot` accepts:
- **sortBy** - which time kind determines the descending sort order (default `total`).
- **histogramBuckets** - bucket count for the global frame histogram (default 32).

The returned `Snapshot` contains:

```cpp
struct Snapshot
{
    uint64 frameIndex;                      // frame counter at snapshot time
    std::vector<ComponentEntry> components; // one entry per registered component
    PaintProfileSummary globalFrameTotal;   // per-frame total across all components
    PaintProfileHistogram globalFrameHistogram;
};
```

Each `ComponentEntry` gives you:

```cpp
struct ComponentEntry
{
    String              name;       // component title at snapshot time
    PaintProfileStats*  stats;      // live pointer - may be stale after destruction
    PaintProfileSummary self;
    PaintProfileSummary children;
    PaintProfileSummary framework;
    PaintProfileSummary total;
};
```

A `PaintProfileSummary` exposes: `lastMicros`, `minMicros`, `maxMicros`, `meanMicros`, `p50Micros`, `p95Micros`, `p99Micros`, and `sampleCount`.

---

## Step 4 - Interpret the Data

### Performance thresholds

| Range | Meaning |
|---|---|
| < 500 µs | Normal - no action needed |
| 500 µs – 2 ms | Warm - worth investigating if sustained |
| > 2 ms | Hot - likely causing dropped frames at 60 Hz |

At 60 Hz, the full frame budget is ~16.7 ms. A single component that consistently takes > 2 ms for its own paint is a significant contributor.

### Reading a summary

```cpp
const auto& entry = snap.components[0];

// Is the component itself expensive, or is it due to children?
double selfCost     = entry.self.p95Micros;
double childrenCost = entry.children.p95Micros;

// p95 is the most useful signal: it captures spikes while ignoring outliers
double worstNormal  = entry.total.p95Micros;
```

Use **p95** as the primary signal. `maxMicros` is useful for catching spikes, but a single GC or OS event can inflate it. `meanMicros` smooths over spikes that matter.

### Log a snapshot to the console

```cpp
#if YUP_ENABLE_COMPONENT_PAINT_PROFILING
auto snap = profileSession->createSnapshot (yup::PaintProfileTimeKind::total, 32);

yup::Logger::outputDebugString ("Paint profile - frame " + yup::String (snap.frameIndex));
yup::Logger::outputDebugString (yup::String::formatted ("%-30s %9s %9s %9s %9s", "Widget", "last", "mean", "p95", "max"));
for (const auto& entry : snap.components)
{
    yup::Logger::outputDebugString (
        yup::String::formatted ("%-30s %7.2f ms %7.2f ms %7.2f ms %7.2f ms",
            entry.name.toRawUTF8(),
            entry.total.lastMicros  / 1000.0,
            entry.total.meanMicros  / 1000.0,
            entry.total.p95Micros   / 1000.0,
            entry.total.maxMicros   / 1000.0));
}
#endif
```

---

## Step 5 - Inspect Raw Samples

When you need more detail than aggregated statistics, pull the ring buffer directly:

```cpp
#if YUP_ENABLE_COMPONENT_PAINT_PROFILING
if (auto* stats = myComponent.getPaintProfileStats())
{
    const auto samples = stats->copySamples(); // chronological order, oldest first

    for (const auto& s : samples)
    {
        // s.selfMicros, s.childrenMicros, s.frameworkMicros, s.totalMicros
        // s.frameIndex, s.paintIndex
        // s.componentBounds, s.repaintArea
        // s.renderContinuous, s.selfPaintSkipped
    }

    auto summary   = stats->summarize (yup::PaintProfileTimeKind::total);
    auto histogram = stats->createHistogram (yup::PaintProfileTimeKind::self, 32);
}
#endif
```

`copySamples()` returns samples in chronological order regardless of the ring-buffer write position.

---

## Step 6 - Reset and Pause

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

## Tips for Accurate Measurements

- **Warm up first.** Discard the first second of data after a `reset()` - the JIT-equivalent effects (GPU shader compilation, OS scheduling) inflate early samples.
- **Isolate one change at a time.** Use `setPaused(true)` on the session while switching components so the history stays clean.
- **Prefer p95 over max.** A single OS preemption can produce a multi-millisecond outlier that distorts `maxMicros`.
- **Separate self from children.** A high `total` with a low `self` means the component's own paint is fine but its children are costly - recurse down the tree.
- **Check `renderContinuous`.** Samples with `renderContinuous = true` in the ring buffer indicate the component is requesting continuous repaints (animation loops). Every such component adds baseline CPU pressure even when nothing is animating visually.
- **Use `setOpaque(true)`.** Opaque components allow the renderer to skip painting the background beneath them. It is one of the cheapest paint optimisations available.
