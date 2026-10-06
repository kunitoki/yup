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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

namespace
{

//==============================================================================
// A minimal RenderableTarget for testing offscreen Graphics constructors.
//==============================================================================
class TrackingOffscreenTarget : public RenderableTarget
{
public:
    TrackingOffscreenTarget (int targetWidth, int targetHeight)
        : width (targetWidth)
        , height (targetHeight)
    {
    }

    int getWidth() const noexcept override { return width; }

    int getHeight() const noexcept override { return height; }

    rive::gpu::RenderTarget* getRenderTarget() noexcept override { return nullptr; }

    rive::gpu::RenderContext* getRenderContext() noexcept override { return nullptr; }

    rive::rcp<rive::gpu::Texture> adoptAsTexture() override { return nullptr; }

private:
    int width;
    int height;
};

} // namespace

//==============================================================================
// Offscreen Graphics construction tests
//==============================================================================

class GraphicsOffscreenTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (GraphicsOffscreenTests, ConstructWithOwnedTargetDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    EXPECT_NO_THROW ({
        Graphics g (*context, std::move (target), 0xFF000000u);
    });
}

TEST_F (GraphicsOffscreenTests, ConstructWithReferencedTargetDoesNotCrash)
{
    TrackingOffscreenTarget target (128, 64);
    EXPECT_NO_THROW ({
        Graphics g (*context, target, 0xFF000000u);
    });
}

TEST_F (GraphicsOffscreenTests, IsOffscreenReturnsTrueForOffscreenConstructed)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_TRUE (g.isOffscreen());
}

TEST_F (GraphicsOffscreenTests, IsOffscreenReturnsFalseForRendererConstructed)
{
    auto renderer = context->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.isOffscreen());
}

TEST_F (GraphicsOffscreenTests, CommitOffscreenTargetSucceeds)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_TRUE (g.commitOffscreenTarget());
}

TEST_F (GraphicsOffscreenTests, CommitOffscreenTargetIsIdempotent)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_TRUE (g.commitOffscreenTarget());
    EXPECT_FALSE (g.commitOffscreenTarget()); // Already committed.
}

TEST_F (GraphicsOffscreenTests, CommitToImageReturnsFalseWhenNoImageTarget)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    // No associated image, so commitToImage should fail.
    EXPECT_FALSE (g.commitToImage());
}

TEST_F (GraphicsOffscreenTests, ReadPixelsToImageReturnsFalseWhenNoImageTarget)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_FALSE (g.readPixelsToImage());
}

TEST_F (GraphicsOffscreenTests, DrawingAreaDefaultsToTargetSizeForOffscreen)
{
    TrackingOffscreenTarget target (128, 64);
    Graphics g (*context, target, 0xFF000000u);

    auto area = g.getDrawingArea();
    EXPECT_FLOAT_EQ (area.getWidth(), 128.0f);
    EXPECT_FLOAT_EQ (area.getHeight(), 64.0f);
}

//==============================================================================
// Offscreen drawing operations
//==============================================================================

TEST_F (GraphicsOffscreenTests, FillAllOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW (g.fillAll());
}

TEST_F (GraphicsOffscreenTests, FillRectOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.setFillColor (Color (0xFFFF0000));
        g.fillRect (10.0f, 10.0f, 50.0f, 30.0f);
    });
}

TEST_F (GraphicsOffscreenTests, StrokeRectOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.setStrokeColor (Color (0xFF00FF00));
        g.setStrokeWidth (2.0f);
        g.strokeRect (5.0f, 5.0f, 100.0f, 50.0f);
    });
}

TEST_F (GraphicsOffscreenTests, PathDrawingOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    Path path;
    path.moveTo (10.0f, 10.0f);
    path.lineTo (50.0f, 10.0f);
    path.lineTo (30.0f, 50.0f);
    path.close();

    EXPECT_NO_THROW ({
        g.setFillColor (Color (0xFF0000FF));
        g.fillPath (path);
        g.setStrokeColor (Color (0xFFFF0000));
        g.strokePath (path);
    });
}

TEST_F (GraphicsOffscreenTests, EllipseDrawingOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.setFillColor (Color (0xFFFF00FF));
        g.fillEllipse (20.0f, 10.0f, 60.0f, 40.0f);
        g.strokeEllipse (Rectangle<float> (20.0f, 10.0f, 60.0f, 40.0f));
    });
}

TEST_F (GraphicsOffscreenTests, RoundedRectDrawingOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.fillRoundedRect (5.0f, 5.0f, 100.0f, 50.0f, 3.0f, 5.0f, 7.0f, 9.0f);
        g.strokeRoundedRect (5.0f, 5.0f, 100.0f, 50.0f, 4.0f);
        g.fillRoundedRect (Rectangle<float> (10.0f, 10.0f, 80.0f, 40.0f), 6.0f);
        g.strokeRoundedRect (Rectangle<float> (10.0f, 10.0f, 80.0f, 40.0f), 2.0f, 3.0f, 4.0f, 5.0f);
    });
}

TEST_F (GraphicsOffscreenTests, LineDrawingOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.setStrokeColor (Color (0xFFFFFFFF));
        g.strokeLine (0.0f, 0.0f, 127.0f, 63.0f);
        g.strokeLine (Point<float> (10.0f, 10.0f), Point<float> (100.0f, 50.0f));
    });
}

TEST_F (GraphicsOffscreenTests, SaveRestoreStateOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    g.setFillColor (Color (0xFFFF0000));
    {
        auto state = g.saveState();
        g.setFillColor (Color (0xFF00FF00));
        EXPECT_EQ (g.getFillColor(), Color (0xFF00FF00));
    }
    EXPECT_EQ (g.getFillColor(), Color (0xFFFF0000));
}

TEST_F (GraphicsOffscreenTests, ClipPathOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.setClipPath (Rectangle<float> (10.0f, 10.0f, 50.0f, 40.0f));
    });

    Path clipPath;
    clipPath.addEllipse (20.0f, 10.0f, 80.0f, 40.0f);
    EXPECT_NO_THROW ({
        g.setClipPath (clipPath);
    });
}

TEST_F (GraphicsOffscreenTests, FittedTextOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (Size<float> (120.0f, 60.0f));
        modifier.appendText ("Test", Font());
    }

    Rectangle<float> textRect (4.0f, 4.0f, 120.0f, 56.0f);

    EXPECT_NO_THROW ({
        g.fillFittedText (styledText, textRect);
        g.strokeFittedText (styledText, textRect);
    });
}

TEST_F (GraphicsOffscreenTests, FittedTextConvenienceOverloadsDoNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    Rectangle<float> textRect (4.0f, 4.0f, 120.0f, 56.0f);

    EXPECT_NO_THROW ({
        g.fillFittedText ("Hello world", Font().withHeight (14.0f), textRect, Justification::center);
        g.strokeFittedText ("Hello world", Font().withHeight (14.0f), textRect, Justification::topLeft);
        g.fillFittedText ("Hello world", Font().withHeight (14.0f), textRect, Justification::bottomRight);
        g.strokeFittedText ("Hello world", Font().withHeight (14.0f), textRect, Justification::right);
    });
}

TEST_F (GraphicsOffscreenTests, EmptyFittedTextReturnsEarly)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    Rectangle<float> textRect (4.0f, 4.0f, 120.0f, 56.0f);

    EXPECT_NO_THROW ({
        g.fillFittedText ("", Font(), textRect);
        g.strokeFittedText ("", Font(), textRect);
    });
}

TEST_F (GraphicsOffscreenTests, ImageDrawingOnOffscreenReturnsEarly)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    Image testImage (32, 32, PixelFormat::RGBA);
    testImage.fill (0xFFFF0000u);

    // drawImage / drawImageAt should not crash (though texture creation fails on headless).
    EXPECT_NO_THROW ({
        g.drawImage (testImage, Rectangle<float> (0.0f, 0.0f, 32.0f, 32.0f));
        g.drawImageAt (testImage, Point<float> (10.0f, 10.0f));
    });
}

TEST_F (GraphicsOffscreenTests, DrawTextureWithNullDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    EXPECT_NO_THROW ({
        g.drawTexture (nullptr, Rectangle<float> (0.0f, 0.0f, 32.0f, 32.0f));
    });
}

