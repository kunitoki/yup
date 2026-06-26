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

auto makeHeadlessGraphics (int w = 64, int h = 64)
{
    struct HeadlessGfx
    {
        std::unique_ptr<GraphicsContext> ctx;
        std::unique_ptr<rive::Renderer> renderer;
        std::unique_ptr<Graphics> graphics;
    };

    HeadlessGfx g;
    g.ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    g.renderer = g.ctx->makeRenderer (w, h);
    g.graphics = std::make_unique<Graphics> (*g.ctx, *g.renderer);
    return g;
}

} // namespace

// ==============================================================================
// Root SVG element parsing
// ==============================================================================

TEST (SVGParserTests, ParseMinimalSVGTag)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg></svg>"));
}

TEST (SVGParserTests, ParseSVGWithXmlDeclaration)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<?xml version=\"1.0\" encoding=\"UTF-8\"?><svg viewBox=\"0 0 10 10\"></svg>"));
    EXPECT_FLOAT_EQ (10.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (10.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGWithNamespace)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 20 30\"></svg>"));
    EXPECT_FLOAT_EQ (20.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (30.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGWithXlinkNamespace)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 40 40\"></svg>"));
}

TEST (SVGParserTests, ParseSVGWithWidthAndHeight)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg width=\"120\" height=\"80\"></svg>"));
    EXPECT_FLOAT_EQ (120.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (80.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGViewBoxTakesPrecedenceOverWidthHeight)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg viewBox=\"0 0 50 25\" width=\"200\" height=\"100\"></svg>"));
    EXPECT_FLOAT_EQ (50.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (25.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGWithViewBoxNonZeroOrigin)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg viewBox=\"10 20 100 80\"></svg>"));
    EXPECT_FLOAT_EQ (100.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (80.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGReturnsFalseForNonSVGRoot)
{
    Drawable d;
    EXPECT_FALSE (d.parseSVG ("<html><body></body></html>"));
}

TEST (SVGParserTests, ParseSVGReturnsFalseForInvalidXML)
{
    Drawable d;
    EXPECT_FALSE (d.parseSVG ("not xml at all"));
}

TEST (SVGParserTests, ParseSVGReturnsFalseForEmptyString)
{
    Drawable d;
    EXPECT_FALSE (d.parseSVG (""));
}

TEST (SVGParserTests, ParseSVGReturnsFalseForPartialXML)
{
    Drawable d;
    EXPECT_FALSE (d.parseSVG ("<svg viewBox=\"0 0 100 100\">"));
}

// ==============================================================================
// Path element — command coverage
// ==============================================================================

TEST (SVGParserTests, ParsePathWithMoveToLineTo)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 20 L 80 90\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithRelativeMoveToLineTo)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"m 10 20 l 70 70\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithHorizontalAndVerticalLines)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 50 H 90 V 10 H 10 V 50\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithRelativeHorizontalVertical)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 50 h 80 v -40 h -80 v 40\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithCubicBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 C 40 10 65 10 95 80\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithRelativeCubicBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 c 30 -70 55 -70 85 0\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithSmoothCubicBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 C 40 10 65 10 95 80 S 150 150 180 80\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithRelativeSmoothCubicBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 c 30 -70 55 -70 85 0 s 55 70 85 0\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithQuadraticBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 Q 95 10 180 80\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithRelativeQuadraticBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 q 85 -70 170 0\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithSmoothQuadraticBezier)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 80 Q 95 10 180 80 T 350 80\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithArcAbsolute)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 50 A 40 40 0 1 0 90 50\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithArcRelative)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 50 a 40 40 0 1 0 80 0\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithArcSweepFlags)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg>"
                             "<path d=\"M 10 50 A 40 40 0 0 0 90 50\" />"
                             "<path d=\"M 10 50 A 40 40 0 0 1 90 50\" />"
                             "<path d=\"M 10 50 A 40 40 0 1 0 90 50\" />"
                             "<path d=\"M 10 50 A 40 40 0 1 1 90 50\" />"
                             "</svg>"));
}

TEST (SVGParserTests, ParsePathWithClosePath)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 10 L 90 10 L 90 90 L 10 90 Z\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithLowercaseClose)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 10 L 90 10 L 90 90 L 10 90 z\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithMultipleSubpaths)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 10 L 50 10 Z M 60 10 L 90 10 Z\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithImplicitLineTo)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 10 10 20 20 30 10\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithCompactNotation)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M10,10L50,50L90,10Z\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithInvalidStartCommandStillSucceeds)
{
    Drawable d;
    // Child element failures don't abort the document parse — the bad path is
    // silently skipped so the document still reports success.
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"X 10 10\" /></svg>"));
}

TEST (SVGParserTests, ParsePathReturnsTrueForEmptyD)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithEvenOddFillRule)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 50 10 L 61 40 L 98 40 L 68 60 L 79 90 L 50 70 L 21 90 L 32 60 L 2 40 L 39 40 Z\" fill-rule=\"evenodd\" /></svg>"));
}

TEST (SVGParserTests, ParsePathWithNonZeroFillRule)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><path d=\"M 50 10 L 61 40 L 98 40 L 68 60 Z\" fill-rule=\"nonzero\" /></svg>"));
}

// ==============================================================================
// Basic shapes
// ==============================================================================

TEST (SVGParserTests, ParseRectBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"10\" y=\"20\" width=\"80\" height=\"60\" /></svg>"));
}

TEST (SVGParserTests, ParseRectWithRoundedCorners)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"5\" y=\"5\" width=\"90\" height=\"90\" rx=\"10\" ry=\"10\" /></svg>"));
}

TEST (SVGParserTests, ParseRectWithOnlyRx)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"5\" y=\"5\" width=\"90\" height=\"90\" rx=\"10\" /></svg>"));
}

TEST (SVGParserTests, ParseRectWithOnlyRy)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"5\" y=\"5\" width=\"90\" height=\"90\" ry=\"10\" /></svg>"));
}

TEST (SVGParserTests, ParseRectWithZeroDimensions)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"0\" height=\"0\" /></svg>"));
}

TEST (SVGParserTests, ParseCircleBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><circle cx=\"50\" cy=\"50\" r=\"40\" /></svg>"));
}

TEST (SVGParserTests, ParseCircleAtOrigin)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><circle r=\"20\" /></svg>"));
}

TEST (SVGParserTests, ParseCircleWithZeroRadius)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><circle cx=\"50\" cy=\"50\" r=\"0\" /></svg>"));
}

TEST (SVGParserTests, ParseEllipseBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><ellipse cx=\"50\" cy=\"50\" rx=\"40\" ry=\"25\" /></svg>"));
}

TEST (SVGParserTests, ParseEllipseAtOrigin)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><ellipse rx=\"30\" ry=\"20\" /></svg>"));
}

TEST (SVGParserTests, ParseLineBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><line x1=\"10\" y1=\"10\" x2=\"90\" y2=\"90\" /></svg>"));
}

TEST (SVGParserTests, ParseLineHorizontal)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><line x1=\"10\" y1=\"50\" x2=\"90\" y2=\"50\" /></svg>"));
}

TEST (SVGParserTests, ParseLineVertical)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><line x1=\"50\" y1=\"10\" x2=\"50\" y2=\"90\" /></svg>"));
}

TEST (SVGParserTests, ParseLineSamePoint)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><line x1=\"50\" y1=\"50\" x2=\"50\" y2=\"50\" /></svg>"));
}

TEST (SVGParserTests, ParsePolygonTriangle)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><polygon points=\"50,10 90,90 10,90\" /></svg>"));
}

