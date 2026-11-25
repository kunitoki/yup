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
#include <yup_gui_markup/yup_gui_markup.h>
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
        comp.flags.paintAsOffscreenRoot = value;
    }

    static bool isPaintAsOffscreenRoot (const Component& comp)
    {
        return comp.flags.paintAsOffscreenRoot;
    }

    static bool isCachedToTextureOptionSet (const Component& comp)
    {
        return comp.flags.cachedToTexture;
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

    static void setOnDesktop (Component& comp, bool value)
    {
        comp.flags.onDesktop = value;
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
        ++paintCount;
        g.setFillColor (Color (0xff336699));
        g.fillAll();
    }

    int paintCount = 0;
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

    /** The shaders of Rive are compiled synchronously: its fallback shaders, which draw until the
        specialized ones are compiled in the background, are a level off them. The images compared
        would depend on when each of them was rendered.
    */
    static void SetUpTestSuite()
    {
        GraphicsContext::Options options;
        options.synchronousShaderCompilations = true;

        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, options);
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

TEST_F (ComponentEffectGpuTest, SnapshotScaleMultipliesTheImageSize)
{
    auto comp = makeComp ("test", 64, 32);

    const auto image = comp->snapshotToImage (*gpuContext, true, 2.0f);
    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 128);
    EXPECT_EQ (image.getHeight(), 64);

    // The drawing is scaled with the canvas, so it reaches the far corner
    EXPECT_GT (image.getPixelColor (124, 60).getAlpha(), 0);
}

TEST_F (ComponentEffectGpuTest, SnapshotScaleIsKeptThroughAnEffect)
{
    auto comp = makeComp ("test", 64, 32);
    comp->setComponentEffect (ReferenceCountedObjectPtr<CountingEffect> (new CountingEffect()));

    const auto image = comp->snapshotToImage (*gpuContext, true, 2.0f);
    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 128);
    EXPECT_EQ (image.getHeight(), 64);
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

TEST_F (ComponentEffectGpuTest, RepaintRedrawsCacheIntoSameCanvas)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 256, 256);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 256, 256);
    auto firstCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);
    EXPECT_EQ (comp->paintCount, 1);

    triggerPaintOnCanvas (*comp, 256, 256);
    EXPECT_EQ (comp->paintCount, 1);

    comp->repaint();
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp).get(), firstCanvas.get());

    triggerPaintOnCanvas (*comp, 256, 256);
    EXPECT_EQ (comp->paintCount, 2);
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (*comp).get(), firstCanvas.get());
}

TEST_F (ComponentEffectGpuTest, ClearsCachedCanvasWhenEnabled)
{
    Component comp ("test");

    auto canvas = GpuCanvas::create (*gpuContext, 64, 64);
    ASSERT_NE (canvas, nullptr);

    ComponentHelper::setCachedTextureCanvas (comp, canvas);
    EXPECT_NE (ComponentHelper::getCachedTextureCanvas (comp), nullptr);

    comp.setCachedToTexture (true);
    EXPECT_EQ (ComponentHelper::getCachedTextureCanvas (comp), nullptr);
}

TEST_F (ComponentEffectGpuTest, RepaintWithRectRedrawsCache)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 128, 128);
    EXPECT_EQ (comp->paintCount, 1);

    comp->repaint (Rectangle<float> (0, 0, 10, 10));
    triggerPaintOnCanvas (*comp, 128, 128);
    EXPECT_EQ (comp->paintCount, 2);
}