TEST_F (GraphicsOffscreenTests, GradientFillOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    ColorGradient linearGrad (
        Color (0xFFFF0000), 0.0f, 0.0f, Color (0xFF0000FF), 100.0f, 100.0f, ColorGradient::Linear);

    ColorGradient radialGrad (
        Color (0xFF00FF00), 50.0f, 50.0f, Color (0xFFFFFF00), 0.0f, 0.0f, ColorGradient::Radial);

    EXPECT_NO_THROW ({
        g.setFillColorGradient (linearGrad);
        g.fillRect (10.0f, 10.0f, 50.0f, 30.0f);

        g.setStrokeColorGradient (radialGrad);
        g.setStrokeWidth (2.0f);
        g.strokeRect (70.0f, 10.0f, 50.0f, 30.0f);
    });
}

TEST_F (GraphicsOffscreenTests, SingleStopGradientDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    ColorGradient singleStop (
        Color (0xFFFF0000), 0.0f, 0.0f, Color (0xFFFF0000), 0.0f, 0.0f, ColorGradient::Linear);
    // Add only one stop to force the single-stop path.
    singleStop.clearStops();
    singleStop.addColorStop (Color (0xFF00FF00), 0.0f, 0.0f, 0.0f);
    ASSERT_EQ (singleStop.getStops().size(), 1u);

    EXPECT_NO_THROW ({
        g.setFillColorGradient (singleStop);
        g.fillRect (10.0f, 10.0f, 50.0f, 30.0f);
    });
}

TEST_F (GraphicsOffscreenTests, EmptyGradientDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    ColorGradient emptyGrad (
        Color (0xFFFF0000), 0.0f, 0.0f, Color (0xFFFF0000), 0.0f, 0.0f, ColorGradient::Linear);
    emptyGrad.clearStops();

    EXPECT_NO_THROW ({
        g.setFillColorGradient (emptyGrad);
        g.fillRect (10.0f, 10.0f, 50.0f, 30.0f);
    });
}

//==============================================================================
// TransparencyLayer tests
//==============================================================================

class TransparencyLayerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
        renderer = context->makeRenderer (200, 200);
        ASSERT_NE (renderer, nullptr);
        graphics = std::make_unique<Graphics> (*context, *renderer);
    }

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    std::unique_ptr<Graphics> graphics;
};

TEST_F (TransparencyLayerTests, BeginTransparencyLayerWithTinyAreaReturnsInvalidLayer)
{
    // Width or height of 0 produces an invalid layer.
    auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 0.0f, 10.0f), 0.5f);
    EXPECT_FALSE (layer.isValid());
}

TEST_F (TransparencyLayerTests, BeginTransparencyLayerWithNegativeAreaReturnsInvalidLayer)
{
    auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, -10.0f, 10.0f), 0.5f);
    EXPECT_FALSE (layer.isValid());
}

TEST_F (TransparencyLayerTests, InvalidLayerCommitReturnsFalse)
{
    auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 0.0f, 10.0f), 0.5f);
    EXPECT_FALSE (layer.isValid());
    EXPECT_FALSE (layer.commit());
}

TEST_F (TransparencyLayerTests, BeginTransparencyLayerWithValidAreaDoesNotCrash)
{
    EXPECT_NO_THROW ({
        auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 0.5f);
    });
}

TEST_F (TransparencyLayerTests, MoveConstructedLayer)
{
    auto layer1 = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 0.5f);
    Graphics::TransparencyLayer layer2 (std::move (layer1));

    // layer1 should be committed/finished after move.
    EXPECT_FALSE (layer1.isValid());

    // layer2 should be valid or invalid depending on headless support.
    EXPECT_NO_THROW (layer2.commit());
}

TEST_F (TransparencyLayerTests, MoveAssignedLayer)
{
    auto layer1 = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 0.5f);
    auto layer2 = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 0.3f);

    layer2 = std::move (layer1);

    EXPECT_FALSE (layer1.isValid());
    EXPECT_NO_THROW (layer2.commit());
}

//==============================================================================
// Blend mode coverage
//==============================================================================

TEST_F (TransparencyLayerTests, AllBlendModesSetWithoutCrash)
{
    std::array blendModes = {
        BlendMode::SrcOver,
        BlendMode::Screen,
        BlendMode::Multiply,
        BlendMode::Overlay,
        BlendMode::Darken,
        BlendMode::Lighten,
        BlendMode::ColorDodge,
        BlendMode::ColorBurn,
        BlendMode::HardLight,
        BlendMode::SoftLight,
        BlendMode::Difference,
        BlendMode::Exclusion,
        BlendMode::Hue,
        BlendMode::Saturation,
        BlendMode::Color,
        BlendMode::Luminosity
    };

    for (const auto& mode : blendModes)
    {
        EXPECT_NO_THROW (graphics->setBlendMode (mode));
        EXPECT_EQ (graphics->getBlendMode(), mode);
    }
}

TEST_F (TransparencyLayerTests, BlendModeDefaultCoverage)
{
    // Verify that each blend mode round-trips without triggering the default case.
    graphics->setBlendMode (BlendMode::SrcOver);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::SrcOver);

    graphics->setBlendMode (BlendMode::Color);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::Color);

    graphics->setBlendMode (BlendMode::Luminosity);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::Luminosity);
}

//==============================================================================
// Justification conversion (toHorizontalAlign / toVerticalAlign)
//==============================================================================

TEST_F (TransparencyLayerTests, JustificationConversionsDoNotCrash)
{
    Rectangle<float> textRect (4.0f, 4.0f, 120.0f, 56.0f);

    EXPECT_NO_THROW ({
        graphics->fillFittedText ("left", Font().withHeight (14.0f), textRect, Justification::left);
        graphics->fillFittedText ("right", Font().withHeight (14.0f), textRect, Justification::right);
        graphics->fillFittedText ("hCenter", Font().withHeight (14.0f), textRect, Justification::horizontalCenter);
        graphics->fillFittedText ("top", Font().withHeight (14.0f), textRect, Justification::top);
        graphics->fillFittedText ("bottom", Font().withHeight (14.0f), textRect, Justification::bottom);
        graphics->fillFittedText ("vCenter", Font().withHeight (14.0f), textRect, Justification::verticalCenter);
        graphics->fillFittedText ("centered", Font().withHeight (14.0f), textRect, Justification::center);
    });
}

//==============================================================================
// Graphics renderTexture / drawTexture coverage
//==============================================================================

class GraphicsTextureRenderingTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
        renderer = context->makeRenderer (200, 200);
        ASSERT_NE (renderer, nullptr);
        graphics = std::make_unique<Graphics> (*context, *renderer);
    }

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    std::unique_ptr<Graphics> graphics;
};

TEST_F (GraphicsTextureRenderingTests, DrawImageWithValidImageDoesNotCrash)
{
    // drawImage calls createTextureIfNotPresent which returns false on headless,
    // but should not crash.
    Image testImage (32, 32, PixelFormat::RGBA);
    testImage.fill (0xFF112233u);

    EXPECT_NO_THROW ({
        graphics->drawImage (testImage, Rectangle<float> (10.0f, 10.0f, 32.0f, 32.0f));
        graphics->drawImageAt (testImage, Point<float> (5.0f, 5.0f));
    });
}

TEST_F (GraphicsTextureRenderingTests, DrawImageWithInvalidImageDoesNotCrash)
{
    Image invalid;
    EXPECT_NO_THROW ({
        graphics->drawImage (invalid, Rectangle<float> (10.0f, 10.0f, 32.0f, 32.0f));
    });
}

TEST_F (GraphicsTextureRenderingTests, DrawTextureWithNullDoesNotCrash)
{
    EXPECT_NO_THROW ({
        graphics->drawTexture (nullptr, Rectangle<float> (0.0f, 0.0f, 64.0f, 64.0f));
    });
}

TEST_F (GraphicsTextureRenderingTests, DrawTextureOnOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    Graphics g (*context, std::move (target), 0xFF000000u);

    Image img (16, 16, PixelFormat::RGBA);
    img.fill (0xFFAABBCCu);

    EXPECT_NO_THROW ({
        g.drawImage (img, Rectangle<float> (0.0f, 0.0f, 16.0f, 16.0f));
        g.drawTexture (nullptr, Rectangle<float> (0.0f, 0.0f, 16.0f, 16.0f));
    });
}

//==============================================================================
// StrokeJoin / StrokeCap all values
//==============================================================================

