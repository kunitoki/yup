/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <cmath>

using namespace yup;

class GraphicsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
        renderer = context->makeRenderer (200, 200);
        ASSERT_NE (renderer, nullptr);
        graphics = std::make_unique<Graphics> (*context, *renderer);
    }

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    std::unique_ptr<Graphics> graphics;
};

TEST_F (GraphicsTest, Default_Constructor)
{
    EXPECT_FLOAT_EQ (graphics->getContextScale(), 1.0f);
    EXPECT_EQ (graphics->getStrokeJoin(), StrokeJoin::Miter);
    EXPECT_EQ (graphics->getStrokeCap(), StrokeCap::Square);
    EXPECT_EQ (graphics->getFillColor(), Color (0xff000000));
    EXPECT_EQ (graphics->getStrokeColor(), Color (0xff000000));
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 1.0f);
    EXPECT_FLOAT_EQ (graphics->getFeather(), 0.0f);
    EXPECT_TRUE (graphics->getDrawingArea().isEmpty());
    EXPECT_TRUE (graphics->getTransform().isIdentity());
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::SrcOver);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 1.0f);
}

TEST_F (GraphicsTest, Fill_Color_Operations)
{
    // Test setting and getting fill color
    Color testColor (0xff00ff00); // Green
    graphics->setFillColor (testColor);
    EXPECT_EQ (graphics->getFillColor(), testColor);

    // Test with alpha
    Color transparentRed (0x80ff0000);
    graphics->setFillColor (transparentRed);
    EXPECT_EQ (graphics->getFillColor(), transparentRed);
}

TEST_F (GraphicsTest, Stroke_Color_Operations)
{
    // Test setting and getting stroke color
    Color testColor (0xff0000ff); // Blue
    graphics->setStrokeColor (testColor);
    EXPECT_EQ (graphics->getStrokeColor(), testColor);

    // Test with different alpha
    Color semiTransparentYellow (0xc0ffff00);
    graphics->setStrokeColor (semiTransparentYellow);
    EXPECT_EQ (graphics->getStrokeColor(), semiTransparentYellow);
}

TEST_F (GraphicsTest, Color_Gradient_Operations)
{
    // Test fill gradient
    ColorGradient fillGradient (
        Color (0xffff0000), 0.0f, 0.0f, // Red start
        Color (0xff0000ff),
        100.0f,
        100.0f, // Blue end
        ColorGradient::Linear);
    graphics->setFillColorGradient (fillGradient);

    auto retrievedFillGradient = graphics->getFillColorGradient();
    EXPECT_EQ (retrievedFillGradient.getType(), ColorGradient::Linear);
    EXPECT_EQ (retrievedFillGradient.getStartColor(), Color (0xffff0000));
    EXPECT_EQ (retrievedFillGradient.getFinishColor(), Color (0xff0000ff));

    // Test stroke gradient
    ColorGradient strokeGradient (
        Color (0xff00ff00), 50.0f, 50.0f, // Green center
        Color (0xffffff00),
        0.0f,
        0.0f, // Yellow edge
        ColorGradient::Radial);
    graphics->setStrokeColorGradient (strokeGradient);

    auto retrievedStrokeGradient = graphics->getStrokeColorGradient();
    EXPECT_EQ (retrievedStrokeGradient.getType(), ColorGradient::Radial);
    EXPECT_EQ (retrievedStrokeGradient.getStartColor(), Color (0xff00ff00));
    EXPECT_EQ (retrievedStrokeGradient.getFinishColor(), Color (0xffffff00));
}

TEST_F (GraphicsTest, Stroke_Properties)
{
    // Test stroke width
    graphics->setStrokeWidth (5.0f);
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 5.0f);

    // Test negative stroke width (should be clamped to 0)
    graphics->setStrokeWidth (-2.0f);
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 0.0f);

    // Test stroke join
    graphics->setStrokeJoin (StrokeJoin::Round);
    EXPECT_EQ (graphics->getStrokeJoin(), StrokeJoin::Round);

    graphics->setStrokeJoin (StrokeJoin::Bevel);
    EXPECT_EQ (graphics->getStrokeJoin(), StrokeJoin::Bevel);

    // Test stroke cap
    graphics->setStrokeCap (StrokeCap::Round);
    EXPECT_EQ (graphics->getStrokeCap(), StrokeCap::Round);

    graphics->setStrokeCap (StrokeCap::Butt);
    EXPECT_EQ (graphics->getStrokeCap(), StrokeCap::Butt);
}

