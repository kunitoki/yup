/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

// ==============================================================================
// Constructor and Default State Tests
// ==============================================================================

TEST (DrawableTests, DefaultConstructorCreatesEmptyDrawable)
{
    Drawable drawable;

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_EQ (0.0f, bounds.getWidth());
    EXPECT_EQ (0.0f, bounds.getHeight());
}

TEST (DrawableTests, DefaultBoundsAreEmpty)
{
    Drawable drawable;

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_TRUE (bounds.isEmpty());
}

// ==============================================================================
// Clear Tests
// ==============================================================================

TEST (DrawableTests, ClearResetsDrawable)
{
    Drawable drawable;

    drawable.clear();

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_TRUE (bounds.isEmpty());
}

TEST (DrawableTests, ClearMultipleTimes)
{
    Drawable drawable;

    drawable.clear();
    drawable.clear();
    drawable.clear();

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_TRUE (bounds.isEmpty());
}

// ==============================================================================
// Parse SVG Tests
// ==============================================================================

TEST (DrawableTests, ParseNonExistentFileReturnsFalse)
{
    Drawable drawable;
    File nonExistentFile ("/path/to/nonexistent/file.svg");

    bool result = drawable.parseSVG (nonExistentFile);

    EXPECT_FALSE (result);
}

TEST (DrawableTests, ParseDirectoryReturnsFalse)
{
    Drawable drawable;
    File directory = File::getCurrentWorkingDirectory();

    bool result = drawable.parseSVG (directory);

    EXPECT_FALSE (result);
}