TEST_F (ComponentEffectGpuTest, ResizeRecreatesCacheAtNewSize)
{
    if (! gpuContext)
        return;
    auto comp = makeComp ("test", 128, 128);
    comp->setVisible (true);
    comp->setCachedToTexture (true);

    triggerPaintOnCanvas (*comp, 128, 128);
    auto firstCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (firstCanvas, nullptr);

    comp->setBounds (0, 0, 256, 192);
    triggerPaintOnCanvas (*comp, 256, 192);

    auto secondCanvas = ComponentHelper::getCachedTextureCanvas (*comp);
    ASSERT_NE (secondCanvas, nullptr);
    EXPECT_NE (secondCanvas.get(), firstCanvas.get());
    EXPECT_EQ (secondCanvas->getWidth(), 256);
    EXPECT_EQ (secondCanvas->getHeight(), 192);
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

    // The effect runs on every paint, over the subtree's last rendering, whether or not
    // any of its content was dirty.
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

TEST_F (ComponentEffectGpuTest, EffectCanvasAndBoundsCoverThePaintMargin)
{
    if (! gpuContext)
        return;

    struct BoundsRecordingEffect : ComponentEffect
    {
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float> bounds) override
        {
            appliedBounds = bounds;
        }

        Rectangle<float> appliedBounds;
    };

    auto comp = makeComp ("effected", 64, 48);
    comp->setVisible (true);
    comp->setPaintMargin (4.0f, 6.0f, 8.0f, 10.0f);

    auto effect = ReferenceCountedObjectPtr<BoundsRecordingEffect> (new BoundsRecordingEffect());
    comp->setComponentEffect (effect);

    triggerPaintOnCanvas (*comp, 64, 48, 2.0f);

    auto canvas = ComponentHelper::getEffectOffscreenCanvas (*comp);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (152, canvas->getWidth());
    EXPECT_EQ (128, canvas->getHeight());
    EXPECT_EQ (effect->appliedBounds, (Rectangle<float> { -4.0f, -6.0f, 76.0f, 64.0f }));

    // A margin that is not a whole number of pixels grows to the next one, 7.5 to 8 here,
    // so what is inside the bounds stays on the pixels it has without a margin
    comp->setPaintMargin (5.0f);

    triggerPaintOnCanvas (*comp, 64, 48, 1.5f);

    canvas = ComponentHelper::getEffectOffscreenCanvas (*comp);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (112, canvas->getWidth());
    EXPECT_EQ (88, canvas->getHeight());
    EXPECT_NEAR (-16.0f / 3.0f, effect->appliedBounds.getX(), 1.0e-4f);
    EXPECT_NEAR (64.0f + 32.0f / 3.0f, effect->appliedBounds.getWidth(), 1.0e-4f);
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

TEST_F (ComponentEffectGpuTest, WhatAComponentKeepsOfItsPaintingIsPaintedAgainWhenTheThemeChanges)
{
    const auto makeTheme = []
    {
        ApplicationTheme::Ptr theme (new ApplicationTheme());
        theme->setPalette (ThemePalette ({}, ThemePalette::Mode::dark));
        return theme;
    };

    const auto oldTheme = ApplicationTheme::getGlobalTheme();
    const ErasedScopeGuard restoreTheme ([oldTheme]
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
    });

    auto theme = makeTheme();
    ApplicationTheme::setGlobalTheme (theme);

    // A texture cache, an opaque component behind an effect and a subtree rendered to a texture
    auto cached = makeComp ("cached", 64, 64);
    cached->setVisible (true);
    cached->setCachedToTexture (true);

    FillComponent behindEffect ("behindEffect");
    behindEffect.setBounds (0, 0, 64, 64);
    behindEffect.setVisible (true);
    behindEffect.setComponentEffect (new CountingEffect());

    auto rendered = makeComp ("rendered", 64, 64);
    rendered->setVisible (true);

    const auto paintAll = [&]
    {
        triggerPaintOnCanvas (*cached, 64, 64);
        triggerPaintOnCanvas (behindEffect, 64, 64);
        EXPECT_NE (rendered->renderToTexture (*gpuContext), nullptr);
    };

    const auto expectPainted = [&] (int times)
    {
        EXPECT_EQ (cached->paintCount, times);
        EXPECT_EQ (behindEffect.paintCount, times);
        EXPECT_EQ (rendered->paintCount, times);
    };

    paintAll();
    paintAll();
    expectPainted (1);

    theme->setPalette (ThemePalette ({}, ThemePalette::Mode::light));
    paintAll();
    paintAll();
    expectPainted (2);

    // Another theme, set up the way the first one was
    ApplicationTheme::setGlobalTheme (makeTheme());
    paintAll();
    paintAll();
    expectPainted (3);
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

// =============================================================================
// Partial repaint under an effect
// =============================================================================

class ComponentEffectPartialRepaintTest : public ComponentEffectGpuTest
{
protected:
    /** A child that fills itself with a color and counts its paints. */
    class CountingChild : public Component
    {
    public:
        explicit CountingChild (Color colorToFill)
            : fill (colorToFill)
        {
        }

        void paint (Graphics& g) override
        {
            ++paints;
            g.setFillColor (fill);
            g.fillAll();
        }

        Color fill;
        int paints = 0;
    };

    void SetUp() override
    {
        ComponentEffectGpuTest::SetUp();

        if (gpuContext == nullptr)
            return;

        // An opaque, clipped root: the conditions under which only dirty content repaints.
        root = makeComp ("root", 128, 128);
        root->enableRenderingUnclipped (false);
        root->setVisible (true);
        root->addAndMakeVisible (left);
        root->addAndMakeVisible (right);
        left.setBounds (0, 0, 64, 128);
        right.setBounds (64, 0, 64, 128);

        effect = new CountingEffect();
        root->setComponentEffect (effect);

        paintRoot();
        left.paints = 0;
        right.paints = 0;
    }

    void paintRoot() { triggerPaintOnCanvas (*root, roundToInt (root->getWidth()), roundToInt (root->getHeight())); }

    static bool isNear (Color a, Color b)
    {
        return std::abs (a.getRed() - b.getRed()) <= 2 && std::abs (a.getGreen() - b.getGreen()) <= 2 && std::abs (a.getBlue() - b.getBlue()) <= 2;
    }

    std::unique_ptr<FillComponent> root;
    CountingChild left { Color (0xffff0000) };
    CountingChild right { Color (0xff0000ff) };
    ReferenceCountedObjectPtr<CountingEffect> effect;
};

TEST_F (ComponentEffectPartialRepaintTest, RepaintsOnlyTheDirtyChild)
{
    const auto applied = effect->applyCount;

    left.repaint();
    paintRoot();

    EXPECT_EQ (1, left.paints);
    EXPECT_EQ (0, right.paints);
    EXPECT_GT (effect->applyCount, applied);
}

TEST_F (ComponentEffectPartialRepaintTest, KeepsTheUndirtiedPixels)
{
    left.fill = Color (0xff00ff00);
    left.repaint();
    paintRoot();

    const auto image = ComponentHelper::getEffectOffscreenCanvas (*root)->asImage();
    ASSERT_TRUE (image.isValid());

    EXPECT_TRUE (isNear (Color (0xff00ff00), image.getPixelColor (32, 64)));
    EXPECT_TRUE (isNear (Color (0xff0000ff), image.getPixelColor (96, 64)));
}

TEST_F (ComponentEffectPartialRepaintTest, NothingDirtyPaintsNoChildButAppliesTheEffect)
{
    const auto applied = effect->applyCount;

    paintRoot();

    EXPECT_EQ (0, left.paints);
    EXPECT_EQ (0, right.paints);
    EXPECT_GT (effect->applyCount, applied);
}

TEST_F (ComponentEffectPartialRepaintTest, TransparentRootRepaintsEverything)
{
    root->setOpaque (false);

    left.repaint();
    paintRoot();

    EXPECT_EQ (1, left.paints);
    EXPECT_EQ (1, right.paints);
}

TEST_F (ComponentEffectPartialRepaintTest, UnclippedRootRepaintsEverything)
{
    root->enableRenderingUnclipped (true);

    left.repaint();
    paintRoot();

    EXPECT_EQ (1, right.paints);
}

TEST_F (ComponentEffectPartialRepaintTest, ResizingRepaintsEverything)
{
    root->setBounds (0, 0, 160, 128);
    paintRoot();

    EXPECT_EQ (1, left.paints);
    EXPECT_EQ (1, right.paints);
}

TEST_F (ComponentEffectPartialRepaintTest, TransformChangeDirtiesTheOldAndNewArea)
{
    // Moving the left half over the right one must repaint where it was and where it is.
    left.setTransform (AffineTransform::translation (10.0f, 0.0f));
    paintRoot();

    EXPECT_EQ (1, left.paints);
    EXPECT_EQ (1, right.paints);
}

TEST_F (ComponentEffectPartialRepaintTest, EffectCanvasHoldsWhatIsPaintedInThePaintMargin)
{
    // A strip in the left margin, over the whole height of the canvas
    CountingChild marker { Color (0xff00ff00) };
    marker.setBounds (-8, -8, 8, 144);
    root->addAndMakeVisible (marker);
    root->setPaintMargin (8.0f);
    paintRoot();

    const auto image = ComponentHelper::getEffectOffscreenCanvas (*root)->asImage();
    ASSERT_TRUE (image.isValid());
    ASSERT_EQ (144, image.getWidth());
    ASSERT_EQ (144, image.getHeight());

    // The bounds start at (8, 8) in the canvas, after the margin
    EXPECT_TRUE (isNear (Color (0xff00ff00), image.getPixelColor (4, 72)));
    EXPECT_TRUE (isNear (Color (0xffff0000), image.getPixelColor (40, 72)));
    EXPECT_TRUE (isNear (Color (0xff0000ff), image.getPixelColor (104, 72)));
    EXPECT_TRUE (image.getPixelColor (140, 72).isTransparent());
}

TEST_F (ComponentEffectPartialRepaintTest, DamageInsideTheBoundsIsRepaintedWhereItSitsInTheCanvas)
{
    root->setPaintMargin (8.0f);
    paintRoot();
    left.paints = 0;
    right.paints = 0;

    left.fill = Color (0xff00ff00);
    left.repaint();
    paintRoot();

    EXPECT_EQ (1, left.paints);
    EXPECT_EQ (0, right.paints);

    const auto image = ComponentHelper::getEffectOffscreenCanvas (*root)->asImage();
    ASSERT_TRUE (image.isValid());

    // The left half covers 8 to 72 in the canvas
    EXPECT_TRUE (isNear (Color (0xff00ff00), image.getPixelColor (10, 72)));
    EXPECT_TRUE (isNear (Color (0xff00ff00), image.getPixelColor (70, 72)));
    EXPECT_TRUE (isNear (Color (0xff0000ff), image.getPixelColor (74, 72)));
}

TEST_F (ComponentEffectPartialRepaintTest, DamageInThePaintMarginRepaintsEverything)
{
    CountingChild marker { Color (0xff00ff00) };
    marker.setOpaque (false);
    marker.setBounds (-8, -8, 8, 144);
    root->addAndMakeVisible (marker);
    root->setPaintMargin (8.0f);
    paintRoot();
    left.paints = 0;
    right.paints = 0;

    // Nothing opaque covers the margin again, so what was painted there has to be cleared
    marker.fill = Color (0x00000000);
    marker.repaint();
    paintRoot();

    EXPECT_EQ (1, left.paints);
    EXPECT_EQ (1, right.paints);

    const auto image = ComponentHelper::getEffectOffscreenCanvas (*root)->asImage();
    ASSERT_TRUE (image.isValid());
    EXPECT_TRUE (image.getPixelColor (4, 72).isTransparent());
}

// =============================================================================
// Paint margin under an effect and in a cached texture
// =============================================================================

class ComponentPaintMarginGpuTest : public ComponentEffectGpuTest
{
protected:
    /** Draws the subtree as it was rendered. */
    struct PassThroughEffect : ComponentEffect
    {
        void apply (Graphics& g, GpuTexture::Ptr texture, Rectangle<float> bounds) override
        {
            appliedBounds = bounds;
            g.drawTexture (texture, bounds);
        }

        Rectangle<float> appliedBounds;
    };

    /** Draws nothing of what it is given. */
    struct BlankEffect : ComponentEffect
    {
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}
    };

    /** Fills its bounds, and counts its paints. */
    class Patch : public Component
    {
    public:
        explicit Patch (Color colorToFill)
            : fill (colorToFill)
        {
        }

        void paint (Graphics& g) override
        {
            ++paints;
            g.setFillColor (fill);
            g.fillAll();
        }

        Color fill;
        int paints = 0;
    };

    /** Fills its bounds, and paints around them as far as `reach`: a disc on its first corner, a
        strip after its right edge and one after its bottom edge. */
    class Subject : public Component
    {
    public:
        explicit Subject (Color colorToFill)
            : fill (colorToFill)
        {
            setOpaque (false);
        }

        void paint (Graphics& g) override
        {
            ++paints;
            g.setFillColor (fill);
            g.fillAll();

            if (reach <= 0.0f)
                return;

            g.setFillColor (around);
            g.fillEllipse (-reach, -reach, reach * 2.0f, reach * 2.0f);
            g.fillRect (getWidth(), 0.0f, reach, getHeight());
            g.fillRect (0.0f, getHeight(), getWidth(), reach);
        }

        Color fill;
        Color around { 0xffffffff };
        float reach = 0.0f;
        int paints = 0;
    };

    /** A subject in a backdrop, with a child painting around itself too, and one that reaches out
        of the subject. */
    struct Scene
    {
        explicit Scene (float margin)
        {
            root.setBounds (0.0f, 0.0f, 96.0f, 80.0f);
            root.setVisible (true);

            subject.setComponentID ("subject");
            subject.reach = margin;
            subject.setPaintMargin (margin);
            subject.setBounds (24.0f, 20.0f, 40.0f, 32.0f);
            root.addAndMakeVisible (subject);

            nested.reach = jmin (margin, 3.0f);
            nested.setPaintMargin (nested.reach);
            nested.setBounds (20.0f, 6.0f, 14.0f, 12.0f);
            subject.addAndMakeVisible (nested);

            reaching.setBounds (-2.0f, 16.0f, 10.0f, 8.0f);
            subject.addAndMakeVisible (reaching);
        }

        void style (const String& text)
        {
            auto sheet = YuiParser::parseStyleSheet (text);
            ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();

            root.setStyleSheet (sheet.getReference());
            root.getStyleContext()->restyleNow();
        }

        Patch root { Color (0xff203040) };
        Subject subject { Color (0x80ff4020) };
        Subject nested { Color (0xc020c040) };
        Patch reaching { Color (0xff00ccff) };
    };

    /** Paints a scene as a window would, and returns what is shown. */
    Image render (Scene& scene, float scale)
    {
        auto canvas = GpuCanvas::create (*gpuContext, roundToInt (scene.root.getWidth() * scale), roundToInt (scene.root.getHeight() * scale));
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw ({}, scale);
        ComponentHelper::triggerPaint (scene.root, g, scene.root.getLocalBounds(), false);
        canvas->commit();

        return canvas->asImage();
    }

    /** Returns the largest difference of a channel between two pixels. */
    static int difference (Color a, Color b)
    {
        return jmax (std::abs (a.getRed() - b.getRed()),
                     std::abs (a.getGreen() - b.getGreen()),
                     std::abs (a.getBlue() - b.getBlue()),
                     std::abs (a.getAlpha() - b.getAlpha()));
    }

    /** Passes if no pixel of two images differs by more than what is allowed, and tells where they do. */
    static ::testing::AssertionResult showTheSame (const Image& first, const Image& second, int allowed = tolerance)
    {
        if (! first.isValid() || ! second.isValid() || first.getWidth() != second.getWidth() || first.getHeight() != second.getHeight())
            return ::testing::AssertionFailure() << "the images can't be compared";

        int count = 0, largest = 0;
        Point<int> worst;
        Rectangle<int> area;

        for (int y = 0; y < first.getHeight(); ++y)
        {
            for (int x = 0; x < first.getWidth(); ++x)
            {
                const auto distance = difference (first.getPixelColor (x, y), second.getPixelColor (x, y));
                if (distance <= allowed)
                    continue;

                area = count++ == 0 ? Rectangle<int> (x, y, 1, 1) : area.unionWith ({ x, y, 1, 1 });

                if (distance > largest)
                {
                    largest = distance;
                    worst = { x, y };
                }
            }
        }

        if (count == 0)
            return ::testing::AssertionSuccess();

        return ::testing::AssertionFailure()
            << count << " pixels differ, by up to " << largest << " at (" << worst.getX() << ", " << worst.getY() << "): "
            << String::toHexString (static_cast<int64> (first.getPixelColor (worst.getX(), worst.getY()).getARGB())) << " and "
            << String::toHexString (static_cast<int64> (second.getPixelColor (worst.getX(), worst.getY()).getARGB()))
            << ", all inside (" << area.getX() << ", " << area.getY() << ") - (" << area.getRight() << ", " << area.getBottom() << ")";
    }

    /** Returns true if the strip the subject paints after its right edge is in the image. The row
        read is the middle one, so it is the same whichever way up the image is. */
    static bool showsWhatIsPaintedAround (const Image& image, const Scene& scene, float scale)
    {
        const auto x = roundToInt ((scene.subject.getRight() + scene.subject.reach * 0.5f) * scale);

        return image.isValid() && image.getPixelColor (x, image.getHeight() / 2) == scene.subject.around;
    }

    static constexpr int tolerance = 2;

    /** A canvas dithers and rounds what is drawn in it, so a second one is a level further off. */
    static constexpr int toleranceThroughTwoCanvases = tolerance + 1;
};

