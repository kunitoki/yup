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

SVGDocument::Ptr parse (const char* svgText)
{
    return SVGParser::parse (StringRef (svgText));
}

} // namespace

// ==============================================================================
// SVGDocument null / root validation
// ==============================================================================

TEST (SVGDocumentTests, ParseNullptrForEmptyString)
{
    EXPECT_EQ (nullptr, parse (""));
}

TEST (SVGDocumentTests, ParseNullptrForNonSVGRoot)
{
    EXPECT_EQ (nullptr, parse ("<html></html>"));
}

TEST (SVGDocumentTests, ParseNullptrForMalformedXML)
{
    EXPECT_EQ (nullptr, parse ("not xml"));
}

TEST (SVGDocumentTests, ParseReturnsNonNullForMinimalSVG)
{
    EXPECT_NE (nullptr, parse ("<svg></svg>"));
}

// ==============================================================================
// SVGData::viewBox, size and bounds
// ==============================================================================

TEST (SVGDocumentTests, ViewBoxParsedFromAttribute)
{
    auto doc = parse ("<svg viewBox=\"0 0 200 100\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FLOAT_EQ (0.0f, data.viewBox.getX());
        EXPECT_FLOAT_EQ (0.0f, data.viewBox.getY());
        EXPECT_FLOAT_EQ (200.0f, data.viewBox.getWidth());
        EXPECT_FLOAT_EQ (100.0f, data.viewBox.getHeight());
    });
}

TEST (SVGDocumentTests, ViewBoxWithNonZeroOrigin)
{
    auto doc = parse ("<svg viewBox=\"10 20 80 60\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FLOAT_EQ (10.0f, data.viewBox.getX());
        EXPECT_FLOAT_EQ (20.0f, data.viewBox.getY());
        EXPECT_FLOAT_EQ (80.0f, data.viewBox.getWidth());
        EXPECT_FLOAT_EQ (60.0f, data.viewBox.getHeight());
    });
}

TEST (SVGDocumentTests, SizeFallsBackToViewBoxWhenWidthHeightAbsent)
{
    auto doc = parse ("<svg viewBox=\"0 0 50 40\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FLOAT_EQ (50.0f, data.size.getWidth());
        EXPECT_FLOAT_EQ (40.0f, data.size.getHeight());
    });
}

TEST (SVGDocumentTests, ExplicitWidthHeightOverridesViewBoxForSize)
{
    auto doc = parse ("<svg viewBox=\"0 0 50 40\" width=\"120\" height=\"90\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FLOAT_EQ (120.0f, data.size.getWidth());
        EXPECT_FLOAT_EQ (90.0f, data.size.getHeight());
        // viewBox remains as declared
        EXPECT_FLOAT_EQ (50.0f, data.viewBox.getWidth());
        EXPECT_FLOAT_EQ (40.0f, data.viewBox.getHeight());
    });
}

TEST (SVGDocumentTests, BoundsMatchViewBox)
{
    auto doc = parse ("<svg viewBox=\"0 0 300 150\"></svg>");
    ASSERT_NE (nullptr, doc);

    EXPECT_FLOAT_EQ (300.0f, doc->getBounds().getWidth());
    EXPECT_FLOAT_EQ (150.0f, doc->getBounds().getHeight());
}

TEST (SVGDocumentTests, BoundsMatchWidthHeightWhenNoViewBox)
{
    auto doc = parse ("<svg width=\"400\" height=\"250\"></svg>");
    ASSERT_NE (nullptr, doc);

    EXPECT_FLOAT_EQ (400.0f, doc->getBounds().getWidth());
    EXPECT_FLOAT_EQ (250.0f, doc->getBounds().getHeight());
}

TEST (SVGDocumentTests, BoundsIncludeNestedGroupContentWhenNoViewBoxOrSize)
{
    auto doc = parse ("<svg><g transform=\"translate(10,20)\"><circle cx=\"100\" cy=\"50\" r=\"25\" /></g></svg>");
    ASSERT_NE (nullptr, doc);

    auto bounds = doc->getBounds();
    EXPECT_FLOAT_EQ (85.0f, bounds.getX());
    EXPECT_FLOAT_EQ (45.0f, bounds.getY());
    EXPECT_FLOAT_EQ (50.0f, bounds.getWidth());
    EXPECT_FLOAT_EQ (50.0f, bounds.getHeight());

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FLOAT_EQ (85.0f, data.viewBox.getX());
        EXPECT_FLOAT_EQ (45.0f, data.viewBox.getY());
        EXPECT_FLOAT_EQ (50.0f, data.viewBox.getWidth());
        EXPECT_FLOAT_EQ (50.0f, data.viewBox.getHeight());
    });
}

// ==============================================================================
// SVGData root fill / stroke flags
// ==============================================================================

TEST (SVGDocumentTests, RootHasFillByDefault)
{
    auto doc = parse ("<svg></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasFill);
    });
}

TEST (SVGDocumentTests, RootFillNoneDisablesRootHasFill)
{
    auto doc = parse ("<svg fill=\"none\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FALSE (data.rootHasFill);
    });
}

TEST (SVGDocumentTests, RootFillColorIsSetWhenFillAttributePresent)
{
    auto doc = parse ("<svg fill=\"#ff0000\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasFill);
        ASSERT_TRUE (data.rootFillColor.has_value());
    });
}

TEST (SVGDocumentTests, RootHasStrokeFalseByDefault)
{
    auto doc = parse ("<svg></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_FALSE (data.rootHasStroke);
    });
}

TEST (SVGDocumentTests, RootStrokeColorIsSetWhenStrokeAttributePresent)
{
    auto doc = parse ("<svg stroke=\"blue\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.rootHasStroke);
        ASSERT_TRUE (data.rootStrokeColor.has_value());
    });
}

// ==============================================================================
// SVGData::elements  (top-level element tree)
// ==============================================================================

TEST (SVGDocumentTests, EmptySVGHasZeroElements)
{
    auto doc = parse ("<svg></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (0u, data.elements.size());
    });
}

TEST (SVGDocumentTests, SingleRectProducesOneTopLevelElement)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (1u, data.elements.size());
        EXPECT_EQ (String ("rect"), data.elements[0]->tagName);
    });
}

TEST (SVGDocumentTests, MultipleTopLevelElementsAreAllCollected)
{
    auto doc = parse ("<svg>"
                      "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" />"
                      "<circle cx=\"20\" cy=\"20\" r=\"5\" />"
                      "<line x1=\"0\" y1=\"0\" x2=\"30\" y2=\"30\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (3u, data.elements.size());
        EXPECT_EQ (String ("rect"), data.elements[0]->tagName);
        EXPECT_EQ (String ("circle"), data.elements[1]->tagName);
        EXPECT_EQ (String ("line"), data.elements[2]->tagName);
    });
}

TEST (SVGDocumentTests, GroupChildrenAreNestedNotTopLevel)
{
    auto doc = parse ("<svg><g><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></g></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_EQ (String ("g"), data.elements[0]->tagName);
        ASSERT_EQ (1u, data.elements[0]->children.size());
        EXPECT_EQ (String ("rect"), data.elements[0]->children[0]->tagName);
    });
}

TEST (SVGDocumentTests, DefsElementIsHiddenAndNotInTopLevelElements)
{
    auto doc = parse ("<svg>"
                      "<defs><rect id=\"r\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></defs>"
                      "<circle cx=\"5\" cy=\"5\" r=\"5\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        // defs is parsed as hidden, and its children shouldn't be top-level elements
        // only the circle should appear
        ASSERT_EQ (2u, data.elements.size());
        // defs comes first as a hidden element, circle second
        const auto* defs = data.elements[0].get();
        EXPECT_TRUE (defs->hidden);
        EXPECT_EQ (String ("defs"), defs->tagName);
        EXPECT_EQ (String ("circle"), data.elements[1]->tagName);
    });
}

// ==============================================================================
// SVGData::elementsById
// ==============================================================================

TEST (SVGDocumentTests, ElementWithIdIsRegisteredInById)
{
    auto doc = parse ("<svg><rect id=\"myRect\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.elementsById.contains ("myRect"));
        ASSERT_NE (nullptr, data.elementsById["myRect"].get());
        EXPECT_EQ (String ("rect"), data.elementsById["myRect"]->tagName);
    });
}

TEST (SVGDocumentTests, ElementWithoutIdIsNotInById)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (0, data.elementsById.size());
    });
}

TEST (SVGDocumentTests, MultipleIdsAllRegistered)
{
    auto doc = parse ("<svg>"
                      "<rect id=\"r1\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" />"
                      "<circle id=\"c1\" cx=\"20\" cy=\"20\" r=\"5\" />"
                      "<g id=\"g1\"><line id=\"l1\" x1=\"0\" y1=\"0\" x2=\"10\" y2=\"10\" /></g>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (4, data.elementsById.size());
        EXPECT_TRUE (data.elementsById.contains ("r1"));
        EXPECT_TRUE (data.elementsById.contains ("c1"));
        EXPECT_TRUE (data.elementsById.contains ("g1"));
        EXPECT_TRUE (data.elementsById.contains ("l1"));
    });
}

// ==============================================================================
// SVGElement path geometry
// ==============================================================================

