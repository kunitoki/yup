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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

PaintProfileSample makeSample (double totalMicros,
                               double selfMicros = 0.0,
                               double childrenMicros = 0.0)
{
    PaintProfileSample s;
    s.totalMicros = totalMicros;
    s.selfMicros = selfMicros;
    s.childrenMicros = childrenMicros;
    s.frameworkMicros = totalMicros - selfMicros - childrenMicros;
    return s;
}

} // namespace

// =============================================================================
// Component profiling API tests
// =============================================================================

TEST (ComponentProfilingTests, ProfilingDisabledByDefault)
{
    Component comp;

    EXPECT_FALSE (comp.isPaintProfilingEnabled());
    EXPECT_EQ (nullptr, comp.getPaintProfileStats());
}

TEST (ComponentProfilingTests, EnableProfilingCreatesStats)
{
    Component comp;
    comp.setPaintProfilingEnabled (true);

    EXPECT_TRUE (comp.isPaintProfilingEnabled());
    EXPECT_NE (nullptr, comp.getPaintProfileStats());
}

TEST (ComponentProfilingTests, DisableProfilingClearsStats)
{
    Component comp;
    comp.setPaintProfilingEnabled (true);
    comp.setPaintProfilingEnabled (false);

    EXPECT_FALSE (comp.isPaintProfilingEnabled());
    EXPECT_EQ (nullptr, comp.getPaintProfileStats());
}

TEST (ComponentProfilingTests, ReEnableProfilingAfterDisable)
{
    Component comp;
    comp.setPaintProfilingEnabled (true);
    comp.setPaintProfilingEnabled (false);
    comp.setPaintProfilingEnabled (true);

    EXPECT_TRUE (comp.isPaintProfilingEnabled());
    EXPECT_NE (nullptr, comp.getPaintProfileStats());
}

TEST (ComponentProfilingTests, ResetProfilingClearsSamples)
{
    Component comp;
    comp.setPaintProfilingEnabled (true);

    auto* stats = comp.getPaintProfileStats();
    stats->recordSample (makeSample (10.0));
    stats->recordSample (makeSample (20.0));
    ASSERT_EQ (2, stats->getSampleCount());

    comp.resetPaintProfiling();

    EXPECT_EQ (0, comp.getPaintProfileStats()->getSampleCount());
    EXPECT_TRUE (comp.isPaintProfilingEnabled());
}

TEST (ComponentProfilingTests, ResetProfilingOnDisabledComponentIsNoOp)
{
    Component comp;

    EXPECT_NO_FATAL_FAILURE (comp.resetPaintProfiling());
    EXPECT_EQ (nullptr, comp.getPaintProfileStats());
}

TEST (ComponentProfilingTests, OptionsPreservedOnEnable)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 50;
    opts.minimumSampleMicros = 5.0;
    opts.includeBounds = false;

    Component comp;
    comp.setPaintProfilingEnabled (true, opts);

    auto* stats = comp.getPaintProfileStats();
    ASSERT_NE (nullptr, stats);

    auto stored = stats->getOptions();
    EXPECT_EQ (50, stored.sampleCapacity);
    EXPECT_DOUBLE_EQ (5.0, stored.minimumSampleMicros);
    EXPECT_FALSE (stored.includeBounds);
}

TEST (ComponentProfilingTests, GetPaintProfileNameFromTitle)
{
    Component comp;
    comp.setTitle ("MyComponent");

    EXPECT_EQ ("MyComponent", comp.getPaintProfileName());
}

TEST (ComponentProfilingTests, GetPaintProfileNameFromID)
{
    Component comp ("myID");

    EXPECT_EQ ("myID", comp.getPaintProfileName());
}

TEST (ComponentProfilingTests, GetPaintProfileNameFallback)
{
    Component comp;

    EXPECT_EQ ("Component", comp.getPaintProfileName());
}

TEST (ComponentProfilingTests, GetPaintProfileNamePrefersTitle)
{
    Component comp ("myID");
    comp.setTitle ("MyTitle");

    EXPECT_EQ ("MyTitle", comp.getPaintProfileName());
}

