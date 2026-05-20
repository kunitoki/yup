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

namespace yup
{

//==============================================================================

/** Process-wide singleton that coordinates paint profiling across all components.

    PaintProfiler maintains a registry of components that have profiling enabled,
    provides frame-level bookkeeping (beginFrame / endFrame), and exposes snapshot
    and histogram queries over the collected data.

    The recommended entry point for callers is startSession(), which returns a
    RAII ScopedSession handle that enables profiling on a component subtree and
    automatically disables it when the handle is destroyed.
*/
class PaintProfiler
{
public:
    //==============================================================================

    /** A record describing one profiled component within a snapshot. */
    struct ComponentEntry
    {
        /** Pointer to the profiled component (may be stale after the snapshot is taken). */
        Component* component = nullptr;

        /** Display name of the component at snapshot time. */
        String name;

        /** Pointer to the stats object owned by the component (may be stale). */
        PaintProfileStats* stats = nullptr;

        /** Statistical summary for the component's own paint time. */
        PaintProfileSummary self;

        /** Statistical summary for the time spent painting this component's children. */
        PaintProfileSummary children;

        /** Statistical summary for framework bookkeeping time. */
        PaintProfileSummary framework;

        /** Statistical summary for the total paint time. */
        PaintProfileSummary total;
    };

    //==============================================================================

    /** An immutable snapshot of all currently profiled components. */
    struct Snapshot
    {
        /** Monotonically increasing index of the frame at which this snapshot was taken. */
        uint64 frameIndex = 0;

        /** One entry per registered component, sorted by the requested PaintProfileTimeKind. */
        std::vector<ComponentEntry> components;

        /** Summary of global per-frame timing derived from globalFrameStats. */
        PaintProfileSummary globalFrameTotal;

        /** Histogram of global per-frame timing. */
        PaintProfileHistogram globalFrameHistogram;
    };

    //==============================================================================

    /**
        RAII handle for a profiling session over a component subtree.

        Created by PaintProfiler::startSession(). The destructor disables profiling
        for the same components that were enabled at construction time.
    */
    class ScopedSession
    {
    public:
        /** Destructor disables profiling for all components enabled by this session. */
        ~ScopedSession();

        /** Pauses or resumes sample recording for this session.

            When paused, the profiler global enabled state is not changed; instead
            the session records the paused state so that reset() and createSnapshot()
            reflect a quiescent view.

            @param shouldBePaused  Pass true to pause, false to resume.
        */
        void setPaused (bool shouldBePaused);

        /** Returns true if this session is currently paused. */
        bool isPaused() const;

        /** Clears all samples collected by this session's components. */
        void reset();

        /** Creates an immutable snapshot of the current profiling state.

            @param sortBy           Which time kind to use when sorting components in the snapshot.
            @param histogramBuckets Number of buckets to use for the global frame histogram.
            @returns An immutable Snapshot of the current profiling state.
        */
        Snapshot createSnapshot (PaintProfileTimeKind sortBy = PaintProfileTimeKind::total,
                                 int histogramBuckets = 32) const;

    private:
        friend class PaintProfiler;

        ScopedSession (PaintProfiler& profiler,
                       Component& root,
                       PaintProfileOptions options);

        PaintProfiler& profiler;
        WeakReference<Component> root;
        PaintProfileOptions options;
        bool paused = false;
        std::vector<WeakReference<Component>> enabledComponents;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopedSession)
    };

    //==============================================================================

    /** Returns the process-wide singleton instance. */
    static PaintProfiler& getInstance();

    //==============================================================================

    /** Enables or disables global profiling.

        When disabled, no new samples are recorded but all per-component state and
        the registry are preserved.

        @param shouldBeEnabled  Pass true to enable, false to disable.
    */
    void setEnabled (bool shouldBeEnabled);

    /** Returns true when global profiling is enabled. */
    bool isEnabled() const;

    //==============================================================================

    /** Enables profiling on the subtree rooted at root and returns an RAII handle.

        The handle's destructor will disable profiling for each component that was
        enabled by this call, regardless of the current global enabled state.

        @param root     The root component of the subtree to profile.
        @param options  Options forwarded to each component's PaintProfileStats.
        @returns A unique_ptr to a ScopedSession that disables profiling on destruction.
    */
    std::unique_ptr<ScopedSession> startSession (Component& root,
                                                 PaintProfileOptions options = {});

    /** Recursively enables profiling for root and all its current children.

        @param root     The root component of the subtree.
        @param options  Options forwarded to each component's PaintProfileStats.
    */
    void enableSubtree (Component& root, PaintProfileOptions options = {});

    /** Recursively disables profiling for root and all its current children.

        @param root  The root component of the subtree.
    */
    void disableSubtree (Component& root);

    /** Recursively resets all samples for root and all its current children.

        @param root  The root component of the subtree.
    */
    void resetSubtree (Component& root);

    /** Resets all samples for every registered component. */
    void resetAll();

    //==============================================================================

    /** Creates an immutable snapshot of all registered components.

        @param sortBy           Which time kind determines the sort order (descending by p95).
        @param histogramBuckets Number of buckets for the global frame histogram.
        @returns An immutable Snapshot populated with all currently registered components.
    */
    Snapshot createSnapshot (PaintProfileTimeKind sortBy = PaintProfileTimeKind::total,
                             int histogramBuckets = 32) const;

    /** Creates a histogram for a specific component.

        @param component        The component whose samples should be histogrammed.
        @param kind             Which time field to histogram.
        @param histogramBuckets Number of equal-width buckets.
        @returns A PaintProfileHistogram, or a default-constructed one if the component
                 is not registered.
    */
    PaintProfileHistogram createHistogramForComponent (const Component& component,
                                                       PaintProfileTimeKind kind,
                                                       int histogramBuckets = 32) const;

    //==============================================================================

    /** Called by the native renderer before processing repaint areas.

        Increments the frame counter and resets the per-frame paint index.
    */
    void beginFrame();

    /** Called by the native renderer after all repaint areas have been processed. */
    void endFrame();

    /** Returns the current frame index for use in sample construction. */
    uint64 getCurrentFrameIndex() const noexcept { return currentFrameIndex; }

private:
    friend class Component;

    /** Called by Component::setPaintProfilingEnabled to register a component. */
    void registerComponent (Component& component, PaintProfileStats& stats);

    /** Called by Component::setPaintProfilingEnabled to deregister a component. */
    void deregisterComponent (const Component& component);

    mutable CriticalSection registryLock;
    std::vector<std::pair<Component*, PaintProfileStats*>> registry;

    std::unique_ptr<PaintProfileStats> globalFrameStats;
    uint64 currentFrameIndex = 0;
    std::atomic<uint64> globalPaintIndex { 0 };
    double frameStartMicros = 0.0;
    bool enabled = true;

    PaintProfiler();
    ~PaintProfiler();

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaintProfiler)
};

} // namespace yup
