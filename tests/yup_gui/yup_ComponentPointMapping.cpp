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

// =============================================================================
// Input mapping through transforms, effects and parent hooks
// =============================================================================

class ComponentPointMappingTests : public ::testing::Test
{
protected:
    /** Displays the content zoomed 2x around the center of the component. */
    class ZoomEffect : public ComponentEffect
    {
    public:
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}

        std::optional<Point<float>> displayToContent (Point<float> displayPoint, Rectangle<float> bounds) const override
        {
            const auto center = bounds.getCenter();
            return center + (displayPoint - center) * 0.5f;
        }

        std::optional<Point<float>> contentToDisplay (Point<float> contentPoint, Rectangle<float> bounds) const override
        {
            const auto center = bounds.getCenter();
            return center + (contentPoint - center) * 2.0f;
        }
    };

    /** An effect whose mapping is never defined. */
    class DegenerateEffect : public ComponentEffect
    {
    public:
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}

        std::optional<Point<float>> displayToContent (Point<float>, Rectangle<float>) const override { return std::nullopt; }

        std::optional<Point<float>> contentToDisplay (Point<float>, Rectangle<float>) const override { return std::nullopt; }
    };

    /** Presents its children mirrored horizontally and at half size, like a host projecting them. */
    class MirroringHost : public Component
    {
    public:
        std::optional<Point<float>> getChildPointFromLocal (const Component&, Point<float> localPoint) const override
        {
            return Point<float> ((getWidth() - localPoint.getX()) * 0.5f, localPoint.getY() * 0.5f);
        }

        std::optional<Point<float>> getLocalPointFromChild (const Component&, Point<float> childPoint) const override
        {
            return Point<float> (getWidth() - childPoint.getX() * 2.0f, childPoint.getY() * 2.0f);
        }
    };

    class DropTarget : public Component
        , public DragAndDropTarget
    {
    public:
        bool isInterestedInDragSource (const DragAndDropSourceDetails&) override { return true; }

        bool itemDropped (const DragAndDropSourceDetails& details) override
        {
            lastDropPosition = details.localPosition;
            return true;
        }

        Point<float> lastDropPosition;
    };

    void SetUp() override
    {
        root.setBounds (0.0f, 0.0f, 400.0f, 400.0f);
        root.setVisible (true);
    }

    static void addVisible (Component& parent, Component& child, Rectangle<float> bounds)
    {
        child.setBounds (bounds);
        parent.addAndMakeVisible (child);
    }

    static void expectNear (Point<float> actual, Point<float> expected, float tolerance = 1.0e-3f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
    }

    static MouseEvent eventAt (Point<float> topLevelPosition)
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), topLevelPosition);
    }

    Component root { "root" };
    Component parent { "parent" };
    Component child { "child" };
};

TEST_F (ComponentPointMappingTests, UntransformedHierarchyMapsByPositionOnly)
{
    addVisible (root, parent, { 50.0f, 50.0f, 200.0f, 150.0f });
    addVisible (parent, child, { 25.0f, 25.0f, 100.0f, 75.0f });

    EXPECT_EQ (root.findComponentAt ({ 80.0f, 80.0f }), &child);
    EXPECT_EQ (root.findComponentAt ({ 60.0f, 60.0f }), &parent);
    EXPECT_EQ (child.getLocalPointFromTopLevel ({ 80.0f, 80.0f }), Point<float> (5.0f, 5.0f));
    EXPECT_EQ (*child.getParentPointFromLocal ({ 5.0f, 5.0f }), Point<float> (30.0f, 30.0f));
}