// =============================================================================
// PaintProfiler singleton tests
// =============================================================================

class PaintProfilerFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        profiler = &PaintProfiler::getInstance();
        profiler->setEnabled (true);
        profiler->resetAll();
    }

    PaintProfiler* profiler = nullptr;
};

TEST_F (PaintProfilerFixture, SingletonReturnsSameInstance)
{
    EXPECT_EQ (&PaintProfiler::getInstance(), &PaintProfiler::getInstance());
}

TEST_F (PaintProfilerFixture, IsEnabledAfterSetupReset)
{
    EXPECT_TRUE (profiler->isEnabled());
}

TEST_F (PaintProfilerFixture, SetEnabledFalseDisables)
{
    profiler->setEnabled (false);

    EXPECT_FALSE (profiler->isEnabled());
}

TEST_F (PaintProfilerFixture, SetEnabledTrueRenables)
{
    profiler->setEnabled (false);
    profiler->setEnabled (true);

    EXPECT_TRUE (profiler->isEnabled());
}

TEST_F (PaintProfilerFixture, EnableSubtreeEnablesRoot)
{
    Component root;

    profiler->enableSubtree (root);

    EXPECT_TRUE (root.isPaintProfilingEnabled());
    EXPECT_NE (nullptr, root.getPaintProfileStats());
}

TEST_F (PaintProfilerFixture, EnableSubtreeEnablesChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    profiler->enableSubtree (root);

    EXPECT_TRUE (root.isPaintProfilingEnabled());
    EXPECT_TRUE (child.isPaintProfilingEnabled());
}

TEST_F (PaintProfilerFixture, EnableSubtreeEnablesDeepHierarchy)
{
    Component root;
    Component mid;
    Component leaf;
    root.addChildComponent (mid);
    mid.addChildComponent (leaf);

    profiler->enableSubtree (root);

    EXPECT_TRUE (root.isPaintProfilingEnabled());
    EXPECT_TRUE (mid.isPaintProfilingEnabled());
    EXPECT_TRUE (leaf.isPaintProfilingEnabled());
}

TEST_F (PaintProfilerFixture, DisableSubtreeDisablesRootAndChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    profiler->enableSubtree (root);
    profiler->disableSubtree (root);

    EXPECT_FALSE (root.isPaintProfilingEnabled());
    EXPECT_FALSE (child.isPaintProfilingEnabled());
}

TEST_F (PaintProfilerFixture, ResetSubtreeResetsRootStats)
{
    Component root;
    profiler->enableSubtree (root);

    auto* stats = root.getPaintProfileStats();
    stats->recordSample (makeSample (10.0));
    stats->recordSample (makeSample (20.0));
    ASSERT_EQ (2, stats->getSampleCount());

    profiler->resetSubtree (root);

    EXPECT_EQ (0, root.getPaintProfileStats()->getSampleCount());
}

TEST_F (PaintProfilerFixture, ResetSubtreeResetsChildStats)
{
    Component root;
    Component child;
    root.addChildComponent (child);
    profiler->enableSubtree (root);

    child.getPaintProfileStats()->recordSample (makeSample (15.0));
    ASSERT_EQ (1, child.getPaintProfileStats()->getSampleCount());

    profiler->resetSubtree (root);

    EXPECT_EQ (0, child.getPaintProfileStats()->getSampleCount());
}

TEST_F (PaintProfilerFixture, ResetAllClearsAllRegisteredStats)
{
    Component comp1;
    Component comp2;
    profiler->enableSubtree (comp1);
    profiler->enableSubtree (comp2);

    comp1.getPaintProfileStats()->recordSample (makeSample (10.0));
    comp2.getPaintProfileStats()->recordSample (makeSample (20.0));

    profiler->resetAll();

    EXPECT_EQ (0, comp1.getPaintProfileStats()->getSampleCount());
    EXPECT_EQ (0, comp2.getPaintProfileStats()->getSampleCount());
}