TEST_F (ComponentPaintMarginGpuTest, AnEffectShowsWhatDirectPaintingShows)
{
    for (const auto scale : { 1.0f, 2.0f, 1.5f })
    {
        for (const auto margin : { 0.0f, 8.0f, 2.5f })
        {
            SCOPED_TRACE (String ("scale ") + String (scale) + ", margin " + String (margin));

            Scene direct (margin), effected (margin);
            effected.subject.setComponentEffect (new PassThroughEffect());
            effected.nested.setComponentEffect (new PassThroughEffect());

            const auto expected = render (direct, scale);
            EXPECT_EQ (margin > 0.0f, showsWhatIsPaintedAround (expected, direct, scale));

            // The nested child is drawn in its canvas, then in the one of the subject
            EXPECT_TRUE (showTheSame (expected, render (effected, scale), toleranceThroughTwoCanvases));
        }
    }
}

TEST_F (ComponentPaintMarginGpuTest, AStyleFilterShowsWhatIsPaintedInThePaintMargin)
{
    for (const auto scale : { 1.0f, 2.0f, 1.5f })
    {
        SCOPED_TRACE (String ("scale ") + String (scale));

        Scene direct (8.0f), filtered (8.0f), both (8.0f);
        filtered.style ("style { #subject { filter: hue-rotate(0deg); } }");
        both.style ("style { #subject { filter: hue-rotate(0deg); } }");
        both.subject.setComponentEffect (new PassThroughEffect());

        const auto expected = render (direct, scale);
        EXPECT_TRUE (showsWhatIsPaintedAround (expected, direct, scale));
        EXPECT_TRUE (showTheSame (expected, render (filtered, scale)));

        // With an effect too, the filter runs over what the effect drew in a canvas of its own
        EXPECT_TRUE (showTheSame (expected, render (both, scale), toleranceThroughTwoCanvases));
    }
}