TEST (SVGParserTests, ParsePolygonSquare)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><polygon points=\"10 10 90 10 90 90 10 90\" /></svg>"));
}

TEST (SVGParserTests, ParsePolygonWithCommasAndSpaces)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><polygon points=\"10,10, 90,10, 90,90, 10,90\" /></svg>"));
}

TEST (SVGParserTests, ParsePolygonWithEmptyPoints)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><polygon points=\"\" /></svg>"));
}

TEST (SVGParserTests, ParsePolylineBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><polyline points=\"10,10 50,50 90,10\" /></svg>"));
}

TEST (SVGParserTests, ParsePolylineOpenPath)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><polyline points=\"10,50 30,10 50,50 70,10 90,50\" /></svg>"));
}

// ==============================================================================
// Groups and structural elements
// ==============================================================================

TEST (SVGParserTests, ParseGroupEmpty)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><g></g></svg>"));
}

TEST (SVGParserTests, ParseGroupWithSingleChild)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><g><rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" /></g></svg>"));
}

TEST (SVGParserTests, ParseGroupWithMultipleChildren)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><g>"
                             "<rect x=\"10\" y=\"10\" width=\"30\" height=\"30\" />"
                             "<circle cx=\"70\" cy=\"30\" r=\"20\" />"
                             "<line x1=\"10\" y1=\"80\" x2=\"90\" y2=\"80\" />"
                             "</g></svg>"));
}

TEST (SVGParserTests, ParseDeepNestedGroups)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><g><g><g><g>"
                             "<rect x=\"10\" y=\"10\" width=\"10\" height=\"10\" />"
                             "</g></g></g></g></svg>"));
}

TEST (SVGParserTests, ParseGroupWithId)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><g id=\"layer1\"><rect x=\"5\" y=\"5\" width=\"90\" height=\"90\" /></g></svg>"));
}

TEST (SVGParserTests, ParseGroupWithClass)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><g class=\"myGroup\"><rect x=\"5\" y=\"5\" width=\"90\" height=\"90\" /></g></svg>"));
}

TEST (SVGParserTests, ParseDefsEmpty)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><defs></defs></svg>"));
}

TEST (SVGParserTests, ParseDefsWithMultipleDefinitions)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"g1\"><stop offset=\"0\" stop-color=\"red\" /><stop offset=\"1\" stop-color=\"blue\" /></linearGradient>"
        "<radialGradient id=\"g2\"><stop offset=\"0\" stop-color=\"green\" /><stop offset=\"1\" stop-color=\"yellow\" /></radialGradient>"
        "<clipPath id=\"c1\"><rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" /></clipPath>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#g1)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseSymbolHiddenByDefault)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<symbol id=\"sym\" viewBox=\"0 0 10 10\">"
        "<circle cx=\"5\" cy=\"5\" r=\"5\" fill=\"red\" />"
        "</symbol>"
        "</svg>"));
}

TEST (SVGParserTests, ParseUseWithHref)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs><rect id=\"r\" x=\"0\" y=\"0\" width=\"20\" height=\"20\" /></defs>"
        "<use href=\"#r\" x=\"10\" y=\"10\" />"
        "<use href=\"#r\" x=\"50\" y=\"50\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseUseWithXlinkHref)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 100 100\">"
        "<defs><circle id=\"c\" cx=\"10\" cy=\"10\" r=\"10\" /></defs>"
        "<use xlink:href=\"#c\" x=\"30\" y=\"30\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseUseWithMissingReference)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<use href=\"#doesnotexist\" x=\"10\" y=\"10\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseUseWithViewportSize)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<symbol id=\"icon\" viewBox=\"0 0 10 10\">"
        "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" />"
        "</symbol>"
        "</defs>"
        "<use href=\"#icon\" x=\"10\" y=\"10\" width=\"40\" height=\"40\" />"
        "<use href=\"#icon\" x=\"60\" y=\"60\" width=\"30\" height=\"30\" />"
        "</svg>"));
}

// ==============================================================================
// Text elements
// ==============================================================================

TEST (SVGParserTests, ParseTextBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><text x=\"10\" y=\"50\">Hello World</text></svg>"));
}

TEST (SVGParserTests, ParseTextWithFontAttributes)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg>"
        "<text x=\"10\" y=\"50\" font-family=\"Arial\" font-size=\"16\" font-weight=\"bold\">Bold Text</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseTextWithTextAnchor)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg>"
        "<text x=\"50\" y=\"50\" text-anchor=\"middle\">Centered</text>"
        "<text x=\"50\" y=\"70\" text-anchor=\"start\">Left</text>"
        "<text x=\"50\" y=\"90\" text-anchor=\"end\">Right</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseTextWithTspanChildren)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg>"
        "<text x=\"10\" y=\"20\">"
        "<tspan x=\"10\" dy=\"0\">Line one</tspan>"
        "<tspan x=\"10\" dy=\"1.2em\">Line two</tspan>"
        "<tspan x=\"10\" dy=\"1.2em\">Line three</tspan>"
        "</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseTspanWithDxDy)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg>"
        "<text x=\"10\" y=\"50\">"
        "before"
        "<tspan dx=\"5\" dy=\"-10\">lifted</tspan>"
        "after"
        "</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseTextWithMultiplePositions)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg>"
        "<text x=\"10 30 50 70\" y=\"50\">A B C D</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseTextWithTransform)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg>"
        "<text x=\"0\" y=\"0\" transform=\"rotate(-45) translate(40 40)\">Rotated</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseTextEmpty)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><text x=\"10\" y=\"50\"></text></svg>"));
}

// ==============================================================================
// Image elements
// ==============================================================================

TEST (SVGParserTests, ParseImageWithDataUri)
{
    Drawable d;
    // 1x1 red PNG as data URI
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<image x=\"10\" y=\"10\" width=\"80\" height=\"80\" "
        "href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI6QAAAABJRU5ErkJggg==\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseImageWithCustomResolver)
{
    Drawable d;

    Drawable::ParseOptions opts;
    opts.imageResolver = [] (StringRef href, const File&) -> std::optional<Image>
    {
        if (String (href.text) == "test-icon")
        {
            Image img (4, 4, PixelFormat::RGBA);
            img.fill (0xff0000ff);
            return img;
        }
        return std::nullopt;
    };

    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<image x=\"10\" y=\"10\" width=\"80\" height=\"80\" href=\"test-icon\" />"
        "</svg>",
        opts));
}

TEST (SVGParserTests, ParseImageWithXlinkHref)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 100 100\">"
        "<image x=\"0\" y=\"0\" width=\"100\" height=\"100\" "
        "xlink:href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI6QAAAABJRU5ErkJggg==\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseImageWithMissingHref)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><image x=\"0\" y=\"0\" width=\"50\" height=\"50\" /></svg>"));
}

TEST (SVGParserTests, ParseImageDefaultsToPreservingAspectRatio)
{
    auto doc = SVGParser::parse ("<svg><image x=\"0\" y=\"0\" width=\"50\" height=\"25\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());

        const auto& image = *data.elements.front();
        EXPECT_EQ (Fitting::scaleToFit, image.preserveAspectRatioFitting);
        EXPECT_TRUE (image.preserveAspectRatioJustification.testFlags (Justification::center));
    });
}

TEST (SVGParserTests, ParseImagePreserveAspectRatioAttribute)
{
    auto doc = SVGParser::parse ("<svg><image x=\"0\" y=\"0\" width=\"50\" height=\"25\" preserveAspectRatio=\"xMaxYMin slice\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());

        const auto& image = *data.elements.front();
        EXPECT_EQ (Fitting::scaleToFill, image.preserveAspectRatioFitting);
        EXPECT_TRUE (image.preserveAspectRatioJustification.testFlags (Justification::right));
        EXPECT_TRUE (image.preserveAspectRatioJustification.testFlags (Justification::top));
    });
}