TEST_F (GraphicsTest, Rendering_Properties)
{
    // Test feather
    graphics->setFeather (2.5f);
    EXPECT_FLOAT_EQ (graphics->getFeather(), 2.5f);

    // Test negative feather (should be clamped to 0)
    graphics->setFeather (-1.0f);
    EXPECT_FLOAT_EQ (graphics->getFeather(), 0.0f);

    // Test opacity
    graphics->setOpacity (0.7f);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.7f);

    // Test opacity clamping
    graphics->setOpacity (1.5f);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 1.0f);

    graphics->setOpacity (-0.2f);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.0f);

    // Test blend modes
    graphics->setBlendMode (BlendMode::Screen);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::Screen);

    graphics->setBlendMode (BlendMode::Multiply);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::Multiply);

    graphics->setBlendMode (BlendMode::Overlay);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::Overlay);
}

TEST_F (GraphicsTest, Drawing_Area_Operations)
{
    Rectangle<float> testArea (10.0f, 20.0f, 100.0f, 150.0f);
    graphics->setDrawingArea (testArea);

    auto retrievedArea = graphics->getDrawingArea();
    EXPECT_FLOAT_EQ (retrievedArea.getX(), 10.0f);
    EXPECT_FLOAT_EQ (retrievedArea.getY(), 20.0f);
    EXPECT_FLOAT_EQ (retrievedArea.getWidth(), 100.0f);
    EXPECT_FLOAT_EQ (retrievedArea.getHeight(), 150.0f);
}

TEST_F (GraphicsTest, Transform_Operations_Identity)
{
    // Test setting identity transform
    auto identityTransform = AffineTransform::identity();
    graphics->setTransform (identityTransform);
    EXPECT_TRUE (graphics->getTransform().isIdentity());

    // Test translation
    auto translation = AffineTransform::translation (50.0f, 30.0f);
    graphics->setTransform (translation);
    auto retrievedTransform = graphics->getTransform();
    EXPECT_FLOAT_EQ (retrievedTransform.getTranslateX(), 50.0f);
    EXPECT_FLOAT_EQ (retrievedTransform.getTranslateY(), 30.0f);

    // Test scaling
    auto scaling = AffineTransform::scaling (2.0f, 1.5f);
    graphics->setTransform (scaling);
    retrievedTransform = graphics->getTransform();
    EXPECT_FLOAT_EQ (retrievedTransform.getScaleX(), 2.0f);
    EXPECT_FLOAT_EQ (retrievedTransform.getScaleY(), 1.5f);
}

TEST_F (GraphicsTest, Transform_Operations_Rotation)
{
    // Test rotation
    constexpr float pi = 3.14159265358979323846f;
    constexpr float angle = pi / 4.0f; // 45 degrees

    auto rotation = AffineTransform::rotation (angle);
    graphics->setTransform (rotation);
    auto retrievedTransform = graphics->getTransform();
    EXPECT_NEAR (retrievedTransform.getScaleX(), std::cos (angle), 1e-5f);
    EXPECT_NEAR (retrievedTransform.getShearY(), std::sin (angle), 1e-5f);
}