TEST_F (ComponentPaintMarginGpuTest, ACachedComponentShowsWhatDirectPaintingShows)
{
    for (const auto scale : { 1.0f, 2.0f, 1.5f })
    {
        for (const auto margin : { 0.0f, 8.0f, 2.5f })
        {
            SCOPED_TRACE (String ("scale ") + String (scale) + ", margin " + String (margin));

            Scene direct (margin), cached (margin);
            cached.subject.setCachedToTexture (true);

            const auto expected = render (direct, scale);
            EXPECT_TRUE (showTheSame (expected, render (cached, scale)));

            // Painted again from the texture it kept
            EXPECT_TRUE (showTheSame (expected, render (cached, scale)));
            EXPECT_EQ (1, cached.subject.paints);
        }
    }
}

TEST_F (ComponentPaintMarginGpuTest, TheCachedTextureHoldsThePaintMargin)
{
    Scene scene (8.0f);
    scene.subject.setCachedToTexture (true);
    render (scene, 2.0f);

    auto canvas = ComponentHelper::getCachedTextureCanvas (scene.subject);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (112, canvas->getWidth());
    EXPECT_EQ (96, canvas->getHeight());

    // A margin that grows is a new texture, painted again
    scene.subject.setPaintMargin (12.0f);
    render (scene, 2.0f);

    canvas = ComponentHelper::getCachedTextureCanvas (scene.subject);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (128, canvas->getWidth());
    EXPECT_EQ (112, canvas->getHeight());
    EXPECT_EQ (2, scene.subject.paints);
}