TEST_F (TransparencyLayerTests, AllStrokeJoinsRoundTrip)
{
    std::array joins = { StrokeJoin::Miter, StrokeJoin::Round, StrokeJoin::Bevel };

    for (const auto& join : joins)
    {
        EXPECT_NO_THROW (graphics->setStrokeJoin (join));
        EXPECT_EQ (graphics->getStrokeJoin(), join);
    }
}

TEST_F (TransparencyLayerTests, AllStrokeCapsRoundTrip)
{
    std::array caps = { StrokeCap::Butt, StrokeCap::Round, StrokeCap::Square };

    for (const auto& cap : caps)
    {
        EXPECT_NO_THROW (graphics->setStrokeCap (cap));
        EXPECT_EQ (graphics->getStrokeCap(), cap);
    }
}

//==============================================================================
// Graphics::convertRawPathToRenderPath variants
//==============================================================================

namespace
{

// Internal functions declared in yup_Graphics but accessible via the module header.
// Exercise the non-identity transform path by using an actual transform.

} // namespace

TEST_F (TransparencyLayerTests, FillAndStrokePathWithNonIdentityTransform)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));
    graphics->setTransform (AffineTransform::translation (50.0f, 30.0f).scaled (2.0f, 2.0f));

    Path path;
    path.addRectangle (0.0f, 0.0f, 50.0f, 50.0f);

    EXPECT_NO_THROW ({
        graphics->fillPath (path);
        graphics->strokePath (path);
    });
}

TEST_F (TransparencyLayerTests, FillPathWithGradientAndNonIdentityTransform)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    ColorGradient grad (
        Color (0xFFFF0000), 0.0f, 0.0f, Color (0xFF00FF00), 50.0f, 50.0f, ColorGradient::Radial);
    graphics->setFillColorGradient (grad);
    graphics->setTransform (AffineTransform::rotation (MathConstants<float>::halfPi / 2.0f, 0.0f, 0.0f));

    Path path;
    path.addEllipse (0.0f, 0.0f, 100.0f, 100.0f);

    EXPECT_NO_THROW ({
        graphics->fillPath (path);
    });
}

//==============================================================================
// Graphics destructor with committed offscreen — no double endOffscreen
//==============================================================================

TEST_F (GraphicsOffscreenTests, DestructorWithCommittedOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    {
        Graphics g (*context, std::move (target), 0xFF000000u);
        EXPECT_TRUE (g.commitOffscreenTarget());
    }
    // Destructor should not call endOffscreen since committed is true.
}

TEST_F (GraphicsOffscreenTests, DestructorWithUncommittedOffscreenDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (128, 64);
    {
        Graphics g (*context, std::move (target), 0xFF000000u);
        // Leave uncommitted — destructor calls endOffscreen.
    }
}

//==============================================================================
// Graphics with nullptr offscreenTarget in owned-target constructor
//==============================================================================

TEST_F (GraphicsOffscreenTests, ConstructWithNullOwnedTargetDoesNotCrash)
{
    EXPECT_NO_THROW ({
        Graphics g (*context, std::unique_ptr<RenderableTarget> (nullptr), 0xFF000000u);
        EXPECT_FALSE (g.isOffscreen());
    });
}

//==============================================================================
// Graphics complex text rendering (fitted text with gradient)
//==============================================================================

TEST_F (GraphicsOffscreenTests, FillFittedTextWithGradientDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (200, 100);
    Graphics g (*context, std::move (target), 0xFF000000u);

    ColorGradient textGrad (
        Color (0xFFFF0000), 0.0f, 0.0f, Color (0xFF0000FF), 150.0f, 0.0f, ColorGradient::Linear);
    g.setFillColorGradient (textGrad);

    StyledText styled;
    {
        auto mod = styled.startUpdate();
        mod.setMaxSize (Size<float> (180.0f, 80.0f));
        mod.appendText ("Gradient Text", Font().withHeight (20.0f));
    }

    EXPECT_NO_THROW ({
        g.fillFittedText (styled, Rectangle<float> (10.0f, 10.0f, 180.0f, 80.0f));
    });
}

TEST_F (GraphicsOffscreenTests, StrokeFittedTextWithGradientDoesNotCrash)
{
    auto target = std::make_unique<TrackingOffscreenTarget> (200, 100);
    Graphics g (*context, std::move (target), 0xFF000000u);

    ColorGradient textGrad (
        Color (0xFF00FF00), 0.0f, 0.0f, Color (0xFFFF0000), 100.0f, 50.0f, ColorGradient::Radial);
    g.setStrokeColorGradient (textGrad);
    g.setStrokeWidth (2.0f);

    StyledText styled;
    {
        auto mod = styled.startUpdate();
        mod.setMaxSize (Size<float> (180.0f, 80.0f));
        mod.appendText ("Stroke Grad", Font().withHeight (18.0f));
    }

    EXPECT_NO_THROW ({
        g.strokeFittedText (styled, Rectangle<float> (10.0f, 10.0f, 180.0f, 80.0f));
    });
}

TEST_F (TransparencyLayerTests, InvalidLayerTakesNoMask)
{
    auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 0.0f, 10.0f), 1.0f);

    EXPECT_EQ (nullptr, layer.addMask());
}

TEST_F (TransparencyLayerTests, EachAddMaskCreatesAnotherMask)
{
    auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 1.0f);
    if (! layer.isValid())
        GTEST_SKIP() << "Transparency layers need offscreen targets, see GraphicsGpuPixelTests";

    auto* first = layer.addMask (LayerMaskMode::Luminance);
    auto* second = layer.addMask (LayerMaskMode::InvertedAlpha);

    ASSERT_NE (nullptr, first);
    ASSERT_NE (nullptr, second);
    EXPECT_NE (first, second);
}

TEST_F (TransparencyLayerTests, MaskedLayerCommitDoesNotCrash)
{
    auto layer = graphics->beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 1.0f);
    if (! layer.isValid())
        GTEST_SKIP() << "Transparency layers need offscreen targets, see GraphicsGpuPixelTests";

    if (auto* mask = layer.addMask (LayerMaskMode::Alpha))
    {
        mask->setFillColor (Colors::white);
        mask->fillEllipse (10.0f, 10.0f, 80.0f, 80.0f);
    }

    EXPECT_NO_THROW (layer.commit());
    EXPECT_EQ (nullptr, layer.addMask());
}

//==============================================================================
// Pixel tests on a real GPU, reading back what was drawn
//==============================================================================

#ifndef YUP_GRAPHICS_TESTS_GPU_PIXEL_HOST
namespace
{

/** No GPU context for the pixel tests on this platform, so they skip. */
class GraphicsGpuPixelTestHost
{
public:
    GraphicsContext* getContext() const noexcept { return nullptr; }

    void makeCurrent() {}
};

} // namespace
#endif

