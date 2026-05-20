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
// PaintProfiler
//==============================================================================

PaintProfiler::PaintProfiler()
    : globalFrameStats (std::make_unique<PaintProfileStats> (PaintProfileOptions {}))
{
}

PaintProfiler::~PaintProfiler() = default;

//==============================================================================

PaintProfiler& PaintProfiler::getInstance()
{
    static PaintProfiler instance;
    return instance;
}

//==============================================================================

void PaintProfiler::setEnabled (bool shouldBeEnabled)
{
    enabled = shouldBeEnabled;
}

bool PaintProfiler::isEnabled() const
{
    return enabled;
}

//==============================================================================

void PaintProfiler::beginFrame()
{
    ++currentFrameIndex;
    globalPaintIndex.store (0, std::memory_order_relaxed);
    frameStartMicros = Time::highResolutionTicksToSeconds (Time::getHighResolutionTicks()) * 1.0e6;
}

void PaintProfiler::endFrame()
{
    const double endMicros = Time::highResolutionTicksToSeconds (Time::getHighResolutionTicks()) * 1.0e6;

    PaintProfileSample sample;
    sample.frameIndex = currentFrameIndex;
    sample.totalMicros = endMicros - frameStartMicros;
    globalFrameStats->recordSample (sample);
}

//==============================================================================

void PaintProfiler::registerComponent (Component& component, PaintProfileStats& stats)
{
    const ScopedLock sl (registryLock);

    registry.erase (std::remove_if (registry.begin(), registry.end(), [&] (const auto& entry)
    {
        return entry.first == &component;
    }),
                    registry.end());

    registry.emplace_back (&component, &stats);
}

void PaintProfiler::deregisterComponent (const Component& component)
{
    const ScopedLock sl (registryLock);

    registry.erase (std::remove_if (registry.begin(), registry.end(), [&] (const auto& entry)
    {
        return entry.first == &component;
    }),
                    registry.end());
}

//==============================================================================

void PaintProfiler::enableSubtree (Component& root, PaintProfileOptions options)
{
    root.setPaintProfilingEnabled (true, options);

    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        if (auto* child = root.getChildComponent (i))
            enableSubtree (*child, options);
    }
}

void PaintProfiler::disableSubtree (Component& root)
{
    root.setPaintProfilingEnabled (false);

    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        if (auto* child = root.getChildComponent (i))
            disableSubtree (*child);
    }
}

void PaintProfiler::resetSubtree (Component& root)
{
    {
        const ScopedLock sl (registryLock);

        for (auto& [component, stats] : registry)
        {
            if (component == &root)
            {
                stats->reset();
                break;
            }
        }
    }

    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        if (auto* child = root.getChildComponent (i))
            resetSubtree (*child);
    }
}

void PaintProfiler::resetAll()
{
    const ScopedLock sl (registryLock);

    for (auto& [component, stats] : registry)
        stats->reset();

    globalFrameStats->reset();
}

//==============================================================================

std::unique_ptr<PaintProfiler::ScopedSession> PaintProfiler::startSession (Component& root,
                                                                           PaintProfileOptions options)
{
    return std::unique_ptr<ScopedSession> (new ScopedSession (*this, root, options));
}

//==============================================================================

PaintProfiler::Snapshot PaintProfiler::createSnapshot (PaintProfileTimeKind sortBy,
                                                       int histogramBuckets) const
{
    std::vector<std::pair<Component*, PaintProfileStats*>> registryCopy;

    {
        const ScopedLock sl (registryLock);
        registryCopy = registry;
    }

    Snapshot snapshot;
    snapshot.frameIndex = currentFrameIndex;
    snapshot.components.reserve (registryCopy.size());

    for (auto& [component, stats] : registryCopy)
    {
        ComponentEntry entry;
        entry.component = component;
        entry.name = component->getPaintProfileName();
        entry.stats = stats;
        entry.self = stats->summarize (PaintProfileTimeKind::self);
        entry.children = stats->summarize (PaintProfileTimeKind::children);
        entry.framework = stats->summarize (PaintProfileTimeKind::framework);
        entry.total = stats->summarize (PaintProfileTimeKind::total);

        snapshot.components.push_back (std::move (entry));
    }

    std::sort (snapshot.components.begin(), snapshot.components.end(), [sortBy] (const ComponentEntry& a, const ComponentEntry& b)
    {
        auto getP95 = [sortBy] (const ComponentEntry& entry) -> double
        {
            switch (sortBy)
            {
                case PaintProfileTimeKind::self:
                    return entry.self.p95Micros;
                case PaintProfileTimeKind::children:
                    return entry.children.p95Micros;
                case PaintProfileTimeKind::framework:
                    return entry.framework.p95Micros;
                case PaintProfileTimeKind::total:
                    return entry.total.p95Micros;
                default:
                    return entry.total.p95Micros;
            }
        };

        return getP95 (a) > getP95 (b);
    });

    snapshot.globalFrameTotal = globalFrameStats->summarize (PaintProfileTimeKind::total);
    snapshot.globalFrameHistogram = globalFrameStats->createHistogram (PaintProfileTimeKind::total, histogramBuckets);

    return snapshot;
}

PaintProfileHistogram PaintProfiler::createHistogramForComponent (const Component& component,
                                                                  PaintProfileTimeKind kind,
                                                                  int histogramBuckets) const
{
    PaintProfileStats* found = nullptr;

    {
        const ScopedLock sl (registryLock);

        for (auto& [comp, stats] : registry)
        {
            if (comp == &component)
            {
                found = stats;
                break;
            }
        }
    }

    if (found == nullptr)
        return {};

    return found->createHistogram (kind, histogramBuckets);
}

//==============================================================================
// PaintProfiler::ScopedSession
//==============================================================================

PaintProfiler::ScopedSession::ScopedSession (PaintProfiler& profilerRef,
                                             Component& rootComponent,
                                             PaintProfileOptions sessionOptions)
    : profiler (profilerRef)
    , root (&rootComponent)
    , options (sessionOptions)
{
    profiler.enableSubtree (rootComponent, sessionOptions);

    // Walk the same subtree to capture weak references for later cleanup.
    std::function<void (Component&)> collectComponents = [&] (Component& component)
    {
        enabledComponents.emplace_back (&component);

        for (int i = 0; i < component.getNumChildComponents(); ++i)
        {
            if (auto* child = component.getChildComponent (i))
                collectComponents (*child);
        }
    };

    collectComponents (rootComponent);
}

PaintProfiler::ScopedSession::~ScopedSession()
{
    for (auto& weakComponent : enabledComponents)
    {
        if (auto* component = weakComponent.get())
            component->setPaintProfilingEnabled (false);
    }
}

//==============================================================================

void PaintProfiler::ScopedSession::setPaused (bool shouldBePaused)
{
    paused = shouldBePaused;
}

bool PaintProfiler::ScopedSession::isPaused() const
{
    return paused;
}

void PaintProfiler::ScopedSession::reset()
{
    if (auto* rootComponent = root.get())
        profiler.resetSubtree (*rootComponent);
}

PaintProfiler::Snapshot PaintProfiler::ScopedSession::createSnapshot (PaintProfileTimeKind sortBy,
                                                                      int histogramBuckets) const
{
    return profiler.createSnapshot (sortBy, histogramBuckets);
}

} // namespace yup