TEST (SVGDocumentTests, RectElementHasPathSet)
{
    auto doc = parse ("<svg><rect x=\"10\" y=\"20\" width=\"80\" height=\"60\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_FALSE (elem.path->isEmpty());
        auto b = elem.path->getBounds();
        EXPECT_FLOAT_EQ (10.0f, b.getX());
        EXPECT_FLOAT_EQ (20.0f, b.getY());
        EXPECT_FLOAT_EQ (80.0f, b.getWidth());
        EXPECT_FLOAT_EQ (60.0f, b.getHeight());
    });
}

TEST (SVGDocumentTests, CircleElementHasPathSet)
{
    auto doc = parse ("<svg><circle cx=\"50\" cy=\"50\" r=\"30\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_FALSE (elem.path->isEmpty());
        auto b = elem.path->getBounds();
        EXPECT_NEAR (20.0f, b.getX(), 0.1f);
        EXPECT_NEAR (20.0f, b.getY(), 0.1f);
        EXPECT_NEAR (60.0f, b.getWidth(), 0.5f);
        EXPECT_NEAR (60.0f, b.getHeight(), 0.5f);
    });
}

TEST (SVGDocumentTests, EllipseElementHasPathSet)
{
    auto doc = parse ("<svg><ellipse cx=\"50\" cy=\"50\" rx=\"40\" ry=\"20\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_FALSE (elem.path->isEmpty());
        auto b = elem.path->getBounds();
        EXPECT_NEAR (10.0f, b.getX(), 0.1f);
        EXPECT_NEAR (30.0f, b.getY(), 0.1f);
        EXPECT_NEAR (80.0f, b.getWidth(), 0.5f);
        EXPECT_NEAR (40.0f, b.getHeight(), 0.5f);
    });
}

TEST (SVGDocumentTests, LineElementHasPathSet)
{
    auto doc = parse ("<svg><line x1=\"10\" y1=\"20\" x2=\"90\" y2=\"80\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_FALSE (elem.path->isEmpty());
    });
}

TEST (SVGDocumentTests, PathElementHasPathSetWithCorrectBounds)
{
    auto doc = parse ("<svg><path d=\"M 10 10 L 90 90\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_FALSE (elem.path->isEmpty());
        auto b = elem.path->getBounds();
        EXPECT_NEAR (10.0f, b.getX(), 0.01f);
        EXPECT_NEAR (10.0f, b.getY(), 0.01f);
        EXPECT_NEAR (80.0f, b.getWidth(), 0.5f);
        EXPECT_NEAR (80.0f, b.getHeight(), 0.5f);
    });
}

TEST (SVGDocumentTests, EmptyPathDAttributeProducesEmptyPath)
{
    auto doc = parse ("<svg><path d=\"\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_TRUE (elem.path->isEmpty());
    });
}

TEST (SVGDocumentTests, PathWithEvenOddFillRuleUsesNonZeroWindingFalse)
{
    auto doc = parse ("<svg><path d=\"M 50 10 L 90 90 L 10 90 Z\" fill-rule=\"evenodd\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_FALSE (elem.path->isUsingNonZeroWinding());
        ASSERT_TRUE (elem.fillRule.has_value());
        EXPECT_EQ (String ("evenodd"), *elem.fillRule);
    });
}

TEST (SVGDocumentTests, PathWithNonZeroFillRuleUsesNonZeroWindingTrue)
{
    auto doc = parse ("<svg><path d=\"M 50 10 L 90 90 L 10 90 Z\" fill-rule=\"nonzero\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.path.has_value());
        EXPECT_TRUE (elem.path->isUsingNonZeroWinding());
    });
}

// ==============================================================================
// SVGElement fill / stroke attributes
// ==============================================================================

TEST (SVGDocumentTests, FillColorAttribute)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"#ff0000\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fillColor.has_value());
        EXPECT_FALSE (elem.noFill);
        EXPECT_FALSE (elem.fillCurrentColor);
    });
}

TEST (SVGDocumentTests, FillNoneSetNoFill)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        EXPECT_TRUE (elem.noFill);
        EXPECT_FALSE (elem.fillColor.has_value());
    });
}

TEST (SVGDocumentTests, FillCurrentColorSetsFillCurrentColor)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"currentColor\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        EXPECT_TRUE (elem.fillCurrentColor);
        EXPECT_FALSE (elem.fillColor.has_value());
        EXPECT_FALSE (elem.noFill);
    });
}

TEST (SVGDocumentTests, FillUrlSetsFillUrl)
{
    auto doc = parse ("<svg>"
                      "<defs><linearGradient id=\"g\"><stop offset=\"0\" stop-color=\"red\" /></linearGradient></defs>"
                      "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"url(#g)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (2u, data.elements.size());
        const auto& elem = *data.elements[1];
        ASSERT_TRUE (elem.fillUrl.has_value());
        EXPECT_EQ (String ("g"), *elem.fillUrl);
    });
}

TEST (SVGDocumentTests, StrokeColorAttribute)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" stroke=\"blue\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.strokeColor.has_value());
        EXPECT_FALSE (elem.noStroke);
    });
}

TEST (SVGDocumentTests, StrokeNoneSetsNoStroke)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" stroke=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        EXPECT_TRUE (elem.noStroke);
        EXPECT_FALSE (elem.strokeColor.has_value());
    });
}

TEST (SVGDocumentTests, StrokeWidthAttribute)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 0\" stroke=\"black\" stroke-width=\"3.5\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.strokeWidth.has_value());
        EXPECT_FLOAT_EQ (3.5f, *elem.strokeWidth);
    });
}

TEST (SVGDocumentTests, StrokeLineJoinRound)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 10 L 20 0\" stroke=\"black\" stroke-linejoin=\"round\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.strokeJoin.has_value());
        EXPECT_EQ (StrokeJoin::Round, *elem.strokeJoin);
    });
}

TEST (SVGDocumentTests, StrokeLineJoinMiter)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 10 L 20 0\" stroke=\"black\" stroke-linejoin=\"miter\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_EQ (StrokeJoin::Miter, *data.elements[0]->strokeJoin);
    });
}

TEST (SVGDocumentTests, StrokeLineJoinBevel)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 10 L 20 0\" stroke=\"black\" stroke-linejoin=\"bevel\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_EQ (StrokeJoin::Bevel, *data.elements[0]->strokeJoin);
    });
}

TEST (SVGDocumentTests, StrokeLineCapButt)
{
    auto doc = parse ("<svg><line x1=\"0\" y1=\"0\" x2=\"10\" y2=\"0\" stroke=\"black\" stroke-linecap=\"butt\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        ASSERT_TRUE (data.elements[0]->strokeCap.has_value());
        EXPECT_EQ (StrokeCap::Butt, *data.elements[0]->strokeCap);
    });
}

TEST (SVGDocumentTests, StrokeLineCapRound)
{
    auto doc = parse ("<svg><line x1=\"0\" y1=\"0\" x2=\"10\" y2=\"0\" stroke=\"black\" stroke-linecap=\"round\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_TRUE (data.elements[0]->strokeCap.has_value());
        EXPECT_EQ (StrokeCap::Round, *data.elements[0]->strokeCap);
    });
}

TEST (SVGDocumentTests, StrokeLineCapSquare)
{
    auto doc = parse ("<svg><line x1=\"0\" y1=\"0\" x2=\"10\" y2=\"0\" stroke=\"black\" stroke-linecap=\"square\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_TRUE (data.elements[0]->strokeCap.has_value());
        EXPECT_EQ (StrokeCap::Square, *data.elements[0]->strokeCap);
    });
}

TEST (SVGDocumentTests, StrokeMiterLimitAttribute)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 10 L 20 0\" stroke=\"black\" stroke-miterlimit=\"2.5\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_FLOAT_EQ (2.5f, data.elements[0]->strokeMiterLimit);
    });
}

TEST (SVGDocumentTests, StrokeMiterLimitBelowOneIsClampedToOne)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 10\" stroke=\"black\" stroke-miterlimit=\"0.1\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_FLOAT_EQ (1.0f, data.elements[0]->strokeMiterLimit);
    });
}

TEST (SVGDocumentTests, DefaultStrokeMiterLimitIsFour)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 10\" stroke=\"black\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_FLOAT_EQ (4.0f, data.elements[0]->strokeMiterLimit);
    });
}

TEST (SVGDocumentTests, StrokeDashArrayAttribute)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 100 0\" stroke=\"black\" stroke-dasharray=\"5 3\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.strokeDashArray.has_value());
        ASSERT_EQ (2, elem.strokeDashArray->size());
        EXPECT_FLOAT_EQ (5.0f, (*elem.strokeDashArray)[0]);
        EXPECT_FLOAT_EQ (3.0f, (*elem.strokeDashArray)[1]);
    });
}

TEST (SVGDocumentTests, StrokeDashOffsetAttribute)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 100 0\" stroke=\"black\" stroke-dasharray=\"5 3\" stroke-dashoffset=\"2\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.strokeDashOffset.has_value());
        EXPECT_FLOAT_EQ (2.0f, *elem.strokeDashOffset);
    });
}

TEST (SVGDocumentTests, OpacityAttribute)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" opacity=\"0.6\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.opacity.has_value());
        EXPECT_FLOAT_EQ (0.6f, *elem.opacity);
    });
}

