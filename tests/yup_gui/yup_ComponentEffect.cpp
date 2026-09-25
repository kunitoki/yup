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

    static GpuCanvas::Ptr getEffectOffscreenCanvas (const Component& comp)
    {
        return comp.effectOffscreenCanvas;
    }

    static void triggerPaint (Component& comp, Graphics& g, const Rectangle<float>& repaintArea, bool renderContinuous = false)
    {
        comp.internalPaint (g, RectangleList<float> { repaintArea }, renderContinuous);
    }

    static void triggerPaint (Component& comp, Graphics& g, const RectangleList<float>& repaintRegion, bool renderContinuous = false)
    {
        comp.internalPaint (g, repaintRegion, renderContinuous);
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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
    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (ctx, nullptr);
    EXPECT_FALSE (comp.snapshotToImage (*ctx).isValid());
}

TEST_F (ComponentEffectTest, HeadlessContextReturnsEmpty)
{
    Component comp ("test");
    comp.setBounds (0, 0, 200, 200);

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (ctx, nullptr);
    EXPECT_FALSE (comp.snapshotToImage (*ctx).isValid());
}

TEST_F (ComponentEffectTest, IncludeEffectsFlagDoesNotCrashWithHeadless)
{
    Component comp ("test");
    comp.setBounds (0, 0, 200, 200);

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {});
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

    void triggerPaintOnCanvas (Component& comp, int w, int h, float scale = 1.0f)
    {
        auto canvas = GpuCanvas::create (*gpuContext, roundToInt (w * scale), roundToInt (h * scale));
        ASSERT_NE (canvas, nullptr);
        auto& g = canvas->beginDraw ({}, scale);
        ComponentHelper::triggerPaint (comp, g, comp.getLocalBounds(), false);
        canvas->commit();
    }

    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
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

    // The effect path always re-renders the full subtree, because a child repaint
    // doesn't invalidate the parent's cache and the effect must track live child
    // content. Caching is therefore ineffective here, which is what applyCount
    // rising on every paint shows.
    triggerPaintOnCanvas (*comp, 128, 128);
    EXPECT_GT (effect->applyCount, applyCountAfterFirst);

    // The canvas itself is reused while the component size is unchanged - only its
    // contents are redrawn - so no GPU render target is reallocated per frame.
    auto secondCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (secondCanvas, nullptr);
    EXPECT_EQ (firstCanvas.get(), secondCanvas.get());
}

TEST_F (ComponentEffectGpuTest, EffectCanvasIsReusedUntilTheComponentIsResized)
{
    if (! gpuContext)
        return;

    auto comp = makeComp ("test", 128, 128);
    comp->setVisible (true);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    triggerPaintOnCanvas (*comp, 128, 128);

    // Holding this reference also keeps the assertions below honest: the replaced
    // canvas stays alive, so a new one cannot be allocated at the same address.
    auto firstCanvas = ComponentHelper::getEffectOffscreenCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);
    EXPECT_EQ (128, firstCanvas->getWidth());
    EXPECT_EQ (128, firstCanvas->getHeight());

    // Unchanged size: the same canvas is drawn into again.
    triggerPaintOnCanvas (*comp, 128, 128);
    EXPECT_EQ (firstCanvas.get(), ComponentHelper::getEffectOffscreenCanvas (*comp).get());

    // A resize leaves the cached canvas the wrong size, so it has to be replaced.
    comp->setBounds (0, 0, 64, 64);
    triggerPaintOnCanvas (*comp, 64, 64);

    auto resizedCanvas = ComponentHelper::getEffectOffscreenCanvas (*comp);
    ASSERT_NE (resizedCanvas, nullptr);
    EXPECT_NE (firstCanvas.get(), resizedCanvas.get());
    EXPECT_EQ (64, resizedCanvas->getWidth());
    EXPECT_EQ (64, resizedCanvas->getHeight());
}

// =============================================================================
// Effect composite and component transforms
// =============================================================================