// ==============================================================================
// Gradients
// ==============================================================================

TEST (SVGParserTests, ParseLinearGradientVertical)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"vg\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\">"
        "<stop offset=\"0\" stop-color=\"white\" />"
        "<stop offset=\"1\" stop-color=\"black\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#vg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseLinearGradientHorizontal)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"hg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">"
        "<stop offset=\"0\" stop-color=\"#ff0000\" />"
        "<stop offset=\"0.5\" stop-color=\"#00ff00\" />"
        "<stop offset=\"1\" stop-color=\"#0000ff\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#hg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseLinearGradientUserSpaceOnUse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"ug\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" y1=\"0\" x2=\"100\" y2=\"0\">"
        "<stop offset=\"0\" stop-color=\"red\" />"
        "<stop offset=\"1\" stop-color=\"blue\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#ug)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseLinearGradientWithGradientTransform)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"tg\" gradientTransform=\"rotate(45)\">"
        "<stop offset=\"0\" stop-color=\"yellow\" />"
        "<stop offset=\"1\" stop-color=\"orange\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#tg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseLinearGradientHrefInheritance)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"base\">"
        "<stop offset=\"0\" stop-color=\"red\" />"
        "<stop offset=\"1\" stop-color=\"blue\" />"
        "</linearGradient>"
        "<linearGradient id=\"derived\" href=\"#base\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\" />"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#derived)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseLinearGradientWithStopOpacity)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"og\">"
        "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"1\" />"
        "<stop offset=\"1\" stop-color=\"blue\" stop-opacity=\"0\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#og)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseRadialGradientBasic)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<radialGradient id=\"rg\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\">"
        "<stop offset=\"0\" stop-color=\"white\" />"
        "<stop offset=\"1\" stop-color=\"black\" />"
        "</radialGradient>"
        "</defs>"
        "<circle cx=\"50\" cy=\"50\" r=\"50\" fill=\"url(#rg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseRadialGradientWithFocalPoint)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<radialGradient id=\"fg\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\" fx=\"0.3\" fy=\"0.3\">"
        "<stop offset=\"0\" stop-color=\"yellow\" />"
        "<stop offset=\"1\" stop-color=\"red\" />"
        "</radialGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#fg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseRadialGradientUserSpaceOnUse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<radialGradient id=\"urg\" gradientUnits=\"userSpaceOnUse\" cx=\"50\" cy=\"50\" r=\"40\">"
        "<stop offset=\"0\" stop-color=\"white\" />"
        "<stop offset=\"1\" stop-color=\"gray\" />"
        "</radialGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"100\" fill=\"url(#urg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseGradientSpreadMethodReflect)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 100\">"
        "<defs>"
        "<linearGradient id=\"sg\" x1=\"0\" y1=\"0\" x2=\"0.25\" y2=\"0\" spreadMethod=\"reflect\">"
        "<stop offset=\"0\" stop-color=\"red\" />"
        "<stop offset=\"1\" stop-color=\"blue\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"200\" height=\"100\" fill=\"url(#sg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseGradientSpreadMethodRepeat)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 100\">"
        "<defs>"
        "<linearGradient id=\"rsg\" x1=\"0\" y1=\"0\" x2=\"0.25\" y2=\"0\" spreadMethod=\"repeat\">"
        "<stop offset=\"0\" stop-color=\"green\" />"
        "<stop offset=\"1\" stop-color=\"yellow\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"200\" height=\"100\" fill=\"url(#rsg)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseGradientUsedOnStroke)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"sg\">"
        "<stop offset=\"0\" stop-color=\"red\" />"
        "<stop offset=\"1\" stop-color=\"blue\" />"
        "</linearGradient>"
        "</defs>"
        "<path d=\"M 10 50 L 90 50\" stroke=\"url(#sg)\" stroke-width=\"10\" fill=\"none\" />"
        "</svg>"));
}

// ==============================================================================
// ClipPath
// ==============================================================================

TEST (SVGParserTests, ParseClipPathWithRect)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<clipPath id=\"rc\">"
        "<rect x=\"20\" y=\"20\" width=\"60\" height=\"60\" />"
        "</clipPath>"
        "</defs>"
        "<circle cx=\"50\" cy=\"50\" r=\"50\" clip-path=\"url(#rc)\" fill=\"red\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseClipPathWithCircle)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<clipPath id=\"cc\">"
        "<circle cx=\"50\" cy=\"50\" r=\"40\" />"
        "</clipPath>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" clip-path=\"url(#cc)\" fill=\"blue\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseClipPathWithPath)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<clipPath id=\"pc\">"
        "<path d=\"M 50 10 L 90 90 L 10 90 Z\" />"
        "</clipPath>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" clip-path=\"url(#pc)\" fill=\"green\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseClipPathWithEvenOddClipRule)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<clipPath id=\"eoc\">"
        "<path d=\"M 50 10 L 61 40 L 98 40 L 68 60 L 79 90 L 50 70 L 21 90 L 32 60 L 2 40 L 39 40 Z\" clip-rule=\"evenodd\" />"
        "</clipPath>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"gold\" clip-path=\"url(#eoc)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseClipPathNestedUse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<rect id=\"cr\" x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "<clipPath id=\"uc\">"
        "<use href=\"#cr\" />"
        "</clipPath>"
        "</defs>"
        "<circle cx=\"50\" cy=\"50\" r=\"50\" clip-path=\"url(#uc)\" fill=\"purple\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseClipPathWithMissingId)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" clip-path=\"url(#nonexistent)\" fill=\"red\" />"
        "</svg>"));
}

// ==============================================================================
// Mask
// ==============================================================================

TEST (SVGParserTests, ParseMaskWithWhiteFill)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<mask id=\"wm\">"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"white\" />"
        "</mask>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"red\" mask=\"url(#wm)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMaskWithGradientAlpha)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"mg\">"
        "<stop offset=\"0\" stop-color=\"white\" stop-opacity=\"1\" />"
        "<stop offset=\"1\" stop-color=\"white\" stop-opacity=\"0\" />"
        "</linearGradient>"
        "<mask id=\"gm\">"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#mg)\" />"
        "</mask>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"blue\" mask=\"url(#gm)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMaskUserSpaceOnUse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<mask id=\"um\" maskUnits=\"userSpaceOnUse\">"
        "<circle cx=\"50\" cy=\"50\" r=\"40\" fill=\"white\" />"
        "</mask>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"green\" mask=\"url(#um)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMaskObjectBoundingBox)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<mask id=\"obm\" maskUnits=\"objectBoundingBox\">"
        "<rect x=\"0.1\" y=\"0.1\" width=\"0.8\" height=\"0.8\" fill=\"white\" />"
        "</mask>"
        "</defs>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"blue\" mask=\"url(#obm)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMaskAppliedToGroup)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<mask id=\"gm2\">"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"50\" fill=\"white\" />"
        "</mask>"
        "</defs>"
        "<g mask=\"url(#gm2)\">"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"red\" />"
        "<circle cx=\"50\" cy=\"50\" r=\"30\" fill=\"blue\" />"
        "</g>"
        "</svg>"));
}

// ==============================================================================
// Markers
// ==============================================================================