TEST (SVGDocumentTests, FillOpacityAttribute)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"red\" fill-opacity=\"0.4\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fillOpacity.has_value());
        EXPECT_FLOAT_EQ (0.4f, *elem.fillOpacity);
    });
}

TEST (SVGDocumentTests, StrokeOpacityAttribute)
{
    auto doc = parse ("<svg><path d=\"M 0 0 L 10 0\" stroke=\"blue\" stroke-opacity=\"0.7\" fill=\"none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.strokeOpacity.has_value());
        EXPECT_FLOAT_EQ (0.7f, *elem.strokeOpacity);
    });
}

// ==============================================================================
// SVGElement transform
// ==============================================================================

TEST (SVGDocumentTests, TransformTranslateAttribute)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" transform=\"translate(5 10)\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.transform.has_value());
        EXPECT_FALSE (elem.transform->isIdentity());
    });
}

TEST (SVGDocumentTests, NoTransformAttributeProducesNoTransform)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        EXPECT_FALSE (elem.transform.has_value());
    });
}

// ==============================================================================
// SVGElement visibility
// ==============================================================================

TEST (SVGDocumentTests, SymbolIsHidden)
{
    auto doc = parse ("<svg><symbol id=\"s\" viewBox=\"0 0 10 10\"><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></symbol></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_TRUE (data.elements[0]->hidden);
        EXPECT_TRUE (data.elements[0]->isSymbol);
    });
}

TEST (SVGDocumentTests, DisplayNoneHidesElement)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" style=\"display:none\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_TRUE (data.elements[0]->hidden);
    });
}

TEST (SVGDocumentTests, VisibilityHiddenHidesElement)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" style=\"visibility:hidden\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_TRUE (data.elements[0]->hidden);
    });
}

// ==============================================================================
// SVGElement text
// ==============================================================================

TEST (SVGDocumentTests, TextElementHasTextContent)
{
    auto doc = parse ("<svg><text x=\"10\" y=\"50\">Hello</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        EXPECT_EQ (String ("text"), elem.tagName);
        ASSERT_TRUE (elem.text.has_value());
        EXPECT_EQ (String ("Hello"), *elem.text);
        ASSERT_TRUE (elem.textPosition.has_value());
        EXPECT_FLOAT_EQ (10.0f, elem.textPosition->getX());
        EXPECT_FLOAT_EQ (50.0f, elem.textPosition->getY());
    });
}

TEST (SVGDocumentTests, TextElementFontSize)
{
    auto doc = parse ("<svg><text x=\"0\" y=\"0\" font-size=\"18\">Test</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fontSize.has_value());
        EXPECT_FLOAT_EQ (18.0f, *elem.fontSize);
    });
}

TEST (SVGDocumentTests, TextElementFontFamily)
{
    auto doc = parse ("<svg><text x=\"0\" y=\"0\" font-family=\"Arial\">Test</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fontFamily.has_value());
        EXPECT_EQ (String ("Arial"), *elem.fontFamily);
    });
}

TEST (SVGDocumentTests, TextElementFontWeightBold)
{
    auto doc = parse ("<svg><text x=\"0\" y=\"0\" font-weight=\"bold\">Bold</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fontWeight.has_value());
        EXPECT_EQ (700, *elem.fontWeight);
    });
}

TEST (SVGDocumentTests, TextElementFontWeightNumeric)
{
    auto doc = parse ("<svg><text x=\"0\" y=\"0\" style=\"font-weight: 600\">Semi-bold</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fontWeight.has_value());
        EXPECT_EQ (600, *elem.fontWeight);
    });
}

TEST (SVGDocumentTests, TextElementItalic)
{
    auto doc = parse ("<svg><text x=\"0\" y=\"0\" style=\"font-style: italic\">Italic</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.fontItalic.has_value());
        EXPECT_TRUE (*elem.fontItalic);
    });
}

TEST (SVGDocumentTests, TextElementTextAnchor)
{
    auto doc = parse ("<svg><text x=\"50\" y=\"50\" text-anchor=\"middle\">Center</text></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.textAnchor.has_value());
        EXPECT_EQ (String ("middle"), *elem.textAnchor);
    });
}

TEST (SVGDocumentTests, TspanChildPositionResolvedFromParent)
{
    auto doc = parse (
        "<svg>"
        "<text x=\"10\" y=\"20\">"
        "<tspan x=\"10\" dy=\"0\">Line 1</tspan>"
        "<tspan x=\"10\" dy=\"15\">Line 2</tspan>"
        "</text>"
        "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& textElem = *data.elements[0];
        EXPECT_EQ (String ("text"), textElem.tagName);
        ASSERT_EQ (2u, textElem.children.size());

        const auto& tspan1 = *textElem.children[0];
        EXPECT_EQ (String ("tspan"), tspan1.tagName);
        ASSERT_TRUE (tspan1.textPosition.has_value());
        EXPECT_FLOAT_EQ (10.0f, tspan1.textPosition->getX());
        EXPECT_FLOAT_EQ (20.0f, tspan1.textPosition->getY());

        const auto& tspan2 = *textElem.children[1];
        EXPECT_EQ (String ("tspan"), tspan2.tagName);
        ASSERT_TRUE (tspan2.textPosition.has_value());
        EXPECT_FLOAT_EQ (10.0f, tspan2.textPosition->getX());
        EXPECT_FLOAT_EQ (35.0f, tspan2.textPosition->getY());
    });
}

// ==============================================================================
// SVGElement use / reference
// ==============================================================================

TEST (SVGDocumentTests, UseElementHasReferenceSet)
{
    auto doc = parse ("<svg>"
                      "<defs><rect id=\"r\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></defs>"
                      "<use href=\"#r\" x=\"20\" y=\"30\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (2u, data.elements.size());
        const auto& useElem = *data.elements[1];
        EXPECT_EQ (String ("use"), useElem.tagName);
        ASSERT_TRUE (useElem.reference.has_value());
        EXPECT_EQ (String ("r"), *useElem.reference);
    });
}

TEST (SVGDocumentTests, UseElementTranslationEmbeddedInTransform)
{
    auto doc = parse ("<svg>"
                      "<defs><rect id=\"r\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></defs>"
                      "<use href=\"#r\" x=\"15\" y=\"25\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (2u, data.elements.size());
        const auto& useElem = *data.elements[1];
        // x/y on <use> is baked into a translate transform
        ASSERT_TRUE (useElem.transform.has_value());
        EXPECT_FALSE (useElem.transform->isIdentity());
    });
}

TEST (SVGDocumentTests, UseElementWithNoHrefHasNoReference)
{
    auto doc = parse ("<svg><use x=\"10\" y=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& useElem = *data.elements[0];
        EXPECT_FALSE (useElem.reference.has_value());
    });
}

// ==============================================================================
// SVGElement viewBox and viewport
// ==============================================================================

TEST (SVGDocumentTests, NestedSVGHasViewBoxSet)
{
    auto doc = parse ("<svg viewBox=\"0 0 100 100\">"
                      "<svg x=\"10\" y=\"10\" viewBox=\"0 0 50 50\" width=\"40\" height=\"40\">"
                      "<circle cx=\"25\" cy=\"25\" r=\"20\" />"
                      "</svg>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& svgElem = *data.elements[0];
        EXPECT_EQ (String ("svg"), svgElem.tagName);
        ASSERT_TRUE (svgElem.viewBox.has_value());
        EXPECT_FLOAT_EQ (50.0f, svgElem.viewBox->getWidth());
        EXPECT_FLOAT_EQ (50.0f, svgElem.viewBox->getHeight());
        ASSERT_TRUE (svgElem.viewportBounds.has_value());
        EXPECT_FLOAT_EQ (10.0f, svgElem.viewportBounds->getX());
        EXPECT_FLOAT_EQ (10.0f, svgElem.viewportBounds->getY());
        EXPECT_FLOAT_EQ (40.0f, svgElem.viewportBounds->getWidth());
        EXPECT_FLOAT_EQ (40.0f, svgElem.viewportBounds->getHeight());
        EXPECT_FALSE (svgElem.viewportSize.has_value());
    });
}

TEST (SVGDocumentTests, SymbolHasViewBoxAndIsSymbolFlagSet)
{
    auto doc = parse ("<svg>"
                      "<symbol id=\"sym\" viewBox=\"0 0 20 10\">"
                      "<rect x=\"0\" y=\"0\" width=\"20\" height=\"10\" />"
                      "</symbol>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& sym = *data.elements[0];
        EXPECT_TRUE (sym.isSymbol);
        ASSERT_TRUE (sym.viewBox.has_value());
        EXPECT_FLOAT_EQ (20.0f, sym.viewBox->getWidth());
        EXPECT_FLOAT_EQ (10.0f, sym.viewBox->getHeight());
    });
}

// ==============================================================================
// SVGElement markers, mask, clipPath URLs
// ==============================================================================

TEST (SVGDocumentTests, MarkerStartAttributeStoredOnElement)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"m\"></marker></defs>"
                      "<path d=\"M 0 0 L 10 0\" stroke=\"black\" fill=\"none\" marker-start=\"url(#m)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        const auto* pathElem = data.elementsById["path_id"].get();
        // Find path in top-level elements
        for (const auto& e : data.elements)
        {
            if (e->tagName == "path")
            {
                ASSERT_TRUE (e->markerStart.has_value());
                EXPECT_EQ (String ("m"), *e->markerStart);
                return;
            }
        }
        ADD_FAILURE() << "path element not found";
    });
}

