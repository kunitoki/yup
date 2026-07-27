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
#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

#include "../mocks/yup_gui.h"

using namespace yup;
using ::testing::_;
using ::testing::NiceMock;

// =============================================================================
// Concrete effect for testing
// =============================================================================

namespace
{

class CountingEffect : public ComponentEffect
{
public:
    void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override
    {
        ++applyCount;
    }

    int applyCount = 0;
};

} // namespace

// =============================================================================
// Helper to access private Component members (declared as friend in Component.h)
// =============================================================================

namespace yup
{

template <>
class ComponentTestHelper<ComponentEffect>
{
public:
    static void setPaintAsOffscreenRoot (Component& comp, bool value)
    {
        comp.options.paintAsOffscreenRoot = value;
    }

    static bool isPaintAsOffscreenRoot (const Component& comp)
    {
        return comp.options.paintAsOffscreenRoot;
    }

    static bool isCachedToTextureOptionSet (const Component& comp)
    {
        return comp.options.cachedToTexture;
    }

    static ComponentEffect::Ptr getComponentEffect (const Component& comp)
    {
        return comp.componentEffect;
    }

    static GpuCanvas::Ptr getCachedTextureCanvas (const Component& comp)
    {
        return comp.cachedTextureCanvas;
    }

    static void setCachedTextureCanvas (Component& comp, GpuCanvas::Ptr canvas)
    {
        comp.cachedTextureCanvas = canvas;
    }

    static void triggerPaint (Component& comp, Graphics& g, const Rectangle<float>& repaintArea, bool renderContinuous = false)
    {
        comp.internalPaint (g, repaintArea, renderContinuous);
    }
};

} // namespace yup

// =============================================================================
// ComponentEffect Base Class Tests
// =============================================================================

class ComponentEffectTest : public ::testing::Test
{
protected:
    using ComponentHelper = yup::ComponentTestHelper<yup::ComponentEffect>;
};

TEST_F (ComponentEffectTest, ConstructsWithValidPtr)
{
    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    EXPECT_NE (effect, nullptr);
}

TEST_F (ComponentEffectTest, ReferenceCountingWorks)
{
    auto effect1 = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    {
        auto effect2 = effect1;
        EXPECT_EQ (effect1.get(), effect2.get());
    }
    // effect2 out of scope, effect1 still holds ref
    EXPECT_NE (effect1, nullptr);
}

TEST_F (ComponentEffectTest, PtrCanBeNull)
{
    ComponentEffect::Ptr effect;
    EXPECT_EQ (effect, nullptr);
}

TEST_F (ComponentEffectTest, ImplicitUpcastToBasePtr)
{
    auto concrete = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    ComponentEffect::Ptr base = concrete;
    EXPECT_NE (base, nullptr);
    EXPECT_EQ (base.get(), concrete.get());
}

TEST_F (ComponentEffectTest, ApplyIsCalled)
{
    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    EXPECT_EQ (effect->applyCount, 0);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto renderer = ctx->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*ctx, *renderer);

    effect->apply (g, nullptr, { 0, 0, 100, 100 });
    EXPECT_EQ (effect->applyCount, 1);
}

// =============================================================================
// Component::setComponentEffect / getComponentEffect
// =============================================================================

TEST_F (ComponentEffectTest, ReturnsNullByDefault)
{
    Component comp ("test");
    EXPECT_EQ (comp.getComponentEffect(), nullptr);
    EXPECT_EQ (ComponentHelper::getComponentEffect (comp), nullptr);
}

TEST_F (ComponentEffectTest, StoresAndReturnsPtr)
{
    Component comp ("test");
    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp.setComponentEffect (effect);

    EXPECT_NE (comp.getComponentEffect(), nullptr);
    EXPECT_EQ (comp.getComponentEffect().get(), effect.get());
    EXPECT_EQ (ComponentHelper::getComponentEffect (comp).get(), effect.get());
}

TEST_F (ComponentEffectTest, NullClearsEffect)
{
    Component comp ("test");
    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp.setComponentEffect (effect);
    EXPECT_NE (comp.getComponentEffect(), nullptr);

    comp.setComponentEffect (nullptr);
    EXPECT_EQ (comp.getComponentEffect(), nullptr);
}