TEST_F (ComponentEffectGpuTest, EffectCompositeAppliesTheComponentTransformOnce)
{
    if (! gpuContext)
        return;

    struct TransformRecordingEffect : ComponentEffect
    {
        void apply (Graphics& g, GpuTexture::Ptr, Rectangle<float>) override
        {
            localToTarget = g.getTransform().translated (g.getDrawingArea().getTopLeft());
        }

        AffineTransform localToTarget;
    };

    struct TransformRecordingComponent : FillComponent
    {
        using FillComponent::FillComponent;

        void paint (Graphics& g) override
        {
            FillComponent::paint (g);
            localToTarget = g.getTransform().translated (g.getDrawingArea().getTopLeft());
        }

        AffineTransform localToTarget;
    };

    auto root = makeComp ("root", 256, 256);
    root->setVisible (true);

    FillComponent effected ("effected");
    effected.enableRenderingUnclipped (true);
    effected.setBounds (10, 20, 64, 64);
    effected.setTransform (AffineTransform::scaling (2.0f));
    root->addAndMakeVisible (effected);

    TransformRecordingComponent probe ("probe");
    probe.enableRenderingUnclipped (true);
    probe.setBounds (5, 5, 20, 20);
    effected.addAndMakeVisible (probe);

    auto effect = ReferenceCountedObjectPtr<TransformRecordingEffect> (new TransformRecordingEffect());
    effected.setComponentEffect (effect);

    triggerPaintOnCanvas (*root, 256, 256);

    // The subtree is rendered untransformed into the offscreen texture...
    EXPECT_TRUE (probe.localToTarget.approximatelyEqualTo (AffineTransform::translation (5.0f, 5.0f)));

    // ...and the composite applies the component transform exactly once.
    EXPECT_TRUE (effect->localToTarget.approximatelyEqualTo (AffineTransform::scaling (2.0f).translated (10.0f, 20.0f)));
}

// =============================================================================
// Opacity is applied once when compositing offscreen content
// =============================================================================

TEST_F (ComponentEffectGpuTest, EffectSubtreeIsRenderedOpaqueAndCompositedWithOpacity)
{
    if (! gpuContext)
        return;

    struct OpacityRecordingEffect : ComponentEffect
    {
        void apply (Graphics& g, GpuTexture::Ptr, Rectangle<float>) override
        {
            compositeOpacity = g.getOpacity();
        }

        float compositeOpacity = -1.0f;
    };

    struct OpacityRecordingComponent : FillComponent
    {
        using FillComponent::FillComponent;

        void paint (Graphics& g) override
        {
            FillComponent::paint (g);
            paintOpacity = g.getOpacity();
        }

        float paintOpacity = -1.0f;
    };

    auto comp = makeComp ("effected", 64, 64);
    comp->setVisible (true);
    comp->setOpacity (0.5f);

    OpacityRecordingComponent probe ("probe");
    probe.enableRenderingUnclipped (true);
    probe.setBounds (4, 4, 16, 16);
    comp->addAndMakeVisible (probe);

    auto effect = ReferenceCountedObjectPtr<OpacityRecordingEffect> (new OpacityRecordingEffect());
    comp->setComponentEffect (effect);

    triggerPaintOnCanvas (*comp, 64, 64);

    EXPECT_NEAR (1.0f, probe.paintOpacity, 0.01f);
    EXPECT_NEAR (0.5f, effect->compositeOpacity, 0.01f); // stored as uint8, 127/255
}

TEST_F (ComponentEffectGpuTest, CachedTextureIsPaintedAtFullOpacity)
{
    if (! gpuContext)
        return;

    struct OpacityRecordingComponent : FillComponent
    {
        using FillComponent::FillComponent;

        void paint (Graphics& g) override
        {
            FillComponent::paint (g);
            paintOpacity = g.getOpacity();
        }

        float paintOpacity = -1.0f;
    };

    OpacityRecordingComponent comp ("cached");
    comp.enableRenderingUnclipped (true);
    comp.setBounds (0, 0, 64, 64);
    comp.setVisible (true);
    comp.setOpacity (0.5f);
    comp.setCachedToTexture (true);

    triggerPaintOnCanvas (comp, 64, 64);

    ASSERT_NE (ComponentHelper::getCachedTextureCanvas (comp), nullptr);
    EXPECT_NEAR (1.0f, comp.paintOpacity, 0.01f);
}