TEST (SVGDocumentTests, MarkerEndAttributeStoredOnElement)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"arrow\"></marker></defs>"
                      "<path d=\"M 0 0 L 10 0\" stroke=\"black\" fill=\"none\" marker-end=\"url(#arrow)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        for (const auto& e : data.elements)
        {
            if (e->tagName == "path")
            {
                ASSERT_TRUE (e->markerEnd.has_value());
                EXPECT_EQ (String ("arrow"), *e->markerEnd);
                return;
            }
        }
        ADD_FAILURE() << "path element not found";
    });
}

TEST (SVGDocumentTests, MarkerShorthandSetsAllThreeMarkers)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"dot\"></marker></defs>"
                      "<polyline points=\"0,0 10,10 20,0\" stroke=\"black\" fill=\"none\" marker=\"url(#dot)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        for (const auto& e : data.elements)
        {
            if (e->tagName == "polyline")
            {
                ASSERT_TRUE (e->markerStart.has_value());
                ASSERT_TRUE (e->markerMid.has_value());
                ASSERT_TRUE (e->markerEnd.has_value());
                EXPECT_EQ (String ("dot"), *e->markerStart);
                EXPECT_EQ (String ("dot"), *e->markerMid);
                EXPECT_EQ (String ("dot"), *e->markerEnd);
                return;
            }
        }
        ADD_FAILURE() << "polyline element not found";
    });
}

TEST (SVGDocumentTests, MaskUrlAttributeStoredOnElement)
{
    auto doc = parse ("<svg>"
                      "<defs><mask id=\"fade\"></mask></defs>"
                      "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" mask=\"url(#fade)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        for (const auto& e : data.elements)
        {
            if (e->tagName == "rect")
            {
                ASSERT_TRUE (e->maskUrl.has_value());
                EXPECT_EQ (String ("fade"), *e->maskUrl);
                return;
            }
        }
        ADD_FAILURE() << "rect element not found";
    });
}

TEST (SVGDocumentTests, ClipPathUrlAttributeStoredOnElement)
{
    auto doc = parse ("<svg>"
                      "<defs><clipPath id=\"clip\"><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></clipPath></defs>"
                      "<circle cx=\"5\" cy=\"5\" r=\"5\" clip-path=\"url(#clip)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        for (const auto& e : data.elements)
        {
            if (e->tagName == "circle")
            {
                ASSERT_TRUE (e->clipPathUrl.has_value());
                EXPECT_EQ (String ("clip"), *e->clipPathUrl);
                return;
            }
        }
        ADD_FAILURE() << "circle element not found";
    });
}

TEST (SVGDocumentTests, BlendModeStoredOnElement)
{
    auto doc = parse ("<svg>"
                      "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"blue\" style=\"mix-blend-mode: multiply\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        ASSERT_TRUE (elem.blendMode.has_value());
        EXPECT_EQ (BlendMode::Multiply, *elem.blendMode);
    });
}

// ==============================================================================
// SVGData::gradients
// ==============================================================================

TEST (SVGDocumentTests, LinearGradientParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"lg\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "<stop offset=\"1\" stop-color=\"blue\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        const auto& g = *data.gradients[0];
        EXPECT_EQ (SVGGradient::Linear, g.type);
        EXPECT_EQ (String ("lg"), g.id);
        ASSERT_EQ (2u, g.stops.size());
        EXPECT_FLOAT_EQ (0.0f, g.stops[0].offset);
        EXPECT_FLOAT_EQ (1.0f, g.stops[1].offset);
    });
}

TEST (SVGDocumentTests, LinearGradientRegisteredById)
{
    auto doc = parse ("<svg>"
                      "<defs><linearGradient id=\"myGrad\"><stop offset=\"0\" stop-color=\"green\" /></linearGradient></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.gradientsById.contains ("myGrad"));
        ASSERT_NE (nullptr, data.gradientsById["myGrad"].get());
        EXPECT_EQ (SVGGradient::Linear, data.gradientsById["myGrad"]->type);
    });
}

TEST (SVGDocumentTests, RadialGradientType)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<radialGradient id=\"rg\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\">"
                      "<stop offset=\"0\" stop-color=\"white\" />"
                      "<stop offset=\"1\" stop-color=\"black\" />"
                      "</radialGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        EXPECT_EQ (SVGGradient::Radial, data.gradients[0]->type);
    });
}

TEST (SVGDocumentTests, GradientUserSpaceOnUseUnits)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"u\" gradientUnits=\"userSpaceOnUse\" x1=\"0\" y1=\"0\" x2=\"100\" y2=\"0\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        EXPECT_EQ (SVGGradient::UserSpaceOnUse, data.gradients[0]->units);
        EXPECT_TRUE (data.gradients[0]->hasUnits);
    });
}

TEST (SVGDocumentTests, GradientObjectBoundingBoxUnits)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"obb\" gradientUnits=\"objectBoundingBox\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        EXPECT_EQ (SVGGradient::ObjectBoundingBox, data.gradients[0]->units);
    });
}

TEST (SVGDocumentTests, GradientSpreadMethodPad)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"g\" spreadMethod=\"pad\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        EXPECT_EQ (String ("pad"), data.gradients[0]->spreadMethod);
    });
}

TEST (SVGDocumentTests, GradientSpreadMethodReflect)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"g\" spreadMethod=\"reflect\">"
                      "<stop offset=\"0\" stop-color=\"red\" /></linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        EXPECT_EQ (String ("reflect"), data.gradients[0]->spreadMethod);
    });
}

TEST (SVGDocumentTests, GradientStopOpacity)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"g\">"
                      "<stop offset=\"0\" stop-color=\"red\" stop-opacity=\"0.5\" />"
                      "<stop offset=\"1\" stop-color=\"blue\" stop-opacity=\"0.9\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        const auto& g = *data.gradients[0];
        ASSERT_EQ (2u, g.stops.size());
        EXPECT_FLOAT_EQ (0.5f, g.stops[0].opacity);
        EXPECT_FLOAT_EQ (0.9f, g.stops[1].opacity);
    });
}

TEST (SVGDocumentTests, LinearGradientStartEndCoordinates)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"g\" x1=\"0.1\" y1=\"0.2\" x2=\"0.8\" y2=\"0.9\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        const auto& g = *data.gradients[0];
        EXPECT_TRUE (g.hasStart);
        EXPECT_TRUE (g.hasEnd);
        EXPECT_NEAR (0.1f, g.start.getX(), 0.001f);
        EXPECT_NEAR (0.2f, g.start.getY(), 0.001f);
        EXPECT_NEAR (0.8f, g.end.getX(), 0.001f);
        EXPECT_NEAR (0.9f, g.end.getY(), 0.001f);
    });
}

TEST (SVGDocumentTests, RadialGradientCenterAndRadius)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<radialGradient id=\"rg\" cx=\"0.4\" cy=\"0.6\" r=\"0.3\">"
                      "<stop offset=\"0\" stop-color=\"white\" />"
                      "</radialGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        const auto& g = *data.gradients[0];
        EXPECT_TRUE (g.hasCenter);
        EXPECT_TRUE (g.hasRadius);
        EXPECT_NEAR (0.4f, g.center.getX(), 0.001f);
        EXPECT_NEAR (0.6f, g.center.getY(), 0.001f);
        EXPECT_NEAR (0.3f, g.radius, 0.001f);
    });
}

TEST (SVGDocumentTests, RadialGradientFocalPoint)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<radialGradient id=\"rg\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\" fx=\"0.3\" fy=\"0.4\">"
                      "<stop offset=\"0\" stop-color=\"yellow\" />"
                      "</radialGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        const auto& g = *data.gradients[0];
        EXPECT_TRUE (g.hasFocal);
        EXPECT_NEAR (0.3f, g.focal.getX(), 0.001f);
        EXPECT_NEAR (0.4f, g.focal.getY(), 0.001f);
    });
}

TEST (SVGDocumentTests, GradientHrefResolvesStops)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"base\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "<stop offset=\"1\" stop-color=\"blue\" />"
                      "</linearGradient>"
                      "<linearGradient id=\"derived\" href=\"#base\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\" />"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (2u, data.gradients.size());
        EXPECT_TRUE (data.gradientsById.contains ("derived"));
        const auto& derived = *data.gradientsById["derived"];
        // href stored as string
        EXPECT_EQ (String ("base"), derived.href);
    });
}

TEST (SVGDocumentTests, MultipleGradientsBothStored)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"g1\"><stop offset=\"0\" stop-color=\"red\" /></linearGradient>"
                      "<radialGradient id=\"g2\"><stop offset=\"0\" stop-color=\"blue\" /></radialGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (2u, data.gradients.size());
        EXPECT_TRUE (data.gradientsById.contains ("g1"));
        EXPECT_TRUE (data.gradientsById.contains ("g2"));
    });
}

// ==============================================================================
// SVGData::filters
// ==============================================================================

TEST (SVGDocumentTests, FilterParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs><filter id=\"blur\"><feGaussianBlur stdDeviation=\"4\" /></filter></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.filters.size());
        const auto& f = *data.filters[0];
        EXPECT_EQ (String ("blur"), f.id);
        ASSERT_EQ (1u, f.primitives.size());
        auto blur = dynamic_cast<const SVGFEGaussianBlur*> (f.primitives[0].get());
        ASSERT_NE (nullptr, blur);
        EXPECT_FLOAT_EQ (4.0f, blur->stdDeviation);
    });
}