/** Draws with the platform's GPU context: Metal on macOS, OpenGL on Linux, see native/. */
class GraphicsGpuPixelTests : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        host = std::make_unique<GraphicsGpuPixelTestHost>();
        gpuContext = host->getContext();
        if (gpuContext == nullptr)
            return;

        auto canvas = GpuCanvas::create (*gpuContext, size, size);
        if (canvas == nullptr)
        {
            gpuContext = nullptr;
            return;
        }

        // Layer masks only apply in frames drawn with raster ordering, which GL needs fragment shader interlock for
        auto* renderContext = dynamic_cast<rive::gpu::RenderContext*> (canvas->beginDraw().getFactory());
        layerMasksApply = renderContext != nullptr && renderContext->frameSupportsLayerMask();
        canvas->commit();
    }

    static void TearDownTestSuite()
    {
        gpuContext = nullptr;
        host.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No GPU graphics context available";

        host->makeCurrent();
    }

    /** Draws into a transparent size x size canvas and returns its RGBA pixels, or nothing on failure. */
    static std::vector<uint8> render (const std::function<void (Graphics&)>& draw)
    {
        auto canvas = GpuCanvas::create (*gpuContext, size, size);
        if (canvas == nullptr)
            return {};

        draw (canvas->beginDraw());

        std::vector<uint8> pixels (static_cast<std::size_t> (size * size * 4));
        if (! canvas->commit() || ! canvas->readPixels (pixels.data(), pixels.size()))
            return {};

        return pixels;
    }

    static int alphaAt (const std::vector<uint8>& pixels, int x, int y)
    {
        return pixels[static_cast<std::size_t> ((y * size + x) * 4 + 3)];
    }

    /** A layer filled red, masked by white drawn over the left half in the given mode. */
    static void drawLeftHalfMaskedLayer (Graphics& g, LayerMaskMode mode, bool leaveClipOnLayer)
    {
        auto layer = g.beginTransparencyLayer ({ 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
        ASSERT_TRUE (layer.isValid());

        auto& layerGraphics = layer.getGraphics();
        layerGraphics.setFillColor (Colors::red);
        layerGraphics.fillAll();

        if (leaveClipOnLayer)
            layerGraphics.setClipPath (Rectangle<float> (0.0f, 0.0f, 16.0f, 16.0f));

        auto* mask = layer.addMask (mode);
        ASSERT_NE (nullptr, mask);
        mask->setFillColor (Colors::white);
        mask->fillRect (0.0f, 0.0f, static_cast<float> (size) * 0.5f, static_cast<float> (size));

        EXPECT_TRUE (layer.commit());
    }

    static int channelAt (const std::vector<uint8>& pixels, int x, int y, int channel)
    {
        return pixels[static_cast<std::size_t> ((y * size + x) * 4 + channel)];
    }

    /** An opaque 2 x 1 image: red on the left, blue on the right. */
    static Image createRedBlueImage()
    {
        Image image (2, 1);
        auto bytes = image.getRawData();
        const uint8 red[] = { 255, 0, 0, 255 };
        const uint8 blue[] = { 0, 0, 255, 255 };
        std::copy (std::begin (red), std::end (red), bytes.begin());
        std::copy (std::begin (blue), std::end (blue), bytes.begin() + 4);
        return image;
    }

    static Image createWhiteImage()
    {
        Image image (4, 4);
        for (auto& byte : image.getRawData())
            byte = 255;

        return image;
    }

    /** Shapes text in the COLR test font, where "A" is red on the left half and blue on the right,
        "B" is one layer in the text color, "C" a radial gradient from red at the center to blue,
        and "D" a red square with a blue left half layered on top.
    */
    static void shapeColorGlyphText (StyledText& text, const String& string, float width = static_cast<float> (size))
    {
        auto font = loadTestFont ("YupColrTest.ttf");
        ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();

        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ width, static_cast<float> (size) });
        modifier.appendText (string, font.getValue().withHeight (48.0f));
    }

    /** Counts the opaque pixels within [minX, maxX) that match a color test. */
    static int countPixels (const std::vector<uint8>& pixels, const std::function<bool (int r, int g, int b)>& matches, int minX = 0, int maxX = size)
    {
        int count = 0;
        for (int y = 0; y < size; ++y)
        {
            for (int x = minX; x < maxX; ++x)
            {
                const auto* pixel = pixels.data() + static_cast<std::size_t> ((y * size + x) * 4);
                if (pixel[3] > 200 && matches (pixel[0], pixel[1], pixel[2]))
                    ++count;
            }
        }

        return count;
    }

    /** Wraps the GPU context and flushes its frames into a target of the test's choosing, with the
        default GraphicsContext::suspendFrame() rather than the backend's own.
    */
    class FlushingContext : public GraphicsContext
    {
    public:
        FlushingContext (GraphicsContext& contextToWrap, rive::gpu::RenderTarget* targetToFlushInto)
            : wrapped (contextToWrap)
            , target (targetToFlushInto)
        {
        }

        GpuPlatform getPlatform() const noexcept override { return wrapped.getPlatform(); }

        GpuDevice::Ptr getGpuDevice() const noexcept override { return wrapped.getGpuDevice(); }

        rive::Factory* getFactory() override { return wrapped.getFactory(); }

        rive::gpu::RenderContext* getRenderContext() override { return wrapped.getRenderContext(); }

        rive::gpu::RenderTarget* getRenderTarget() override { return target; }

        std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override { return wrapped.makeRenderer (width, height); }

        void onSizeChanged (void*, int, int, float, uint32_t) override {}

        void begin (const rive::gpu::RenderContext::FrameDescriptor& descriptor) override { wrapped.begin (descriptor); }

        void end (void*) override {}

    private:
        GraphicsContext& wrapped;
        rive::gpu::RenderTarget* target = nullptr;
    };

    static ResultValue<Font> loadTestFont (StringRef fileName)
    {
        return Font::loadFontFromFile (
#if YUP_EMSCRIPTEN
            File ("/")
#else
            File (__FILE__).getParentDirectory().getParentDirectory()
#endif
                .getChildFile ("data/fonts")
                .getChildFile (fileName));
    }

    /** Counts the visible pixels in the rows [minY, maxY). */
    static int countVisiblePixelsInRows (const std::vector<uint8>& pixels, int minY, int maxY)
    {
        int count = 0;
        for (int y = minY; y < maxY; ++y)
            for (int x = 0; x < size; ++x)
                if (alphaAt (pixels, x, y) > 50)
                    ++count;

        return count;
    }

    static constexpr int size = 64;
    static std::unique_ptr<GraphicsGpuPixelTestHost> host;
    static GraphicsContext* gpuContext;
    static bool layerMasksApply;
};

std::unique_ptr<GraphicsGpuPixelTestHost> GraphicsGpuPixelTests::host;
GraphicsContext* GraphicsGpuPixelTests::gpuContext = nullptr;
bool GraphicsGpuPixelTests::layerMasksApply = false;