TEST (SVGParserTests, ParseMarkerOrientAuto)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"ma\" markerWidth=\"8\" markerHeight=\"8\" refX=\"4\" refY=\"4\" orient=\"auto\">"
        "<circle cx=\"4\" cy=\"4\" r=\"3\" fill=\"red\" />"
        "</marker>"
        "</defs>"
        "<path d=\"M 10 50 L 50 20 L 90 50\" stroke=\"black\" stroke-width=\"2\""
        " marker-start=\"url(#ma)\" marker-mid=\"url(#ma)\" marker-end=\"url(#ma)\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkerOrientFixed)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"mf\" markerWidth=\"6\" markerHeight=\"6\" refX=\"3\" refY=\"3\" orient=\"45\">"
        "<polygon points=\"0,0 6,3 0,6\" fill=\"blue\" />"
        "</marker>"
        "</defs>"
        "<path d=\"M 10 50 L 90 50\" stroke=\"black\" stroke-width=\"2\" marker-end=\"url(#mf)\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkerOrientAutoStartReverse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"mar\" markerWidth=\"10\" markerHeight=\"7\" refX=\"10\" refY=\"3.5\" orient=\"auto-start-reverse\">"
        "<polygon points=\"0 0, 10 3.5, 0 7\" fill=\"black\" />"
        "</marker>"
        "</defs>"
        "<path d=\"M 10 50 L 90 50\" stroke=\"black\" stroke-width=\"2\""
        " marker-start=\"url(#mar)\" marker-end=\"url(#mar)\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkerStrokeWidthUnits)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"msw\" markerUnits=\"strokeWidth\" markerWidth=\"5\" markerHeight=\"5\" refX=\"2.5\" refY=\"2.5\" orient=\"auto\">"
        "<circle cx=\"2.5\" cy=\"2.5\" r=\"2\" fill=\"green\" />"
        "</marker>"
        "</defs>"
        "<line x1=\"10\" y1=\"50\" x2=\"90\" y2=\"50\" stroke=\"black\" stroke-width=\"4\" marker-end=\"url(#msw)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkerUserSpaceOnUse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"mus\" markerUnits=\"userSpaceOnUse\" markerWidth=\"15\" markerHeight=\"15\" refX=\"7.5\" refY=\"7.5\" orient=\"auto\">"
        "<rect x=\"0\" y=\"0\" width=\"15\" height=\"15\" fill=\"none\" stroke=\"red\" />"
        "</marker>"
        "</defs>"
        "<line x1=\"10\" y1=\"50\" x2=\"90\" y2=\"50\" stroke=\"black\" stroke-width=\"2\" marker-end=\"url(#mus)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkerWithViewBox)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"mvb\" viewBox=\"0 0 10 10\" markerWidth=\"4\" markerHeight=\"4\" refX=\"5\" refY=\"5\" orient=\"auto\">"
        "<circle cx=\"5\" cy=\"5\" r=\"4\" fill=\"purple\" />"
        "</marker>"
        "</defs>"
        "<path d=\"M 10 50 L 50 10 L 90 50\" stroke=\"black\" stroke-width=\"2\" marker-end=\"url(#mvb)\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkerShorthand)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"msh\" markerWidth=\"6\" markerHeight=\"6\" refX=\"3\" refY=\"3\" orient=\"auto\">"
        "<circle cx=\"3\" cy=\"3\" r=\"2\" fill=\"orange\" />"
        "</marker>"
        "</defs>"
        "<polyline points=\"10,50 50,20 90,50\" stroke=\"black\" stroke-width=\"2\" marker=\"url(#msh)\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseMarkersOnPolyline)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"dot\" markerWidth=\"4\" markerHeight=\"4\" refX=\"2\" refY=\"2\">"
        "<circle cx=\"2\" cy=\"2\" r=\"2\" fill=\"red\" />"
        "</marker>"
        "</defs>"
        "<polyline points=\"10,50 30,20 50,50 70,20 90,50\""
        " stroke=\"black\" stroke-width=\"1\""
        " marker-start=\"url(#dot)\" marker-mid=\"url(#dot)\" marker-end=\"url(#dot)\" fill=\"none\" />"
        "</svg>"));
}

// ==============================================================================
// Patterns
// ==============================================================================

TEST (SVGParserTests, ParsePatternUserSpaceOnUse)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"pu\" patternUnits=\"userSpaceOnUse\" x=\"0\" y=\"0\" width=\"20\" height=\"20\">"
        "<circle cx=\"10\" cy=\"10\" r=\"8\" fill=\"blue\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#pu)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParsePatternObjectBoundingBox)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"po\" patternUnits=\"objectBoundingBox\" x=\"0\" y=\"0\" width=\"0.2\" height=\"0.2\">"
        "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"red\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"url(#po)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParsePatternWithViewBox)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"pvb\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\" viewBox=\"0 0 10 10\">"
        "<circle cx=\"5\" cy=\"5\" r=\"4\" fill=\"green\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#pvb)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParsePatternWithTransform)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"pt\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\""
        " patternTransform=\"rotate(45)\">"
        "<rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" fill=\"navy\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#pt)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParsePatternHrefInheritance)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"base-pat\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\">"
        "<rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" fill=\"teal\" />"
        "</pattern>"
        "<pattern id=\"derived-pat\" href=\"#base-pat\" patternTransform=\"rotate(30)\" />"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#derived-pat)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParsePatternWithComplexContent)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"complex\" patternUnits=\"userSpaceOnUse\" width=\"30\" height=\"30\">"
        "<rect x=\"0\" y=\"0\" width=\"30\" height=\"30\" fill=\"white\" />"
        "<circle cx=\"15\" cy=\"15\" r=\"10\" fill=\"none\" stroke=\"black\" stroke-width=\"1\" />"
        "<line x1=\"0\" y1=\"15\" x2=\"30\" y2=\"15\" stroke=\"gray\" stroke-width=\"0.5\" />"
        "<line x1=\"15\" y1=\"0\" x2=\"15\" y2=\"30\" stroke=\"gray\" stroke-width=\"0.5\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#complex)\" />"
        "</svg>"));
}

// ==============================================================================
// Filters
// ==============================================================================

TEST (SVGParserTests, ParseFilterGaussianBlur)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<filter id=\"blur\">"
        "<feGaussianBlur stdDeviation=\"3\" />"
        "</filter>"
        "</defs>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" filter=\"url(#blur)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseFilterWithHref)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<filter id=\"base-blur\">"
        "<feGaussianBlur stdDeviation=\"2\" />"
        "</filter>"
        "<filter id=\"derived-blur\" href=\"#base-blur\" />"
        "</defs>"
        "<circle cx=\"50\" cy=\"50\" r=\"40\" fill=\"blue\" filter=\"url(#derived-blur)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseFilterNoneResetsFilter)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<filter id=\"f\"><feGaussianBlur stdDeviation=\"5\" /></filter>"
        "</defs>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"green\" filter=\"url(#f)\" />"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" style=\"filter:none\" />"
        "</svg>"));
}

// ==============================================================================
// CSS styling
// ==============================================================================