TEST (SVGDocumentTests, FEBlendParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs><filter id=\"f\"><feBlend mode=\"multiply\" in=\"SourceGraphic\" in2=\"SourceGraphic\" /></filter></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.filters.size());
        const auto& f = *data.filters[0];
        EXPECT_EQ (String ("f"), f.id);
        ASSERT_EQ (1u, f.primitives.size());
        auto blend = dynamic_cast<const SVGFEBlend*> (f.primitives[0].get());
        ASSERT_NE (nullptr, blend);
        EXPECT_EQ (BlendMode::Multiply, blend->mode);
        EXPECT_EQ (String ("SourceGraphic"), blend->in);
        EXPECT_EQ (String ("SourceGraphic"), blend->in2);
    });
}

TEST (SVGDocumentTests, FEBlendAllModes)
{
    struct ModeTest
    {
        const char* modeStr;
        BlendMode expectedMode;
    };

    static const ModeTest tests[] = {
        { "normal", BlendMode::SrcOver },
        { "multiply", BlendMode::Multiply },
        { "screen", BlendMode::Screen },
        { "overlay", BlendMode::Overlay },
        { "darken", BlendMode::Darken },
        { "lighten", BlendMode::Lighten },
        { "color-dodge", BlendMode::ColorDodge },
        { "color-burn", BlendMode::ColorBurn },
        { "hard-light", BlendMode::HardLight },
        { "soft-light", BlendMode::SoftLight },
        { "difference", BlendMode::Difference },
        { "exclusion", BlendMode::Exclusion },
        { "hue", BlendMode::Hue },
        { "saturation", BlendMode::Saturation },
        { "color", BlendMode::Color },
        { "luminosity", BlendMode::Luminosity },
    };

    for (const auto& t : tests)
    {
        String svg = "<svg><defs><filter id=\"f\"><feBlend mode=\"";
        svg += t.modeStr;
        svg += "\" /></filter></defs></svg>";

        auto doc = parse (svg.toRawUTF8());
        ASSERT_NE (nullptr, doc) << t.modeStr;

        doc->visit ([&] (const SVGData& data)
        {
            ASSERT_EQ (1u, data.filters.size());
            ASSERT_EQ (1u, data.filters[0]->primitives.size());
            auto blend = dynamic_cast<const SVGFEBlend*> (data.filters[0]->primitives[0].get());
            ASSERT_NE (nullptr, blend) << t.modeStr;
            EXPECT_EQ (t.expectedMode, blend->mode) << t.modeStr;
        });
    }
}

TEST (SVGDocumentTests, FEBlendDefaultsInWhenEmpty)
{
    auto doc = parse ("<svg>"
                      "<defs><filter id=\"f\"><feBlend mode=\"screen\" /></filter></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.filters.size());
        ASSERT_EQ (1u, data.filters[0]->primitives.size());
        auto blend = dynamic_cast<const SVGFEBlend*> (data.filters[0]->primitives[0].get());
        ASSERT_NE (nullptr, blend);
        EXPECT_EQ (String ("SourceGraphic"), blend->in);
    });
}

TEST (SVGDocumentTests, FilterChainMultiplePrimitives)
{
    auto doc = parse ("<svg>"
                      "<defs><filter id=\"f\">"
                      "<feGaussianBlur stdDeviation=\"3\" result=\"blur\" />"
                      "<feBlend mode=\"multiply\" in=\"SourceGraphic\" in2=\"blur\" />"
                      "</filter></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.filters.size());
        const auto& f = *data.filters[0];
        ASSERT_EQ (2u, f.primitives.size());

        auto blur = dynamic_cast<const SVGFEGaussianBlur*> (f.primitives[0].get());
        ASSERT_NE (nullptr, blur);
        EXPECT_FLOAT_EQ (3.0f, blur->stdDeviation);
        EXPECT_EQ (String ("blur"), blur->result);

        auto blend = dynamic_cast<const SVGFEBlend*> (f.primitives[1].get());
        ASSERT_NE (nullptr, blend);
        EXPECT_EQ (BlendMode::Multiply, blend->mode);
        EXPECT_EQ (String ("SourceGraphic"), blend->in);
        EXPECT_EQ (String ("blur"), blend->in2);
    });
}

TEST (SVGDocumentTests, FilterRegisteredById)
{
    auto doc = parse ("<svg>"
                      "<defs><filter id=\"myFilter\"><feGaussianBlur stdDeviation=\"2\" /></filter></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.filtersById.contains ("myFilter"));
    });
}

TEST (SVGDocumentTests, FilterOnElementStoredAsUrl)
{
    auto doc = parse ("<svg>"
                      "<defs><filter id=\"f\"><feGaussianBlur stdDeviation=\"3\" /></filter></defs>"
                      "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" filter=\"url(#f)\" />"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        for (const auto& e : data.elements)
        {
            if (e->tagName == "rect")
            {
                ASSERT_TRUE (e->filterUrl.has_value());
                EXPECT_EQ (String ("f"), *e->filterUrl);
                return;
            }
        }
        ADD_FAILURE() << "rect not found";
    });
}

// ==============================================================================
// Gradient parsing edge cases
// ==============================================================================

TEST (SVGDocumentTests, GradientUnitsHandlesInkscapeNamespacePrefix)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<linearGradient id=\"g1\" gradientUnits=\"xuserSpaceOnUse\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">"
                      "<stop offset=\"0\" stop-color=\"red\" />"
                      "<stop offset=\"1\" stop-color=\"blue\" />"
                      "</linearGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        EXPECT_EQ (SVGGradient::UserSpaceOnUse, data.gradients[0]->units);
    });
}

TEST (SVGDocumentTests, RadialGradientWithFocalPoint)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<radialGradient id=\"rg\" cx=\"0.5\" cy=\"0.5\" r=\"0.5\" fx=\"0.3\" fy=\"0.3\">"
                      "<stop offset=\"0\" stop-color=\"yellow\" />"
                      "<stop offset=\"1\" stop-color=\"red\" />"
                      "</radialGradient>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.gradients.size());
        const auto& g = *data.gradients[0];
        EXPECT_TRUE (g.hasFocal);
        EXPECT_FLOAT_EQ (0.3f, g.focal.getX());
        EXPECT_FLOAT_EQ (0.3f, g.focal.getY());
    });
}

// ==============================================================================
// SVGData::clipPaths
// ==============================================================================

TEST (SVGDocumentTests, ClipPathParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<clipPath id=\"cp\">"
                      "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" />"
                      "</clipPath>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.clipPaths.size());
        const auto& cp = *data.clipPaths[0];
        EXPECT_EQ (String ("cp"), cp.id);
        EXPECT_EQ (1u, cp.elements.size());
        EXPECT_EQ (String ("rect"), cp.elements[0]->tagName);
    });
}

TEST (SVGDocumentTests, ClipPathRegisteredById)
{
    auto doc = parse ("<svg>"
                      "<defs><clipPath id=\"myClip\"><circle cx=\"5\" cy=\"5\" r=\"5\" /></clipPath></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.clipPathsById.contains ("myClip"));
    });
}

TEST (SVGDocumentTests, ClipPathDefaultUnitsUserSpaceOnUse)
{
    auto doc = parse ("<svg>"
                      "<defs><clipPath id=\"cp\"><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></clipPath></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.clipPaths.size());
        EXPECT_EQ (SVGClipPath::UserSpaceOnUse, data.clipPaths[0]->units);
    });
}

TEST (SVGDocumentTests, ClipPathWithMultipleElements)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<clipPath id=\"multi\">"
                      "<rect x=\"0\" y=\"0\" width=\"50\" height=\"100\" />"
                      "<rect x=\"50\" y=\"0\" width=\"50\" height=\"100\" />"
                      "</clipPath>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.clipPaths.size());
        EXPECT_EQ (2u, data.clipPaths[0]->elements.size());
    });
}

TEST (SVGDocumentTests, ClipRuleOnClipPathChildElement)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<clipPath id=\"cp\">"
                      "<path d=\"M 50 10 L 90 90 L 10 90 Z\" clip-rule=\"evenodd\" />"
                      "</clipPath>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.clipPaths.size());
        const auto& cp = *data.clipPaths[0];
        ASSERT_EQ (1u, cp.elements.size());
        ASSERT_TRUE (cp.elements[0]->clipRule.has_value());
        EXPECT_EQ (String ("evenodd"), *cp.elements[0]->clipRule);
    });
}

TEST (SVGDocumentTests, ClipPathRegistersChildIds)
{
    auto doc = parse ("<svg>"
                      "<defs><clipPath id=\"cp\"><circle id=\"c1\" cx=\"10\" cy=\"10\" r=\"5\" /></clipPath></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.elementsById.contains ("c1"));
        auto elem = data.elementsById["c1"];
        ASSERT_NE (nullptr, elem);
        ASSERT_TRUE (elem->path.has_value());
    });
}