TEST_F (ComponentPaintMarginGpuTest, APartialRepaintShowsWhatAFullOneShows)
{
    const auto makeOpaque = [] (Scene& scene)
    {
        scene.subject.fill = Color (0xffff4020);
        scene.subject.setOpaque (true);
        scene.subject.setComponentEffect (new PassThroughEffect());
    };

    for (const auto scale : { 1.0f, 2.0f, 1.5f })
    {
        SCOPED_TRACE (String ("scale ") + String (scale));

        Scene repainted (8.0f);
        makeOpaque (repainted);
        render (repainted, scale);

        // Inside the bounds: only what changed is painted again, over what the canvas kept
        repainted.nested.fill = Color (0xff2040ff);
        repainted.nested.repaint();

        {
            Scene fresh (8.0f);
            makeOpaque (fresh);
            fresh.nested.fill = Color (0xff2040ff);

            EXPECT_TRUE (showTheSame (render (fresh, scale), render (repainted, scale)));
        }

        EXPECT_EQ (2, repainted.nested.paints);
        EXPECT_EQ (1, repainted.reaching.paints);

        // In the margin: nothing covers what was painted there, so everything is painted again
        repainted.subject.around = Color (0xff000000);
        repainted.subject.repaint();

        {
            Scene fresh (8.0f);
            makeOpaque (fresh);
            fresh.nested.fill = Color (0xff2040ff);
            fresh.subject.around = Color (0xff000000);

            EXPECT_TRUE (showTheSame (render (fresh, scale), render (repainted, scale)));
        }

        EXPECT_EQ (2, repainted.reaching.paints);
    }
}