// =============================================================================
// Offscreen canvases follow the target scale
// =============================================================================

TEST_F (ComponentEffectGpuTest, EffectCanvasMatchesTheTargetScale)
{
    if (! gpuContext)
        return;

    auto comp = makeComp ("effected", 64, 48);
    comp->setVisible (true);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    comp->setComponentEffect (effect);

    triggerPaintOnCanvas (*comp, 64, 48, 2.0f);

    auto canvas = ComponentHelper::getEffectOffscreenCanvas (*comp);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (128, canvas->getWidth());
    EXPECT_EQ (96, canvas->getHeight());
}

TEST_F (ComponentEffectGpuTest, CachedCanvasIsRecreatedWhenTheScaleChanges)
{
    if (! gpuContext)
        return;

    auto comp = makeComp ("cached", 64, 48);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 64, 48, 1.0f);

    auto firstCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);
    EXPECT_EQ (64, firstCanvas->getWidth());
    EXPECT_EQ (48, firstCanvas->getHeight());

    // No repaint(): the scale change alone must invalidate the cached texture.
    triggerPaintOnCanvas (*comp, 64, 48, 2.0f);

    auto scaledCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (scaledCanvas, nullptr);
    EXPECT_NE (firstCanvas.get(), scaledCanvas.get());
    EXPECT_EQ (128, scaledCanvas->getWidth());
    EXPECT_EQ (96, scaledCanvas->getHeight());
}

// =============================================================================
// Render to texture
// =============================================================================

TEST_F (ComponentEffectGpuTest, RenderToTextureIsReusedUntilTheSubtreeRepaints)
{
    if (! gpuContext)
        return;

    struct CountingFillComponent : FillComponent
    {
        using FillComponent::FillComponent;

        void paint (Graphics& g) override
        {
            FillComponent::paint (g);
            ++paintCount;
        }

        int paintCount = 0;
    };

    auto panel = makeComp ("panel", 64, 48);
    panel->setVisible (true);

    CountingFillComponent child ("child");
    child.enableRenderingUnclipped (true);
    child.setBounds (4, 4, 16, 16);
    panel->addAndMakeVisible (child);

    auto texture = panel->renderToTexture (*gpuContext);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (64, texture->getWidth());
    EXPECT_EQ (48, texture->getHeight());
    EXPECT_EQ (1, child.paintCount);

    // Nothing repainted: the texture is returned without rendering again.
    EXPECT_NE (panel->renderToTexture (*gpuContext), nullptr);
    EXPECT_EQ (1, child.paintCount);

    // A repaint anywhere in the subtree marks it dirty.
    child.repaint();
    EXPECT_NE (panel->renderToTexture (*gpuContext), nullptr);
    EXPECT_EQ (2, child.paintCount);

    // A resize renders at the new size.
    panel->setBounds (0, 0, 32, 32);
    texture = panel->renderToTexture (*gpuContext);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (32, texture->getWidth());
    EXPECT_EQ (3, child.paintCount);
}

TEST_F (ComponentEffectGpuTest, RenderToTextureAppliesTheEffect)
{
    if (! gpuContext)
        return;

    auto panel = makeComp ("panel", 64, 64);
    panel->setVisible (true);

    auto effect = ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect());
    panel->setComponentEffect (effect);

    EXPECT_NE (panel->renderToTexture (*gpuContext), nullptr);
    EXPECT_EQ (1, effect->applyCount);
}

TEST_F (ComponentEffectGpuTest, RenderToTextureHonorsScale)
{
    if (! gpuContext)
        return;

    auto panel = makeComp ("panel", 64, 48);
    panel->setVisible (true);

    auto texture = panel->renderToTexture (*gpuContext, 2.0f);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (128, texture->getWidth());
    EXPECT_EQ (96, texture->getHeight());

    // No repaint(): a different scale alone renders again at the new size.
    texture = panel->renderToTexture (*gpuContext, 1.0f);
    ASSERT_NE (texture, nullptr);
    EXPECT_EQ (64, texture->getWidth());
    EXPECT_EQ (48, texture->getHeight());
}

#endif // YUP_MAC