TEST_F (GraphicsGpuPixelTests, AlphaMaskShowsTheLayerWhereTheMaskIsOpaque)
{
    const auto pixels = render ([] (Graphics& g)
    {
        drawLeftHalfMaskedLayer (g, LayerMaskMode::Alpha, false);
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    if (! layerMasksApply)
        GTEST_SKIP() << "Drawn, but this GPU lacks the raster ordering layer masks need";

    EXPECT_GT (alphaAt (pixels, 16, 32), 200);
    EXPECT_LT (alphaAt (pixels, 48, 32), 50);
}

TEST_F (GraphicsGpuPixelTests, InvertedAlphaMaskShowsTheOtherSide)
{
    const auto pixels = render ([] (Graphics& g)
    {
        drawLeftHalfMaskedLayer (g, LayerMaskMode::InvertedAlpha, false);
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    if (! layerMasksApply)
        GTEST_SKIP() << "Drawn, but this GPU lacks the raster ordering layer masks need";

    EXPECT_LT (alphaAt (pixels, 16, 32), 50);
    EXPECT_GT (alphaAt (pixels, 48, 32), 200);
}

TEST_F (GraphicsGpuPixelTests, ClipLeftOnTheLayerDoesNotLimitTheMask)
{
    const auto pixels = render ([] (Graphics& g)
    {
        drawLeftHalfMaskedLayer (g, LayerMaskMode::Alpha, true);
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    if (! layerMasksApply)
        GTEST_SKIP() << "Drawn, but this GPU lacks the raster ordering layer masks need";

    // The fill was drawn before the clip, so it covers the layer: the mask, not the leftover clip,
    // decides what shows, keeping the whole left half and hiding the right half
    EXPECT_GT (alphaAt (pixels, 24, 32), 200);
    EXPECT_LT (alphaAt (pixels, 48, 32), 50);
}

TEST_F (GraphicsGpuPixelTests, StackedMasksMultiply)
{
    const auto pixels = render ([] (Graphics& g)
    {
        auto layer = g.beginTransparencyLayer ({ 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
        ASSERT_TRUE (layer.isValid());

        layer.getGraphics().setFillColor (Colors::red);
        layer.getGraphics().fillAll();

        auto* leftHalf = layer.addMask (LayerMaskMode::Alpha);
        auto* topHalf = layer.addMask (LayerMaskMode::Luminance);
        ASSERT_NE (nullptr, leftHalf);
        ASSERT_NE (nullptr, topHalf);

        leftHalf->setFillColor (Colors::white);
        leftHalf->fillRect (0.0f, 0.0f, static_cast<float> (size) * 0.5f, static_cast<float> (size));

        topHalf->setFillColor (Colors::white);
        topHalf->fillRect (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) * 0.5f);

        EXPECT_TRUE (layer.commit());
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    if (! layerMasksApply)
        GTEST_SKIP() << "Drawn, but this GPU lacks the raster ordering layer masks need";

    EXPECT_GT (alphaAt (pixels, 16, 16), 200);
    EXPECT_LT (alphaAt (pixels, 48, 16), 50);
    EXPECT_LT (alphaAt (pixels, 16, 48), 50);
}

TEST_F (GraphicsGpuPixelTests, ImageMeshCoversItsArea)
{
    const auto pixels = render ([] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), ImageMesh::createGrid ({ 16.0f, 16.0f, 32.0f, 32.0f }, 2, 2));
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 32, 32), 200);
    EXPECT_LT (alphaAt (pixels, 8, 8), 50);
    EXPECT_LT (alphaAt (pixels, 56, 56), 50);
}

TEST_F (GraphicsGpuPixelTests, MeshInstancesLandAtTheirTransforms)
{
    const auto pixels = render ([] (Graphics& g)
    {
        std::vector<ImageMeshInstance> instances (2);
        instances[0].transform = AffineTransform::translation (8.0f, 8.0f);
        instances[1].transform = AffineTransform::translation (40.0f, 40.0f);

        g.drawImageMeshInstanced (createWhiteImage(), ImageMesh::createGrid ({ 0.0f, 0.0f, 8.0f, 8.0f }, 1, 1), instances);
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 12, 12), 200);
    EXPECT_GT (alphaAt (pixels, 44, 44), 200);
    EXPECT_LT (alphaAt (pixels, 28, 28), 50);
}

TEST_F (GraphicsGpuPixelTests, MeshTextureCoordinatesStartAtTheImagesTopLeft)
{
    const auto pixels = render ([] (Graphics& g)
    {
        g.drawImageMesh (createRedBlueImage(), ImageMesh::createGrid ({ 0.0f, 0.0f, 64.0f, 64.0f }, 1, 1), { ImageWrap::Clamp, ImageWrap::Clamp, ImageFilter::Nearest });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (channelAt (pixels, 16, 32, 0), 200);
    EXPECT_LT (channelAt (pixels, 16, 32, 2), 50);
    EXPECT_LT (channelAt (pixels, 48, 32, 0), 50);
    EXPECT_GT (channelAt (pixels, 48, 32, 2), 200);
}

TEST_F (GraphicsGpuPixelTests, MeshInstanceTextureOffsetAndScalePickACell)
{
    const auto pixels = render ([] (Graphics& g)
    {
        // The instance shows only the right half of the image, the blue one
        std::vector<ImageMeshInstance> instances (1);
        instances[0].textureOffset = { 0.5f, 0.0f };
        instances[0].textureScale = { 0.5f, 1.0f };

        g.drawImageMeshInstanced (createRedBlueImage(),
                                  ImageMesh::createGrid ({ 0.0f, 0.0f, 64.0f, 64.0f }, 1, 1),
                                  instances,
                                  { ImageWrap::Clamp, ImageWrap::Clamp, ImageFilter::Nearest });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (channelAt (pixels, 16, 32, 2), 200);
    EXPECT_LT (channelAt (pixels, 16, 32, 0), 50);
}

TEST_F (GraphicsGpuPixelTests, MovedVerticesAreUploadedAgain)
{
    auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 16.0f, 16.0f }, 1, 1);

    const auto first = render ([&] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), mesh);
    });
    ASSERT_EQ (first.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (first, 8, 8), 200);

    const std::vector<Point<float>> moved { { 40.0f, 40.0f }, { 56.0f, 40.0f }, { 40.0f, 56.0f }, { 56.0f, 56.0f } };
    ASSERT_TRUE (mesh.setVertices (moved));

    const auto second = render ([&] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), mesh);
    });
    ASSERT_EQ (second.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_LT (alphaAt (second, 8, 8), 50);
    EXPECT_GT (alphaAt (second, 48, 48), 200);
}

TEST_F (GraphicsGpuPixelTests, FrameDescriptorPassesTriangulationThresholds)
{
    auto canvas = GpuCanvas::create (*gpuContext, size, size);
    ASSERT_NE (canvas, nullptr);

    GpuFrameDescriptor frameDesc;
    frameDesc.triangulationThresholds.minArea = 100.0f;
    frameDesc.triangulationThresholds.maxVerbs = 32;
    frameDesc.triangulationThresholds.frameBudgetMs = 0.5f;

    auto& g = canvas->beginDraw (frameDesc);

    auto* renderContext = dynamic_cast<rive::gpu::RenderContext*> (g.getFactory());
    ASSERT_NE (nullptr, renderContext);

    const auto& thresholds = renderContext->frameDescriptor().triangulationThresholds;
    EXPECT_FLOAT_EQ (100.0f, thresholds.minArea);
    EXPECT_EQ (32u, thresholds.maxVerbs);
    EXPECT_FLOAT_EQ (0.5f, thresholds.frameBudgetMs);

    EXPECT_TRUE (canvas->commit());
}

TEST_F (GraphicsGpuPixelTests, CenteredClipStrokeKeepsOnlyTheBand)
{
    const auto pixels = render ([] (Graphics& g)
    {
        g.setClipStroke (Path().addRectangle (16.0f, 16.0f, 32.0f, 32.0f), StrokeType (8.0f));
        g.setFillColor (Colors::red);
        g.fillAll();
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 16, 32), 200);
    EXPECT_LT (alphaAt (pixels, 32, 32), 50);
    EXPECT_LT (alphaAt (pixels, 4, 32), 50);
}

TEST_F (GraphicsGpuPixelTests, InsideClipStrokeKeepsOnlyTheInnerBand)
{
    const auto pixels = render ([] (Graphics& g)
    {
        g.setClipStroke (Path().addRectangle (16.0f, 16.0f, 32.0f, 32.0f), StrokeType (8.0f).withPosition (StrokePosition::Inside));
        g.setFillColor (Colors::red);
        g.fillAll();
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 20, 32), 200);
    EXPECT_LT (alphaAt (pixels, 12, 32), 50);
    EXPECT_LT (alphaAt (pixels, 32, 32), 50);
}

TEST_F (GraphicsGpuPixelTests, DrawingAfterClipStrokeIsNotShifted)
{
    const auto pixels = render ([] (Graphics& g)
    {
        // A transformed stroke clip wide enough to keep everything, then a plain fill
        g.addTransform (AffineTransform::translation (3.0f, 5.0f).scaled (1.5f));
        g.setClipStroke (Path().addRectangle (0.0f, 0.0f, 10.0f, 10.0f), StrokeType (200.0f));
        g.setTransform (AffineTransform());
        g.setFillColor (Colors::red);
        g.fillRect (40.0f, 40.0f, 8.0f, 8.0f);
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 44, 44), 200);
    EXPECT_LT (alphaAt (pixels, 36, 44), 50);
    EXPECT_LT (alphaAt (pixels, 50, 44), 50);
}

TEST_F (GraphicsGpuPixelTests, FillsColorEmojiInTheirOwnColors)
{
    auto emojiFont = Font::loadColorEmojiSystemFont();
    if (emojiFont.failed())
        GTEST_SKIP() << "No system color emoji font: " << emojiFont.getErrorMessage();

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ static_cast<float> (size), static_cast<float> (size) });
        modifier.appendText (String::fromUTF8 ("\xf0\x9f\x98\x80"), emojiFont.getValue().withHeight (48.0f));
    }

    const auto pixels = render ([&text] (Graphics& g)
    {
        // A black fill would paint a monochrome outline in black
        g.setFillColor (Colors::black);
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    int colorfulPixels = 0;
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
        const auto [minChannel, maxChannel] = std::minmax ({ pixels[i], pixels[i + 1], pixels[i + 2] });
        if (pixels[i + 3] > 128 && maxChannel - minChannel > 64)
            ++colorfulPixels;
    }

    EXPECT_GT (colorfulPixels, 100);
}

TEST_F (GraphicsGpuPixelTests, FillsLayeredColorGlyphsWithTheirPaletteColors)
{
    StyledText text;
    shapeColorGlyphText (text, "A");

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Colors::white);
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // Unpaletted layers would all be black: find both colors, red to the left of blue
    int redPixels = 0, bluePixels = 0;
    float redX = 0.0f, blueX = 0.0f;

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const auto* pixel = pixels.data() + static_cast<std::size_t> ((y * size + x) * 4);
            if (pixel[3] < 200)
                continue;

            if (pixel[0] > 200 && pixel[2] < 50)
            {
                ++redPixels;
                redX += static_cast<float> (x);
            }
            else if (pixel[2] > 200 && pixel[0] < 50)
            {
                ++bluePixels;
                blueX += static_cast<float> (x);
            }
        }
    }

    ASSERT_GT (redPixels, 20);
    ASSERT_GT (bluePixels, 20);
    EXPECT_LT (redX / static_cast<float> (redPixels), blueX / static_cast<float> (bluePixels));
}

