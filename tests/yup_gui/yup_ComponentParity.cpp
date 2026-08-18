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

#include <gtest/gtest.h>

#include <yup_gui/yup_gui.h>
#include <yup_graphics/yup_graphics.h>

#include "../helpers/yup_GpuTestDevice.h"
#include "../helpers/yup_PixelCompare.h"

using namespace yup;

//==============================================================================
// Component tree parity: render a real component hierarchy through the same
// subtree walk the window uses, and assert the result on every backend.
//
// Component::snapshotToImage drives renderSubtreeOffscreen, which is the walk
// that paints children, applies opacity and clips each child to its bounds. It
// needs nothing but a GraphicsContext and a non-zero size, so the whole of
// layers L0 to L4 is covered here without a window, a swapchain or a
// compositor in the way.
//==============================================================================

namespace
{

constexpr int kRootSize = 64;

/** A component that fills a rectangle in its own coordinate space. */
class FillingComponent : public Component
{
public:
    FillingComponent (Color colorToUse, Rectangle<float> areaToFill)
        : fillColor (colorToUse)
        , area (areaToFill)
    {
    }

    /** Fills the component's whole local bounds. */
    explicit FillingComponent (Color colorToUse)
        : fillColor (colorToUse)
        , fillsEverything (true)
    {
    }

    void paint (Graphics& g) override
    {
        g.setFillColor (fillColor);

        if (fillsEverything)
            g.fillAll();
        else
            g.fillRect (area);
    }

private:
    Color fillColor;
    Rectangle<float> area;
    bool fillsEverything = false;
};

} // namespace

class ComponentParityTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (! gpu.isValid())
        {
            if (test::GpuTestDevice::isGpuRequired())
                FAIL() << "YUP_TEST_REQUIRE_GPU is set but no " << test::GpuTestDevice::getPlatformName()
                       << " device could be created: " << gpu.getFailureReason();

            GTEST_SKIP() << "no usable GPU device: " << gpu.getFailureReason();
        }

        context = gpu.getGraphicsContext();
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "no GPU-backed GraphicsContext on this backend";
    }

    /** Snapshots a component subtree and returns it as a comparable bitmap. */
    test::Bitmap snapshot (Component& component)
    {
        return test::bitmapFromImage (component.snapshotToImage (*context));
    }

    static ::testing::AssertionResult pixelIs (const test::Bitmap& bitmap, int x, int y, uint32_t expected, const char* region)
    {
        if (! bitmap.isValid())
            return ::testing::AssertionFailure() << "snapshot was not produced";

        if (x < 0 || y < 0 || x >= bitmap.width || y >= bitmap.height)
            return ::testing::AssertionFailure() << region << " at (" << x << ", " << y << ") is outside the "
                                                 << bitmap.width << "x" << bitmap.height << " snapshot";

        const auto found = bitmap.getPixel (x, y);

        if (found == expected)
            return ::testing::AssertionSuccess();

        return ::testing::AssertionFailure()
            << region << " at (" << x << ", " << y << ") is 0x" << String::toHexString (static_cast<int> (found))
            << ", expected 0x" << String::toHexString (static_cast<int> (expected))
            << " (0xAABBGGRR)";
    }

    test::GpuTestDevice gpu;
    GraphicsContext* context = nullptr;
};

//==============================================================================
// A component's own paint must reach the snapshot at the right size.
//==============================================================================

TEST_F (ComponentParityTests, SnapshotCapturesTheComponentsOwnPaint)
{
    FillingComponent root (Colors::red);
    root.setBounds (0, 0, kRootSize, kRootSize);

    const auto bitmap = snapshot (root);

    ASSERT_TRUE (bitmap.isValid());
    EXPECT_EQ (bitmap.width, kRootSize);
    EXPECT_EQ (bitmap.height, kRootSize);
    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (255, 0, 0, 255), "component_paint"));
}

//==============================================================================
// A child must be painted over its parent, at the child's own position. This is
// the tree walk doing the coordinate translation the window path relies on.
//==============================================================================

TEST_F (ComponentParityTests, ChildrenPaintOverTheParentAtTheirOwnPosition)
{
    FillingComponent root (Colors::white);
    root.setBounds (0, 0, kRootSize, kRootSize);

    FillingComponent child (Colors::black);
    child.setBounds (16, 16, 16, 16);
    root.addAndMakeVisible (child);

    const auto bitmap = snapshot (root);

    ASSERT_TRUE (bitmap.isValid());

    const auto black = test::packRGBA (0, 0, 0, 255);
    const auto white = test::packRGBA (255, 255, 255, 255);

    EXPECT_TRUE (pixelIs (bitmap, 24, 24, black, "inside the child"));
    EXPECT_TRUE (pixelIs (bitmap, 8, 24, white, "left of the child"));
    EXPECT_TRUE (pixelIs (bitmap, 40, 24, white, "right of the child"));
    EXPECT_TRUE (pixelIs (bitmap, 24, 8, white, "above the child"));
    EXPECT_TRUE (pixelIs (bitmap, 24, 40, white, "below the child"));
}