TEST_F (ComponentEffectTest, ReplacingEffectWorks)
{
    Component comp ("test");
    auto e1 = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    auto e2 = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());

    comp.setComponentEffect (e1);
    EXPECT_EQ (ComponentHelper::getComponentEffect (comp).get(), e1.get());

    comp.setComponentEffect (e2);
    EXPECT_EQ (ComponentHelper::getComponentEffect (comp).get(), e2.get());
}

// =============================================================================
// Component::setCachedToTexture / isCachedToTexture
// =============================================================================

TEST_F (ComponentEffectTest, ReturnsFalseByDefault)
{
    Component comp ("test");
    EXPECT_FALSE (comp.isCachedToTexture());
}

TEST_F (ComponentEffectTest, EnablesAndDisables)
{
    Component comp ("test");
    comp.setCachedToTexture (true);
    EXPECT_TRUE (comp.isCachedToTexture());
    EXPECT_TRUE (ComponentHelper::isCachedToTextureOptionSet (comp));

    comp.setCachedToTexture (false);
    EXPECT_FALSE (comp.isCachedToTexture());
    EXPECT_FALSE (ComponentHelper::isCachedToTextureOptionSet (comp));
}

TEST_F (ComponentEffectTest, IdempotentWhenSameValue)
{
    Component comp ("test");
    comp.setCachedToTexture (true);
    EXPECT_TRUE (comp.isCachedToTexture());

    // Setting same value should not change state
    comp.setCachedToTexture (true);
    EXPECT_TRUE (comp.isCachedToTexture());
}

TEST_F (ComponentEffectTest, ClearsCachedCanvasWhenEnabled)
{
    Component comp ("test");

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto canvas = GpuCanvas::create (*ctx, 64, 64);
    if (canvas == nullptr)
        GTEST_SKIP() << "Headless context cannot create GpuCanvas";

    ComponentHelper::setCachedTextureCanvas (comp, canvas);
    EXPECT_NE (ComponentHelper::getCachedTextureCanvas (comp), nullptr);

    comp.setCachedToTexture (true);
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (comp), nullptr);
}

// =============================================================================
// Cache invalidation on repaint / setBounds
// =============================================================================

TEST_F (ComponentEffectTest, RepaintInvalidatesCache)
{
    Component comp ("test");

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto canvas = GpuCanvas::create (*ctx, 64, 64);
    if (canvas == nullptr)
        GTEST_SKIP() << "Headless context cannot create GpuCanvas";

    ComponentHelper::setCachedTextureCanvas (comp, canvas);
    EXPECT_NE (ComponentHelper::getCachedTextureCanvas (comp), nullptr);

    comp.repaint();
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (comp), nullptr);
}

TEST_F (ComponentEffectTest, RepaintWithRectInvalidatesCache)
{
    Component comp ("test");

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto canvas = GpuCanvas::create (*ctx, 64, 64);
    if (canvas == nullptr)
        GTEST_SKIP() << "Headless context cannot create GpuCanvas";

    ComponentHelper::setCachedTextureCanvas (comp, canvas);
    EXPECT_NE (ComponentHelper::getCachedTextureCanvas (comp), nullptr);

    comp.repaint (Rectangle<float> (0, 0, 10, 10));
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (comp), nullptr);
}

TEST_F (ComponentEffectTest, SetBoundsInvalidatesCache)
{
    Component comp ("test");
    comp.setBounds (0, 0, 200, 200);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto canvas = GpuCanvas::create (*ctx, 64, 64);
    if (canvas == nullptr)
        GTEST_SKIP() << "Headless context cannot create GpuCanvas";

    ComponentHelper::setCachedTextureCanvas (comp, canvas);
    EXPECT_NE (ComponentHelper::getCachedTextureCanvas (comp), nullptr);

    comp.setBounds (10, 20, 300, 400);
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (comp), nullptr);
}

// =============================================================================
// paintAsOffscreenRoot flag
// =============================================================================

TEST_F (ComponentEffectTest, OffByDefault)
{
    Component comp ("test");
    EXPECT_FALSE (ComponentHelper::isPaintAsOffscreenRoot (comp));
}

TEST_F (ComponentEffectTest, ToggleWorks)
{
    Component comp ("test");
    ComponentHelper::setPaintAsOffscreenRoot (comp, true);
    EXPECT_TRUE (ComponentHelper::isPaintAsOffscreenRoot (comp));
    ComponentHelper::setPaintAsOffscreenRoot (comp, false);
    EXPECT_FALSE (ComponentHelper::isPaintAsOffscreenRoot (comp));
}