TEST_F (GraphicsGpuPixelTests, ColorGlyphTakesEachDrawsTextColor)
{
    // The same text, so the same prepared layer, drawn twice in one frame in two colors
    StyledText text;
    shapeColorGlyphText (text, "B", size * 0.5f);

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Color (0xff00ff00));
        g.fillFittedText (text, { 0.0f, 0.0f, size * 0.5f, static_cast<float> (size) });
        g.setFillColor (Color (0xffff0000));
        g.fillFittedText (text, { size * 0.5f, 0.0f, size * 0.5f, static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    const auto isGreen = [] (int r, int g, int b) { return g > 200 && r < 50 && b < 50; };
    const auto isRed = [] (int r, int g, int b) { return r > 200 && g < 50 && b < 50; };

    EXPECT_GT (countPixels (pixels, isGreen, 0, size / 2), 20);
    EXPECT_EQ (countPixels (pixels, isRed, 0, size / 2), 0);
    EXPECT_GT (countPixels (pixels, isRed, size / 2, size), 20);
    EXPECT_EQ (countPixels (pixels, isGreen, size / 2, size), 0);
}

TEST_F (GraphicsGpuPixelTests, FillsRadialGradientColorGlyphs)
{
    StyledText text;
    shapeColorGlyphText (text, "C");

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Colors::white);
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // Find the glyph, then compare its center with the middle of its left edge
    int minX = size, maxX = -1, minY = size, maxY = -1;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            if (pixels[static_cast<std::size_t> ((y * size + x) * 4 + 3)] > 200)
            {
                minX = jmin (minX, x);
                maxX = jmax (maxX, x);
                minY = jmin (minY, y);
                maxY = jmax (maxY, y);
            }
        }
    }
    ASSERT_GT (maxX - minX, 8);
    ASSERT_GT (maxY - minY, 8);

    const auto pixelAt = [&pixels] (int x, int y)
    {
        return pixels.data() + static_cast<std::size_t> ((y * size + x) * 4);
    };

    const auto* center = pixelAt ((minX + maxX) / 2, (minY + maxY) / 2);
    const auto* edge = pixelAt (minX + 1, (minY + maxY) / 2);

    EXPECT_GT (center[0], center[2] + 50);
    EXPECT_GT (edge[2], edge[0] + 50);
}

TEST_F (GraphicsGpuPixelTests, ColorGlyphsFollowTheBlendMode)
{
    StyledText text;
    shapeColorGlyphText (text, "A");

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Color (0xff800000));
        g.fillAll();

        g.setBlendMode (BlendMode::Additive);
        g.setAdditiveAmount (1.0f);
        g.setFillColor (Colors::white);
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // Added onto dark red, the blue half keeps the red; drawn normally it would replace it
    const auto isBlueOverRed = [] (int r, int, int b) { return b > 200 && r > 100; };
    const auto isPlainBlue = [] (int r, int, int b) { return b > 200 && r < 50; };

    EXPECT_GT (countPixels (pixels, isBlueOverRed), 20);
    EXPECT_EQ (countPixels (pixels, isPlainBlue), 0);
}

TEST_F (GraphicsGpuPixelTests, MixedColoredAndPlainRunsKeepTheirColors)
{
    const auto fontFile = File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts/Linefont-VariableFont_wdth,wght.ttf");
    auto font = Font::loadFontFromFile (fontFile);
    ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ static_cast<float> (size), static_cast<float> (size) });
        modifier.appendText ("abc", Color (0xffff0000), font.getValue().withHeight (24.0f));
        modifier.appendText ("abc", font.getValue().withHeight (24.0f));
    }

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Color (0xff00ff00));
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // The colored run keeps its color, the plain one takes the fill color
    EXPECT_GT (countPixels (pixels, [] (int r, int g, int b) { return r > 200 && g < 50 && b < 50; }), 0);
    EXPECT_GT (countPixels (pixels, [] (int r, int g, int b) { return g > 200 && r < 50 && b < 50; }), 0);
}

TEST_F (GraphicsGpuPixelTests, ColorGlyphsInMixedRunsTakeTheirRunsColor)
{
    // "B" is one layer in the text color: the colored run draws it red, the plain run in the fill
    const auto fontFile = File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts/YupColrTest.ttf");
    auto font = Font::loadFontFromFile (fontFile);
    ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ static_cast<float> (size), static_cast<float> (size) });
        modifier.appendText ("B", Color (0xffff0000), font.getValue().withHeight (24.0f));
        modifier.appendText ("B", font.getValue().withHeight (24.0f));
    }

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Color (0xff00ff00));
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (countPixels (pixels, [] (int r, int g, int b) { return r > 200 && g < 50 && b < 50; }), 20);
    EXPECT_GT (countPixels (pixels, [] (int r, int g, int b) { return g > 200 && r < 50 && b < 50; }), 20);
}

TEST_F (GraphicsGpuPixelTests, LayeredColorGlyphsBlendAsAWhole)
{
    StyledText text;
    shapeColorGlyphText (text, "D");

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFillColor (Colors::black);
        g.fillAll();

        g.setBlendMode (BlendMode::Additive);
        g.setAdditiveAmount (1.0f);
        g.setFillColor (Colors::white);
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // The blue layer covers the red one before the glyph is added: blending each layer on its own
    // would add the blue onto the red, and the left half would turn magenta
    EXPECT_GT (countPixels (pixels, [] (int r, int, int b) { return b > 200 && r < 50; }), 20);
    EXPECT_GT (countPixels (pixels, [] (int r, int, int b) { return r > 200 && b < 50; }), 20);
    EXPECT_EQ (countPixels (pixels, [] (int r, int, int b) { return r > 200 && b > 200; }), 0);
}


TEST_F (GraphicsGpuPixelTests, SingleStopGradientFillsWithItsColor)
{
    const auto pixels = render ([] (Graphics& g)
    {
        ColorGradient gradient (Color (0xffff0000), 0.0f, 0.0f, Color (0xff0000ff), static_cast<float> (size), 0.0f, ColorGradient::Linear);
        gradient.clearStops();
        gradient.addColorStop (Color (0xff00ff00), 0.0f, 0.0f, 0.0f);

        g.setFillColorGradient (gradient);
        g.fillRect (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    for (const auto x : { 8, 56 })
    {
        EXPECT_GT (alphaAt (pixels, x, 32), 200);
        EXPECT_GT (channelAt (pixels, x, 32, 1), 200);
        EXPECT_LT (channelAt (pixels, x, 32, 0), 50);
        EXPECT_LT (channelAt (pixels, x, 32, 2), 50);
    }
}

TEST_F (GraphicsGpuPixelTests, ClipStrokeUnderADegenerateTransformClipsEverything)
{
    const auto pixels = render ([] (Graphics& g)
    {
        g.setTransform (AffineTransform::scaling (0.0f, 0.0f));
        g.setClipStroke (Path().addRectangle (16.0f, 16.0f, 32.0f, 32.0f), StrokeType (8.0f));
        g.setTransform (AffineTransform());
        g.setFillColor (Colors::red);
        g.fillAll();
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_LT (alphaAt (pixels, 16, 32), 50);
    EXPECT_LT (alphaAt (pixels, 32, 32), 50);
    EXPECT_LT (alphaAt (pixels, 4, 4), 50);
}

TEST_F (GraphicsGpuPixelTests, FillImageCoversTheShapeWithTheImage)
{
    const auto image = createRedBlueImage();

    const auto pixels = render ([&image] (Graphics& g)
    {
        // The 2 x 1 image stretched over the whole canvas
        g.setFillImage (image, AffineTransform::scaling (static_cast<float> (size) * 0.5f, static_cast<float> (size)), { ImageWrap::Clamp, ImageWrap::Clamp, ImageFilter::Nearest });
        g.fillRect (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 16, 32), 200);
    EXPECT_GT (channelAt (pixels, 16, 32, 0), 200);
    EXPECT_LT (channelAt (pixels, 16, 32, 2), 50);
    EXPECT_GT (alphaAt (pixels, 48, 32), 200);
    EXPECT_LT (channelAt (pixels, 48, 32, 0), 50);
    EXPECT_GT (channelAt (pixels, 48, 32, 2), 200);
}

TEST_F (GraphicsGpuPixelTests, InvalidMeshesAndEmptyInstanceListsDrawNothing)
{
    const auto pixels = render ([] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), ImageMesh());
        g.drawImageMeshInstanced (createWhiteImage(), ImageMesh::createGrid ({ 0.0f, 0.0f, 64.0f, 64.0f }, 1, 1), {});
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_LT (alphaAt (pixels, 32, 32), 50);
}

TEST_F (GraphicsGpuPixelTests, CopiedMeshesDrawWithBuffersOfTheirOwn)
{
    auto mesh = ImageMesh::createGrid ({ 0.0f, 0.0f, 16.0f, 16.0f }, 1, 1);

    const auto first = render ([&] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), mesh);
    });
    ASSERT_EQ (first.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (first, 8, 8), 200);

    // Copied once the original has buffers: moving the copy's vertices leaves the original in place
    ImageMesh copy (mesh);
    const std::vector<Point<float>> moved { { 40.0f, 40.0f }, { 56.0f, 40.0f }, { 40.0f, 56.0f }, { 56.0f, 56.0f } };
    ASSERT_TRUE (copy.setVertices (moved));

    const auto second = render ([&] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), mesh);
        g.drawImageMesh (createWhiteImage(), copy);
    });
    ASSERT_EQ (second.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (second, 8, 8), 200);
    EXPECT_GT (alphaAt (second, 48, 48), 200);
    EXPECT_LT (alphaAt (second, 28, 28), 50);

    // A move takes the buffers along
    const ImageMesh movedMesh (std::move (copy));

    const auto third = render ([&] (Graphics& g)
    {
        g.drawImageMesh (createWhiteImage(), movedMesh);
    });
    ASSERT_EQ (third.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (third, 48, 48), 200);
    EXPECT_LT (alphaAt (third, 8, 8), 50);
}

