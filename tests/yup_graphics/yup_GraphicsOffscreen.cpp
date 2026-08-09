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
    singleStop.addColorStop (Color (0xFF00FF00), 0.0f);

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