TEST_F (ComponentPointMappingTests, HitTestFollowsScaledParent)
{
    addVisible (root, parent, { 100.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    // The child is displayed at (120, 120) - (160, 160)
    EXPECT_EQ (root.findComponentAt ({ 130.0f, 130.0f }), &child);
    EXPECT_EQ (root.findComponentAt ({ 115.0f, 115.0f }), &parent);

    // The parent covers (100, 100) - (300, 300) once scaled
    EXPECT_EQ (root.findComponentAt ({ 250.0f, 250.0f }), &parent);
    EXPECT_EQ (root.findComponentAt ({ 90.0f, 90.0f }), &root);
}

TEST_F (ComponentPointMappingTests, HitTestFollowsRotatedChild)
{
    addVisible (root, child, { 50.0f, 50.0f, 100.0f, 20.0f });
    child.setTransform (AffineTransform::rotation (MathConstants<float>::halfPi));

    // Local (50, 10) is displayed at (-10, 50) + (50, 50)
    EXPECT_EQ (root.findComponentAt ({ 40.0f, 100.0f }), &child);
    expectNear (child.getLocalPointFromTopLevel ({ 40.0f, 100.0f }), { 50.0f, 10.0f });

    // Inside the untransformed bounds, but not where the child is displayed
    EXPECT_EQ (root.findComponentAt ({ 100.0f, 60.0f }), &root);
}

TEST_F (ComponentPointMappingTests, RelativeMouseEventFollowsTransforms)
{
    addVisible (root, parent, { 100.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    const auto event = eventAt ({ 130.0f, 130.0f }).withLastMouseDownPosition ({ 140.0f, 124.0f });
    const auto relative = event.withRelativePositionTo (&child);

    expectNear (relative.getPosition(), { 5.0f, 5.0f });
    expectNear (relative.getLastMouseDownPosition(), { 10.0f, 2.0f });
    EXPECT_EQ (relative.getSourceComponent(), &child);
}

TEST_F (ComponentPointMappingTests, RelativeMouseEventKeepsUnsetMouseDownPosition)
{
    addVisible (root, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    child.setTransform (AffineTransform::scaling (2.0f));

    const auto relative = eventAt ({ 20.0f, 20.0f }).withRelativePositionTo (&child);
    EXPECT_EQ (relative.getLastMouseDownPosition(), Point<float>());
}

TEST_F (ComponentPointMappingTests, EffectRoutesClickToTheDisplayedChild)
{
    Component hidden ("hidden");

    addVisible (root, parent, { 0.0f, 0.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 100.0f, 100.0f, 50.0f, 50.0f });
    addVisible (parent, hidden, { 10.0f, 10.0f, 40.0f, 40.0f });
    parent.setComponentEffect (new ZoomEffect());

    // The child content (100..150) is displayed zoomed at (100..200)
    EXPECT_EQ (root.findComponentAt ({ 180.0f, 180.0f }), &child);
    expectNear (child.getLocalPointFromTopLevel ({ 180.0f, 180.0f }), { 40.0f, 40.0f });

    // The hidden child is zoomed out of view, so its undistorted area hits the parent
    EXPECT_EQ (root.findComponentAt ({ 30.0f, 30.0f }), &parent);
}

TEST_F (ComponentPointMappingTests, DegenerateMappingExcludesChild)
{
    addVisible (root, parent, { 0.0f, 0.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 20.0f, 20.0f, 50.0f, 50.0f });
    child.setComponentEffect (new DegenerateEffect());

    EXPECT_EQ (root.findComponentAt ({ 30.0f, 30.0f }), &parent);
    EXPECT_FALSE (child.getLocalPointFromParent ({ 30.0f, 30.0f }).has_value());

    // Conversions that must return a point fall back to the position
    EXPECT_EQ (child.getLocalPointFromTopLevel ({ 30.0f, 30.0f }), Point<float> (10.0f, 10.0f));
    EXPECT_EQ (child.localToScreen (Point<float> (10.0f, 10.0f)), Point<float> (30.0f, 30.0f));
}

TEST_F (ComponentPointMappingTests, SingularTransformExcludesChild)
{
    addVisible (root, child, { 20.0f, 20.0f, 50.0f, 50.0f });
    child.setTransform (AffineTransform::scaling (0.0f, 1.0f));

    EXPECT_EQ (root.findComponentAt ({ 20.0f, 30.0f }), &root);
    EXPECT_FALSE (child.getLocalPointFromParent ({ 20.0f, 30.0f }).has_value());
}

TEST_F (ComponentPointMappingTests, CapturedDragGetsExtrapolatedPositionOutsideTheChild)
{
    addVisible (root, parent, { 100.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    EXPECT_NE (root.findComponentAt ({ 110.0f, 110.0f }), &child);
    expectNear (eventAt ({ 110.0f, 110.0f }).withRelativePositionTo (&child).getPosition(), { -5.0f, -5.0f });
}

TEST_F (ComponentPointMappingTests, LocalToScreenFollowsTransforms)
{
    addVisible (root, parent, { 100.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    expectNear (child.localToScreen (Point<float> (5.0f, 5.0f)), { 130.0f, 130.0f });
    expectNear (child.screenToLocal (Point<float> (130.0f, 130.0f)), { 5.0f, 5.0f });

    const auto screenBounds = child.getScreenBounds();
    expectNear (screenBounds.getTopLeft(), { 120.0f, 120.0f });
    expectNear (screenBounds.getBottomRight(), { 160.0f, 160.0f });
}

TEST_F (ComponentPointMappingTests, LocalToScreenRoundTripsThroughTransformsAndEffects)
{
    addVisible (root, parent, { 120.0f, 80.0f, 150.0f, 100.0f });
    addVisible (parent, child, { 30.0f, 20.0f, 60.0f, 40.0f });
    parent.setTransform (AffineTransform::rotation (0.4f).scaled (1.5f, 0.75f));
    child.setTransform (AffineTransform::shearing (0.3f, 0.0f));
    child.setComponentEffect (new ZoomEffect());

    for (const auto point : { Point<float> (0.0f, 0.0f), Point<float> (12.5f, 30.0f), Point<float> (59.0f, 1.0f), Point<float> (-20.0f, 70.0f) })
        expectNear (child.screenToLocal (child.localToScreen (point)), point);
}

TEST_F (ComponentPointMappingTests, RelativePointBetweenTransformedSiblings)
{
    Component sibling ("sibling");

    addVisible (root, child, { 50.0f, 50.0f, 40.0f, 40.0f });
    addVisible (root, sibling, { 200.0f, 50.0f, 40.0f, 40.0f });
    child.setTransform (AffineTransform::scaling (2.0f));

    // Child local (10, 10) is displayed at (70, 70), which is (-130, 20) for the sibling
    expectNear (child.getRelativePoint (&sibling, { 10.0f, 10.0f }), { -130.0f, 20.0f });
    expectNear (sibling.getLocalPoint (&child, { 10.0f, 10.0f }), { -130.0f, 20.0f });
}

TEST_F (ComponentPointMappingTests, DragAndDropTargetReceivesTransformedPosition)
{
    DropTarget target;

    addVisible (root, parent, { 100.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, target, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (target, DragAndDropData().withText ("drop"), { 130.0f, 130.0f }));
    expectNear (target.lastDropPosition, { 5.0f, 5.0f });
}

TEST_F (ComponentPointMappingTests, DragAndDropTargetReceivesUnwarpedPosition)
{
    DropTarget target;

    addVisible (root, parent, { 0.0f, 0.0f, 200.0f, 200.0f });
    addVisible (parent, target, { 100.0f, 100.0f, 50.0f, 50.0f });
    parent.setComponentEffect (new ZoomEffect());

    EXPECT_TRUE (DragAndDropTarget::dispatchItemDrop (target, DragAndDropData().withText ("drop"), { 180.0f, 180.0f }));
    expectNear (target.lastDropPosition, { 40.0f, 40.0f });
}

TEST_F (ComponentPointMappingTests, ParentHookRoutesInputToManuallyCompositedChild)
{
    MirroringHost host;

    addVisible (root, host, { 0.0f, 0.0f, 200.0f, 100.0f });
    addVisible (host, child, { 0.0f, 0.0f, 100.0f, 50.0f });
    child.setManuallyComposited (true);

    EXPECT_TRUE (child.isManuallyComposited());
    EXPECT_TRUE (child.isShowing());

    EXPECT_EQ (root.findComponentAt ({ 190.0f, 10.0f }), &child);
    expectNear (eventAt ({ 190.0f, 10.0f }).withRelativePositionTo (&child).getPosition(), { 5.0f, 5.0f });
    expectNear (child.localToScreen (Point<float> (5.0f, 5.0f)), { 190.0f, 10.0f });
}

TEST_F (ComponentPointMappingTests, MouseEventHitTestFollowsTransforms)
{
    Component rotated ("rotated");

    addVisible (root, parent, { 100.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 130.0f, 130.0f }), &child);
    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 250.0f, 250.0f }), &parent);

    addVisible (root, rotated, { 50.0f, 300.0f, 100.0f, 20.0f });
    rotated.setTransform (AffineTransform::rotation (MathConstants<float>::halfPi));

    // Local (50, 10) is displayed at (-10, 50) + (50, 300)
    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 40.0f, 350.0f }), &rotated);

    // Inside the untransformed bounds, but not where the component is displayed
    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 100.0f, 310.0f }), &root);
}