TEST_F (GraphicsTest, State_Save_And_Restore)
{
    // Set up initial state
    graphics->setFillColor (Color (0xffff0000));
    graphics->setStrokeWidth (3.0f);
    graphics->setOpacity (0.8f);
    graphics->setBlendMode (BlendMode::Screen);

    // Save state and modify values
    {
        auto savedState = graphics->saveState();

        graphics->setFillColor (Color (0xff00ff00));
        graphics->setStrokeWidth (7.0f);
        graphics->setOpacity (0.5f);
        graphics->setBlendMode (BlendMode::Multiply);

        // Check modified values
        EXPECT_EQ (graphics->getFillColor(), Color (0xff00ff00));
        EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 7.0f);
        EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.5f);
        EXPECT_EQ (graphics->getBlendMode(), BlendMode::Multiply);
    } // savedState destructor should restore here

    // Check that original values are restored
    EXPECT_EQ (graphics->getFillColor(), Color (0xffff0000));
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 3.0f);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.8f);
    EXPECT_EQ (graphics->getBlendMode(), BlendMode::Screen);
}

TEST_F (GraphicsTest, Drawing_Operations_Do_Not_Crash)
{
    // Set up drawing area
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    // Test basic drawing operations (just ensure they don't crash)
    EXPECT_NO_THROW ({
        graphics->fillAll();
        graphics->strokeLine (10.0f, 10.0f, 50.0f, 50.0f);
        graphics->fillRect (20.0f, 20.0f, 30.0f, 40.0f);
        graphics->strokeRect (60.0f, 60.0f, 25.0f, 35.0f);
        graphics->fillRoundedRect (100.0f, 100.0f, 40.0f, 30.0f, 5.0f);
        graphics->strokeRoundedRect (150.0f, 150.0f, 30.0f, 20.0f, 3.0f, 4.0f, 5.0f, 6.0f);
    });
}

TEST_F (GraphicsTest, Path_Drawing_Operations)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    // Create a simple path
    Path testPath;
    testPath.moveTo (10.0f, 10.0f);
    testPath.lineTo (50.0f, 10.0f);
    testPath.lineTo (50.0f, 50.0f);
    testPath.lineTo (10.0f, 50.0f);
    testPath.close();

    // Test path drawing operations
    EXPECT_NO_THROW ({
        graphics->fillPath (testPath);
        graphics->strokePath (testPath);
    });
}

TEST_F (GraphicsTest, Clipping_Operations)
{
    // Test rectangle clipping
    Rectangle<float> clipRect (25.0f, 25.0f, 150.0f, 150.0f);
    EXPECT_NO_THROW ({
        graphics->setClipPath (clipRect);
    });

    // Test path clipping
    Path clipPath;
    clipPath.addEllipse (50.0f, 50.0f, 100.0f, 100.0f);
    EXPECT_NO_THROW ({
        graphics->setClipPath (clipPath);
    });
}

TEST_F (GraphicsTest, Factory_And_Renderer_Access)
{
    // Test that we can access factory and renderer
    auto* factory = graphics->getFactory();
    EXPECT_NE (factory, nullptr);

    auto* renderer = graphics->getRenderer();
    EXPECT_NE (renderer, nullptr);
}

TEST_F (GraphicsTest, Context_Scale)
{
    // Test with different scale
    auto scaledRenderer = context->makeRenderer (200, 200);
    Graphics scaledGraphics (*context, *scaledRenderer, 2.0f);

    EXPECT_FLOAT_EQ (scaledGraphics.getContextScale(), 2.0f);
}

TEST_F (GraphicsTest, Line_Drawing_With_Points)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    Point<float> p1 (10.0f, 20.0f);
    Point<float> p2 (30.0f, 40.0f);

    EXPECT_NO_THROW ({
        graphics->strokeLine (p1, p2);
    });
}

TEST_F (GraphicsTest, Rectangle_Variations)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    Rectangle<float> rect (10.0f, 20.0f, 50.0f, 30.0f);

    EXPECT_NO_THROW ({
        graphics->fillRect (rect);
        graphics->strokeRect (rect);
        graphics->fillRoundedRect (rect, 5.0f);
        graphics->strokeRoundedRect (rect, 3.0f);
        graphics->fillRoundedRect (rect, 2.0f, 3.0f, 4.0f, 5.0f);
        graphics->strokeRoundedRect (rect, 1.0f, 2.0f, 3.0f, 4.0f);
    });
}