TEST (SVGDocumentTests, ClipPathWithUseElement)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<clipPath id=\"star\"><polygon id=\"starShape\" points=\"100,10 190,180 10,60 190,60 10,180\" /></clipPath>"
                      "<clipPath id=\"union\">"
                      "<use xlink:href=\"#starShape\" />"
                      "<circle id=\"circ\" cx=\"100\" cy=\"100\" r=\"50\" />"
                      "</clipPath>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        auto unionCp = data.clipPathsById["union"];
        ASSERT_NE (nullptr, unionCp);
        ASSERT_EQ (2u, unionCp->elements.size());
        EXPECT_TRUE (unionCp->elements[0]->path.has_value());
        EXPECT_TRUE (unionCp->elements[1]->path.has_value());
    });
}

TEST (SVGDocumentTests, ClipPathWithNestedClipPath)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<clipPath id=\"outer\"><circle cx=\"50\" cy=\"50\" r=\"30\" /></clipPath>"
                      "<clipPath id=\"inner\" clip-path=\"url(#outer)\"><circle cx=\"50\" cy=\"50\" r=\"20\" /></clipPath>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        auto innerCp = data.clipPathsById["inner"];
        ASSERT_NE (nullptr, innerCp);
        ASSERT_TRUE (innerCp->clipPathUrl.has_value());
        EXPECT_EQ (String ("outer"), *innerCp->clipPathUrl);
        ASSERT_EQ (1u, innerCp->elements.size());
    });
}

// ==============================================================================
// SVGData::masks
// ==============================================================================

TEST (SVGDocumentTests, MaskParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<mask id=\"m\">"
                      "<circle cx=\"50\" cy=\"50\" r=\"40\" fill=\"white\" />"
                      "</mask>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.masks.size());
        const auto& m = *data.masks[0];
        EXPECT_EQ (String ("m"), m.id);
        EXPECT_EQ (1u, m.elements.size());
        EXPECT_EQ (String ("circle"), m.elements[0]->tagName);
    });
}

TEST (SVGDocumentTests, MaskRegisteredById)
{
    auto doc = parse ("<svg>"
                      "<defs><mask id=\"fadeMask\"><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"white\" /></mask></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.masksById.contains ("fadeMask"));
    });
}

TEST (SVGDocumentTests, MaskDefaultUnitsObjectBoundingBox)
{
    auto doc = parse ("<svg>"
                      "<defs><mask id=\"m\"><rect fill=\"white\" /></mask></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.masks.size());
        EXPECT_EQ (SVGMask::ObjectBoundingBox, data.masks[0]->maskUnits);
    });
}

TEST (SVGDocumentTests, MaskUserSpaceOnUseUnits)
{
    auto doc = parse ("<svg>"
                      "<defs><mask id=\"m\" maskUnits=\"userSpaceOnUse\"><rect fill=\"white\" /></mask></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.masks.size());
        EXPECT_EQ (SVGMask::UserSpaceOnUse, data.masks[0]->maskUnits);
    });
}

TEST (SVGDocumentTests, MaskObjectBoundingBoxUnitsExplicit)
{
    auto doc = parse ("<svg>"
                      "<defs><mask id=\"m\" maskUnits=\"objectBoundingBox\"><rect fill=\"white\" /></mask></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.masks.size());
        EXPECT_EQ (SVGMask::ObjectBoundingBox, data.masks[0]->maskUnits);
    });
}

// ==============================================================================
// SVGData::markers
// ==============================================================================

TEST (SVGDocumentTests, MarkerParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<marker id=\"arrow\" markerWidth=\"10\" markerHeight=\"7\" refX=\"10\" refY=\"3.5\" orient=\"auto\">"
                      "<polygon points=\"0 0, 10 3.5, 0 7\" fill=\"black\" />"
                      "</marker>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        const auto& m = *data.markers[0];
        EXPECT_EQ (String ("arrow"), m.id);
        EXPECT_FLOAT_EQ (10.0f, m.markerWidth);
        EXPECT_FLOAT_EQ (7.0f, m.markerHeight);
        EXPECT_FLOAT_EQ (10.0f, m.refX);
        EXPECT_FLOAT_EQ (3.5f, m.refY);
        EXPECT_EQ (1u, m.elements.size());
    });
}

TEST (SVGDocumentTests, MarkerRegisteredById)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"dot\"><circle cx=\"2\" cy=\"2\" r=\"2\" fill=\"red\" /></marker></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.markersById.contains ("dot"));
    });
}

TEST (SVGDocumentTests, MarkerOrientAutoProducesNullopt)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"m\" orient=\"auto\"></marker></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        const auto& m = *data.markers[0];
        EXPECT_FALSE (m.orient.has_value());
        EXPECT_FALSE (m.orientAutoStartReverse);
    });
}

TEST (SVGDocumentTests, MarkerOrientAutoStartReverseSetFlag)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"m\" orient=\"auto-start-reverse\"></marker></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        const auto& m = *data.markers[0];
        EXPECT_TRUE (m.orientAutoStartReverse);
        EXPECT_FALSE (m.orient.has_value());
    });
}

TEST (SVGDocumentTests, MarkerOrientFixedAngle)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"m\" orient=\"45\"></marker></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        const auto& m = *data.markers[0];
        ASSERT_TRUE (m.orient.has_value());
        EXPECT_FLOAT_EQ (45.0f, *m.orient);
        EXPECT_FALSE (m.orientAutoStartReverse);
    });
}

TEST (SVGDocumentTests, MarkerDefaultUnitsStrokeWidth)
{
    auto doc = parse ("<svg><defs><marker id=\"m\"></marker></defs></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        EXPECT_EQ (SVGMarker::StrokeWidth, data.markers[0]->markerUnits);
    });
}

TEST (SVGDocumentTests, MarkerUserSpaceOnUseUnits)
{
    auto doc = parse ("<svg>"
                      "<defs><marker id=\"m\" markerUnits=\"userSpaceOnUse\"></marker></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        EXPECT_EQ (SVGMarker::UserSpaceOnUse, data.markers[0]->markerUnits);
    });
}

TEST (SVGDocumentTests, MarkerViewBoxParsed)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<marker id=\"m\" viewBox=\"0 0 10 10\" markerWidth=\"4\" markerHeight=\"4\" refX=\"5\" refY=\"5\">"
                      "<circle cx=\"5\" cy=\"5\" r=\"4\" fill=\"red\" />"
                      "</marker>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.markers.size());
        const auto& m = *data.markers[0];
        ASSERT_TRUE (m.viewBox.has_value());
        EXPECT_FLOAT_EQ (10.0f, m.viewBox->getWidth());
        EXPECT_FLOAT_EQ (10.0f, m.viewBox->getHeight());
    });
}

// ==============================================================================
// SVGData::patterns
// ==============================================================================

TEST (SVGDocumentTests, PatternParsedAndStored)
{
    auto doc = parse ("<svg>"
                      "<defs>"
                      "<pattern id=\"p\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\">"
                      "<rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" fill=\"blue\" />"
                      "</pattern>"
                      "</defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.patterns.size());
        const auto& p = *data.patterns[0];
        EXPECT_EQ (String ("p"), p.id);
        EXPECT_FLOAT_EQ (20.0f, p.width);
        EXPECT_FLOAT_EQ (20.0f, p.height);
        EXPECT_EQ (1u, p.elements.size());
    });
}

TEST (SVGDocumentTests, PatternRegisteredById)
{
    auto doc = parse ("<svg>"
                      "<defs><pattern id=\"myPat\" width=\"10\" height=\"10\"><rect width=\"10\" height=\"10\" fill=\"red\" /></pattern></defs>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.patternsById.contains ("myPat"));
    });
}

TEST (SVGDocumentTests, PatternDefaultUnitsObjectBoundingBox)
{
    auto doc = parse ("<svg><defs><pattern id=\"p\" width=\"0.1\" height=\"0.1\"></pattern></defs></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.patterns.size());
        EXPECT_EQ (SVGPattern::ObjectBoundingBox, data.patterns[0]->patternUnits);
    });
}

TEST (SVGDocumentTests, PatternUserSpaceOnUseUnits)
{
    auto doc = parse ("<svg><defs>"
                      "<pattern id=\"p\" patternUnits=\"userSpaceOnUse\" width=\"30\" height=\"30\"></pattern>"
                      "</defs></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.patterns.size());
        EXPECT_EQ (SVGPattern::UserSpaceOnUse, data.patterns[0]->patternUnits);
    });
}

TEST (SVGDocumentTests, PatternViewBox)
{
    auto doc = parse ("<svg><defs>"
                      "<pattern id=\"p\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\" viewBox=\"0 0 10 10\">"
                      "<circle cx=\"5\" cy=\"5\" r=\"4\" fill=\"green\" />"
                      "</pattern></defs></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.patterns.size());
        const auto& p = *data.patterns[0];
        ASSERT_TRUE (p.viewBox.has_value());
        EXPECT_FLOAT_EQ (10.0f, p.viewBox->getWidth());
        EXPECT_FLOAT_EQ (10.0f, p.viewBox->getHeight());
    });
}

TEST (SVGDocumentTests, PatternHrefResolvedAfterParsing)
{
    auto doc = parse ("<svg><defs>"
                      "<pattern id=\"base\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\">"
                      "<rect x=\"2\" y=\"2\" width=\"16\" height=\"16\" fill=\"teal\" />"
                      "</pattern>"
                      "<pattern id=\"derived\" href=\"#base\" />"
                      "</defs></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.patternsById.contains ("derived"));
        const auto& derived = *data.patternsById["derived"];
        // After resolvePatternHrefs, derived should have inherited elements from base
        EXPECT_FALSE (derived.elements.empty());
        EXPECT_FLOAT_EQ (20.0f, derived.width);
        EXPECT_FLOAT_EQ (20.0f, derived.height);
    });
}