TEST_F (ComponentPaintMarginGpuTest, APixelPartlyInsideTheBoundsIsNotPaintedOverWhatWasKept)
{
    // 32.6 pixels high: the last row is covered in part, and painting it again over itself would add up
    Subject subject { Color (0xffff4020) };
    subject.setOpaque (true);
    subject.setBounds (0.0f, 0.0f, 40.0f, 32.6f);
    subject.setVisible (true);
    subject.setComponentEffect (new PassThroughEffect());

    const auto paint = [&]
    {
        triggerPaintOnCanvas (subject, 64, 64);
        return ComponentHelper::getEffectOffscreenCanvas (subject)->asImage();
    };

    const auto fresh = paint();

    for (int count = 0; count < 3; ++count)
    {
        subject.repaint();
        EXPECT_TRUE (showTheSame (fresh, paint()));
    }

    EXPECT_EQ (4, subject.paints);
}

TEST_F (ComponentPaintMarginGpuTest, AMarginLeavesWhatIsInsideTheBoundsOnItsPixels)
{
    // A size that is no whole number of pixels: 40.4 is rounded down and 32.6 up
    const auto paintAlone = [this] (float margin, ReferenceCountedObjectPtr<PassThroughEffect> effect)
    {
        auto subject = std::make_unique<Subject> (Color (0x80ff4020));
        subject->reach = 8.0f;
        subject->setPaintMargin (margin);
        subject->setBounds (0.0f, 0.0f, 40.4f, 32.6f);
        subject->setVisible (true);
        subject->setComponentEffect (effect);

        triggerPaintOnCanvas (*subject, 64, 64);
        return ComponentHelper::getEffectOffscreenCanvas (*subject);
    };

    ReferenceCountedObjectPtr<PassThroughEffect> without (new PassThroughEffect()), with (new PassThroughEffect());

    const auto bare = paintAlone (0.0f, without);
    const auto wide = paintAlone (8.0f, with);
    ASSERT_NE (bare, nullptr);
    ASSERT_NE (wide, nullptr);

    EXPECT_EQ (40, bare->getWidth());
    EXPECT_EQ (33, bare->getHeight());
    EXPECT_EQ (without->appliedBounds, (Rectangle<float> { 0.0f, 0.0f, 40.4f, 32.6f }));

    // The part of a pixel the width lost is held by the margin after it
    EXPECT_EQ (8 + 40 + 9, wide->getWidth());
    EXPECT_EQ (8 + 33 + 8, wide->getHeight());

    // A pixel of the canvas covers as much of the component as it does without a margin
    EXPECT_NEAR (without->appliedBounds.getWidth() / 40.0f, with->appliedBounds.getWidth() / 57.0f, 1.0e-5f);
    EXPECT_NEAR (without->appliedBounds.getHeight() / 33.0f, with->appliedBounds.getHeight() / 49.0f, 1.0e-5f);
    EXPECT_NEAR (-8.0f * 40.4f / 40.0f, with->appliedBounds.getX(), 1.0e-4f);

    const auto bareImage = bare->asImage();
    const auto wideImage = wide->asImage();
    ASSERT_TRUE (bareImage.isValid());
    ASSERT_TRUE (wideImage.isValid());

    // The first and last rows are left out: one of them is only partly inside the bounds
    int largest = 0;

    for (int y = 1; y < 32; ++y)
        for (int x = 0; x < 40; ++x)
            largest = jmax (largest, difference (bareImage.getPixelColor (x, y), wideImage.getPixelColor (x + 8, y + 8)));

    EXPECT_LE (largest, tolerance);
}