TEST (SVGParserTests, ParseCSSTypeSelector)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>rect { fill: green; stroke: black; stroke-width: 2; }</style>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSIdSelector)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>#myRect { fill: blue; opacity: 0.8; }</style>"
        "<rect id=\"myRect\" x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSClassSelector)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>.highlight { fill: yellow; stroke: orange; stroke-width: 3; }</style>"
        "<rect class=\"highlight\" x=\"10\" y=\"10\" width=\"30\" height=\"30\" />"
        "<circle class=\"highlight\" cx=\"70\" cy=\"30\" r=\"20\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSMultipleSelectors)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>"
        "rect, circle { fill: red; }"
        ".special { stroke: blue; stroke-width: 2; }"
        "#unique { opacity: 0.5; }"
        "</style>"
        "<rect x=\"10\" y=\"10\" width=\"30\" height=\"30\" />"
        "<circle class=\"special\" id=\"unique\" cx=\"70\" cy=\"30\" r=\"20\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSInlineStyleOverridesAttribute)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>rect { fill: green; }</style>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" style=\"fill: blue;\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSDisplayNone)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" style=\"display:none\" />"
        "<circle cx=\"50\" cy=\"50\" r=\"30\" fill=\"blue\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSVisibilityHidden)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" style=\"visibility:hidden\" />"
        "<rect x=\"20\" y=\"20\" width=\"60\" height=\"60\" fill=\"blue\" style=\"visibility:visible\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSCurrentColor)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>rect { color: #ff0000; fill: currentColor; }</style>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSFillOpacityAndStrokeOpacity)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\""
        " fill=\"red\" fill-opacity=\"0.7\""
        " stroke=\"blue\" stroke-width=\"3\" stroke-opacity=\"0.4\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSFontProperties)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style>"
        "text { font-family: sans-serif; font-size: 14px; font-weight: bold; font-style: italic; }"
        "</style>"
        "<text x=\"10\" y=\"50\">Styled text</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSLetterAndWordSpacing)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 50\">"
        "<text x=\"10\" y=\"30\" style=\"letter-spacing: 3px; word-spacing: 5px;\">Spaced out text</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSStrokeDasharray)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 50\">"
        "<path d=\"M 10 25 L 90 25\" stroke=\"black\" stroke-width=\"2\""
        " stroke-dasharray=\"5 3\" stroke-dashoffset=\"2\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSStrokeDasharrayNone)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 50\">"
        "<path d=\"M 10 25 L 90 25\" stroke=\"black\" stroke-width=\"2\""
        " stroke-dasharray=\"none\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseCSSMixBlendModeAllValues)
{
    static const char* modes[] = {
        "multiply", "screen", "overlay", "darken", "lighten", "color-dodge", "color-burn", "hard-light", "soft-light", "difference", "exclusion", "hue", "saturation", "color", "luminosity"
    };

    for (auto* mode : modes)
    {
        Drawable d;
        String svg = "<svg viewBox=\"0 0 100 100\">"
                     "<rect x=\"10\" y=\"10\" width=\"60\" height=\"60\" fill=\"red\" />"
                     "<rect x=\"30\" y=\"30\" width=\"60\" height=\"60\" fill=\"blue\" style=\"mix-blend-mode: ";
        svg += mode;
        svg += "\" /></svg>";
        EXPECT_TRUE (d.parseSVG (svg)) << "Failed for blend mode: " << mode;
    }
}

// ==============================================================================
// Stroke and fill properties
// ==============================================================================

TEST (SVGParserTests, ParseStrokeLineJoinValues)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<path d=\"M 10 80 L 50 10 L 90 80\" fill=\"none\" stroke=\"black\" stroke-width=\"5\""
        " stroke-linejoin=\"round\" />"
        "<path d=\"M 10 80 L 50 10 L 90 80\" fill=\"none\" stroke=\"black\" stroke-width=\"5\""
        " stroke-linejoin=\"miter\" />"
        "<path d=\"M 10 80 L 50 10 L 90 80\" fill=\"none\" stroke=\"black\" stroke-width=\"5\""
        " stroke-linejoin=\"bevel\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseStrokeLineCapValues)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<line x1=\"10\" y1=\"20\" x2=\"90\" y2=\"20\" stroke=\"black\" stroke-width=\"10\" stroke-linecap=\"butt\" />"
        "<line x1=\"10\" y1=\"50\" x2=\"90\" y2=\"50\" stroke=\"black\" stroke-width=\"10\" stroke-linecap=\"round\" />"
        "<line x1=\"10\" y1=\"80\" x2=\"90\" y2=\"80\" stroke=\"black\" stroke-width=\"10\" stroke-linecap=\"square\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseStrokeMiterLimit)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<path d=\"M 50 10 L 61 40 L 98 40 Z\" fill=\"none\" stroke=\"black\" stroke-width=\"3\""
        " stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" />"
        "<path d=\"M 50 10 L 61 40 L 98 40 Z\" fill=\"none\" stroke=\"black\" stroke-width=\"3\""
        " stroke-linejoin=\"miter\" stroke-miterlimit=\"10\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseFillNone)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"none\" stroke=\"black\" stroke-width=\"2\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseStrokeNone)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"blue\" stroke=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseColorNamedValues)
{
    static const char* colors[] = {
        "red", "green", "blue", "black", "white", "yellow", "cyan", "magenta", "orange", "purple", "gray", "grey", "silver", "maroon", "navy"
    };

    for (auto* color : colors)
    {
        Drawable d;
        String svg = "<svg viewBox=\"0 0 10 10\"><rect width=\"10\" height=\"10\" fill=\"";
        svg += color;
        svg += "\" /></svg>";
        EXPECT_TRUE (d.parseSVG (svg)) << "Failed for color: " << color;
    }
}

TEST (SVGParserTests, ParseColorHexValues)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" fill=\"#f00\" />"
        "<rect x=\"20\" y=\"0\" width=\"20\" height=\"20\" fill=\"#ff0000\" />"
        "<rect x=\"40\" y=\"0\" width=\"20\" height=\"20\" fill=\"#FF0000\" />"
        "<rect x=\"60\" y=\"0\" width=\"20\" height=\"20\" fill=\"#ff000080\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseColorRgbFunction)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"0\" y=\"0\" width=\"50\" height=\"50\" fill=\"rgb(255,0,0)\" />"
        "<rect x=\"50\" y=\"0\" width=\"50\" height=\"50\" fill=\"rgb(0, 128, 0)\" />"
        "</svg>"));
}

// ==============================================================================
// Transforms
// ==============================================================================

TEST (SVGParserTests, ParseTransformTranslate)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"translate(20 30)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformTranslateSingleValue)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"translate(15)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformScale)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"scale(2)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformScaleXY)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"scale(2 3)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformRotate)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"rotate(45)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformRotateAboutPoint)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"rotate(45 5 5)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformSkewX)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"skewX(30)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformSkewY)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"skewY(20)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformMatrix)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"matrix(1 0 0 1 10 20)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformMatrixCommas)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"matrix(1,0,0,1,10,20)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformChained)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"translate(10 10) rotate(45) scale(2)\" /></svg>"));
}

TEST (SVGParserTests, ParseTransformOnGroup)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg><g transform=\"translate(20 20) scale(0.5)\">"
        "<rect x=\"0\" y=\"0\" width=\"60\" height=\"60\" />"
        "<circle cx=\"30\" cy=\"30\" r=\"20\" />"
        "</g></svg>"));
}

TEST (SVGParserTests, ParseTransformOriginAttribute)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"30\" y=\"30\" width=\"40\" height=\"40\""
        " transform=\"rotate(45)\" transform-origin=\"50 50\" />"
        "</svg>"));
}

// ==============================================================================
// Units
// ==============================================================================

TEST (SVGParserTests, ParseDimensionsInPixels)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg width=\"100px\" height=\"50px\"></svg>"));
    EXPECT_FLOAT_EQ (100.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (50.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseDimensionsInInches)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg viewBox=\"0 0 192 96\" width=\"2in\" height=\"1in\"></svg>"));
    EXPECT_FLOAT_EQ (192.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (96.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseStrokeWidthWithUnits)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<path d=\"M 10 50 L 90 50\" stroke=\"black\" stroke-width=\"1mm\" fill=\"none\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseFontSizeWithUnits)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 100\">"
        "<text x=\"10\" y=\"50\" font-size=\"12pt\">Point size text</text>"
        "<text x=\"10\" y=\"80\" font-size=\"1em\">Em size text</text>"
        "</svg>"));
}