TEST (SVGDocumentTests, PatternWithTransform)
{
    auto doc = parse ("<svg><defs>"
                      "<pattern id=\"p\" patternUnits=\"userSpaceOnUse\" width=\"20\" height=\"20\""
                      " patternTransform=\"rotate(45)\">"
                      "<line x1=\"0\" y1=\"0\" x2=\"20\" y2=\"20\" stroke=\"black\" />"
                      "</pattern></defs></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.patterns.size());
        const auto& p = *data.patterns[0];
        EXPECT_FALSE (p.patternTransform.isIdentity());
    });
}

// ==============================================================================
// SVGData::cssRules
// ==============================================================================

TEST (SVGDocumentTests, CSSTypeRuleParsed)
{
    auto doc = parse ("<svg>"
                      "<style>rect { fill: green; stroke: black; }</style>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.cssRules.size());
        const auto& rule = data.cssRules[0];
        EXPECT_EQ (String ("rect"), rule.selector);
        EXPECT_EQ (1, rule.specificity);
        EXPECT_EQ (2, rule.declarations.size());
    });
}

TEST (SVGDocumentTests, CSSIdRuleHasSpecificity100)
{
    auto doc = parse ("<svg>"
                      "<style>#myId { fill: red; }</style>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.cssRules.size());
        EXPECT_EQ (100, data.cssRules[0].specificity);
    });
}

TEST (SVGDocumentTests, CSSClassRuleHasSpecificity10)
{
    auto doc = parse ("<svg>"
                      "<style>.myClass { fill: blue; }</style>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.cssRules.size());
        EXPECT_EQ (10, data.cssRules[0].specificity);
    });
}

TEST (SVGDocumentTests, CSSMultipleRulesPreserveOrder)
{
    auto doc = parse ("<svg>"
                      "<style>rect { fill: red; } circle { fill: blue; } .x { fill: green; }</style>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (3u, data.cssRules.size());
        EXPECT_EQ (String ("rect"), data.cssRules[0].selector);
        EXPECT_EQ (String ("circle"), data.cssRules[1].selector);
        EXPECT_EQ (String (".x"), data.cssRules[2].selector);
        EXPECT_EQ (0, data.cssRules[0].order);
        EXPECT_EQ (1, data.cssRules[1].order);
        EXPECT_EQ (2, data.cssRules[2].order);
    });
}

TEST (SVGDocumentTests, CSSRulesFromMultipleStyleElements)
{
    auto doc = parse ("<svg>"
                      "<style>rect { fill: red; }</style>"
                      "<g>"
                      "<style>circle { fill: blue; }</style>"
                      "<rect /><circle />"
                      "</g>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        EXPECT_EQ (2u, data.cssRules.size());
    });
}

TEST (SVGDocumentTests, CSSRuleDeclarationsContainPropertyColon)
{
    auto doc = parse ("<svg>"
                      "<style>rect { fill: red; stroke: blue; opacity: 0.5; }</style>"
                      "</svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.cssRules.size());
        const auto& rule = data.cssRules[0];
        EXPECT_EQ (3, rule.declarations.size());
        // Each declaration contains a colon
        for (int i = 0; i < rule.declarations.size(); ++i)
            EXPECT_TRUE (rule.declarations[i].contains (":")) << rule.declarations[i];
    });
}

TEST (SVGDocumentTests, SVGCssParserInlineStyleAppliesPresentationProperties)
{
    SVGData data;
    SVGCssParser parser (data);
    SVGElement element;
    element.tagName = "rect";

    parser.parseCSSStyle ("fill: none;"
                          "stroke: currentColor;"
                          "stroke-width: 3.5;"
                          "stroke-linejoin: round;"
                          "stroke-linecap: square;"
                          "opacity: 0.25;"
                          "visibility: collapse;"
                          "font-family: \"Yup Sans\";"
                          "font-size: 18px;"
                          "text-anchor: middle;"
                          "letter-spacing: 2px;"
                          "word-spacing: 3px;"
                          "font-weight: bolder;"
                          "font-style: oblique;"
                          "clip-path: url(#clip);"
                          "mask: url(#mask);"
                          "marker-start: url(#start);"
                          "marker-mid: url(#mid);"
                          "marker-end: url(#end);"
                          "stroke-miterlimit: 0.25;"
                          "filter: url(#blur);"
                          "stroke-dasharray: 4 6;"
                          "stroke-dashoffset: 2;"
                          "clip-rule: evenodd;"
                          "unsupported-property: ignored",
                          element);

    EXPECT_TRUE (element.noFill);
    EXPECT_TRUE (element.strokeCurrentColor);
    ASSERT_TRUE (element.strokeWidth.has_value());
    EXPECT_FLOAT_EQ (3.5f, *element.strokeWidth);
    ASSERT_TRUE (element.strokeJoin.has_value());
    EXPECT_EQ (StrokeJoin::Round, *element.strokeJoin);
    ASSERT_TRUE (element.strokeCap.has_value());
    EXPECT_EQ (StrokeCap::Square, *element.strokeCap);
    ASSERT_TRUE (element.opacity.has_value());
    EXPECT_FLOAT_EQ (0.25f, *element.opacity);
    EXPECT_TRUE (element.hidden);
    ASSERT_TRUE (element.fontFamily.has_value());
    EXPECT_EQ (String ("Yup Sans"), *element.fontFamily);
    ASSERT_TRUE (element.fontSize.has_value());
    EXPECT_FLOAT_EQ (18.0f, *element.fontSize);
    ASSERT_TRUE (element.textAnchor.has_value());
    EXPECT_EQ (String ("middle"), *element.textAnchor);
    ASSERT_TRUE (element.letterSpacing.has_value());
    EXPECT_FLOAT_EQ (2.0f, *element.letterSpacing);
    ASSERT_TRUE (element.wordSpacing.has_value());
    EXPECT_FLOAT_EQ (3.0f, *element.wordSpacing);
    ASSERT_TRUE (element.fontWeight.has_value());
    EXPECT_EQ (700, *element.fontWeight);
    ASSERT_TRUE (element.fontItalic.has_value());
    EXPECT_TRUE (*element.fontItalic);
    ASSERT_TRUE (element.clipPathUrl.has_value());
    EXPECT_EQ (String ("clip"), *element.clipPathUrl);
    ASSERT_TRUE (element.maskUrl.has_value());
    EXPECT_EQ (String ("mask"), *element.maskUrl);
    ASSERT_TRUE (element.markerStart.has_value());
    EXPECT_EQ (String ("start"), *element.markerStart);
    ASSERT_TRUE (element.markerMid.has_value());
    EXPECT_EQ (String ("mid"), *element.markerMid);
    ASSERT_TRUE (element.markerEnd.has_value());
    EXPECT_EQ (String ("end"), *element.markerEnd);
    EXPECT_FLOAT_EQ (1.0f, element.strokeMiterLimit);
    ASSERT_TRUE (element.filterUrl.has_value());
    EXPECT_EQ (String ("blur"), *element.filterUrl);
    ASSERT_TRUE (element.strokeDashArray.has_value());
    ASSERT_EQ (2, element.strokeDashArray->size());
    EXPECT_FLOAT_EQ (4.0f, (*element.strokeDashArray)[0]);
    EXPECT_FLOAT_EQ (6.0f, (*element.strokeDashArray)[1]);
    ASSERT_TRUE (element.strokeDashOffset.has_value());
    EXPECT_FLOAT_EQ (2.0f, *element.strokeDashOffset);
    ASSERT_TRUE (element.clipRule.has_value());
    EXPECT_EQ (String ("evenodd"), *element.clipRule);
}

