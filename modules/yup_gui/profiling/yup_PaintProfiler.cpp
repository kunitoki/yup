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

namespace
{

double ticksToMicros (double ticks)
{
    return Time::highResolutionTicksToSeconds (static_cast<int64> (ticks)) * 1.0e6;
}

String getComponentPaintProfileName (const Component& component)
{
    if (component.getTitle().isNotEmpty())
        return component.getTitle();

    if (component.getComponentID().isNotEmpty())
        return component.getComponentID();

    return "Component";
}

} // namespace

//==============================================================================

std::atomic<int> PaintProfiler::registeredComponentCount { 0 };

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

bool PaintProfiler::hasRegisteredComponents() noexcept
{
    return registeredComponentCount.load (std::memory_order_relaxed) > 0;
}

void PaintProfiler::removeExpiredRegistryEntries() const
{
    const auto previousSize = registry.size();

    std::erase_if (registry, [] (const auto& entry)
    {
        return entry.component.get() == nullptr;
    });

    const auto removedCount = previousSize - registry.size();

    if (removedCount > 0)
        registeredComponentCount.fetch_sub (static_cast<int> (removedCount), std::memory_order_release);
}

//==============================================================================

void PaintProfiler::setEnabled (bool shouldBeEnabled)
{
    enabled.store (shouldBeEnabled, std::memory_order_release);

    const ScopedLock sl (registryLock);

    removeExpiredRegistryEntries();

    for (auto& entry : registry)
    {
        auto* component = entry.component.get();

        if (component == nullptr)
            continue;

        if (shouldBeEnabled)
            component->addComponentListener (this);
        else
            component->removeComponentListener (this);
    }
}

bool PaintProfiler::isEnabled() const
{
    return enabled.load (std::memory_order_acquire);
}

bool PaintProfiler::isActive() const noexcept
{
    return isEnabled() && hasRegisteredComponents();
}

//==============================================================================

void PaintProfiler::beginFrame()
{
    ++currentFrameIndex;
    globalPaintIndex.store (0, std::memory_order_relaxed);
    frameStartMicros = ticksToMicros (Time::getHighResolutionTicks());
}

void PaintProfiler::endFrame()
{
    const double endMicros = ticksToMicros (Time::getHighResolutionTicks());

    PaintProfileSample sample;
    sample.frameIndex = currentFrameIndex;
    sample.totalMicros = endMicros - frameStartMicros;
    globalFrameStats->recordSample (sample);
}

//==============================================================================

void PaintProfiler::enableComponent (Component& component, PaintProfileOptions options)
{
    bool registeredNewComponent = false;

    const ScopedLock sl (registryLock);

    removeExpiredRegistryEntries();

    auto existing = std::find_if (registry.begin(), registry.end(), [&] (const auto& entry)
    {
        return entry.component.get() == &component;
    });

    if (existing != registry.end())
    {
        if (existing->stats == nullptr || existing->stats->getCapacity() != options.sampleCapacity)
            existing->stats = std::make_unique<PaintProfileStats> (options);

        if (isEnabled())
            component.addComponentListener (this);

        return;
    }

    registry.push_back ({ WeakReference<Component> (&component), std::make_unique<PaintProfileStats> (options) });
    registeredNewComponent = true;

    if (isEnabled())
        component.addComponentListener (this);

    if (registeredNewComponent)
        registeredComponentCount.fetch_add (1, std::memory_order_release);
}

void PaintProfiler::disableComponent (Component& component)
{
    bool removedComponent = false;

    const ScopedLock sl (registryLock);

    removeExpiredRegistryEntries();

    const auto previousSize = registry.size();

    std::erase_if (registry, [&] (const auto& entry)
    {
        return entry.component.get() == &component;
    });

    removedComponent = registry.size() != previousSize;

    if (removedComponent)
        component.removeComponentListener (this);

    if (removedComponent)
        registeredComponentCount.fetch_sub (1, std::memory_order_release);
}

bool PaintProfiler::isComponentEnabled (const Component& component) const
{
    const ScopedLock sl (registryLock);

    removeExpiredRegistryEntries();

    return std::any_of (registry.begin(), registry.end(), [&] (const auto& entry)
    {
        return entry.component.get() == &component;
    });
}

void PaintProfiler::resetComponent (Component& component)
{
    if (auto* stats = getStatsForComponent (component))
        stats->reset();
}