// ==============================================================================
// PreserveAspectRatio
// ==============================================================================

TEST (SVGParserTests, ParsePreserveAspectRatioNone)
{
    auto doc = SVGParser::parse ("<svg viewBox=\"0 0 100 100\" preserveAspectRatio=\"none\"></svg>");
    ASSERT_NE (doc, nullptr);
    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasPreserveAspectRatio);
        EXPECT_EQ (Fitting::fill, data.rootPreserveAspectRatioFitting);
    });
}

TEST (SVGParserTests, ParsePreserveAspectRatioXMidYMid)
{
    auto doc = SVGParser::parse ("<svg viewBox=\"0 0 100 100\" preserveAspectRatio=\"xMidYMid meet\"></svg>");
    ASSERT_NE (doc, nullptr);
    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasPreserveAspectRatio);
        EXPECT_EQ (Fitting::scaleToFit, data.rootPreserveAspectRatioFitting);
        EXPECT_TRUE (data.rootPreserveAspectRatioJustification.testFlags (Justification::horizontalCenter));
        EXPECT_TRUE (data.rootPreserveAspectRatioJustification.testFlags (Justification::verticalCenter));
    });
}

TEST (SVGParserTests, ParsePreserveAspectRatioXMinYMin)
{
    auto doc = SVGParser::parse ("<svg viewBox=\"0 0 100 100\" preserveAspectRatio=\"xMinYMin meet\"></svg>");
    ASSERT_NE (doc, nullptr);
    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasPreserveAspectRatio);
        EXPECT_EQ (Fitting::scaleToFit, data.rootPreserveAspectRatioFitting);
        EXPECT_TRUE (data.rootPreserveAspectRatioJustification.testFlags (Justification::left));
        EXPECT_TRUE (data.rootPreserveAspectRatioJustification.testFlags (Justification::top));
    });
}

TEST (SVGParserTests, ParsePreserveAspectRatioXMaxYMax)
{
    auto doc = SVGParser::parse ("<svg viewBox=\"0 0 100 100\" preserveAspectRatio=\"xMaxYMax slice\"></svg>");
    ASSERT_NE (doc, nullptr);
    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasPreserveAspectRatio);
        EXPECT_EQ (Fitting::scaleToFill, data.rootPreserveAspectRatioFitting);
        EXPECT_TRUE (data.rootPreserveAspectRatioJustification.testFlags (Justification::right));
        EXPECT_TRUE (data.rootPreserveAspectRatioJustification.testFlags (Justification::bottom));
    });
}

TEST (SVGParserTests, ParsePreserveAspectRatioWithInternalEntities)
{
    auto doc = SVGParser::parse (
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<!DOCTYPE svg ["
        "<!ENTITY Smile \""
        "\n"
        "<rect x='.5' y='.5' width='29' height='39' fill='black' stroke='red'/>"
        "\n"
        "<g transform='translate(0, 5)'>"
        "\n"
        "<circle cx='15' cy='15' r='10' fill='yellow'/>"
        "\n"
        "</g>"
        "\n"
        "\">"
        "<!ENTITY Viewport \"<rect x='.5' y='.5' width='49' height='29' fill='none' stroke='blue'/>\">"
        "]>"
        "<svg width=\"100\" height=\"100\" viewBox=\"0 0 100 100\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<g transform=\"translate(10, 10)\">&Viewport;"
        "<svg preserveAspectRatio=\"xMidYMid meet\" viewBox=\"0 0 30 40\" width=\"50\" height=\"30\">&Smile;</svg>"
        "</g>"
        "</svg>");

    ASSERT_NE (doc, nullptr);

    int nestedSmileCircles = 0;
    doc->visit ([&] (const SVGData& data)
    {
        std::function<void (const SVGElement&, bool)> visitElement = [&] (const SVGElement& element, bool insideNestedSVG)
        {
            const auto isNestedSVG = insideNestedSVG || (element.tagName == "svg" && element.viewBox.has_value());

            if (isNestedSVG && element.tagName == "circle")
                ++nestedSmileCircles;

            for (const auto& child : element.children)
                visitElement (*child, isNestedSVG);
        };

        for (const auto& element : data.elements)
            visitElement (*element, false);
    });

    EXPECT_EQ (1, nestedSmileCircles);
}

TEST (SVGParserTests, ParsePreserveAspectRatioOnSymbol)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<symbol id=\"s\" viewBox=\"0 0 20 10\" preserveAspectRatio=\"xMidYMid meet\">"
        "<rect x=\"0\" y=\"0\" width=\"20\" height=\"10\" fill=\"teal\" />"
        "</symbol>"
        "<use href=\"#s\" x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "</svg>"));
}

// ==============================================================================
// Nested SVG
// ==============================================================================

TEST (SVGParserTests, ParseNestedSVG)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 200\">"
        "<rect x=\"0\" y=\"0\" width=\"200\" height=\"200\" fill=\"lightgray\" />"
        "<svg x=\"20\" y=\"20\" width=\"80\" height=\"80\" viewBox=\"0 0 40 40\">"
        "<circle cx=\"20\" cy=\"20\" r=\"18\" fill=\"red\" />"
        "</svg>"
        "<svg x=\"120\" y=\"20\" width=\"60\" height=\"60\" viewBox=\"0 0 30 30\">"
        "<rect x=\"3\" y=\"3\" width=\"24\" height=\"24\" fill=\"blue\" />"
        "</svg>"
        "</svg>"));
}

TEST (SVGParserTests, ParseNestedSVGWithPercentageDimensions)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<svg x=\"10\" y=\"10\" width=\"50%\" height=\"50%\" viewBox=\"0 0 20 20\">"
        "<circle cx=\"10\" cy=\"10\" r=\"8\" fill=\"orange\" />"
        "</svg>"
        "</svg>"));
}

// ==============================================================================
// Cascading and inheritance
// ==============================================================================

TEST (SVGParserTests, ParseColorInheritanceFromParent)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<g color=\"#ff0000\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"currentColor\" />"
        "</g>"
        "</svg>"));
}

TEST (SVGParserTests, ParseFontInheritanceFromParent)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 100\">"
        "<g font-family=\"monospace\" font-size=\"16\">"
        "<text x=\"10\" y=\"30\">Parent font</text>"
        "<g font-size=\"12\">"
        "<text x=\"10\" y=\"60\">Child font</text>"
        "</g>"
        "</g>"
        "</svg>"));
}

TEST (SVGParserTests, ParseOpacityInheritance)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<g opacity=\"0.5\">"
        "<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" fill=\"red\" />"
        "<rect x=\"50\" y=\"50\" width=\"40\" height=\"40\" fill=\"blue\" opacity=\"0.8\" />"
        "</g>"
        "</svg>"));
}

// ==============================================================================
// Complex real-world documents
// ==============================================================================

TEST (SVGParserTests, ParseComplexIconWithMultiplePaths)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 24 24\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path d=\"M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2z\" fill=\"#1976D2\" />"
        "<path d=\"M13 7h-2v2h2V7zm0 4h-2v6h2v-6z\" fill=\"white\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseComplexLogoWithGroupsAndGradients)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 50\">"
        "<defs>"
        "<linearGradient id=\"bg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">"
        "<stop offset=\"0\" stop-color=\"#ff6b6b\" />"
        "<stop offset=\"0.5\" stop-color=\"#feca57\" />"
        "<stop offset=\"1\" stop-color=\"#48dbfb\" />"
        "</linearGradient>"
        "</defs>"
        "<rect width=\"100\" height=\"50\" fill=\"url(#bg)\" />"
        "<g transform=\"translate(50 25)\">"
        "<circle r=\"15\" fill=\"white\" fill-opacity=\"0.3\" />"
        "<text text-anchor=\"middle\" dy=\"0.35em\" font-size=\"10\" fill=\"white\">LOGO</text>"
        "</g>"
        "</svg>"));
}

