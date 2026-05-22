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

TEST (DrawableTests, ParseSVGFromString)
{
    Drawable drawable;

    bool result = drawable.parseSVG ("<svg viewBox=\"0 0 20 10\"><rect width=\"20\" height=\"10\" /></svg>");

    EXPECT_TRUE (result);
    EXPECT_EQ (20.0f, drawable.getBounds().getWidth());
    EXPECT_EQ (10.0f, drawable.getBounds().getHeight());

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (32, 32);
    Graphics graphics (*context, *renderer);

    EXPECT_NO_THROW ({
        drawable.paint (graphics, Rectangle<float> (0.0f, 0.0f, 32.0f, 32.0f));
    });
}

TEST (DrawableTests, ParseSVGFromStringWithCSSCascadeAndCurrentColor)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>"
        "rect.base { color: #ff0000; fill: currentColor; stroke: #0000ff; stroke-width: 2; }"
        "#hidden { display: none; }"
        "</style>"
        "<rect class=\"base\" x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "<circle id=\"hidden\" cx=\"50\" cy=\"50\" r=\"10\" />"
        "</svg>");

    EXPECT_TRUE (result);
}

TEST (DrawableTests, ParseSVGWithParseOptionsImageResolver)
{
    Drawable drawable;

    Drawable::ParseOptions options;
    options.imageResolver = [] (StringRef href, const File&) -> std::optional<Image>
    {
        if (href == StringRef ("custom-image"))
        {
            Image image (2, 2, PixelFormat::RGBA);
            image.fill (0xffff0000);
            return image;
        }

        return std::nullopt;
    };

    bool result = drawable.parseSVG (
        "<svg viewBox=\"0 0 10 10\">"
        "<image href=\"custom-image\" x=\"1\" y=\"1\" width=\"8\" height=\"8\" />"
        "</svg>",
        options);

    EXPECT_TRUE (result);
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

TEST (DrawableTests, ParseSVGWithSymbolAndUse)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<symbol id=\"icon\" viewBox=\"0 0 10 10\">"
        "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" />"
        "</symbol>"
        "</defs>"
        "<use href=\"#icon\" x=\"20\" y=\"30\" width=\"40\" height=\"40\" />"
        "</svg>");

    EXPECT_TRUE (result);
}

TEST (DrawableTests, ParseSVGWithDefsXLinkUseAndTspanFlow)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"80\" height=\"40\">"
        "<defs>"
        "<rect id=\"bar\" width=\"40\" height=\"10\" fill=\"#88a4b9\" />"
        "<line id=\"tick\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"8\" stroke=\"black\" stroke-width=\"1\" />"
        "</defs>"
        "<use xlink:href=\"#bar\" x=\"10\" y=\"5\" />"
        "<use xlink:href=\"#tick\" transform=\"translate(12, 2)\" />"
        "<text transform=\"translate(10, 20)\">"
        "<tspan x=\"0\" dy=\"1em\">first</tspan>"
        "<tspan x=\"0\" dy=\"1em\">second</tspan>"
        "</text>"
        "</svg>");

    EXPECT_TRUE (result);

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (80, 40);
    Graphics graphics (*context, *renderer);

    EXPECT_NO_THROW ({
        drawable.paint (graphics, Rectangle<float> (0.0f, 0.0f, 80.0f, 40.0f));
    });
}

TEST (DrawableTests, ParseSVGWithUnitsNestedViewportTextAndDash)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg width=\"2in\" height=\"1in\" viewBox=\"0 0 192 96\">"
        "<svg x=\"10\" y=\"10\" width=\"50%\" height=\"50%\" viewBox=\"0 0 20 20\" preserveAspectRatio=\"xMidYMid meet\">"
        "<path d=\"M 0 10 L 20 10\" fill=\"none\" stroke=\"black\" stroke-width=\"1mm\" stroke-dasharray=\"2 1\" />"
        "<text x=\"10\" y=\"16\" text-anchor=\"middle\" font-size=\"12\">OK<tspan dx=\"2\">!</tspan></text>"
        "</svg>"
        "</svg>");

    EXPECT_TRUE (result);
    EXPECT_EQ (192.0f, drawable.getBounds().getWidth());
    EXPECT_EQ (96.0f, drawable.getBounds().getHeight());
}

TEST (DrawableTests, ParseSVGWithTransformOrigin)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg width=\"360\" height=\"360\">"
        "<text x=\"-15\" y=\"195\" font-size=\"20\" fill=\"black\" transform=\"rotate(-90)\" transform-origin=\"20 195\">"
        "Sweep flag"
        "</text>"
        "</svg>");

    EXPECT_TRUE (result);

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (360, 360);
    Graphics graphics (*context, *renderer);

    EXPECT_NO_THROW ({
        drawable.paint (graphics, Rectangle<float> (0.0f, 0.0f, 360.0f, 360.0f));
    });
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