TEST_F (ComponentPointMappingTests, MouseEventHitTestFollowsEffectMapping)
{
    addVisible (root, parent, { 0.0f, 0.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 100.0f, 100.0f, 50.0f, 50.0f });
    parent.setComponentEffect (new ZoomEffect());

    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 180.0f, 180.0f }), &child);
}

TEST_F (ComponentPointMappingTests, MouseEventHitTestExcludesChildWithDegenerateMapping)
{
    addVisible (root, parent, { 0.0f, 0.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 20.0f, 20.0f, 50.0f, 50.0f });
    child.setComponentEffect (new DegenerateEffect());

    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 30.0f, 30.0f }), &parent);
}

TEST_F (ComponentPointMappingTests, MouseEventHitTestFollowsParentHook)
{
    MirroringHost host;

    addVisible (root, host, { 0.0f, 0.0f, 200.0f, 100.0f });
    addVisible (host, child, { 0.0f, 0.0f, 100.0f, 50.0f });
    child.setManuallyComposited (true);

    EXPECT_EQ (root.findComponentAtForMouseEvent ({ 190.0f, 10.0f }), &child);
}

TEST_F (ComponentPointMappingTests, PopupParentIsTheClosestPresentingAncestor)
{
    addVisible (root, parent, { 10.0f, 10.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 50.0f, 50.0f });

    EXPECT_EQ (root.getPopupParentComponent(), &root);
    EXPECT_EQ (child.getPopupParentComponent(), &root);

    parent.setTransform (AffineTransform::scaling (2.0f));
    EXPECT_EQ (child.getPopupParentComponent(), &parent);

    parent.setTransform ({});
    parent.setManuallyComposited (true);
    EXPECT_EQ (child.getPopupParentComponent(), &parent);

    // Effects are post-processing: popups stay readable on the top level
    parent.setManuallyComposited (false);
    parent.setComponentEffect (new ZoomEffect());
    EXPECT_EQ (child.getPopupParentComponent(), &root);
}