TEST (SVGParserTests, ParseChartWithMultipleElements)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 120\">"
        "<rect x=\"0\" y=\"0\" width=\"200\" height=\"120\" fill=\"#f8f9fa\" />"
        "<line x1=\"30\" y1=\"10\" x2=\"30\" y2=\"100\" stroke=\"#dee2e6\" stroke-width=\"1\" />"
        "<line x1=\"30\" y1=\"100\" x2=\"190\" y2=\"100\" stroke=\"#dee2e6\" stroke-width=\"1\" />"
        "<polyline points=\"30,80 70,60 110,40 150,55 190,30\""
        " fill=\"none\" stroke=\"#339af0\" stroke-width=\"2\" />"
        "<polyline points=\"30,90 70,75 110,70 150,80 190,65\""
        " fill=\"none\" stroke=\"#51cf66\" stroke-width=\"2\" />"
        "<text x=\"30\" y=\"115\" font-size=\"6\" text-anchor=\"middle\">Q1</text>"
        "<text x=\"70\" y=\"115\" font-size=\"6\" text-anchor=\"middle\">Q2</text>"
        "<text x=\"110\" y=\"115\" font-size=\"6\" text-anchor=\"middle\">Q3</text>"
        "<text x=\"150\" y=\"115\" font-size=\"6\" text-anchor=\"middle\">Q4</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseSVGWithAllBasicShapes)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 200\">"
        "<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" rx=\"5\" fill=\"red\" />"
        "<circle cx=\"90\" cy=\"30\" r=\"20\" fill=\"green\" />"
        "<ellipse cx=\"150\" cy=\"30\" rx=\"30\" ry=\"15\" fill=\"blue\" />"
        "<line x1=\"10\" y1=\"80\" x2=\"190\" y2=\"80\" stroke=\"black\" stroke-width=\"2\" />"
        "<polygon points=\"10,100 50,90 70,120 30,130\" fill=\"orange\" />"
        "<polyline points=\"90,90 110,110 130,90 150,110 170,90\""
        " fill=\"none\" stroke=\"purple\" stroke-width=\"2\" />"
        "<path d=\"M 10 150 Q 50 130 90 150 T 170 150\" fill=\"none\" stroke=\"teal\" stroke-width=\"2\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseArrowheadSVG)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"start\" markerWidth=\"10\" markerHeight=\"7\" refX=\"10\" refY=\"3.5\" orient=\"auto-start-reverse\">"
        "<polygon points=\"0 0, 10 3.5, 0 7\" fill=\"black\" />"
        "</marker>"
        "<marker id=\"end\" markerWidth=\"10\" markerHeight=\"7\" refX=\"0\" refY=\"3.5\" orient=\"auto\">"
        "<polygon points=\"0 0, 10 3.5, 0 7\" fill=\"black\" />"
        "</marker>"
        "</defs>"
        "<path d=\"M 20 20 C 30 10 70 10 80 20 S 90 50 80 60 S 50 90 20 80 S 5 50 20 20\""
        " fill=\"none\" stroke=\"black\" stroke-width=\"2\""
        " marker-start=\"url(#start)\" marker-end=\"url(#end)\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseFlowchartSVG)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 300\">"
        "<defs>"
        "<marker id=\"arr\" markerWidth=\"8\" markerHeight=\"6\" refX=\"8\" refY=\"3\" orient=\"auto\">"
        "<polygon points=\"0 0, 8 3, 0 6\" fill=\"gray\" />"
        "</marker>"
        "</defs>"
        "<rect x=\"60\" y=\"10\" width=\"80\" height=\"30\" rx=\"4\" fill=\"#e7f3ff\" stroke=\"#1971c2\" stroke-width=\"1\" />"
        "<text x=\"100\" y=\"30\" text-anchor=\"middle\" font-size=\"8\">Start</text>"
        "<line x1=\"100\" y1=\"40\" x2=\"100\" y2=\"70\" stroke=\"gray\" stroke-width=\"1\" marker-end=\"url(#arr)\" />"
        "<rect x=\"40\" y=\"70\" width=\"120\" height=\"40\" rx=\"4\" fill=\"#fff9db\" stroke=\"#e67700\" stroke-width=\"1\" />"
        "<text x=\"100\" y=\"95\" text-anchor=\"middle\" font-size=\"8\">Process</text>"
        "<line x1=\"100\" y1=\"110\" x2=\"100\" y2=\"140\" stroke=\"gray\" stroke-width=\"1\" marker-end=\"url(#arr)\" />"
        "<polygon points=\"100,140 150,165 100,190 50,165\" fill=\"#e6fcf5\" stroke=\"#087f5b\" stroke-width=\"1\" />"
        "<text x=\"100\" y=\"170\" text-anchor=\"middle\" font-size=\"8\">Decision?</text>"
        "</svg>"));
}

TEST (SVGParserTests, ParseHatchPatternSVG)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"hatch\" patternUnits=\"userSpaceOnUse\" width=\"10\" height=\"10\""
        " patternTransform=\"rotate(45 0 0)\">"
        "<line x1=\"0\" y1=\"0\" x2=\"0\" y2=\"10\" style=\"stroke:#888; stroke-width:1\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#hatch)\" />"
        "</svg>"));
}

// ==============================================================================
// Render tests (no crash)
// ==============================================================================

TEST (SVGParserTests, RenderAllShapesToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 200\">"
        "<rect x=\"10\" y=\"10\" width=\"40\" height=\"40\" fill=\"red\" />"
        "<circle cx=\"90\" cy=\"30\" r=\"20\" fill=\"green\" />"
        "<ellipse cx=\"150\" cy=\"30\" rx=\"30\" ry=\"15\" fill=\"blue\" />"
        "<line x1=\"10\" y1=\"80\" x2=\"190\" y2=\"80\" stroke=\"black\" stroke-width=\"2\" />"
        "<polygon points=\"10,100 50,90 70,120 30,130\" fill=\"orange\" />"
        "<polyline points=\"90,90 110,110 130,90\" fill=\"none\" stroke=\"purple\" stroke-width=\"2\" />"
        "<path d=\"M 10 150 Q 50 130 90 150 T 170 150\" fill=\"none\" stroke=\"teal\" stroke-width=\"2\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics (128, 128);
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 128.0f, 128.0f }));
}

TEST (SVGParserTests, RenderGradientsToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<linearGradient id=\"lg\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">"
        "<stop offset=\"0\" stop-color=\"red\" />"
        "<stop offset=\"1\" stop-color=\"blue\" />"
        "</linearGradient>"
        "<radialGradient id=\"rg\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\">"
        "<stop offset=\"0\" stop-color=\"white\" />"
        "<stop offset=\"1\" stop-color=\"black\" />"
        "</radialGradient>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"50\" height=\"100\" fill=\"url(#lg)\" />"
        "<rect x=\"50\" y=\"0\" width=\"50\" height=\"100\" fill=\"url(#rg)\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderClipPathToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<clipPath id=\"c\"><circle cx=\"50\" cy=\"50\" r=\"40\" /></clipPath>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"red\" clip-path=\"url(#c)\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderMaskToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<mask id=\"m\">"
        "<circle cx=\"50\" cy=\"50\" r=\"40\" fill=\"white\" />"
        "</mask>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"blue\" mask=\"url(#m)\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderMarkersToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<marker id=\"a\" markerWidth=\"8\" markerHeight=\"6\" refX=\"8\" refY=\"3\" orient=\"auto\">"
        "<polygon points=\"0 0, 8 3, 0 6\" fill=\"black\" />"
        "</marker>"
        "</defs>"
        "<path d=\"M 10 50 L 90 50\" stroke=\"black\" stroke-width=\"2\" marker-end=\"url(#a)\" fill=\"none\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderPatternToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<pattern id=\"p\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\">"
        "<rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" fill=\"green\" />"
        "</pattern>"
        "</defs>"
        "<rect x=\"0\" y=\"0\" width=\"100\" height=\"100\" fill=\"url(#p)\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderTextToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 200 80\">"
        "<text x=\"10\" y=\"30\" font-size=\"14\" fill=\"black\">Hello, World!</text>"
        "<text x=\"10\" y=\"60\" font-size=\"12\" font-weight=\"bold\" fill=\"red\">Bold Red</text>"
        "</svg>"));

    auto g = makeHeadlessGraphics (128, 64);
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 128.0f, 64.0f }));
}

