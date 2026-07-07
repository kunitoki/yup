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

    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (comp));
    EXPECT_EQ (nullptr, PaintProfiler::getInstance().getStatsForComponent (comp));
}

TEST (ComponentProfilingTests, EnableProfilingCreatesStats)
{
    Component comp;
    PaintProfiler::getInstance().enableComponent (comp);

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (comp));
    EXPECT_NE (nullptr, PaintProfiler::getInstance().getStatsForComponent (comp));
}

TEST (ComponentProfilingTests, DisableProfilingClearsStats)
{
    Component comp;
    PaintProfiler::getInstance().enableComponent (comp);
    PaintProfiler::getInstance().disableComponent (comp);

    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (comp));
    EXPECT_EQ (nullptr, PaintProfiler::getInstance().getStatsForComponent (comp));
}

TEST (ComponentProfilingTests, ReEnableProfilingAfterDisable)
{
    Component comp;
    PaintProfiler::getInstance().enableComponent (comp);
    PaintProfiler::getInstance().disableComponent (comp);
    PaintProfiler::getInstance().enableComponent (comp);

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (comp));
    EXPECT_NE (nullptr, PaintProfiler::getInstance().getStatsForComponent (comp));
}

TEST (ComponentProfilingTests, ResetProfilingClearsSamples)
{
    Component comp;
    PaintProfiler::getInstance().enableComponent (comp);

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (comp);
    stats->recordSample (makeSample (10.0));
    stats->recordSample (makeSample (20.0));
    ASSERT_EQ (2, stats->getSampleCount());

    PaintProfiler::getInstance().resetComponent (comp);

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (comp)->getSampleCount());
    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (comp));
}

TEST (ComponentProfilingTests, ResetProfilingOnDisabledComponentIsNoOp)
{
    Component comp;

    EXPECT_NO_FATAL_FAILURE (PaintProfiler::getInstance().resetComponent (comp));
    EXPECT_EQ (nullptr, PaintProfiler::getInstance().getStatsForComponent (comp));
}

TEST (ComponentProfilingTests, OptionsPreservedOnEnable)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 50;
    opts.minimumSampleMicros = 5.0;
    opts.includeBounds = false;

    Component comp;
    PaintProfiler::getInstance().enableComponent (comp, opts);

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (comp);
    ASSERT_NE (nullptr, stats);

    auto stored = stats->getOptions();
    EXPECT_EQ (50, stored.sampleCapacity);
    EXPECT_DOUBLE_EQ (5.0, stored.minimumSampleMicros);
    EXPECT_FALSE (stored.includeBounds);
}

TEST (ComponentProfilingTests, GetPaintProfileNameFromTitle)
{
    auto& profiler = PaintProfiler::getInstance();
    Component comp;
    comp.setTitle ("MyComponent");
    profiler.enableComponent (comp);

    auto snapshot = profiler.createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ ("MyComponent", snapshot.components[0].name);
}

TEST (ComponentProfilingTests, GetPaintProfileNameFromID)
{
    auto& profiler = PaintProfiler::getInstance();
    Component comp ("myID");
    profiler.enableComponent (comp);

    auto snapshot = profiler.createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ ("myID", snapshot.components[0].name);
}

TEST (ComponentProfilingTests, GetPaintProfileNameFallback)
{
    auto& profiler = PaintProfiler::getInstance();
    Component comp;
    profiler.enableComponent (comp);

    auto snapshot = profiler.createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ ("Component", snapshot.components[0].name);
}

TEST (ComponentProfilingTests, GetPaintProfileNamePrefersTitle)
{
    auto& profiler = PaintProfiler::getInstance();
    Component comp ("myID");
    comp.setTitle ("MyTitle");
    profiler.enableComponent (comp);

    auto snapshot = profiler.createSnapshot();

    ASSERT_EQ (1u, snapshot.components.size());
    EXPECT_EQ ("MyTitle", snapshot.components[0].name);
}