TEST_F (GraphicsGpuPixelTests, ImagesWithoutPixelsGetNoTexture)
{
    const Image empty;
    EXPECT_FALSE (empty.createTextureIfNotPresent (*gpuContext));
    EXPECT_EQ (empty.getGpuTexture(), nullptr);

    const Image image (4, 4);
    EXPECT_TRUE (image.createTextureIfNotPresent (*gpuContext));
    EXPECT_NE (image.getGpuTexture(), nullptr);
}

TEST_F (GraphicsGpuPixelTests, DefaultSuspendFrameFlushesTheFrameIntoTheRenderTarget)
{
    auto device = gpuContext->getGpuDevice();
    ASSERT_NE (device, nullptr);

    auto target = device->createOffscreenTarget (size, size);
    ASSERT_NE (target, nullptr);

    FlushingContext context (*gpuContext, target->getRenderTarget());
    auto renderer = context.makeRenderer (size, size);
    ASSERT_NE (renderer, nullptr);

    // Loading rather than clearing, so the wrapped context does not clear a window it does not have.
    // The fill covers the whole target, so what was loaded does not matter.
    rive::gpu::RenderContext::FrameDescriptor descriptor;
    descriptor.renderTargetWidth = static_cast<uint32_t> (size);
    descriptor.renderTargetHeight = static_cast<uint32_t> (size);
    descriptor.loadAction = rive::gpu::LoadAction::preserveRenderTarget;
    context.begin (descriptor);

    Graphics g (context, *renderer);
    g.setFillColor (Colors::red);
    g.fillRect (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));

    context.suspendFrame();

    std::vector<uint8> pixels (static_cast<std::size_t> (size * size * 4));
    ASSERT_TRUE (device->readOffscreenPixels (*target, pixels.data(), pixels.size()));

    EXPECT_GT (alphaAt (pixels, 32, 32), 200);
    EXPECT_GT (channelAt (pixels, 32, 32, 0), 200);
    EXPECT_LT (channelAt (pixels, 32, 32, 2), 50);
}

TEST_F (GraphicsGpuPixelTests, EllipsisOverflowClipsTextToItsArea)
{
    auto font = loadTestFont ("YupColrTest.ttf");
    ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();

    // "D" is 38 pixels tall at 48 pixels, laid out from the top and drawn into an area 16 pixels tall
    const auto renderD = [&font] (StyledText::TextOverflow overflow)
    {
        StyledText text;
        {
            auto modifier = text.startUpdate();
            modifier.setMaxSize ({ static_cast<float> (size), static_cast<float> (size) });
            modifier.setVerticalAlign (StyledText::top);
            modifier.setOverflow (overflow);
            modifier.appendText ("D", font.getValue().withHeight (48.0f));
        }

        return render ([&text] (Graphics& g)
        {
            g.setFillColor (Colors::white);
            g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), 16.0f });
        });
    };

    const auto clipped = renderD (StyledText::ellipsis);
    const auto unclipped = renderD (StyledText::visible);
    ASSERT_EQ (clipped.size(), static_cast<std::size_t> (size * size * 4));
    ASSERT_EQ (unclipped.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (countVisiblePixelsInRows (clipped, 0, 16), 100);
    EXPECT_EQ (countVisiblePixelsInRows (clipped, 20, size), 0);
    EXPECT_GT (countVisiblePixelsInRows (unclipped, 20, size), 100);
}

TEST_F (GraphicsGpuPixelTests, FeatheredTextFillLeavesColoredRunsSharp)
{
    auto font = loadTestFont ("Linefont-VariableFont_wdth,wght.ttf");
    ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ static_cast<float> (size), static_cast<float> (size) });
        modifier.appendText ("abc", Color (0xffff0000), font.getValue().withHeight (24.0f));
        modifier.appendText ("abc", font.getValue().withHeight (24.0f));
    }

    const auto pixels = render ([&text] (Graphics& g)
    {
        g.setFeather (2.0f);
        g.setFillColor (Color (0xff00ff00));
        g.fillFittedText (text, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // The colored run is drawn with its own paint, the plain one softened in the fill color
    int softGreenPixels = 0;
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
        if (pixels[i + 1] > pixels[i] + 16 && pixels[i + 1] > pixels[i + 2] + 16)
            ++softGreenPixels;
    }

    EXPECT_GT (countPixels (pixels, [] (int r, int g, int b) { return r > 200 && g < 50 && b < 50; }), 0);
    EXPECT_GT (softGreenPixels, 0);
}

//==============================================================================
// What Graphics asks a renderer to draw, checked without a GPU
//==============================================================================

class GraphicsDrawCallTests : public ::testing::Test
{
protected:
    struct DrawCalls
    {
        int paths = 0;
        int clips = 0;
        int layerMasks = 0;
    };

    /** Counts the draws it is asked for. */
    class CountingRenderer : public rive::Renderer
    {
    public:
        explicit CountingRenderer (DrawCalls& callsToCount)
            : calls (callsToCount)
        {
        }

        using rive::Renderer::drawImage;
        using rive::Renderer::drawImageMesh;

        void save() override {}

        void restore() override {}

        void transform (const rive::Mat2D&) override {}

        void drawPath (rive::RenderPath*, rive::RenderPaint*) override { ++calls.paths; }

        void clipPath (rive::RenderPath*) override { ++calls.clips; }

        void drawImage (const rive::RenderImage*, rive::ImageSampler, rive::BlendMode, float) override {}

        void drawImageMesh (const rive::RenderImage*,
                            rive::ImageSampler,
                            rive::rcp<rive::RenderBuffer>,
                            rive::rcp<rive::RenderBuffer>,
                            rive::rcp<rive::RenderBuffer>,
                            uint32_t,
                            uint32_t,
                            rive::BlendMode,
                            float) override {}

        void applyLayerMask (const rive::RenderImage*, rive::ImageSampler, rive::LayerMaskMode) override { ++calls.layerMasks; }

        void modulateOpacity (float) override {}

    private:
        DrawCalls& calls;
    };

    /** A headless context whose device hands out the mock targets given to it, and whose renderers count their draws. */
    class MockTargetContext : public GraphicsContext
    {
    public:
        MockTargetContext (GpuDevice::Ptr deviceToUse, DrawCalls& callsToCount)
            : device (std::move (deviceToUse))
            , calls (callsToCount)
        {
        }

        GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Headless; }

        GpuDevice::Ptr getGpuDevice() const noexcept override { return device; }

        rive::Factory* getFactory() override { return headless->getFactory(); }

        rive::gpu::RenderContext* getRenderContext() override { return nullptr; }

        rive::gpu::RenderTarget* getRenderTarget() override { return nullptr; }

        std::unique_ptr<rive::Renderer> makeRenderer (int, int) override { return std::make_unique<CountingRenderer> (calls); }

        void onSizeChanged (void*, int, int, float, uint32_t) override {}

        void begin (const rive::gpu::RenderContext::FrameDescriptor&) override {}

        void end (void*) override {}

    private:
        std::unique_ptr<GraphicsContext> headless = GraphicsContext::createContext (GpuPlatform::Headless, {});
        GpuDevice::Ptr device;
        DrawCalls& calls;
    };

    void SetUp() override
    {
        device = new OreAndTargetGpuDevice (nullptr, nullptr);
        context = std::make_unique<MockTargetContext> (GpuDevice::Ptr (device.get()), calls);
        renderer = context->makeRenderer (size, size);
        graphics = std::make_unique<Graphics> (*context, *renderer);
        graphics->setDrawingArea ({ 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });
    }

    /** A target without a render canvas, whose texture is adopted instead. */
    static std::unique_ptr<MockOffscreenTarget> targetWithTexture()
    {
        return MockOffscreenTarget::withGpuTexture (size, size);
    }

    /** A target with neither a render canvas nor a texture. */
    static std::unique_ptr<MockOffscreenTarget> targetWithoutTexture()
    {
        return std::make_unique<::testing::NiceMock<MockOffscreenTarget>> (size, size);
    }

    static Font loadTestFont (StringRef fileName)
    {
        auto font = Font::loadFontFromFile (
#if YUP_EMSCRIPTEN
            File ("/")
#else
            File (__FILE__).getParentDirectory().getParentDirectory()
#endif
                .getChildFile ("data/fonts")
                .getChildFile (fileName));
        EXPECT_TRUE (font.wasOk()) << font.getErrorMessage();
        return font.valueOr (Font());
    }

    static constexpr int size = 64;
    const Rectangle<float> area { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) };

    DrawCalls calls;
    ReferenceCountedObjectPtr<OreAndTargetGpuDevice> device;
    std::unique_ptr<MockTargetContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    std::unique_ptr<Graphics> graphics;
};