TEST (DrawableTests, PaintSVGWithTransformedClipPath)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<clipPath id=\"clip\"><path d=\"M 10 10 L 80 10 L 80 80 L 10 80 z\" /></clipPath>"
        "</defs>"
        "<path d=\"M 10 10 L 80 10 L 80 80 L 10 80 z\" transform=\"translate(5, 0)\" clip-path=\"url(#clip)\" fill=\"#666666\" />"
        "</svg>");

    EXPECT_TRUE (result);

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (64, 64);
    Graphics graphics (*context, *renderer);

    EXPECT_NO_THROW ({
        drawable.paint (graphics, Rectangle<float> (0.0f, 0.0f, 64.0f, 64.0f));
    });
}

TEST (DrawableTests, PaintSVGWithScimitarClipPathAndGradientStroke)
{
    Drawable drawable;

    const String scimitarPath = "M 171.59375,-167.8125 L 153.4375,-131.09375 C 153.4375,-131.09375 240.05975,-44.592207 260.53125,61.53125 "
                                "C 263.78902,59.713413 267.53809,58.6875 271.53125,58.6875 C 283.99674,58.687502 294.11733,68.78455 294.15625,81.25 "
                                "L 294.15625,81.3125 C 294.15624,93.802829 284.02158,103.9375 271.53125,103.9375 "
                                "C 269.20004,103.9375 266.9604,103.59314 264.84375,102.9375 C 265.00283,118.53432 263.43644,134.33614 259.71875,150.1875 "
                                "C 279.93177,155.71176 336.35552,161.63753 367.0625,234.84375 C 388.95186,159.67792 354.15709,-29.134107 171.59375,-167.8125 z ";

    String svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"466.11172\" height=\"265.35126\">"
        << "<defs>"
        << "<linearGradient id=\"linearGradient6110\">"
        << "<stop offset=\"0\" stop-color=\"#ffffff\" />"
        << "<stop offset=\"1\" stop-color=\"#6e6e6e\" />"
        << "</linearGradient>"
        << "<linearGradient id=\"linearGradient4429\" xlink:href=\"#linearGradient6110\" gradientUnits=\"userSpaceOnUse\" spreadMethod=\"reflect\" "
        << "x1=\"365.06906\" y1=\"318.85867\" x2=\"375.43167\" y2=\"352.76584\" "
        << "gradientTransform=\"matrix(0.9061819,1.3321141,-1.3321141,0.9061819,401.82647,-748.87542)\" />"
        << "<clipPath id=\"clipPath6467\"><path d=\"" << scimitarPath << "\" /></clipPath>"
        << "</defs>"
        << "<g transform=\"matrix(0.8057349,-1.0705499,1.0705499,0.8057349,-463.13727,65.232307)\">"
        << "<g transform=\"matrix(0.7664195,0,0,0.7664195,-4.7078914,247.90097)\">"
        << "<path d=\"" << scimitarPath << "\" "
        << "style=\"fill:#c3c3c3;fill-opacity:1;fill-rule:nonzero;stroke:url(#linearGradient4429);stroke-width:5.31668139;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1\" "
        << "clip-path=\"url(#clipPath6467)\" />"
        << "</g>"
        << "</g>"
        << "</svg>";

    bool result = drawable.parseSVG (svg);

    EXPECT_TRUE (result);

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (96, 96);
    Graphics graphics (*context, *renderer);
    graphics.setDrawingArea (Rectangle<float> (23.0f, 17.0f, 96.0f, 96.0f));

    EXPECT_NO_THROW ({
        drawable.paint (graphics, Rectangle<float> (0.0f, 0.0f, 96.0f, 96.0f));
    });
}

TEST (DrawableTests, PaintSVGWithTransformedRadialGradient)
{
    Drawable drawable;

    bool result = drawable.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<radialGradient id=\"grad\" cx=\"50\" cy=\"50\" r=\"10\" gradientUnits=\"userSpaceOnUse\" gradientTransform=\"matrix(1,0,0,3,0,-100)\">"
        "<stop offset=\"0\" stop-color=\"#ffffff\" />"
        "<stop offset=\"1\" stop-color=\"#000000\" />"
        "</radialGradient>"
        "</defs>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"url(#grad)\" />"
        "</svg>");

    EXPECT_TRUE (result);

    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    auto renderer = context->makeRenderer (64, 64);
    Graphics graphics (*context, *renderer);

    EXPECT_NO_THROW ({
        drawable.paint (graphics, Rectangle<float> (0.0f, 0.0f, 64.0f, 64.0f));
    });
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

    EXPECT_FALSE (result);

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