TEST_F (ComponentPaintMarginGpuTest, ACanvasKeepsItsRoomWhenThePaintMarginShrinks)
{
    for (const auto scale : { 1.0f, 2.0f, 1.5f })
    {
        SCOPED_TRACE (String ("scale ") + String (scale));

        // The subject paints as far as 8 around its bounds, whatever its margin lets through
        const auto showsWhatDirectPaintingShows = [&] (Scene& scene, float margin)
        {
            Scene direct (8.0f);
            direct.subject.setPaintMargin (margin);
            scene.subject.setPaintMargin (margin);

            return showTheSame (render (direct, scale), render (scene, scale));
        };

        Scene effected (8.0f), cached (8.0f);
        auto effect = ReferenceCountedObjectPtr<PassThroughEffect> (new PassThroughEffect());
        effected.subject.setComponentEffect (effect);
        cached.subject.setCachedToTexture (true);

        render (effected, scale);
        render (cached, scale);

        const auto effectCanvas = ComponentHelper::getEffectOffscreenCanvas (effected.subject);
        const auto cachedCanvas = ComponentHelper::getCachedTextureCanvas (cached.subject);
        const auto bounds = effect->appliedBounds;
        ASSERT_NE (effectCanvas, nullptr);
        ASSERT_NE (cachedCanvas, nullptr);

        // Less is painted in the canvas there is, and less of it is shown
        for (const auto margin : { 3.0f, 0.0f, 5.5f, 8.0f })
        {
            SCOPED_TRACE (String ("margin ") + String (margin));

            EXPECT_TRUE (showsWhatDirectPaintingShows (effected, margin));
            EXPECT_TRUE (showsWhatDirectPaintingShows (cached, margin));

            EXPECT_EQ (effectCanvas.get(), ComponentHelper::getEffectOffscreenCanvas (effected.subject).get());
            EXPECT_EQ (cachedCanvas.get(), ComponentHelper::getCachedTextureCanvas (cached.subject).get());
            EXPECT_EQ (bounds, effect->appliedBounds);
        }

        // A margin that grows past the room there is gets a new canvas
        EXPECT_TRUE (showsWhatDirectPaintingShows (effected, 12.0f));
        EXPECT_TRUE (showsWhatDirectPaintingShows (cached, 12.0f));

        EXPECT_GT (ComponentHelper::getEffectOffscreenCanvas (effected.subject)->getWidth(), effectCanvas->getWidth());
        EXPECT_GT (ComponentHelper::getCachedTextureCanvas (cached.subject)->getWidth(), cachedCanvas->getWidth());
    }
}