TEST (DrawableTests, ParseEmptyFileReturnsFalse)
{
    Drawable drawable;

    // Create a temporary empty file
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_empty.svg");
    tempFile.replaceWithText ("");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_FALSE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseInvalidXMLReturnsFalse)
{
    Drawable drawable;

    // Create a temporary file with invalid XML
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_invalid.svg");
    tempFile.replaceWithText ("This is not valid XML");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_FALSE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseNonSVGXMLReturnsFalse)
{
    Drawable drawable;

    // Create a temporary file with valid XML but not SVG
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_non_svg.xml");
    tempFile.replaceWithText ("<root><element>data</element></root>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_FALSE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseMinimalValidSVG)
{
    Drawable drawable;

    // Create a minimal valid SVG
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_minimal.svg");
    tempFile.replaceWithText ("<svg></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithViewBox)
{
    Drawable drawable;

    // Create SVG with viewBox
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_viewbox.svg");
    tempFile.replaceWithText ("<svg viewBox=\"0 0 100 100\"></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_EQ (100.0f, bounds.getWidth());
    EXPECT_EQ (100.0f, bounds.getHeight());

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithWidthHeight)
{
    Drawable drawable;

    // Create SVG with width and height
    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_size.svg");
    tempFile.replaceWithText ("<svg width=\"200\" height=\"150\"></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_EQ (200.0f, bounds.getWidth());
    EXPECT_EQ (150.0f, bounds.getHeight());

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithPathElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_path.svg");
    tempFile.replaceWithText ("<svg><path d=\"M 10 10 L 90 90\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ClearAfterParseResetsDrawable)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_clear_after.svg");
    tempFile.replaceWithText ("<svg viewBox=\"0 0 100 100\"></svg>");

    drawable.parseSVG (tempFile);
    drawable.clear();

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_TRUE (bounds.isEmpty());

    tempFile.deleteFile();
}

// ==============================================================================
// Paint Tests (Basic)
// ==============================================================================

TEST (DrawableTests, PaintEmptyDrawableDoesNotCrash)
{
    Drawable drawable;

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (100, 100);
    Graphics graphics (*context, *renderer);

    // Should not crash
    drawable.paint (graphics);
}

TEST (DrawableTests, PaintWithFittingDoesNotCrash)
{
    Drawable drawable;

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (100, 100);
    Graphics graphics (*context, *renderer);

    Rectangle<float> targetArea (0.0f, 0.0f, 100.0f, 100.0f);

    // Should not crash with empty drawable
    drawable.paint (graphics, targetArea, Fitting::scaleToFit, Justification::center);
}

TEST (DrawableTests, PaintWithVariousFittingModes)
{
    Drawable drawable;

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (100, 100);
    Graphics graphics (*context, *renderer);

    Rectangle<float> targetArea (0.0f, 0.0f, 100.0f, 100.0f);

    Fitting fittingModes[] = {
        Fitting::none,
        Fitting::scaleToFit,
        Fitting::fitWidth,
        Fitting::fitHeight,
        Fitting::scaleToFill,
        Fitting::fill,
        Fitting::centerInside,
        Fitting::centerCrop,
        Fitting::stretchWidth,
        Fitting::stretchHeight,
        Fitting::tile
    };

    for (auto fitting : fittingModes)
    {
        // Should not crash
        drawable.paint (graphics, targetArea, fitting, Justification::center);
    }
}

TEST (DrawableTests, PaintWithVariousJustifications)
{
    Drawable drawable;

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (100, 100);
    Graphics graphics (*context, *renderer);

    Rectangle<float> targetArea (0.0f, 0.0f, 100.0f, 100.0f);

    Justification justifications[] = {
        Justification::topLeft,
        Justification::centerTop,
        Justification::topRight,
        Justification::centerLeft,
        Justification::center,
        Justification::centerRight,
        Justification::bottomLeft,
        Justification::centerBottom,
        Justification::bottomRight
    };

    for (auto justification : justifications)
    {
        // Should not crash
        drawable.paint (graphics, targetArea, Fitting::scaleToFit, justification);
    }
}

TEST (DrawableTests, PaintWithEmptyTargetArea)
{
    Drawable drawable;

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (100, 100);
    Graphics graphics (*context, *renderer);

    Rectangle<float> emptyArea (0.0f, 0.0f, 0.0f, 0.0f);

    // Should not crash or render anything
    drawable.paint (graphics, emptyArea, Fitting::scaleToFit, Justification::center);
}

TEST (DrawableTests, PaintWithNegativeArea)
{
    Drawable drawable;

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (100, 100);
    Graphics graphics (*context, *renderer);

    Rectangle<float> negativeArea (0.0f, 0.0f, -100.0f, -100.0f);

    // Should handle gracefully
    drawable.paint (graphics, negativeArea, Fitting::scaleToFit, Justification::center);
}

// ==============================================================================
// Multiple Parse Tests
// ==============================================================================

TEST (DrawableTests, ParseMultipleTimes)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_multiple.svg");
    tempFile.replaceWithText ("<svg viewBox=\"0 0 100 100\"></svg>");

    bool result1 = drawable.parseSVG (tempFile);
    bool result2 = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result1);
    EXPECT_TRUE (result2);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseDifferentFilesClearsOldContent)
{
    Drawable drawable;

    File tempFile1 = File::getSpecialLocation (File::tempDirectory).getChildFile ("test1.svg");
    tempFile1.replaceWithText ("<svg viewBox=\"0 0 100 100\"></svg>");

    File tempFile2 = File::getSpecialLocation (File::tempDirectory).getChildFile ("test2.svg");
    tempFile2.replaceWithText ("<svg viewBox=\"0 0 200 200\"></svg>");

    drawable.parseSVG (tempFile1);
    drawable.parseSVG (tempFile2);

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_EQ (200.0f, bounds.getWidth());
    EXPECT_EQ (200.0f, bounds.getHeight());

    tempFile1.deleteFile();
    tempFile2.deleteFile();
}

// ==============================================================================
// SVG Element Tests
// ==============================================================================

TEST (DrawableTests, ParseSVGWithRectElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_rect.svg");
    tempFile.replaceWithText ("<svg><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithCircleElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_circle.svg");
    tempFile.replaceWithText ("<svg><circle cx=\"50\" cy=\"50\" r=\"40\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithEllipseElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_ellipse.svg");
    tempFile.replaceWithText ("<svg><ellipse cx=\"50\" cy=\"50\" rx=\"40\" ry=\"30\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithLineElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_line.svg");
    tempFile.replaceWithText ("<svg><line x1=\"0\" y1=\"0\" x2=\"100\" y2=\"100\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithPolygonElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_polygon.svg");
    tempFile.replaceWithText ("<svg><polygon points=\"10,10 90,10 50,90\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithPolylineElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_polyline.svg");
    tempFile.replaceWithText ("<svg><polyline points=\"10,10 50,50 90,10\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithGroupElement)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_group.svg");
    tempFile.replaceWithText ("<svg><g><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" /></g></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithNestedGroups)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_nested.svg");
    tempFile.replaceWithText ("<svg><g><g><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" /></g></g></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

// ==============================================================================
// SVG Style Tests
// ==============================================================================

TEST (DrawableTests, ParseSVGWithFillColor)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_fill.svg");
    tempFile.replaceWithText ("<svg><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" fill=\"red\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithStrokeColor)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_stroke.svg");
    tempFile.replaceWithText ("<svg><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" stroke=\"blue\" stroke-width=\"2\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithOpacity)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_opacity.svg");
    tempFile.replaceWithText ("<svg><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" opacity=\"0.5\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithTransform)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_transform.svg");
    tempFile.replaceWithText ("<svg><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" transform=\"translate(10,20)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithStyleAttribute)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_style.svg");
    tempFile.replaceWithText ("<svg><rect x=\"10\" y=\"10\" width=\"80\" height=\"60\" style=\"fill:red;stroke:blue;stroke-width:2\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

// ==============================================================================
// SVG Transform Tests
// ==============================================================================

TEST (DrawableTests, ParseTransformTranslate)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_translate.svg");
    tempFile.replaceWithText ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"translate(10, 20)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseTransformScale)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_scale.svg");
    tempFile.replaceWithText ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"scale(2)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseTransformRotate)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_rotate.svg");
    tempFile.replaceWithText ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"rotate(45)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseTransformMatrix)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_matrix.svg");
    tempFile.replaceWithText ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"matrix(1, 0, 0, 1, 10, 20)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseMultipleTransforms)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_multi_transform.svg");
    tempFile.replaceWithText ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"translate(10, 20) scale(2) rotate(45)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

// ==============================================================================
// SVG Gradient Tests
// ==============================================================================

TEST (DrawableTests, ParseSVGWithLinearGradient)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_linear_gradient.svg");
    tempFile.replaceWithText (
        "<svg><defs><linearGradient id=\"grad1\">"
        "<stop offset=\"0%\" stop-color=\"red\"/>"
        "<stop offset=\"100%\" stop-color=\"blue\"/>"
        "</linearGradient></defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#grad1)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithRadialGradient)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_radial_gradient.svg");
    tempFile.replaceWithText (
        "<svg><defs><radialGradient id=\"grad1\">"
        "<stop offset=\"0%\" stop-color=\"yellow\"/>"
        "<stop offset=\"100%\" stop-color=\"green\"/>"
        "</radialGradient></defs>"
        "<circle cx=\"50\" cy=\"50\" r=\"40\" fill=\"url(#grad1)\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

// ==============================================================================
// Edge Cases and Error Handling
// ==============================================================================

TEST (DrawableTests, ParseSVGWithInvalidPath)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_invalid_path.svg");
    tempFile.replaceWithText ("<svg><path d=\"INVALID PATH DATA\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    // Path::fromString always returns true, so parsing succeeds even with invalid data
    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithEmptyPath)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_empty_path.svg");
    tempFile.replaceWithText ("<svg><path d=\"\" /></svg>");

    bool result = drawable.parseSVG (tempFile);

    // Path::fromString always returns true, so parsing succeeds even with empty path
    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithMalformedViewBox)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_malformed_viewbox.svg");
    tempFile.replaceWithText ("<svg viewBox=\"invalid data\"></svg>");

    bool result = drawable.parseSVG (tempFile);

    // Should still parse the SVG element
    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithPartialViewBox)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_partial_viewbox.svg");
    tempFile.replaceWithText ("<svg viewBox=\"0 0\"></svg>"); // Only 2 values instead of 4

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithNegativeDimensions)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_negative_dims.svg");
    tempFile.replaceWithText ("<svg width=\"-100\" height=\"-100\"></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

TEST (DrawableTests, ParseSVGWithZeroDimensions)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_zero_dims.svg");
    tempFile.replaceWithText ("<svg width=\"0\" height=\"0\"></svg>");

    bool result = drawable.parseSVG (tempFile);

    EXPECT_TRUE (result);

    tempFile.deleteFile();
}

// ==============================================================================
// Bounds Calculation Tests
// ==============================================================================

TEST (DrawableTests, GetBoundsAfterClear)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_bounds_clear.svg");
    tempFile.replaceWithText ("<svg viewBox=\"0 0 100 100\"></svg>");

    drawable.parseSVG (tempFile);
    drawable.clear();

    Rectangle<float> bounds = drawable.getBounds();
    EXPECT_TRUE (bounds.isEmpty());

    tempFile.deleteFile();
}

TEST (DrawableTests, GetBoundsWithViewBoxTakesPrecedence)
{
    Drawable drawable;

    File tempFile = File::getSpecialLocation (File::tempDirectory).getChildFile ("test_bounds_viewbox.svg");
    tempFile.replaceWithText ("<svg viewBox=\"0 0 100 100\" width=\"200\" height=\"200\"></svg>");

    drawable.parseSVG (tempFile);

    Rectangle<float> bounds = drawable.getBounds();
    // ViewBox should take precedence
    EXPECT_EQ (100.0f, bounds.getWidth());
    EXPECT_EQ (100.0f, bounds.getHeight());

    tempFile.deleteFile();
}