TEST (ComponentProfilingTests, PaintProfilingCanBeDisabledPerComponent)
{
    Component comp;

    EXPECT_FALSE (comp.isPaintProfilingDisabled());

    comp.setPaintProfilingDisabled (true);
    EXPECT_TRUE (comp.isPaintProfilingDisabled());

    comp.setPaintProfilingDisabled (false);
    EXPECT_FALSE (comp.isPaintProfilingDisabled());
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

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (root));
    EXPECT_NE (nullptr, PaintProfiler::getInstance().getStatsForComponent (root));
}

TEST_F (PaintProfilerFixture, EnableSubtreeEnablesChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    profiler->enableSubtree (root);

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (root));
    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (child));
}

TEST_F (PaintProfilerFixture, EnableSubtreeEnablesDeepHierarchy)
{
    Component root;
    Component mid;
    Component leaf;
    root.addChildComponent (mid);
    mid.addChildComponent (leaf);

    profiler->enableSubtree (root);

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (root));
    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (mid));
    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (leaf));
}

TEST_F (PaintProfilerFixture, DeletedEnabledComponentIsRemovedFromRegistry)
{
    auto comp = std::make_unique<Component>();
    profiler->enableComponent (*comp);

    EXPECT_TRUE (PaintProfiler::hasRegisteredComponents());
    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (*comp));

    comp.reset();

    EXPECT_FALSE (PaintProfiler::hasRegisteredComponents());
    EXPECT_TRUE (profiler->createSnapshot().components.empty());
}

TEST_F (PaintProfilerFixture, DeletedComponentWhileProfilerDisabledIsPrunedFromRegistry)
{
    profiler->setEnabled (false);

    auto comp = std::make_unique<Component>();
    profiler->enableComponent (*comp);

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (*comp));

    comp.reset();
    profiler->setEnabled (true);

    Component addressReuseProbe;
    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (addressReuseProbe));
    EXPECT_TRUE (profiler->createSnapshot().components.empty());
    EXPECT_FALSE (PaintProfiler::hasRegisteredComponents());
}

TEST_F (PaintProfilerFixture, DisableSubtreeDisablesRootAndChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    profiler->enableSubtree (root);
    profiler->disableSubtree (root);

    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (root));
    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (child));
}

TEST_F (PaintProfilerFixture, ResetSubtreeResetsRootStats)
{
    Component root;
    profiler->enableSubtree (root);

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (root);
    stats->recordSample (makeSample (10.0));
    stats->recordSample (makeSample (20.0));
    ASSERT_EQ (2, stats->getSampleCount());

    profiler->resetSubtree (root);

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (root)->getSampleCount());
}

TEST_F (PaintProfilerFixture, ResetSubtreeResetsChildStats)
{
    Component root;
    Component child;
    root.addChildComponent (child);
    profiler->enableSubtree (root);

    PaintProfiler::getInstance().getStatsForComponent (child)->recordSample (makeSample (15.0));
    ASSERT_EQ (1, PaintProfiler::getInstance().getStatsForComponent (child)->getSampleCount());

    profiler->resetSubtree (root);

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (child)->getSampleCount());
}

TEST_F (PaintProfilerFixture, ResetAllClearsAllRegisteredStats)
{
    Component comp1;
    Component comp2;
    profiler->enableSubtree (comp1);
    profiler->enableSubtree (comp2);

    PaintProfiler::getInstance().getStatsForComponent (comp1)->recordSample (makeSample (10.0));
    PaintProfiler::getInstance().getStatsForComponent (comp2)->recordSample (makeSample (20.0));

    profiler->resetAll();

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (comp1)->getSampleCount());
    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (comp2)->getSampleCount());
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
        PaintProfiler::getInstance().getStatsForComponent (slow)->recordSample (makeSample (100.0));

    for (int i = 0; i < 5; ++i)
        PaintProfiler::getInstance().getStatsForComponent (fast)->recordSample (makeSample (10.0));

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
        PaintProfiler::getInstance().getStatsForComponent (highSelf)->recordSample (makeSample (100.0, 80.0, 10.0));

    for (int i = 0; i < 5; ++i)
        PaintProfiler::getInstance().getStatsForComponent (lowSelf)->recordSample (makeSample (100.0, 10.0, 80.0));

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
        PaintProfiler::getInstance().getStatsForComponent (highChildren)->recordSample (makeSample (100.0, 10.0, 80.0));

    for (int i = 0; i < 5; ++i)
        PaintProfiler::getInstance().getStatsForComponent (lowChildren)->recordSample (makeSample (100.0, 80.0, 10.0));

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

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (comp);
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

    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (root));
    EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (child));
}