TEST_F (ComponentEffectTest, BoundsWithOffscreenRootReturnsZeroPosition)
{
    Component comp ("test");
    comp.setBounds (100, 200, 300, 400);

    // Without offscreen root, getBoundsRelativeToTopLevelComponent returns
    // boundsInParent (no parent chain to walk, no onDesktop flag)
    auto bounds = comp.getBoundsRelativeToTopLevelComponent();
    EXPECT_FLOAT_EQ (bounds.getX(), 100.0f);
    EXPECT_FLOAT_EQ (bounds.getY(), 200.0f);

    // With offscreen root, returns withZeroPosition
    ComponentHelper::setPaintAsOffscreenRoot (comp, true);
    bounds = comp.getBoundsRelativeToTopLevelComponent();
    EXPECT_FLOAT_EQ (bounds.getX(), 0.0f);
    EXPECT_FLOAT_EQ (bounds.getY(), 0.0f);
    EXPECT_FLOAT_EQ (bounds.getWidth(), 300.0f);
    EXPECT_FLOAT_EQ (bounds.getHeight(), 400.0f);

    ComponentHelper::setPaintAsOffscreenRoot (comp, false);
}

TEST_F (ComponentEffectTest, ChildBoundsAreRelativeToOffscreenRoot)
{
    Component parent ("parent");
    parent.setBounds (50, 50, 400, 300);

    Component child ("child");
    child.setBounds (10, 20, 100, 80);
    parent.addChildComponent (child);

    // Without offscreen root, child bounds include parent offset
    auto childBounds = child.getBoundsRelativeToTopLevelComponent();
    EXPECT_FLOAT_EQ (childBounds.getX(), 60.0f); // 50 + 10
    EXPECT_FLOAT_EQ (childBounds.getY(), 70.0f); // 50 + 20

    // With parent as offscreen root, child bounds stop at parent
    ComponentHelper::setPaintAsOffscreenRoot (parent, true);
    childBounds = child.getBoundsRelativeToTopLevelComponent();
    EXPECT_FLOAT_EQ (childBounds.getX(), 10.0f);
    EXPECT_FLOAT_EQ (childBounds.getY(), 20.0f);

    ComponentHelper::setPaintAsOffscreenRoot (parent, false);
}

// =============================================================================
// paintChildrenAndOverChildren via internalPaint
// =============================================================================

TEST_F (ComponentEffectTest, ChildrenArePainted)
{
    NiceMock<MockComponent> child1 ("child1");
    NiceMock<MockComponent> child2 ("child2");
    NiceMock<MockComponent> parentMock ("parent");

    parentMock.setBounds (0, 0, 400, 300);
    parentMock.setVisible (true);
    child1.setBounds (10, 10, 100, 80);
    child2.setBounds (150, 20, 120, 90);
    parentMock.addAndMakeVisible (child1);
    parentMock.addAndMakeVisible (child2);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto renderer = ctx->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*ctx, *renderer);

    EXPECT_CALL (child1, paint (_)).Times (1);
    EXPECT_CALL (child2, paint (_)).Times (1);

    ComponentHelper::triggerPaint (parentMock, g, parentMock.getLocalBounds(), false);
}

TEST_F (ComponentEffectTest, PaintOverChildrenIsCalled)
{
    NiceMock<MockComponent> parentMock ("parent");
    parentMock.setBounds (0, 0, 400, 300);
    parentMock.setVisible (true);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto renderer = ctx->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*ctx, *renderer);

    // paint() is also called during normal rendering, so we use NiceMock
    EXPECT_CALL (parentMock, paint (_)).Times (1);
    EXPECT_CALL (parentMock, paintOverChildren (_)).Times (1);

    ComponentHelper::triggerPaint (parentMock, g, parentMock.getLocalBounds(), false);
}

// =============================================================================
// snapshotToImage (failure paths — GPU needed for full validate)
// =============================================================================

TEST_F (ComponentEffectTest, ZeroSizedReturnsEmpty)
{
    Component comp ("test");
    // Default 0×0 size
    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    EXPECT_FALSE (comp.snapshotToImage (*ctx).isValid());
}

TEST_F (ComponentEffectTest, HeadlessContextReturnsEmpty)
{
    Component comp ("test");
    comp.setBounds (0, 0, 200, 200);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    EXPECT_FALSE (comp.snapshotToImage (*ctx).isValid());
}