// =============================================================================
// Painting composes the transforms of the whole hierarchy
// =============================================================================

class ComponentHierarchyPaintTests : public ::testing::Test
{
protected:
    using ComponentHelper = yup::ComponentTestHelper<yup::Component>;

    /** Records the state it is painted with. */
    class PaintProbe : public Component
    {
    public:
        void paint (Graphics& g) override
        {
            ++paintCount;
            lastTransform = g.getTransform();
            lastDrawingArea = g.getDrawingArea();
        }

        /** The mapping from local coordinates to the render target, as Graphics applies it. */
        AffineTransform getLocalToTarget() const
        {
            return lastTransform.translated (lastDrawingArea.getTopLeft());
        }

        int paintCount = 0;
        AffineTransform lastTransform;
        Rectangle<float> lastDrawingArea;
    };

    void SetUp() override
    {
        GraphicsContext::Options options;
        options.allowHeadlessRendering = true;
        context = GraphicsContext::createContext (GpuPlatform::Headless, options);
        ASSERT_NE (nullptr, context);

        renderer = context->makeRenderer (400, 400);
        ASSERT_NE (nullptr, renderer);

        root.setBounds (0.0f, 0.0f, 400.0f, 400.0f);
        root.setVisible (true);
    }

    static void addVisible (Component& parentComponent, Component& childComponent, Rectangle<float> bounds)
    {
        childComponent.setBounds (bounds);
        parentComponent.addAndMakeVisible (childComponent);
    }

    void paintRoot()
    {
        Graphics g (*context, *renderer, 1.0f);
        ComponentHelper::triggerPaint (root, g, root.getLocalBounds(), false);
    }

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    PaintProbe root;
    PaintProbe parent;
    PaintProbe child;
};

TEST_F (ComponentHierarchyPaintTests, UntransformedHierarchyKeepsPositionInDrawingArea)
{
    addVisible (root, parent, { 50.0f, 40.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 10.0f, 5.0f, 20.0f, 20.0f });

    paintRoot();

    ASSERT_EQ (1, child.paintCount);
    EXPECT_TRUE (child.lastTransform.isIdentity());
    EXPECT_EQ (child.lastDrawingArea, Rectangle<float> (60.0f, 45.0f, 20.0f, 20.0f));
}