TEST_F (PaintProfilerFixture, ScopedSessionDestructorDisablesProfiling)
{
    Component root;

    {
        auto session = profiler->startSession (root);
        EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (root));
    }

    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (root));
}

TEST_F (PaintProfilerFixture, ScopedSessionDestructorDisablesChildren)
{
    Component root;
    Component child;
    root.addChildComponent (child);

    {
        auto session = profiler->startSession (root);
        EXPECT_TRUE (PaintProfiler::getInstance().isComponentEnabled (child));
    }

    EXPECT_FALSE (PaintProfiler::getInstance().isComponentEnabled (child));
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

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (root);
    stats->recordSample (makeSample (10.0));
    stats->recordSample (makeSample (20.0));
    ASSERT_EQ (2, stats->getSampleCount());

    session->reset();

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (root)->getSampleCount());
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

// =============================================================================
// Paint profiling integration tests — exercises Component::internalPaint
// =============================================================================

namespace
{

class PaintableComponent : public yup::Component
{
public:
    PaintableComponent() = default;

    void paint (yup::Graphics&) override {}
};

} // namespace

class ComponentPaintProfilingFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GraphicsContext::Options opts;
        opts.allowHeadlessRendering = true;
        context = GraphicsContext::createContext (GraphicsContext::Api::Headless, opts);
        renderer = context->makeRenderer (200, 200);

        profiler = &PaintProfiler::getInstance();
        profiler->setEnabled (true);
        profiler->resetAll();
    }

    void TearDown() override
    {
        profiler->setEnabled (false);
        profiler->resetAll();
    }

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    PaintProfiler* profiler = nullptr;
};

TEST_F (ComponentPaintProfilingFixture, RecordsSampleAfterPaint)
{
    PaintableComponent comp;
    comp.setBounds (0, 0, 100, 100);
    comp.setVisible (true);
    PaintProfiler::getInstance().enableComponent (comp);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (comp, g, comp.getBounds());

    EXPECT_EQ (1, PaintProfiler::getInstance().getStatsForComponent (comp)->getSampleCount());
}

TEST_F (ComponentPaintProfilingFixture, NoSampleWhenProfilerDisabled)
{
    profiler->setEnabled (false);

    PaintableComponent comp;
    comp.setBounds (0, 0, 100, 100);
    comp.setVisible (true);
    PaintProfiler::getInstance().enableComponent (comp);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (comp, g, comp.getBounds());

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (comp)->getSampleCount());
}

TEST_F (ComponentPaintProfilingFixture, NoSampleWhenComponentPaintProfilingDisabled)
{
    PaintableComponent comp;
    comp.setBounds (0, 0, 100, 100);
    comp.setVisible (true);
    comp.setPaintProfilingDisabled (true);
    PaintProfiler::getInstance().enableComponent (comp);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (comp, g, comp.getBounds());

    EXPECT_EQ (0, PaintProfiler::getInstance().getStatsForComponent (comp)->getSampleCount());
}

TEST_F (ComponentPaintProfilingFixture, NoStatsWhenComponentProfilingNotEnabled)
{
    PaintableComponent comp;
    comp.setBounds (0, 0, 100, 100);
    comp.setVisible (true);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (comp, g, comp.getBounds());

    EXPECT_EQ (nullptr, PaintProfiler::getInstance().getStatsForComponent (comp));
}