TEST_F (ComponentPaintMarginGpuTest, AMarginThatShrinksLeavesNothingBehindInTheCanvas)
{
    Subject subject { Color (0xffff4020) };
    subject.setOpaque (true);
    subject.reach = 8.0f;
    subject.setPaintMargin (8.0f);
    subject.setBounds (0.0f, 0.0f, 40.0f, 32.0f);
    subject.setVisible (true);
    subject.setComponentEffect (new PassThroughEffect());

    const auto paint = [&]
    {
        triggerPaintOnCanvas (subject, 64, 64);
        return ComponentHelper::getEffectOffscreenCanvas (subject)->asImage();
    };

    // The middle row, in the strip painted after the right edge
    const Point<int> around (8 + 40 + 4, 24);
    EXPECT_EQ (subject.around, paint().getPixelColor (around.getX(), around.getY()));

    // An opaque subject is painted again inside its bounds only: what it painted around them has to go
    subject.setPaintMargin (0.0f);

    const auto image = paint();
    ASSERT_EQ (56, image.getWidth());
    EXPECT_TRUE (image.getPixelColor (around.getX(), around.getY()).isTransparent());
    EXPECT_EQ (subject.fill, image.getPixelColor (8 + 20, 24));
}

TEST_F (ComponentPaintMarginGpuTest, AStyleShadowGoesThroughTheEffect)
{
    const String shadows ("shadow: 2 3 6 #000000c0; glow: 4 #ffffff80;");
    const String sheet ("style { #subject { " + shadows + " } }");

    for (const auto scale : { 1.0f, 2.0f })
    {
        SCOPED_TRACE (String ("scale ") + String (scale));

        // The subject is translucent, so a shadow painted twice behind it would show
        Scene direct (0.0f), effected (0.0f);
        direct.style (sheet);
        effected.style (sheet);
        effected.subject.setComponentEffect (new PassThroughEffect());

        ASSERT_GT (effected.subject.getPaintMargin().getBottom(), 0.0f);
        EXPECT_TRUE (showTheSame (render (direct, scale), render (effected, scale)));

        // What an effect or a filter does not draw is not shown, the shadow no more than the rest
        Scene hidden (0.0f), blank (0.0f), filtered (0.0f);
        hidden.subject.setVisible (false);
        blank.style (sheet);
        blank.subject.setComponentEffect (new BlankEffect());
        filtered.style ("style { #subject { " + shadows + " filter: opacity(0%); } }");

        const auto expected = render (hidden, scale);
        EXPECT_TRUE (showTheSame (expected, render (blank, scale)));
        EXPECT_TRUE (showTheSame (expected, render (filtered, scale)));
    }
}

TEST_F (ComponentPaintMarginGpuTest, AComponentOnTheDesktopKeepsItsCanvasesAtItsBounds)
{
    Subject subject { Color (0xffff4020) };
    subject.setPaintMargin (8.0f);
    subject.setBounds (0.0f, 0.0f, 64.0f, 48.0f);
    subject.setVisible (true);

    auto effect = ReferenceCountedObjectPtr<PassThroughEffect> (new PassThroughEffect());
    subject.setComponentEffect (effect);

    // Nothing shows outside a window
    ComponentHelper::setOnDesktop (subject, true);
    triggerPaintOnCanvas (subject, 64, 48, 2.0f);
    ComponentHelper::setOnDesktop (subject, false);

    auto canvas = ComponentHelper::getEffectOffscreenCanvas (subject);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (128, canvas->getWidth());
    EXPECT_EQ (96, canvas->getHeight());
    EXPECT_EQ (effect->appliedBounds, subject.getLocalBounds());
}

#endif // YUP_MAC