//==============================================================================
// A child that paints past its own bounds must be clipped to them, or one
// component's paint leaks over its siblings.
//==============================================================================

TEST_F (ComponentParityTests, ChildrenAreClippedToTheirOwnBounds)
{
    FillingComponent root (Colors::white);
    root.setBounds (0, 0, kRootSize, kRootSize);

    // Fills far beyond its own 16x16 bounds.
    FillingComponent child (Colors::black, Rectangle<float> (-64.0f, -64.0f, 256.0f, 256.0f));
    child.setBounds (16, 16, 16, 16);
    root.addAndMakeVisible (child);

    const auto bitmap = snapshot (root);

    ASSERT_TRUE (bitmap.isValid());

    const auto black = test::packRGBA (0, 0, 0, 255);
    const auto white = test::packRGBA (255, 255, 255, 255);

    EXPECT_TRUE (pixelIs (bitmap, 24, 24, black, "inside the child"));
    EXPECT_TRUE (pixelIs (bitmap, 8, 8, white, "outside, up-left"));
    EXPECT_TRUE (pixelIs (bitmap, 48, 48, white, "outside, down-right"));
    EXPECT_TRUE (pixelIs (bitmap, 8, 40, white, "outside, down-left"));
}

//==============================================================================
// Sibling order: a later child paints over an earlier one.
//==============================================================================

TEST_F (ComponentParityTests, LaterSiblingsPaintOverEarlierOnes)
{
    FillingComponent root (Colors::white);
    root.setBounds (0, 0, kRootSize, kRootSize);

    FillingComponent first (Colors::red);
    first.setBounds (8, 8, 32, 32);
    root.addAndMakeVisible (first);

    FillingComponent second (Colors::blue);
    second.setBounds (24, 24, 32, 32);
    root.addAndMakeVisible (second);

    const auto bitmap = snapshot (root);

    ASSERT_TRUE (bitmap.isValid());

    EXPECT_TRUE (pixelIs (bitmap, 12, 12, test::packRGBA (255, 0, 0, 255), "the first child only"));
    EXPECT_TRUE (pixelIs (bitmap, 32, 32, test::packRGBA (0, 0, 255, 255), "the overlap, second on top"));
    EXPECT_TRUE (pixelIs (bitmap, 52, 52, test::packRGBA (0, 0, 255, 255), "the second child only"));
    EXPECT_TRUE (pixelIs (bitmap, 60, 4, test::packRGBA (255, 255, 255, 255), "neither child"));
}

//==============================================================================
// An invisible child must not be painted at all.
//==============================================================================

TEST_F (ComponentParityTests, InvisibleChildrenAreNotPainted)
{
    FillingComponent root (Colors::white);
    root.setBounds (0, 0, kRootSize, kRootSize);

    FillingComponent child (Colors::black);
    child.setBounds (16, 16, 32, 32);
    root.addChildComponent (child); // Added, deliberately not made visible.

    const auto bitmap = snapshot (root);

    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (255, 255, 255, 255), "invisible_child"));
}

//==============================================================================
// Nesting: a grandchild's position must compose both translations rather than
// only its parent's.
//==============================================================================

TEST_F (ComponentParityTests, GrandchildrenComposeTheirAncestorsOffsets)
{
    FillingComponent root (Colors::white);
    root.setBounds (0, 0, kRootSize, kRootSize);

    FillingComponent child (Colors::red);
    child.setBounds (16, 16, 32, 32);
    root.addAndMakeVisible (child);

    FillingComponent grandchild (Colors::black);
    grandchild.setBounds (8, 8, 8, 8); // Local to the child, so 24,24 in the root.
    child.addAndMakeVisible (grandchild);

    const auto bitmap = snapshot (root);

    ASSERT_TRUE (bitmap.isValid());

    EXPECT_TRUE (pixelIs (bitmap, 27, 27, test::packRGBA (0, 0, 0, 255), "inside the grandchild"));
    EXPECT_TRUE (pixelIs (bitmap, 20, 20, test::packRGBA (255, 0, 0, 255), "the child, outside the grandchild"));
    EXPECT_TRUE (pixelIs (bitmap, 4, 4, test::packRGBA (255, 255, 255, 255), "the root, outside the child"));
}

//==============================================================================
// A snapshot of real content must not be uninitialised memory, measured the
// same way tools/check_screenshot.py measures a CI capture.
//==============================================================================

TEST_F (ComponentParityTests, SnapshotIsNotUninitialisedMemory)
{
    FillingComponent root (Colors::white);
    root.setBounds (0, 0, kRootSize, kRootSize);

    FillingComponent child (Colors::black);
    child.setBounds (16, 16, 32, 32);
    root.addAndMakeVisible (child);

    const auto bitmap = snapshot (root);

    ASSERT_TRUE (bitmap.isValid());
    EXPECT_GT (test::measureFlatFraction (bitmap), 0.8);
}