TEST_F (ComponentEffectTest, IncludeEffectsFlagDoesNotCrashWithHeadless)
{
    Component comp ("test");
    comp.setBounds (0, 0, 200, 200);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);

    // Both calls should return empty without crashing
    EXPECT_FALSE (comp.snapshotToImage (*ctx, true).isValid());
    EXPECT_FALSE (comp.snapshotToImage (*ctx, false).isValid());
}

// =============================================================================
// Caching does not prevent child painting
// =============================================================================

TEST_F (ComponentEffectTest, ChildrenStillPaintWhenParentIsCached)
{
    NiceMock<MockComponent> child ("child");
    NiceMock<MockComponent> parentMock ("parent");

    parentMock.setBounds (0, 0, 400, 300);
    parentMock.setVisible (true);
    child.setBounds (10, 10, 100, 80);
    parentMock.setCachedToTexture (true);
    parentMock.addAndMakeVisible (child);

    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);
    auto renderer = ctx->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*ctx, *renderer);

    EXPECT_CALL (child, paint (_)).Times (1);

    ComponentHelper::triggerPaint (parentMock, g, parentMock.getLocalBounds(), false);
}

// =============================================================================
// macOS GPU integration tests
// =============================================================================

#if YUP_MAC

/** Minimal component that fills itself with a solid colour, avoiding the
    default Component::paint() jassert. Unclipped so tests work with a raw
    makeRenderer() that lacks a full render-target frame. */
class FillComponent : public Component
{
public:
    using Component::Component;

    void paint (Graphics& g) override
    {
        g.setFillColor (Color (0xff336699));
        g.fillAll();
    }
};

class ComponentEffectGpuTest : public ::testing::Test
{
protected:
    using ComponentHelper = yup::ComponentTestHelper<yup::ComponentEffect>;

    std::unique_ptr<FillComponent> makeComp (StringRef id, float w, float h)
    {
        auto comp = std::make_unique<FillComponent> (id);
        comp->setBounds (0, 0, w, h);
        comp->enableRenderingUnclipped (true);
        return comp;
    }

    void triggerPaintOnCanvas (Component& comp, int w, int h)
    {
        auto canvas = GpuCanvas::create (*gpuContext, w, h);
        ASSERT_NE (canvas, nullptr);
        auto& g = canvas->beginDraw();
        ComponentHelper::triggerPaint (comp, g, comp.getLocalBounds(), false);
        canvas->commit();
    }

    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GraphicsContext::Metal, {});
        if (gpuContext == nullptr)
            return;

        auto probe = GpuCanvas::create (*gpuContext, 64, 64);
        if (probe == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> ComponentEffectGpuTest::gpuContext;

TEST_F (ComponentEffectGpuTest, SnapshotRendersToImage)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);

    auto image = comp->snapshotToImage (*gpuContext);
    EXPECT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 128);
    EXPECT_EQ (image.getHeight(), 128);

    auto rawData = image.getRawData();
    EXPECT_FALSE (rawData.empty());
    EXPECT_EQ (rawData.size(), 128u * 128u * 4u);
}

TEST_F (ComponentEffectGpuTest, SnapshotWithEffectsAppliesEffect)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    auto image = comp->snapshotToImage (*gpuContext, true);
    EXPECT_TRUE (image.isValid());
    EXPECT_EQ (effect->applyCount, 1);
}

TEST_F (ComponentEffectGpuTest, SnapshotExcludingEffectsSkipsEffect)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    auto image = comp->snapshotToImage (*gpuContext, false);
    EXPECT_TRUE (image.isValid());
    EXPECT_EQ (effect->applyCount, 0);
}

// =============================================================================
// Snapshot to texture (GPU-only, no CPU readback)
// =============================================================================

TEST_F (ComponentEffectGpuTest, SnapshotToTextureReturnsValidTexture)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 200, 150);

    auto tex = comp->snapshotToTexture (*gpuContext);
    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (tex->isValid());
    EXPECT_EQ (tex->getWidth(), 200);
    EXPECT_EQ (tex->getHeight(), 150);
}

TEST_F (ComponentEffectGpuTest, SnapshotToTextureWithEffects)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    auto tex = comp->snapshotToTexture (*gpuContext, true);
    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (tex->isValid());
    EXPECT_EQ (effect->applyCount, 1);
}