TEST_F (PaintProfilerFixture, SnapshotEmptyWithNoRegisteredComponents)
{
    auto snapshot = profiler->createSnapshot();

    EXPECT_TRUE (snapshot.components.empty());
}

TEST_F (PaintProfilerFixture, SnapshotContainsRegisteredComponents)
{
    Component comp1 ("comp1");
    Component comp2 ("comp2");
    profiler->enableSubtree (comp1);
    profiler->enableSubtree (comp2);

    auto snapshot = profiler->createSnapshot();

    EXPECT_EQ (2u, snapshot.components.size());
}

TEST_F (PaintProfilerFixture, SnapshotHasComponentNames)
{
    Component comp ("named");
    profiler->enableSubtree (comp);

    auto snapshot = profiler->createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ ("named", snapshot.components[0].name);
}

TEST_F (PaintProfilerFixture, SnapshotSortedByP95TotalDescending)
{
    Component slow;
    Component fast;
    profiler->enableSubtree (slow);
    profiler->enableSubtree (fast);

    for (int i = 0; i < 5; ++i)
        slow.getPaintProfileStats()->recordSample (makeSample (100.0));

    for (int i = 0; i < 5; ++i)
        fast.getPaintProfileStats()->recordSample (makeSample (10.0));

    auto snapshot = profiler->createSnapshot (PaintProfileTimeKind::total);

    ASSERT_EQ (2u, snapshot.components.size());
    EXPECT_GE (snapshot.components[0].total.p95Micros, snapshot.components[1].total.p95Micros);
}

TEST_F (PaintProfilerFixture, SnapshotSortedByP95SelfDescending)
{
    Component highSelf;
    Component lowSelf;
    profiler->enableSubtree (highSelf);
    profiler->enableSubtree (lowSelf);

    for (int i = 0; i < 5; ++i)
        highSelf.getPaintProfileStats()->recordSample (makeSample (100.0, 80.0, 10.0));

    for (int i = 0; i < 5; ++i)
        lowSelf.getPaintProfileStats()->recordSample (makeSample (100.0, 10.0, 80.0));

    auto snapshot = profiler->createSnapshot (PaintProfileTimeKind::self);

    ASSERT_EQ (2u, snapshot.components.size());
    EXPECT_GE (snapshot.components[0].self.p95Micros, snapshot.components[1].self.p95Micros);
}

TEST_F (PaintProfilerFixture, SnapshotSortedByP95ChildrenDescending)
{
    Component highChildren;
    Component lowChildren;
    profiler->enableSubtree (highChildren);
    profiler->enableSubtree (lowChildren);

    for (int i = 0; i < 5; ++i)
        highChildren.getPaintProfileStats()->recordSample (makeSample (100.0, 10.0, 80.0));

    for (int i = 0; i < 5; ++i)
        lowChildren.getPaintProfileStats()->recordSample (makeSample (100.0, 80.0, 10.0));

    auto snapshot = profiler->createSnapshot (PaintProfileTimeKind::children);

    ASSERT_EQ (2u, snapshot.components.size());
    EXPECT_GE (snapshot.components[0].children.p95Micros, snapshot.components[1].children.p95Micros);
}

TEST_F (PaintProfilerFixture, SnapshotContainsComponentPointers)
{
    Component comp;
    profiler->enableSubtree (comp);

    auto snapshot = profiler->createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ (&comp, snapshot.components[0].component);
}

TEST_F (PaintProfilerFixture, SnapshotHasFrameIndex)
{
    profiler->beginFrame();
    uint64 expectedFrameIndex = profiler->getCurrentFrameIndex();

    Component comp;
    profiler->enableSubtree (comp);

    auto snapshot = profiler->createSnapshot();

    EXPECT_EQ (expectedFrameIndex, snapshot.frameIndex);
}

TEST_F (PaintProfilerFixture, HistogramForUnregisteredComponentIsDefault)
{
    Component comp;

    auto histogram = profiler->createHistogramForComponent (comp, PaintProfileTimeKind::total, 10);

    EXPECT_DOUBLE_EQ (0.0, histogram.rangeMinMicros);
    EXPECT_DOUBLE_EQ (0.0, histogram.rangeMaxMicros);
    EXPECT_TRUE (histogram.buckets.empty());
}