TEST_F (ComponentHierarchyPaintTests, ChildOfScaledParentPaintsUnderComposedTransform)
{
    addVisible (root, parent, { 50.0f, 40.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 5.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    paintRoot();

    ASSERT_EQ (1, child.paintCount);
    const auto expected = AffineTransform::translation (10.0f, 5.0f)
                              .followedBy (AffineTransform::scaling (2.0f))
                              .translated (50.0f, 40.0f);
    EXPECT_TRUE (child.getLocalToTarget().approximatelyEqualTo (expected));
    EXPECT_EQ (child.lastDrawingArea.getSize(), Size<float> (20.0f, 20.0f));
}

TEST_F (ComponentHierarchyPaintTests, ChildOfRotatedParentPaintsUnderComposedTransform)
{
    addVisible (root, parent, { 200.0f, 100.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 20.0f, 30.0f, 30.0f });
    parent.setTransform (AffineTransform::rotation (0.5f));
    child.setTransform (AffineTransform::scaling (0.5f, 2.0f));

    paintRoot();

    ASSERT_EQ (1, child.paintCount);
    const auto expected = AffineTransform::scaling (0.5f, 2.0f)
                              .translated (10.0f, 20.0f)
                              .followedBy (AffineTransform::rotation (0.5f))
                              .translated (200.0f, 100.0f);
    EXPECT_TRUE (child.getLocalToTarget().approximatelyEqualTo (expected));
}

TEST_F (ComponentHierarchyPaintTests, ManuallyCompositedChildIsSkipped)
{
    addVisible (root, parent, { 50.0f, 50.0f, 200.0f, 200.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 50.0f, 50.0f });
    child.setManuallyComposited (true);

    paintRoot();

    EXPECT_EQ (1, parent.paintCount);
    EXPECT_EQ (0, child.paintCount);

    child.setManuallyComposited (false);
    paintRoot();

    EXPECT_EQ (1, child.paintCount);
}

// =============================================================================
// Repaint regions follow transforms, effects and manually composited children
// =============================================================================

class ComponentHierarchyRepaintTests : public ::testing::Test
{
protected:
    class NoOpEffect : public ComponentEffect
    {
    public:
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}
    };

    void SetUp() override
    {
        root.setBounds (0.0f, 0.0f, 400.0f, 400.0f);
        root.setVisible (true);
    }

    static void addVisible (Component& parentComponent, Component& childComponent, Rectangle<float> bounds)
    {
        childComponent.setBounds (bounds);
        parentComponent.addAndMakeVisible (childComponent);
    }

    /** Attaches a recording native to the root once the hierarchy is built. */
    const RectangleList<float>& attachNative()
    {
        auto* native = new StubComponentNative (root, ComponentNative::defaultFlags);
        yup::ComponentTestHelper<Component>::attachNative (root, native);
        return native->getRepaintAreas();
    }

    static void expectSingleArea (const RectangleList<float>& areas, Rectangle<float> expected)
    {
        ASSERT_EQ (1, areas.getNumRectangles());

        const auto area = areas.getRectangles()[0];
        EXPECT_NEAR (area.getX(), expected.getX(), 1.0e-3f);
        EXPECT_NEAR (area.getY(), expected.getY(), 1.0e-3f);
        EXPECT_NEAR (area.getWidth(), expected.getWidth(), 1.0e-3f);
        EXPECT_NEAR (area.getHeight(), expected.getHeight(), 1.0e-3f);
    }

    Component root { "root" };
};

TEST_F (ComponentHierarchyRepaintTests, RotatedChildRepaintsItsTransformedBounds)
{
    Component child;
    addVisible (root, child, { 100.0f, 100.0f, 40.0f, 20.0f });
    child.setTransform (AffineTransform::rotation (MathConstants<float>::halfPi));

    const auto& areas = attachNative();
    child.repaint();

    expectSingleArea (areas, { 80.0f, 100.0f, 20.0f, 40.0f });
}

TEST_F (ComponentHierarchyRepaintTests, ChildOfScaledParentRepaintsScaledArea)
{
    Component parent, child;
    addVisible (root, parent, { 50.0f, 50.0f, 100.0f, 100.0f });
    addVisible (parent, child, { 10.0f, 10.0f, 20.0f, 20.0f });
    parent.setTransform (AffineTransform::scaling (2.0f));

    const auto& areas = attachNative();
    child.repaint (5.0f, 5.0f, 10.0f, 10.0f);

    expectSingleArea (areas, { 80.0f, 80.0f, 20.0f, 20.0f });
}

TEST_F (ComponentHierarchyRepaintTests, RepaintInsideEffectWidensToTheEffectedComponent)
{
    Component effected, child;
    addVisible (root, effected, { 20.0f, 20.0f, 100.0f, 100.0f });
    addVisible (effected, child, { 10.0f, 10.0f, 5.0f, 5.0f });
    effected.setComponentEffect (new NoOpEffect());

    const auto& areas = attachNative();
    child.repaint();

    expectSingleArea (areas, { 20.0f, 20.0f, 100.0f, 100.0f });
}

TEST_F (ComponentHierarchyRepaintTests, RepaintInsideManuallyCompositedChildRepaintsTheWholeHost)
{
    Component host, panel, grandchild;
    addVisible (root, host, { 50.0f, 50.0f, 200.0f, 100.0f });
    addVisible (host, panel, { 0.0f, 0.0f, 100.0f, 50.0f });
    addVisible (panel, grandchild, { 10.0f, 10.0f, 5.0f, 5.0f });
    panel.setManuallyComposited (true);

    const auto& areas = attachNative();
    grandchild.repaint();

    expectSingleArea (areas, { 50.0f, 50.0f, 200.0f, 100.0f });
}