TEST_F (ComponentEffectGpuTest, SnapshotToTextureExcludingEffects)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    auto tex = comp->snapshotToTexture (*gpuContext, false);
    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (tex->isValid());
    EXPECT_EQ (effect->applyCount, 0);
}

TEST_F (ComponentEffectGpuTest, SnapshotToTextureAndImageProduceSameDimensions)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 256, 128);

    auto tex = comp->snapshotToTexture (*gpuContext);
    auto img = comp->snapshotToImage (*gpuContext);

    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (img.isValid());
    EXPECT_EQ (tex->getWidth(), img.getWidth());
    EXPECT_EQ (tex->getHeight(), img.getHeight());
}

TEST_F (ComponentEffectGpuTest, SnapshotZeroSizedComponentReturnsNullTexture)
{
    if (! gpuContext)
        return;
    FillComponent comp ("test");

    auto tex = comp.snapshotToTexture (*gpuContext);
    EXPECT_EQ (tex, nullptr);
}

// =============================================================================
// Cache + Paint
// =============================================================================

TEST_F (ComponentEffectGpuTest, CachedComponentCreatesCanvasOnFirstPaint)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 256, 256);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp), nullptr);

    triggerPaintOnCanvas (*comp, 256, 256);

    auto cached = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (cached, nullptr);
    auto tex = cached->asTexture();
    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (tex->isValid());
}

TEST_F (ComponentEffectGpuTest, CachedComponentReusesTextureOnSubsequentPaints)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 256, 256);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 256, 256);
    auto firstCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);

    triggerPaintOnCanvas (*comp, 256, 256);
    EXPECT_EQ (firstCanvas.get(), ComponentHelper::getCachedTextureCanvas (*comp).get());
}

TEST_F (ComponentEffectGpuTest, RepaintInvalidatesAndRecreatesCache)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 256, 256);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 256, 256);
    auto firstCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);

    comp->repaint();
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp), nullptr);

    triggerPaintOnCanvas (*comp, 256, 256);
    auto secondCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (secondCanvas, nullptr);
    EXPECT_NE (firstCanvas.get(), secondCanvas.get());
}

TEST_F (ComponentEffectGpuTest, DisablingCacheClearsStoredCanvas)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 256, 256);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 256, 256);
    EXPECT_NE (ComponentHelper::getCachedTextureCanvas (*comp), nullptr);

    comp->setCachedToTexture (false);
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp), nullptr);
}

// =============================================================================
// Cache + Snapshot
// =============================================================================

TEST_F (ComponentEffectGpuTest, SnapshotOfCachedComponentStillCapturesSubtree)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 200, 200);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 200, 200);

    auto tex = comp->snapshotToTexture (*gpuContext);
    ASSERT_NE (tex, nullptr);
    EXPECT_EQ (tex->getWidth(), 200);
    EXPECT_EQ (tex->getHeight(), 200);

    auto img = comp->snapshotToImage (*gpuContext);
    EXPECT_TRUE (img.isValid());
    EXPECT_EQ (img.getWidth(), 200);
    EXPECT_EQ (img.getHeight(), 200);
}

TEST_F (ComponentEffectGpuTest, SnapshotDoesNotAffectPaintCache)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 200, 200);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 200, 200);
    auto cachedBefore = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (cachedBefore, nullptr);

    // Snapshot should NOT invalidate the paint cache
    auto img = comp->snapshotToImage (*gpuContext);
    EXPECT_TRUE (img.isValid());
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp).get(), cachedBefore.get());

    auto tex = comp->snapshotToTexture (*gpuContext);
    ASSERT_NE (tex, nullptr);
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp).get(), cachedBefore.get());
}

// =============================================================================
// Effect + Cache
// =============================================================================

TEST_F (ComponentEffectGpuTest, EffectPlusCacheRendersEffectEveryFrame)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    triggerPaintOnCanvas (*comp, 128, 128);
    int applyCountAfterFirst = effect->applyCount;
    EXPECT_GE (applyCountAfterFirst, 1);

    auto firstCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);

    // Effect path always re-renders the full subtree (so cached canvas
    // is recreated), because a child repaint doesn't invalidate the
    // parent's cache. Effect must track live child content.
    triggerPaintOnCanvas (*comp, 128, 128);
    EXPECT_GT (effect->applyCount, applyCountAfterFirst);

    auto secondCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (secondCanvas, nullptr);
    EXPECT_NE (firstCanvas.get(), secondCanvas.get());
}

#endif // YUP_MAC