TEST_F (GraphicsDrawCallTests, CommitToImageAdoptsTheTargetTextureWithoutARenderCanvas)
{
    device->setNextOffscreenTarget (targetWithTexture());

    Image image (size, size);
    Graphics g (*context, image);
    ASSERT_TRUE (g.isOffscreen());

    EXPECT_TRUE (g.commitToImage());
    EXPECT_NE (image.getGpuTexture(), nullptr);
}

TEST_F (GraphicsDrawCallTests, LayerMasksComeFromTheirTargetTextures)
{
    device->setNextOffscreenTarget (targetWithTexture());
    auto layer = graphics->beginTransparencyLayer ({ 0.0f, 0.0f, 32.0f, 32.0f });
    ASSERT_TRUE (layer.isValid());

    // The device has no target left for a mask
    EXPECT_EQ (layer.addMask(), nullptr);

    device->setNextOffscreenTarget (targetWithTexture());
    ASSERT_NE (layer.addMask (LayerMaskMode::InvertedLuminance), nullptr);

    // The mask is applied, but the headless parent has no render context to draw the layer with
    EXPECT_FALSE (layer.commit());
    EXPECT_EQ (calls.layerMasks, 1);
    EXPECT_FALSE (layer.isValid());
}

TEST_F (GraphicsDrawCallTests, LayerWithoutTexturesSkipsItsMasksAndFailsToCommit)
{
    device->setNextOffscreenTarget (targetWithoutTexture());
    auto layer = graphics->beginTransparencyLayer ({ 0.0f, 0.0f, 32.0f, 32.0f });
    ASSERT_TRUE (layer.isValid());

    device->setNextOffscreenTarget (targetWithoutTexture());
    ASSERT_NE (layer.addMask(), nullptr);

    EXPECT_FALSE (layer.commit());
    EXPECT_EQ (calls.layerMasks, 0);
    EXPECT_FALSE (layer.isValid());
}

TEST_F (GraphicsDrawCallTests, FillImageWithoutTextureDrawsOnlyRunsWithTheirOwnColor)
{
    const auto outlineFont = loadTestFont ("Linefont-VariableFont_wdth,wght.ttf");
    const auto colorFont = loadTestFont ("YupColrTest.ttf");

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (area.getSize());
        modifier.appendText ("abc", Color (0xffff0000), outlineFont.withHeight (24.0f));
        modifier.appendText ("abc", outlineFont.withHeight (24.0f));
        modifier.appendText ("B", colorFont.withHeight (24.0f));
    }

    // Headless images get no texture, so the plain outlines and the color glyph have no paint
    graphics->setFillImage (Image (4, 4));
    graphics->fillFittedText (text, area);

    EXPECT_EQ (calls.paths, 1);
}

TEST_F (GraphicsDrawCallTests, StrokeImageWithoutTextureDrawsNoText)
{
    const auto font = loadTestFont ("Linefont-VariableFont_wdth,wght.ttf");

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (area.getSize());
        modifier.appendText ("abc", font.withHeight (24.0f));
    }

    graphics->setStrokeImage (Image (4, 4));
    graphics->strokeFittedText (text, area);
    EXPECT_EQ (calls.paths, 0);

    graphics->setStrokeColor (Colors::white);
    graphics->strokeFittedText (text, area);
    EXPECT_EQ (calls.paths, 1);
}

TEST_F (GraphicsDrawCallTests, EllipsisOverflowClipsTextToItsArea)
{
    const auto font = loadTestFont ("Linefont-VariableFont_wdth,wght.ttf");

    const auto fillText = [&] (StyledText::TextOverflow overflow)
    {
        StyledText text;
        {
            auto modifier = text.startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.setOverflow (overflow);
            modifier.appendText ("abc", font.withHeight (24.0f));
        }

        graphics->fillFittedText (text, area);
    };

    graphics->setFillColor (Colors::white);

    fillText (StyledText::visible);
    EXPECT_EQ (calls.clips, 0);

    fillText (StyledText::ellipsis);
    EXPECT_EQ (calls.clips, 1);
    EXPECT_EQ (calls.paths, 2);
}

TEST_F (GraphicsDrawCallTests, FeatheredFillDrawsEveryRun)
{
    const auto font = loadTestFont ("Linefont-VariableFont_wdth,wght.ttf");

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (area.getSize());
        modifier.appendText ("abc", Color (0xffff0000), font.withHeight (24.0f));
        modifier.appendText ("abc", font.withHeight (24.0f));
    }

    // The colored run with its own paint, the plain one as a feathered outline in the fill paint
    graphics->setFeather (2.0f);
    graphics->setFillColor (Colors::white);
    graphics->fillFittedText (text, area);

    EXPECT_EQ (calls.paths, 2);
}

TEST_F (GraphicsDrawCallTests, ColorGlyphsDrawEachOfTheirLayers)
{
    const auto font = loadTestFont ("YupColrTest.ttf");

    // "A" has two solid layers, "B" one in the text color and "C" one radial gradient
    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (area.getSize());
        modifier.appendText ("ABC", font.withHeight (16.0f));
    }

    graphics->setFillColor (Colors::white);
    graphics->fillFittedText (text, area);

    EXPECT_EQ (calls.paths, 4);
}

TEST_F (GraphicsDrawCallTests, BlendedLayeredColorGlyphsNeedAnOffscreenLayer)
{
    const auto font = loadTestFont ("YupColrTest.ttf");

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (area.getSize());
        modifier.setOverflow (StyledText::ellipsis);
        modifier.appendText ("A", font.withHeight (48.0f));
    }

    graphics->setBlendMode (BlendMode::Additive);
    graphics->setAdditiveAmount (1.0f);
    graphics->setFillColor (Colors::white);

    // On the canvas the glyph is clipped to the text area once more, then needs a layer the device cannot make
    graphics->fillFittedText (text, area);
    EXPECT_EQ (calls.clips, 2);
    EXPECT_EQ (calls.paths, 0);

    // Off the canvas it is skipped before that
    graphics->fillFittedText (text, area.translated (1000.0f, 1000.0f));
    EXPECT_EQ (calls.clips, 3);
    EXPECT_EQ (calls.paths, 0);
}