TEST (SVGParserTests, RenderStrokeDashToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<path d=\"M 10 20 L 90 20\" stroke=\"black\" stroke-width=\"3\" stroke-dasharray=\"8 4\" fill=\"none\" />"
        "<path d=\"M 10 50 L 90 50\" stroke=\"red\" stroke-width=\"2\" stroke-dasharray=\"5 3 1 3\" fill=\"none\" />"
        "<path d=\"M 10 80 L 90 80\" stroke=\"blue\" stroke-width=\"2\" stroke-dasharray=\"10\" fill=\"none\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderUseReferenceToHeadlessContext)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<g id=\"tile\">"
        "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"cyan\" />"
        "<circle cx=\"5\" cy=\"5\" r=\"4\" fill=\"magenta\" />"
        "</g>"
        "</defs>"
        "<use href=\"#tile\" x=\"0\" y=\"0\" />"
        "<use href=\"#tile\" x=\"20\" y=\"0\" />"
        "<use href=\"#tile\" x=\"40\" y=\"0\" />"
        "<use href=\"#tile\" x=\"0\" y=\"20\" />"
        "<use href=\"#tile\" x=\"20\" y=\"20\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderBlendModesToHeadlessContext)
{
    static const char* modes[] = { "multiply", "screen", "overlay", "difference" };

    for (auto* mode : modes)
    {
        Drawable d;
        String svg = "<svg viewBox=\"0 0 100 100\">"
                     "<rect x=\"10\" y=\"10\" width=\"60\" height=\"60\" fill=\"red\" />"
                     "<rect x=\"30\" y=\"30\" width=\"60\" height=\"60\" fill=\"blue\" style=\"mix-blend-mode: ";
        svg += mode;
        svg += "\" /></svg>";
        ASSERT_TRUE (d.parseSVG (svg)) << mode;

        auto g = makeHeadlessGraphics();
        EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f })) << mode;
    }
}

TEST (SVGParserTests, RenderCyclicUseDoesNotCrash)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<g id=\"cycle\"><use href=\"#cycle\" /></g>"
        "</defs>"
        "<use href=\"#cycle\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderMutuallyCyclicUseDoesNotCrash)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<defs>"
        "<g id=\"a\"><use href=\"#b\" /></g>"
        "<g id=\"b\"><use href=\"#a\" /></g>"
        "</defs>"
        "<use href=\"#a\" />"
        "</svg>"));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

TEST (SVGParserTests, RenderDeepRecursionInUseDoesNotCrash)
{
    String inner = "<use href=\"#box\" />";
    String svg = "<svg viewBox=\"0 0 100 100\"><defs><g id=\"box\">";
    for (int i = 0; i < 20; ++i)
        svg += "<use href=\"#box\" />";
    svg += "</g></defs><use href=\"#box\" /></svg>";

    Drawable d;
    ASSERT_TRUE (d.parseSVG (svg));

    auto g = makeHeadlessGraphics();
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 64.0f, 64.0f }));
}

// ==============================================================================
// Stress / edge cases
// ==============================================================================

TEST (SVGParserTests, ParseSVGWithNoContent)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG ("<svg viewBox=\"0 0 100 100\"></svg>"));
    EXPECT_FLOAT_EQ (100.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (100.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGWithManyElements)
{
    String svg = "<svg viewBox=\"0 0 1000 1000\">";
    for (int i = 0; i < 100; ++i)
    {
        float x = float (i % 10) * 100.0f;
        float y = float (i / 10) * 100.0f;
        svg += "<rect x=\"" + String (x + 5.0f) + "\" y=\"" + String (y + 5.0f) + "\" width=\"90\" height=\"90\" fill=\"#"
             + String::toHexString (i * 2621440).paddedLeft ('0', 6).toUpperCase().substring (2)
             + "\" />";
    }
    svg += "</svg>";

    Drawable d;
    EXPECT_TRUE (d.parseSVG (svg));

    auto g = makeHeadlessGraphics (256, 256);
    EXPECT_NO_THROW (d.paint (*g.graphics, { 0.0f, 0.0f, 256.0f, 256.0f }));
}

TEST (SVGParserTests, ParseSVGWithCommentNodes)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<!-- This is a comment -->"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" />"
        "<!-- Another comment -->"
        "</svg>"));
}

TEST (SVGParserTests, ParseSVGWithCDATAInStyle)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<style><![CDATA[rect { fill: blue; }]]></style>"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseAndReparseProducesSameBounds)
{
    const String svg = "<svg viewBox=\"0 0 75 50\"><rect x=\"5\" y=\"5\" width=\"65\" height=\"40\" /></svg>";

    Drawable d;
    ASSERT_TRUE (d.parseSVG (svg));
    const auto bounds1 = d.getBounds();

    ASSERT_TRUE (d.parseSVG (svg));
    const auto bounds2 = d.getBounds();

    EXPECT_FLOAT_EQ (bounds1.getWidth(), bounds2.getWidth());
    EXPECT_FLOAT_EQ (bounds1.getHeight(), bounds2.getHeight());
}

TEST (SVGParserTests, ClearAfterParseThenParseDifferentSVG)
{
    Drawable d;
    ASSERT_TRUE (d.parseSVG ("<svg viewBox=\"0 0 100 100\"><rect width=\"100\" height=\"100\" /></svg>"));
    EXPECT_FLOAT_EQ (100.0f, d.getBounds().getWidth());

    d.clear();
    EXPECT_TRUE (d.getBounds().isEmpty());

    ASSERT_TRUE (d.parseSVG ("<svg viewBox=\"0 0 200 150\"><rect width=\"200\" height=\"150\" /></svg>"));
    EXPECT_FLOAT_EQ (200.0f, d.getBounds().getWidth());
    EXPECT_FLOAT_EQ (150.0f, d.getBounds().getHeight());
}

TEST (SVGParserTests, ParseSVGWithUnknownElements)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\" />"
        "<unknownElement foo=\"bar\">ignored content</unknownElement>"
        "<anotherUnknown />"
        "</svg>"));
}

TEST (SVGParserTests, ParseSVGWithUnknownAttributes)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox=\"0 0 100 100\" unknown-attr=\"value\" data-custom=\"test\">"
        "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\""
        " unknown-fill=\"red\" data-id=\"42\" />"
        "</svg>"));
}

TEST (SVGParserTests, ParseSVGWithWhitespaceInAttributes)
{
    Drawable d;
    EXPECT_TRUE (d.parseSVG (
        "<svg viewBox = \"0 0 100 100\">"
        "<rect x = \"10\" y = \"10\" width = \"80\" height = \"80\" fill = \"red\" />"
        "</svg>"));
}