TEST_F (GraphicsTest, Multiple_State_Nesting)
{
    // Test nested state saving and restoring
    graphics->setFillColor (Color (0xffff0000)); // Red

    {
        auto state1 = graphics->saveState();
        graphics->setFillColor (Color (0xff00ff00)); // Green

        {
            auto state2 = graphics->saveState();
            graphics->setFillColor (Color (0xff0000ff)); // Blue
            EXPECT_EQ (graphics->getFillColor(), Color (0xff0000ff));
        }

        EXPECT_EQ (graphics->getFillColor(), Color (0xff00ff00));
    }

    EXPECT_EQ (graphics->getFillColor(), Color (0xffff0000));
}

TEST_F (GraphicsTest, Opacity_Color_Interaction)
{
    // Test how opacity interacts with colors
    graphics->setFillColor (Color (0xffff0000)); // Opaque red
    graphics->setOpacity (0.5f);

    auto resultColor = graphics->getFillColor();
    // Color should NOT be multiplied by opacity
    EXPECT_EQ (resultColor.getAlpha(), 255); // 255 * 0.5 ≈ 127
    EXPECT_EQ (resultColor.getRed(), 255);
    EXPECT_EQ (resultColor.getGreen(), 0);
    EXPECT_EQ (resultColor.getBlue(), 0);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.5f);
}

TEST_F (GraphicsTest, Opacity_Gradient_Interaction)
{
    ColorGradient gradient (
        Color (0xffff0000), 0.0f, 0.0f, Color (0xff00ff00), 100.0f, 100.0f, ColorGradient::Linear);

    graphics->setFillColorGradient (gradient);
    graphics->setOpacity (0.3f);

    auto resultGradient = graphics->getFillColorGradient();

    // Neither color should be affected by opacity
    auto startColor = resultGradient.getStartColor();
    EXPECT_EQ (startColor.getAlpha(), 255);

    auto endColor = resultGradient.getFinishColor();
    EXPECT_EQ (endColor.getAlpha(), 255);

    EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.3f);
}

TEST_F (GraphicsTest, Text_Rendering_Operations)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    // Create a styled text object
    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (Size<float> (180.0f, 100.0f));
        modifier.setHorizontalAlign (StyledText::center);
        modifier.setVerticalAlign (StyledText::middle);
        modifier.appendText ("abcdefg", Font());
    }

    Rectangle<float> textRect (10.0f, 10.0f, 180.0f, 100.0f);

    // These should not crash even with empty text
    EXPECT_NO_THROW ({
        graphics->fillFittedText (styledText, textRect);
        graphics->strokeFittedText (styledText, textRect);
    });
}

TEST_F (GraphicsTest, Image_Drawing_Operations)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    // Create a simple test image
    Image testImage (10, 10, PixelFormat::RGBA);
    testImage.fill (0xffff0000); // Fill with red

    Point<float> drawPosition (50.0f, 60.0f);

    // This should not crash (though might not render in headless mode)
    EXPECT_NO_THROW ({
        graphics->drawImageAt (testImage, drawPosition);
    });
}

TEST_F (GraphicsTest, Transform_Accumulation)
{
    // Test that transforms accumulate properly using addTransform
    auto translation1 = AffineTransform::translation (10.0f, 20.0f);
    auto translation2 = AffineTransform::translation (5.0f, 15.0f);

    graphics->setTransform (translation1);
    graphics->addTransform (translation2);

    auto result = graphics->getTransform();

    // The transforms should be combined
    EXPECT_FLOAT_EQ (result.getTranslateX(), 15.0f); // 10 + 5
    EXPECT_FLOAT_EQ (result.getTranslateY(), 35.0f); // 20 + 15
}

TEST_F (GraphicsTest, Transform_SetReplace)
{
    // Test that setTransform replaces instead of accumulates
    auto translation1 = AffineTransform::translation (10.0f, 20.0f);
    auto translation2 = AffineTransform::translation (5.0f, 15.0f);

    graphics->setTransform (translation1);
    graphics->setTransform (translation2);

    auto result = graphics->getTransform();

    // The second transform should replace the first
    EXPECT_FLOAT_EQ (result.getTranslateX(), 5.0f);  // Only second transform
    EXPECT_FLOAT_EQ (result.getTranslateY(), 15.0f); // Only second transform
}