TEST_F (ComponentPaintProfilingFixture, SampleHasNonNegativeTotalTime)
{
    PaintableComponent comp;
    comp.setBounds (0, 0, 100, 100);
    comp.setVisible (true);
    PaintProfiler::getInstance().enableComponent (comp);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (comp, g, comp.getBounds());

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (comp);
    ASSERT_EQ (1, stats->getSampleCount());
    EXPECT_GE (stats->getLastSample().totalMicros, 0.0);
}

TEST_F (ComponentPaintProfilingFixture, SelfPaintSkippedWhenOpaqueChildCoversArea)
{
    PaintableComponent parent;
    parent.setBounds (0, 0, 100, 100);
    parent.setVisible (true);
    parent.setOpaque (true);
    PaintProfiler::getInstance().enableComponent (parent);

    PaintableComponent child;
    child.setBounds (0, 0, 100, 100);
    child.setVisible (true);
    child.setOpaque (true);
    parent.addChildComponent (child);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (parent, g, parent.getBounds());

    auto* stats = PaintProfiler::getInstance().getStatsForComponent (parent);
    ASSERT_EQ (1, stats->getSampleCount());
    EXPECT_TRUE (stats->getLastSample().selfPaintSkipped);
}

TEST_F (ComponentPaintProfilingFixture, ChildTimeContributesToParentChildrenMicros)
{
    PaintableComponent parent;
    parent.setBounds (0, 0, 100, 100);
    parent.setVisible (true);
    PaintProfiler::getInstance().enableComponent (parent);

    PaintableComponent child;
    child.setBounds (0, 0, 50, 50);
    child.setVisible (true);
    PaintProfiler::getInstance().enableComponent (child);
    parent.addChildComponent (child);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (parent, g, parent.getBounds());

    auto* parentStats = PaintProfiler::getInstance().getStatsForComponent (parent);
    ASSERT_EQ (1, parentStats->getSampleCount());
    EXPECT_GE (parentStats->getLastSample().childrenMicros, 0.0);
    EXPECT_GE (parentStats->getLastSample().totalMicros, parentStats->getLastSample().childrenMicros);
}

TEST_F (ComponentPaintProfilingFixture, UnprofiledChildTimeContributesToProfiledParentChildrenMicros)
{
    PaintableComponent parent;
    parent.setBounds (0, 0, 100, 100);
    parent.setVisible (true);
    PaintProfiler::getInstance().enableComponent (parent);

    PaintableComponent child;
    child.setBounds (0, 0, 50, 50);
    child.setVisible (true);
    parent.addChildComponent (child);

    Graphics g (*context, *renderer, 1.0f);
    ComponentTestHelper::triggerPaint (parent, g, parent.getBounds());

    auto* parentStats = PaintProfiler::getInstance().getStatsForComponent (parent);
    ASSERT_EQ (1, parentStats->getSampleCount());
    EXPECT_EQ (nullptr, PaintProfiler::getInstance().getStatsForComponent (child));
    EXPECT_GE (parentStats->getLastSample().childrenMicros, 0.0);
    EXPECT_GE (parentStats->getLastSample().totalMicros, parentStats->getLastSample().childrenMicros);
}

TEST_F (ComponentPaintProfilingFixture, MultiplePaintsAccumulateSamples)
{
    PaintableComponent comp;
    comp.setBounds (0, 0, 100, 100);
    comp.setVisible (true);
    PaintProfiler::getInstance().enableComponent (comp);

    constexpr int paintCount = 5;
    for (int i = 0; i < paintCount; ++i)
    {
        Graphics g (*context, *renderer, 1.0f);
        ComponentTestHelper::triggerPaint (comp, g, comp.getBounds());
    }

    EXPECT_EQ (paintCount, PaintProfiler::getInstance().getStatsForComponent (comp)->getSampleCount());
}