TEST_F (PaintProfilerFixture, HistogramForRegisteredComponentContainsAllSamples)
{
    Component comp;
    profiler->enableSubtree (comp);

    auto* stats = comp.getPaintProfileStats();
    for (int i = 1; i <= 5; ++i)
        stats->recordSample (makeSample (static_cast<double> (i) * 10.0));

    auto histogram = profiler->createHistogramForComponent (comp, PaintProfileTimeKind::total, 5);

    ASSERT_EQ (5u, histogram.buckets.size());

    int total = 0;
    for (int count : histogram.buckets)
        total += count;

    EXPECT_EQ (5, total);
}

TEST_F (PaintProfilerFixture, StartSessionEnablesRootAndChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    auto session = profiler->startSession (root);

    EXPECT_TRUE (root.isPaintProfilingEnabled());
    EXPECT_TRUE (child.isPaintProfilingEnabled());
}

TEST_F (PaintProfilerFixture, ScopedSessionDestructorDisablesProfiling)
{
    Component root;

    {
        auto session = profiler->startSession (root);
        EXPECT_TRUE (root.isPaintProfilingEnabled());
    }

    EXPECT_FALSE (root.isPaintProfilingEnabled());
}

TEST_F (PaintProfilerFixture, ScopedSessionDestructorDisablesChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    {
        auto session = profiler->startSession (root);
        EXPECT_TRUE (child.isPaintProfilingEnabled());
    }

    EXPECT_FALSE (child.isPaintProfilingEnabled());
}

TEST_F (PaintProfilerFixture, ScopedSessionInitiallyNotPaused)
{
    Component root;
    auto session = profiler->startSession (root);

    EXPECT_FALSE (session->isPaused());
}

TEST_F (PaintProfilerFixture, ScopedSessionPauseAndResume)
{
    Component root;
    auto session = profiler->startSession (root);

    session->setPaused (true);
    EXPECT_TRUE (session->isPaused());

    session->setPaused (false);
    EXPECT_FALSE (session->isPaused());
}

TEST_F (PaintProfilerFixture, ScopedSessionResetClearsStats)
{
    Component root;
    auto session = profiler->startSession (root);

    auto* stats = root.getPaintProfileStats();
    stats->recordSample (makeSample (10.0));
    stats->recordSample (makeSample (20.0));
    ASSERT_EQ (2, stats->getSampleCount());

    session->reset();

    EXPECT_EQ (0, root.getPaintProfileStats()->getSampleCount());
}

TEST_F (PaintProfilerFixture, ScopedSessionCreateSnapshotReflectsRegisteredComponents)
{
    Component root ("root");
    auto session = profiler->startSession (root);

    auto snapshot = session->createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ ("root", snapshot.components[0].name);
}

TEST_F (PaintProfilerFixture, BeginFrameIncrementsFrameIndex)
{
    uint64 before = profiler->getCurrentFrameIndex();
    profiler->beginFrame();
    uint64 after = profiler->getCurrentFrameIndex();

    EXPECT_EQ (before + 1, after);
}

TEST_F (PaintProfilerFixture, EndFrameRecordsGlobalFrameSample)
{
    profiler->beginFrame();
    profiler->endFrame();

    auto snapshot = profiler->createSnapshot();

    EXPECT_EQ (1, snapshot.globalFrameTotal.sampleCount);
    EXPECT_GE (snapshot.globalFrameTotal.lastMicros, 0.0);
}

TEST_F (PaintProfilerFixture, MultipleFramesAccumulateGlobalStats)
{
    constexpr int frameCount = 5;

    for (int i = 0; i < frameCount; ++i)
    {
        profiler->beginFrame();
        profiler->endFrame();
    }

    auto snapshot = profiler->createSnapshot();

    EXPECT_EQ (frameCount, snapshot.globalFrameTotal.sampleCount);
}