PaintProfileStats* PaintProfiler::getStatsForComponent (const Component& component) const
{
    const ScopedLock sl (registryLock);

    removeExpiredRegistryEntries();

    auto found = std::find_if (registry.begin(), registry.end(), [&] (const auto& entry)
    {
        return entry.component.get() == &component;
    });

    return found != registry.end() ? found->stats.get() : nullptr;
}

//==============================================================================

void PaintProfiler::enableSubtree (Component& root, PaintProfileOptions options)
{
    enableComponent (root, options);

    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        if (auto* child = root.getChildComponent (i))
            enableSubtree (*child, options);
    }
}

void PaintProfiler::disableSubtree (Component& root)
{
    disableComponent (root);

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

        removeExpiredRegistryEntries();

        for (auto& entry : registry)
        {
            if (entry.component.get() == &root)
            {
                entry.stats->reset();
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

    removeExpiredRegistryEntries();

    for (auto& entry : registry)
        entry.stats->reset();

    globalFrameStats->reset();
}

//==============================================================================

std::unique_ptr<PaintProfiler::ScopedSession> PaintProfiler::startSession (Component& root, PaintProfileOptions options)
{
    return std::unique_ptr<ScopedSession> (new ScopedSession (*this, root, options));
}

//==============================================================================

PaintProfiler::Snapshot PaintProfiler::createSnapshot (PaintProfileTimeKind sortBy, int histogramBuckets) const
{
    std::vector<std::pair<Component*, PaintProfileStats*>> registryCopy;

    {
        const ScopedLock sl (registryLock);

        removeExpiredRegistryEntries();

        registryCopy.reserve (registry.size());
        for (auto& entry : registry)
        {
            if (auto* component = entry.component.get())
                registryCopy.emplace_back (component, entry.stats.get());
        }
    }

    Snapshot snapshot;
    snapshot.frameIndex = currentFrameIndex;
    snapshot.components.reserve (registryCopy.size());

    for (auto& [component, stats] : registryCopy)
    {
        ComponentEntry entry;
        entry.component = component;
        entry.name = getComponentPaintProfileName (*component);
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

        removeExpiredRegistryEntries();

        for (auto& entry : registry)
        {
            if (entry.component.get() == &component)
            {
                found = entry.stats.get();
                break;
            }
        }
    }

    if (found == nullptr)
        return {};

    return found->createHistogram (kind, histogramBuckets);
}

void PaintProfiler::componentBeingDeleted (Component& component)
{
    bool removedComponent = false;

    const ScopedLock sl (registryLock);

    const auto previousSize = registry.size();

    std::erase_if (registry, [&] (const auto& entry)
    {
        return entry.component.get() == &component;
    });

    removedComponent = registry.size() != previousSize;

    if (removedComponent)
        registeredComponentCount.fetch_sub (1, std::memory_order_release);
}

void PaintProfiler::componentPaintCompleted (Component& component, const ComponentPaintMetrics& metrics)
{
    if (! isEnabled())
        return;

    if (auto* stats = getStatsForComponent (component))
    {
        PaintProfileSample sample;
        sample.frameIndex = currentFrameIndex;
        sample.paintIndex = globalPaintIndex.fetch_add (1, std::memory_order_relaxed);
        sample.selfMicros = ticksToMicros (static_cast<double> (metrics.selfTicks));
        sample.childrenMicros = ticksToMicros (static_cast<double> (metrics.childrenTicks));
        sample.totalMicros = ticksToMicros (static_cast<double> (metrics.totalTicks));
        sample.frameworkMicros = jmax (0.0, sample.totalMicros - sample.selfMicros - sample.childrenMicros);
        sample.componentBounds = metrics.componentBounds;
        sample.repaintArea = metrics.repaintArea;
        sample.renderContinuous = metrics.renderContinuous;
        sample.selfPaintSkipped = metrics.selfPaintSkipped;
        stats->recordSample (sample);
    }
}

//==============================================================================

PaintProfiler::ScopedSession::ScopedSession (PaintProfiler& profilerRef,
                                             Component& rootComponent,
                                             PaintProfileOptions sessionOptions)
    : profiler (profilerRef)
    , root (&rootComponent)
    , options (sessionOptions)
{
    profiler.enableSubtree (rootComponent, sessionOptions);

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
            profiler.disableComponent (*component);
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

PaintProfiler::Snapshot PaintProfiler::ScopedSession::createSnapshot (PaintProfileTimeKind sortBy, int histogramBuckets) const
{
    return profiler.createSnapshot (sortBy, histogramBuckets);
}

} // namespace yup