TEST_F (GraphicsTest, Edge_Case_Values)
{
    // Test edge case values
    graphics->setStrokeWidth (0.0f);
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 0.0f);

    graphics->setFeather (0.0f);
    EXPECT_FLOAT_EQ (graphics->getFeather(), 0.0f);

    graphics->setOpacity (0.0f);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 0.0f);

    graphics->setOpacity (1.0f);
    EXPECT_FLOAT_EQ (graphics->getOpacity(), 1.0f);

    // Test drawing with zero-sized rectangles (should not crash)
    EXPECT_NO_THROW ({
        graphics->fillRect (0.0f, 0.0f, 0.0f, 0.0f);
        graphics->strokeRect (10.0f, 10.0f, 0.0f, 5.0f);
    });
}

TEST_F (GraphicsTest, Complex_Path_Operations)
{
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 200.0f, 200.0f));

    Path complexPath;

    // Create a more complex path
    complexPath.moveTo (50.0f, 50.0f);
    complexPath.lineTo (100.0f, 50.0f);
    complexPath.quadTo (125.0f, 75.0f, 100.0f, 100.0f);
    complexPath.cubicTo (90.0f, 110.0f, 70.0f, 120.0f, 50.0f, 100.0f);
    complexPath.close();

    // Add a circle to the path
    complexPath.addCenteredEllipse (Point<float> (75.0f, 75.0f), 15.0f, 15.0f);

    EXPECT_NO_THROW ({
        graphics->fillPath (complexPath);
        graphics->strokePath (complexPath);
    });
}

TEST_F (GraphicsTest, All_Blend_Modes)
{
    // Test that all blend modes can be set without issues
    std::vector<BlendMode> allBlendModes = {
        BlendMode::SrcOver,
        BlendMode::Screen,
        BlendMode::Overlay,
        BlendMode::Darken,
        BlendMode::Lighten,
        BlendMode::ColorDodge,
        BlendMode::ColorBurn,
        BlendMode::HardLight,
        BlendMode::SoftLight,
        BlendMode::Difference,
        BlendMode::Exclusion,
        BlendMode::Multiply,
        BlendMode::Hue,
        BlendMode::Saturation,
        BlendMode::Color,
        BlendMode::Luminosity
    };

    for (auto blendMode : allBlendModes)
    {
        EXPECT_NO_THROW ({
            graphics->setBlendMode (blendMode);
            EXPECT_EQ (graphics->getBlendMode(), blendMode);
        });
    }
}

TEST_F (GraphicsTest, State_Independence)
{
    // Test that different graphics instances don't interfere
    auto secondRenderer = context->makeRenderer (200, 200);
    Graphics secondGraphics (*context, *secondRenderer);

    // Set different values on each instance
    graphics->setFillColor (Color (0xffff0000));
    secondGraphics.setFillColor (Color (0xff00ff00));

    graphics->setStrokeWidth (5.0f);
    secondGraphics.setStrokeWidth (10.0f);

    // Values should remain independent
    EXPECT_EQ (graphics->getFillColor(), Color (0xffff0000));
    EXPECT_EQ (secondGraphics.getFillColor(), Color (0xff00ff00));
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 5.0f);
    EXPECT_FLOAT_EQ (secondGraphics.getStrokeWidth(), 10.0f);
}

TEST_F (GraphicsTest, Large_Values)
{
    // Test with large coordinate values
    graphics->setDrawingArea (Rectangle<float> (0.0f, 0.0f, 10000.0f, 10000.0f));

    EXPECT_NO_THROW ({
        graphics->fillRect (1000.0f, 2000.0f, 3000.0f, 4000.0f);
        graphics->strokeLine (0.0f, 0.0f, 9999.0f, 9999.0f);
    });

    // Test with large stroke width
    graphics->setStrokeWidth (1000.0f);
    EXPECT_FLOAT_EQ (graphics->getStrokeWidth(), 1000.0f);
}