TEST (SVGDocumentTests, SVGCssParserApplyStylePropertyCoversAlternateBranches)
{
    SVGData data;
    SVGCssParser parser (data);
    SVGElement element;

    parser.applyStyleProperty ("fill", "url(#gradient)", element);
    ASSERT_TRUE (element.fillUrl.has_value());
    EXPECT_EQ (String ("gradient"), *element.fillUrl);

    parser.applyStyleProperty ("stroke", "none", element);
    EXPECT_TRUE (element.noStroke);

    parser.applyStyleProperty ("stroke", "currentColor", element);
    EXPECT_TRUE (element.strokeCurrentColor);

    parser.applyStyleProperty ("stroke-linejoin", "miter", element);
    ASSERT_TRUE (element.strokeJoin.has_value());
    EXPECT_EQ (StrokeJoin::Miter, *element.strokeJoin);

    parser.applyStyleProperty ("stroke-linejoin", "bevel", element);
    EXPECT_EQ (StrokeJoin::Bevel, *element.strokeJoin);

    parser.applyStyleProperty ("stroke-linecap", "round", element);
    ASSERT_TRUE (element.strokeCap.has_value());
    EXPECT_EQ (StrokeCap::Round, *element.strokeCap);

    parser.applyStyleProperty ("stroke-linecap", "butt", element);
    EXPECT_EQ (StrokeCap::Butt, *element.strokeCap);

    parser.applyStyleProperty ("font-weight", "lighter", element);
    ASSERT_TRUE (element.fontWeight.has_value());
    EXPECT_EQ (400, *element.fontWeight);

    parser.applyStyleProperty ("font-weight", "600", element);
    EXPECT_EQ (600, *element.fontWeight);

    parser.applyStyleProperty ("font-style", "normal", element);
    ASSERT_TRUE (element.fontItalic.has_value());
    EXPECT_FALSE (*element.fontItalic);

    parser.applyStyleProperty ("marker", "url(#marker)", element);
    ASSERT_TRUE (element.markerStart.has_value());
    ASSERT_TRUE (element.markerMid.has_value());
    ASSERT_TRUE (element.markerEnd.has_value());
    EXPECT_EQ (String ("marker"), *element.markerStart);
    EXPECT_EQ (String ("marker"), *element.markerMid);
    EXPECT_EQ (String ("marker"), *element.markerEnd);

    element.filterUrl = String ("previous");
    parser.applyStyleProperty ("filter", "none", element);
    EXPECT_FALSE (element.filterUrl.has_value());

    parser.applyStyleProperty ("filter", "blur(2px)", element);
    EXPECT_FALSE (element.filterUrl.has_value());

    element.strokeDashArray = Array<float> ({ 1.0f, 2.0f });
    parser.applyStyleProperty ("stroke-dasharray", "none", element);
    EXPECT_FALSE (element.strokeDashArray.has_value());

    parser.applyStyleProperty ("fill-rule", "nonzero", element);
    ASSERT_TRUE (element.fillRule.has_value());
    EXPECT_EQ (String ("nonzero"), *element.fillRule);

    parser.applyStyleProperty ("font-variant", "small-caps", element);
    parser.applyStyleProperty ("font-stretch", "condensed", element);
    parser.applyStyleProperty ("font", "italic 12px serif", element);
    parser.applyStyleProperty ("dominant-baseline", "middle", element);
    parser.applyStyleProperty ("alignment-baseline", "central", element);
    parser.applyStyleProperty ("baseline-shift", "super", element);
}

TEST (SVGDocumentTests, SVGCssParserApplyStylePropertyCoversBlendModes)
{
    struct BlendModeCase
    {
        const char* cssValue;
        BlendMode expectedMode;
    };

    const BlendModeCase cases[] = {
        { "multiply", BlendMode::Multiply },
        { "screen", BlendMode::Screen },
        { "overlay", BlendMode::Overlay },
        { "darken", BlendMode::Darken },
        { "lighten", BlendMode::Lighten },
        { "color-dodge", BlendMode::ColorDodge },
        { "color-burn", BlendMode::ColorBurn },
        { "hard-light", BlendMode::HardLight },
        { "soft-light", BlendMode::SoftLight },
        { "difference", BlendMode::Difference },
        { "exclusion", BlendMode::Exclusion },
        { "hue", BlendMode::Hue },
        { "saturation", BlendMode::Saturation },
        { "color", BlendMode::Color },
        { "luminosity", BlendMode::Luminosity }
    };

    SVGData data;
    SVGCssParser parser (data);

    for (const auto& testCase : cases)
    {
        SVGElement element;
        parser.applyStyleProperty ("mix-blend-mode", testCase.cssValue, element);

        ASSERT_TRUE (element.blendMode.has_value()) << testCase.cssValue;
        EXPECT_EQ (testCase.expectedMode, *element.blendMode) << testCase.cssValue;
    }
}

TEST (SVGDocumentTests, SVGCssParserParseStyleElementStoresSpecificityAndOrder)
{
    SVGData data;
    SVGCssParser parser (data);
    XmlElement styleElement ("style");
    styleElement.addTextElement (" , #target { fill: red; }"
                                 ".highlight { stroke: blue; }"
                                 "rect#target { opacity: 0.5; }"
                                 "circle.highlight { fill: green; }"
                                 "path { stroke-width: 2; }");

    parser.parseStyleElement (styleElement);

    ASSERT_EQ (5u, data.cssRules.size());
    EXPECT_EQ (String ("#target"), data.cssRules[0].selector);
    EXPECT_EQ (100, data.cssRules[0].specificity);
    EXPECT_EQ (0, data.cssRules[0].order);
    EXPECT_EQ (String (".highlight"), data.cssRules[1].selector);
    EXPECT_EQ (10, data.cssRules[1].specificity);
    EXPECT_EQ (1, data.cssRules[1].order);
    EXPECT_EQ (String ("rect#target"), data.cssRules[2].selector);
    EXPECT_EQ (101, data.cssRules[2].specificity);
    EXPECT_EQ (2, data.cssRules[2].order);
    EXPECT_EQ (String ("circle.highlight"), data.cssRules[3].selector);
    EXPECT_EQ (11, data.cssRules[3].specificity);
    EXPECT_EQ (3, data.cssRules[3].order);
    EXPECT_EQ (String ("path"), data.cssRules[4].selector);
    EXPECT_EQ (1, data.cssRules[4].specificity);
    EXPECT_EQ (4, data.cssRules[4].order);
}

TEST (SVGDocumentTests, SVGCssParserMatchesSimpleSelectors)
{
    SVGData data;
    SVGCssParser parser (data);
    XmlElement rect ("rect");
    rect.setAttribute ("id", "target");
    rect.setAttribute ("class", "highlight selected");

    EXPECT_TRUE (parser.matchesCssSelector (rect, SVGCssRule { "rect", {}, 0, 0 }));
    EXPECT_TRUE (parser.matchesCssSelector (rect, SVGCssRule { "#target", {}, 0, 0 }));
    EXPECT_TRUE (parser.matchesCssSelector (rect, SVGCssRule { ".highlight", {}, 0, 0 }));
    EXPECT_TRUE (parser.matchesCssSelector (rect, SVGCssRule { "rect#target.highlight", {}, 0, 0 }));

    EXPECT_FALSE (parser.matchesCssSelector (rect, SVGCssRule { "", {}, 0, 0 }));
    EXPECT_FALSE (parser.matchesCssSelector (rect, SVGCssRule { "g rect", {}, 0, 0 }));
    EXPECT_FALSE (parser.matchesCssSelector (rect, SVGCssRule { ".missing", {}, 0, 0 }));
    EXPECT_FALSE (parser.matchesCssSelector (rect, SVGCssRule { "circle.highlight", {}, 0, 0 }));
}

TEST (SVGDocumentTests, SVGCssParserApplyStylesheetRulesUsesSpecificityOrder)
{
    SVGData data;
    data.cssRules.push_back ({ "rect", { "fill: red" }, 1, 1 });
    data.cssRules.push_back ({ ".highlight", { "fill: blue" }, 10, 0 });
    data.cssRules.push_back ({ "#target", { "fill: green" }, 100, 2 });

    SVGCssParser parser (data);
    XmlElement rect ("rect");
    rect.setAttribute ("id", "target");
    rect.setAttribute ("class", "highlight");

    SVGElement element;
    element.tagName = "rect";
    parser.applyStylesheetRules (rect, element);

    ASSERT_TRUE (element.fillColor.has_value());
}

// ==============================================================================
// SVGElement class names
// ==============================================================================

TEST (SVGDocumentTests, ClassAttributeParsedIntoArray)
{
    auto doc = parse ("<svg><rect class=\"foo bar baz\" x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        const auto& elem = *data.elements[0];
        EXPECT_EQ (3, elem.classNames.size());
        EXPECT_TRUE (elem.classNames.contains ("foo"));
        EXPECT_TRUE (elem.classNames.contains ("bar"));
        EXPECT_TRUE (elem.classNames.contains ("baz"));
    });
}

TEST (SVGDocumentTests, SingleClassParsed)
{
    auto doc = parse ("<svg><circle class=\"highlight\" cx=\"5\" cy=\"5\" r=\"5\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_EQ (1, data.elements[0]->classNames.size());
        EXPECT_TRUE (data.elements[0]->classNames.contains ("highlight"));
    });
}

TEST (SVGDocumentTests, NoClassAttributeProducesEmptyArray)
{
    auto doc = parse ("<svg><rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" /></svg>");
    ASSERT_NE (nullptr, doc);

    doc->visit ([] (const SVGData& data)
    {
        ASSERT_EQ (1u, data.elements.size());
        EXPECT_EQ (0, data.elements[0]->classNames.size());
    });
}

// ==============================================================================
// SVGDocument::clear
// ==============================================================================

TEST (SVGDocumentTests, ClearResetsViewBoxToEmpty)
{
    auto doc = parse ("<svg viewBox=\"0 0 200 100\"></svg>");
    ASSERT_NE (nullptr, doc);

    doc->clear();

    EXPECT_TRUE (doc->getBounds().isEmpty());
    doc->visit ([] (const SVGData& data)
    {
        EXPECT_TRUE (data.viewBox.isEmpty());
        EXPECT_EQ (0u, data.elements.size());
        EXPECT_EQ (0u, data.gradients.size());
        EXPECT_EQ (0u, data.filters.size());
        EXPECT_EQ (0u, data.clipPaths.size());
        EXPECT_EQ (0u, data.masks.size());
        EXPECT_EQ (0u, data.markers.size());
        EXPECT_EQ (0u, data.patterns.size());
        EXPECT_EQ (0u, data.cssRules.size());
    });
}
