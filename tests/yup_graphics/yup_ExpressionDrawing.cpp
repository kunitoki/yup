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

#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class ExpressionDrawingTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        values = context.addArray<float> ("values");
        row = context.addInput<int> ("row");
    }

    String createError (StringRef text)
    {
        auto drawing = ExpressionDrawing::create (text, context);
        EXPECT_TRUE (drawing.failed());
        return drawing.failed() ? drawing.getErrorMessage() : String();
    }

    static Font loadTestFont (float height = 14.0f)
    {
        const auto fontFile =
#if YUP_EMSCRIPTEN
            File ("/")
#else
            File (__FILE__)
                .getParentDirectory()
                .getParentDirectory()
#endif
                .getChildFile ("data")
                .getChildFile ("fonts")
                .getChildFile ("Linefont-VariableFont_wdth,wght.ttf");

        auto loaded = Font::loadFontFromFile (fontFile);
        EXPECT_TRUE (loaded.wasOk());
        return loaded.wasOk() ? loaded.getValue().withHeight (height) : Font();
    }

    /** Returns how many segments of a path are of a kind. */
    static int countSegments (const Path& path, Path::Verb verb)
    {
        int count = 0;

        for (const auto segment : path)
            count += segment.verb == verb ? 1 : 0;

        return count;
    }

    /** Returns the points the segments of a kind end at, in order. */
    static std::vector<Point<float>> getSegmentPoints (const Path& path, Path::Verb verb)
    {
        std::vector<Point<float>> points;

        for (const auto segment : path)
            if (segment.verb == verb)
                points.push_back (segment.point);

        return points;
    }

    ExpressionContext context;
    ExpressionArray<float> values;
    ExpressionInput<int> row;
};

//==============================================================================
TEST_F (ExpressionDrawingTests, ConvertsOutputsToExpectedTypes)
{
    auto drawing = ExpressionDrawing::create ("rect = rect (0, 0, 10, 10); strokeWidth = 2; visible = 2 > 1", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    const auto& program = drawing.getReference().getProgram();
    EXPECT_EQ (&ExpressionType::floatType(), program.getOutputType (program.findOutput ("strokeWidth")));
    EXPECT_EQ (&ExpressionType::boolType(), program.getOutputType (program.findOutput ("visible")));
}

TEST_F (ExpressionDrawingTests, ReportsInvalidDrawings)
{
    EXPECT_TRUE (createError ("fill = rgb (255, 0, 0)").contains ("assigns no shape"));
    EXPECT_TRUE (createError ("label = \"a\"").contains ("'labelRect' or 'labelAnchor'"));
    EXPECT_TRUE (createError ("label = \"a\"; labelRect = rect (0, 0, 1, 1); labelAnchor = point (0, 0)").contains ("'labelRect' or 'labelAnchor'"));
    EXPECT_TRUE (createError ("rect = 1").contains ("Cannot convert 'int' to 'rect'"));
    EXPECT_TRUE (createError ("path = rect (0, 0, 1, 1)").contains ("Cannot convert 'rect' to 'path'"));
    EXPECT_TRUE (createError ("rect = rect (0, 0").isNotEmpty());
}

TEST_F (ExpressionDrawingTests, WrapsPrecompiledPrograms)
{
    ExpressionContext drawingContext (context);
    ExpressionDrawing::declareOutputs (drawingContext);

    auto program = CompiledExpression::compile ("point = point (row, values[row]); radius = 3", drawingContext);
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage().toStdString();

    auto drawing = ExpressionDrawing::create (program.getReference());
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();
    EXPECT_EQ (program.getReference().getNumInstructions(), drawing.getReference().getProgram().getNumInstructions());

    auto other = ExpressionDrawing::create (program.getReference());
    EXPECT_TRUE (other.wasOk());
}

TEST_F (ExpressionDrawingTests, IgnoresOutputsWithUnexpectedTypesInPrecompiledPrograms)
{
    addGraphicsExpressionTypes (context);

    auto program = CompiledExpression::compile ("rect = rect (0, 0, 1, 1); strokeWidth = 2", context);
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage().toStdString();
    EXPECT_TRUE (ExpressionDrawing::create (program.getReference()).wasOk());

    auto noShape = CompiledExpression::compile ("rect = 1", context);
    ASSERT_TRUE (noShape.wasOk());
    EXPECT_TRUE (ExpressionDrawing::create (noShape.getReference()).failed());
}

TEST_F (ExpressionDrawingTests, DrawsRowsAndRestoresGraphicsState)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        let h = values[row] * 100;
        visible = values[row] > 0;
        rect = rect (row * 10, 100 - h, 8, h);
        cornerRadius = 2;
        fill = values[row] > 0.5 ? rgb (255, 80, 80) : rgb (80, 160, 255);
        stroke = rgb (0, 0, 0);
        strokeWidth = 3;
        vertex = point (row * 10 + 4, 100 - h);
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    const float data[] = { 0.25f, 0.0f, 0.75f, 1.0f, 0.5f };

    ExpressionState state (drawing.getReference().getProgram(), 2);
    state.bindArray (values, data, (int) std::size (data));
    state.bindRowIndex (row);

    g.setFillColor (Color (0xff123456));
    g.setStrokeWidth (5.0f);

    drawing.getReference().draw (g, state, (int) std::size (data));

    EXPECT_EQ (Color (0xff123456), g.getFillColor());
    EXPECT_FLOAT_EQ (5.0f, g.getStrokeWidth());

    ExpressionDrawing empty;
    ExpressionState emptyState (empty.getProgram());
    empty.draw (g, emptyState, 3);
}

TEST_F (ExpressionDrawingTests, DrawsPathsAndMergedShapes)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        let x = row * 12;
        path = path().addStar (point (x + 6, 20), 5, 2, 6).withNonZeroWinding (row % 3 != 0);
        ellipse = rect (x, 40, 10, 10);
        line = line (x, 60, x + 30, 70);
        rect = rect (x, 80, 10, values[row] * 50);
        fill = row < 8 ? rgb (255, 0, 0) : rgba (0, 0, 255, 0.5);
        stroke = row % 4 == 0 ? rgb (0, 0, 0) : rgb (0, 128, 0);
        strokeWidth = 1;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data (16);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<float> (i % 5) / 4.0f;

    ExpressionState state (drawing.getReference().getProgram(), 4);
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    g.setFillColor (Color (0xff123456));
    g.setStrokeColor (Color (0xff654321));
    g.setStrokeWidth (5.0f);

    drawing.getReference().draw (g, state, (int) data.size());

    EXPECT_EQ (Color (0xff123456), g.getFillColor());
    EXPECT_EQ (Color (0xff654321), g.getStrokeColor());
    EXPECT_FLOAT_EQ (5.0f, g.getStrokeWidth());

    auto filled = ExpressionDrawing::create (R"(
        rect = rect (row * 12, 0, 10, 10);
        point = point (row * 4, 20);
        radius = 3;
        fill = row < 8 ? rgb (255, 0, 0) : rgba (0, 0, 255, 0.5);
    )", context);
    ASSERT_TRUE (filled.wasOk()) << filled.getErrorMessage().toStdString();

    ExpressionState filledState (filled.getReference().getProgram(), 4);
    filledState.bindRowIndex (row);

    filled.getReference().draw (g, filledState, 16);

    g.setOpacity (0.5f);
    drawing.getReference().draw (g, state, (int) data.size());
    filled.getReference().draw (g, filledState, 16);

    EXPECT_EQ (Color (0xff123456), g.getFillColor());
}

TEST_F (ExpressionDrawingTests, CacheReusesShapesWhileOutputsDontChange)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        rect = rect (row * 12, 0, 10, values[row] * 50);
        cornerRadius = 2;
        fill = values[row] > 0.5 ? rgb (255, 0, 0) : rgb (0, 0, 255);
        vertex = point (row * 12 + 5, values[row] * 50);
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data { 0.25f, 0.75f, 1.0f, 0.5f };

    ExpressionState state (drawing.getReference().getProgram(), 2);
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    ExpressionDrawing::Cache cache;
    EXPECT_FALSE (cache.wasRebuilt());

    const auto draw = [&] (int numRows)
    {
        drawing.getReference().draw (g, state, numRows, nullptr, &cache);
        return cache.wasRebuilt();
    };

    EXPECT_TRUE (draw (4));
    EXPECT_FALSE (draw (4));

    data[2] = 0.125f;
    EXPECT_TRUE (draw (4));
    EXPECT_FALSE (draw (4));

    EXPECT_TRUE (draw (3));
    EXPECT_FALSE (draw (3));

    g.setOpacity (0.5f);
    EXPECT_TRUE (draw (3));
    EXPECT_FALSE (draw (3));

    cache.reset();
    EXPECT_FALSE (cache.wasRebuilt());
    EXPECT_TRUE (draw (3));
    EXPECT_FALSE (draw (3));

    auto other = ExpressionDrawing::create ("rect = rect (row * 12, 0, 10, 10)", context);
    ASSERT_TRUE (other.wasOk()) << other.getErrorMessage().toStdString();

    ExpressionState otherState (other.getReference().getProgram());
    otherState.bindRowIndex (row);

    other.getReference().draw (g, otherState, 3, nullptr, &cache);
    EXPECT_TRUE (cache.wasRebuilt());
    other.getReference().draw (g, otherState, 3, nullptr, &cache);
    EXPECT_FALSE (cache.wasRebuilt());
}

TEST_F (ExpressionDrawingTests, CacheRecordsAgainWhenAnimatedOutputsSettle)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create ("rect = rect (row * 12, 0, 10, values[row] * 50)", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data { 0.25f, 0.75f };

    ExpressionState state (drawing.getReference().getProgram());
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    ExpressionDrawing::Cache cache;

    const auto draw = [&]
    {
        drawing.getReference().draw (g, state, (int) data.size(), nullptr, &cache);
        return cache.wasRebuilt();
    };

    for (int frame = 0; frame < 3; ++frame)
    {
        data[0] = 0.1f * (float) frame;
        EXPECT_TRUE (draw());
    }

    // The last animated draw wasn't recorded, so the first settled draw records it
    EXPECT_TRUE (draw());
    EXPECT_FALSE (draw());
}

TEST_F (ExpressionDrawingTests, DrawsStylesTransformsAndClips)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        let x = row * 20;
        rect = rect (x, 10, 16, values[row] * 100);
        topLeftRadius = 4;
        bottomRightRadius = 2;
        fillGradient = linearGradient (rgb (255, 0, 0), point (0, 10), rgb (0, 0, 255), point (0, 110)).withStop (rgb (0, 255, 0), 0.5);
        stroke = rgb (0, 0, 0);
        strokeWidth = 2;
        strokeCap = round;
        strokeJoin = bevel;
        strokeMiterLimit = 2;
        strokePosition = row % 2 == 0 ? inside : outside;
        opacity = 0.75;
        blend = row < 2 ? srcOver : multiply;
        tint = rgb (255, 255, 255);
        transform = transform.rotation (0.1, x + 8, 10);
        clip = rect (0, 0, 150, 150);
        clipPath = path().addEllipse (rect (0, 0, 200, 200));
        label = string (values[row], 2);
        labelRect = rect (x, 120, 16, 10);
        labelColor = rgb (255, 255, 255);
        labelStroke = rgb (0, 0, 0);
        labelJustification = center;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    const float data[] = { 0.25f, 0.5f, 0.75f, 1.0f };

    ExpressionState state (drawing.getReference().getProgram(), 2);
    state.bindArray (values, data, (int) std::size (data));
    state.bindRowIndex (row);

    g.setFillColor (Color (0xff123456));
    g.setStrokeWidth (5.0f);

    drawing.getReference().draw (g, state, (int) std::size (data));

    ExpressionDrawing::Cache cache;
    drawing.getReference().draw (g, state, (int) std::size (data), nullptr, &cache);
    EXPECT_TRUE (cache.wasRebuilt());

    EXPECT_EQ (Color (0xff123456), g.getFillColor());
    EXPECT_FLOAT_EQ (5.0f, g.getStrokeWidth());
    EXPECT_FLOAT_EQ (1.0f, g.getOpacity());
    EXPECT_EQ (BlendMode::SrcOver, g.getBlendMode());
    EXPECT_EQ (StrokePosition::Center, g.getStrokePosition());
    EXPECT_TRUE (g.getTransform().isIdentity());
}

TEST_F (ExpressionDrawingTests, DrawsOnePolylinePerSeries)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        series = row % 3;
        vertex = point (row * 10, values[row] * 100 + series * 20);
        stroke = series == 0 ? rgb (255, 0, 0) : rgb (0, 0, 255);
        strokeWidth = 2;
        feather = series == 2 ? 4 : 0;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data (12);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<float> (i % 4) / 3.0f;

    ExpressionState state (drawing.getReference().getProgram(), 4);
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    g.setStrokeColor (Color (0xff654321));
    drawing.getReference().draw (g, state, (int) data.size());

    EXPECT_EQ (Color (0xff654321), g.getStrokeColor());
    EXPECT_FLOAT_EQ (0.0f, g.getFeather());
}

TEST_F (ExpressionDrawingTests, CacheRebuildsWhenGradientsChange)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        rect = rect (row * 12, 0, 10, 50);
        fillGradient = linearGradient (rgb (255, 0, 0), point (0, 0), rgb (0, 0, 255), point (0, values[0] * 50));
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data { 1.0f };

    ExpressionState state (drawing.getReference().getProgram());
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    ExpressionDrawing::Cache cache;

    const auto draw = [&]
    {
        drawing.getReference().draw (g, state, 4, nullptr, &cache);
        return cache.wasRebuilt();
    };

    EXPECT_TRUE (draw());
    EXPECT_FALSE (draw());

    data[0] = 0.5f;
    EXPECT_TRUE (draw());
    EXPECT_FALSE (draw());
}

TEST_F (ExpressionDrawingTests, CacheReusesShapedLabelsAndUpdatesWhenDefaultFontChanges)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    const auto smallFont = loadTestFont (12.0f);
    const auto largeFont = smallFont.withHeight (22.0f);
    ASSERT_FALSE (smallFont.isEmpty());

    auto drawing = ExpressionDrawing::create (R"(
        label = string (values[row], 1);
        labelRect = rect (row * 40, 10, 36, 24);
        labelColor = rgb (255, 255, 255);
        labelStroke = rgb (0, 0, 0);
        labelJustification = right;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data { 1.5f, 3.0f };

    ExpressionState state (drawing.getReference().getProgram());
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    ExpressionDrawing::Cache cache;

    drawing.getReference().draw (g, state, (int) data.size(), &smallFont, &cache);
    EXPECT_TRUE (cache.wasRebuilt());

    drawing.getReference().draw (g, state, (int) data.size(), &smallFont, &cache);
    EXPECT_FALSE (cache.wasRebuilt());

    drawing.getReference().draw (g, state, (int) data.size(), &largeFont, &cache);
    EXPECT_FALSE (cache.wasRebuilt());
}

TEST_F (ExpressionDrawingTests, OutlineCollectsTheShapesOfTheVisibleRows)
{
    auto drawing = ExpressionDrawing::create (R"(
        rect = rect (row * 20, 0, 10, 10);
        transform = transform.translation (0, 5);
        visible = row != 1;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    ExpressionState state (drawing.getReference().getProgram());
    state.bindRowIndex (row);

    EXPECT_EQ (drawing.getReference().getOutline (state, 3).getBounds(), Rectangle<float> (0.0f, 5.0f, 50.0f, 10.0f));
    EXPECT_EQ (drawing.getReference().getOutline (state, 3, false).getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 10.0f));
    EXPECT_EQ (drawing.getReference().getOutline (state, 2).getBounds(), Rectangle<float> (0.0f, 5.0f, 10.0f, 10.0f));
    EXPECT_TRUE (drawing.getReference().getOutline (state, 0).isEmpty());
}

TEST_F (ExpressionDrawingTests, OutlineJoinsTheVerticesOfASeriesAndSkipsLabels)
{
    auto drawing = ExpressionDrawing::create (R"(
        vertex = point (row * 10, row * row);
        label = "a";
        labelRect = rect (100, 100, 50, 50);
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    ExpressionState state (drawing.getReference().getProgram());
    state.bindRowIndex (row);

    EXPECT_EQ (drawing.getReference().getOutline (state, 4).getBounds(), Rectangle<float> (0.0f, 0.0f, 30.0f, 9.0f));
}

TEST_F (ExpressionDrawingTests, ASeriesWithBasesClosesTheAreaDownToThem)
{
    auto drawing = ExpressionDrawing::create (R"(
        vertex = point (row * 10, row * 2);
        vertexBase = point (row * 10, 20);
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    ExpressionState state (drawing.getReference().getProgram());
    state.bindRowIndex (row);

    const auto area = drawing.getReference().getOutline (state, 3);

    // The line goes through the vertices, then back along the bases, and closes
    EXPECT_EQ (area.getBounds(), Rectangle<float> (0.0f, 0.0f, 20.0f, 20.0f));
    EXPECT_EQ (countSegments (area, Path::Verb::MoveTo), 1);
    EXPECT_EQ (countSegments (area, Path::Verb::Close), 1);

    const auto corners = getSegmentPoints (area, Path::Verb::LineTo);
    ASSERT_EQ (corners.size(), 5u);
    EXPECT_EQ (corners[0], Point<float> (10.0f, 2.0f));
    EXPECT_EQ (corners[1], Point<float> (20.0f, 4.0f));
    EXPECT_EQ (corners[2], Point<float> (20.0f, 20.0f));
    EXPECT_EQ (corners[3], Point<float> (10.0f, 20.0f));
    EXPECT_EQ (corners[4], Point<float> (0.0f, 20.0f));

    // Without bases the vertices are a line, left open
    auto line = ExpressionDrawing::create ("vertex = point (row * 10, row * 2);", context);
    ASSERT_TRUE (line.wasOk()) << line.getErrorMessage().toStdString();

    ExpressionState lineState (line.getReference().getProgram());
    lineState.bindRowIndex (row);

    const auto open = line.getReference().getOutline (lineState, 3);
    EXPECT_EQ (countSegments (open, Path::Verb::LineTo), 2);
    EXPECT_EQ (countSegments (open, Path::Verb::Close), 0);
}

TEST_F (ExpressionDrawingTests, TheBasesOfASeriesCanBeAnotherCurveAndEachSeriesHasItsOwn)
{
    auto drawing = ExpressionDrawing::create (R"(
        series = row % 2;
        let x = (row / 2) * 10;
        vertex = point (x, series * 100 + values[row / 2]);
        vertexBase = point (x, series * 100 + values[row / 2] + 8);
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    const float data[] = { 0.0f, 4.0f, 2.0f };

    ExpressionState state (drawing.getReference().getProgram(), 4);
    state.bindArray (values, data, 3);
    state.bindRowIndex (row);

    // Two bands of three vertices each, one above the other
    const auto bands = drawing.getReference().getOutline (state, 6);

    EXPECT_EQ (bands.getBounds(), Rectangle<float> (0.0f, 0.0f, 20.0f, 112.0f));
    EXPECT_EQ (countSegments (bands, Path::Verb::MoveTo), 2);
    EXPECT_EQ (countSegments (bands, Path::Verb::Close), 2);
    EXPECT_EQ (countSegments (bands, Path::Verb::LineTo), 10);
}

TEST_F (ExpressionDrawingTests, ASmoothedSeriesCurvesThroughItsVertices)
{
    auto drawing = ExpressionDrawing::create (R"(
        vertex = point (row * 10, values[row]);
        vertexSmooth = 1;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    const float data[] = { 0.0f, 10.0f, 0.0f, 10.0f };

    ExpressionState state (drawing.getReference().getProgram());
    state.bindArray (values, data, 4);
    state.bindRowIndex (row);

    const auto curve = drawing.getReference().getOutline (state, 4);

    EXPECT_EQ (countSegments (curve, Path::Verb::LineTo), 0);

    // Every curve ends on a vertex
    const auto ends = getSegmentPoints (curve, Path::Verb::CubicTo);
    ASSERT_EQ (ends.size(), 3u);
    EXPECT_EQ (ends[0], Point<float> (10.0f, 10.0f));
    EXPECT_EQ (ends[1], Point<float> (20.0f, 0.0f));
    EXPECT_EQ (ends[2], Point<float> (30.0f, 10.0f));

    // Two vertices have no neighbours to follow: they are joined by a line
    const auto pair = drawing.getReference().getOutline (state, 2);
    EXPECT_EQ (countSegments (pair, Path::Verb::CubicTo), 0);
    EXPECT_EQ (countSegments (pair, Path::Verb::LineTo), 1);

    // The bases are smoothed as the vertices are, and no smoothing keeps the lines
    auto band = ExpressionDrawing::create (R"(
        vertex = point (row * 10, values[row]);
        vertexBase = point (row * 10, values[row] + 20);
        vertexSmooth = x;
    )", [this] { auto smoothed = context; smoothed.addInput<float> ("x"); return smoothed; }());
    ASSERT_TRUE (band.wasOk()) << band.getErrorMessage().toStdString();

    const auto smoothing = band.getReference().getProgram().getInput<float> ("x");
    ASSERT_TRUE (smoothing.wasOk());

    ExpressionState bandState (band.getReference().getProgram());
    bandState.bindArray (values, data, 4);
    bandState.bindRowIndex (row);

    bandState.set (smoothing.getValue(), 0.5f);
    const auto smooth = band.getReference().getOutline (bandState, 4);
    EXPECT_EQ (countSegments (smooth, Path::Verb::CubicTo), 6);
    EXPECT_EQ (countSegments (smooth, Path::Verb::LineTo), 1);
    EXPECT_EQ (countSegments (smooth, Path::Verb::Close), 1);

    bandState.set (smoothing.getValue(), 0.0f);
    const auto straight = band.getReference().getOutline (bandState, 4);
    EXPECT_EQ (countSegments (straight, Path::Verb::CubicTo), 0);
    EXPECT_EQ (countSegments (straight, Path::Verb::LineTo), 7);
}

TEST_F (ExpressionDrawingTests, CacheDrawsASeriesAgainWithTheBuffersItKept)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    auto drawing = ExpressionDrawing::create (R"(
        series = row % 2;
        vertex = point (row * 10, values[row] * 100);
        vertexBase = point (row * 10, 150);
        vertexSmooth = 0.5;
        fill = rgb (0, 0, 255);
        stroke = rgb (255, 0, 0);
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    std::vector<float> data { 0.1f, 0.4f, 0.2f, 0.8f, 0.6f, 0.3f };

    ExpressionState state (drawing.getReference().getProgram(), 4);
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);

    ExpressionDrawing::Cache cache;

    drawing.getReference().draw (g, state, (int) data.size(), nullptr, &cache);
    EXPECT_TRUE (cache.wasRebuilt());

    drawing.getReference().draw (g, state, (int) data.size(), nullptr, &cache);
    EXPECT_FALSE (cache.wasRebuilt());

    // Levels changing in place, with fewer rows then more, are drawn from the same buffers
    for (int i = 0; i < 4; ++i)
    {
        data[1] = 0.1f * static_cast<float> (i);
        drawing.getReference().draw (g, state, i % 2 == 0 ? 3 : (int) data.size(), nullptr, &cache);
        EXPECT_TRUE (cache.wasRebuilt());
    }
}

TEST_F (ExpressionDrawingTests, RoundsRectanglesWithEllipticalCorners)
{
    auto drawing = ExpressionDrawing::create ("rect = rect (0, 0, 100, 60); cornerSize = size (20, 5); cornerRadius = 30", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    ExpressionState state (drawing.getReference().getProgram());
    const auto outline = drawing.getReference().getOutline (state);

    // The corner size replaces the corner radius: the top edge starts after the horizontal radius
    ASSERT_FALSE (outline.isEmpty());
    EXPECT_EQ ((*outline.begin()).point, Point<float> (20.0f, 0.0f));
}

TEST_F (ExpressionDrawingTests, DrawsLabelsPlacedFromAPointOnTheirBaseline)
{
    auto graphicsContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (graphicsContext, nullptr);
    auto renderer = graphicsContext->makeRenderer (200, 200);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*graphicsContext, *renderer);

    const auto font = loadTestFont();
    ASSERT_FALSE (font.isEmpty());

    auto drawing = ExpressionDrawing::create (R"(
        label = string (row);
        labelAnchor = point (100, 20 + row * 20);
        labelTextAnchor = row == 0 ? textAnchor.start : (row == 1 ? textAnchor.middle : textAnchor.end);
        labelLetterSpacing = 2;
        fillOpacity = 0.5;
        labelStroke = rgb (0, 0, 0);
        strokeOpacity = 0.5;
    )", context);
    ASSERT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

    ExpressionState state (drawing.getReference().getProgram());
    state.bindRowIndex (row);

    ExpressionDrawing::Cache cache;

    drawing.getReference().draw (g, state, 3, &font, &cache);
    EXPECT_TRUE (cache.wasRebuilt());

    drawing.getReference().draw (g, state, 3, &font, &cache);
    EXPECT_FALSE (cache.wasRebuilt());
    EXPECT_FLOAT_EQ (g.getOpacity(), 1.0f);
}

#ifdef YUP_GRAPHICS_TESTS_GPU_PIXEL_HOST
//==============================================================================
/** Draws with the platform's GPU context, through GraphicsGpuPixelTestHost (see native/). */
class ExpressionDrawingGpuTests : public ExpressionDrawingTests
{
protected:
    /** A 16 x 16 tile, opaque red in its left half and empty in its right half. */
    struct HalfRedSource : PaintSource
    {
        Size<float> getContentSize() const override { return { 16.0f, 16.0f }; }

        void paintContent (Graphics& g) override
        {
            g.setFillColor (Colors::red);
            g.fillRect (0.0f, 0.0f, 8.0f, 16.0f);
        }
    };

    static void SetUpTestSuite()
    {
        host = std::make_unique<GraphicsGpuPixelTestHost>();
        gpuContext = host->getContext();
    }

    static void TearDownTestSuite()
    {
        gpuContext = nullptr;
        host.reset();
    }

    void SetUp() override
    {
        ExpressionDrawingTests::SetUp();

        if (gpuContext == nullptr)
            GTEST_SKIP() << "No GPU graphics context available";

        host->makeCurrent();
    }

    /** Compiles a drawing, draws its rows and returns the RGBA pixels of the canvas. */
    std::vector<uint8> render (StringRef text, const std::function<void (ExpressionState&)>& setInputs = nullptr, int numRows = 1)
    {
        auto drawing = ExpressionDrawing::create (text, context);
        EXPECT_TRUE (drawing.wasOk()) << drawing.getErrorMessage().toStdString();

        auto canvas = GpuCanvas::create (*gpuContext, size, size);
        if (drawing.failed() || canvas == nullptr)
            return {};

        ExpressionState state (drawing.getReference().getProgram());
        if (setInputs != nullptr)
            setInputs (state);

        drawing.getReference().draw (canvas->beginDraw(), state, numRows);

        std::vector<uint8> pixels (static_cast<std::size_t> (size * size * 4));
        if (! canvas->commit() || ! canvas->readPixels (pixels.data(), pixels.size()))
            return {};

        return pixels;
    }

    static int alphaAt (const std::vector<uint8>& pixels, int x, int y)
    {
        return pixels[static_cast<std::size_t> ((y * size + x) * 4 + 3)];
    }

    static constexpr int size = 64;
    static std::unique_ptr<GraphicsGpuPixelTestHost> host;
    static GraphicsContext* gpuContext;
};

std::unique_ptr<GraphicsGpuPixelTestHost> ExpressionDrawingGpuTests::host;
GraphicsContext* ExpressionDrawingGpuTests::gpuContext = nullptr;

TEST_F (ExpressionDrawingGpuTests, FillPatternTilesItsSourceOverTheShape)
{
    HalfRedSource source;

    addGraphicsExpressionTypes (context);
    const auto tiles = context.addInput<PatternPaint> ("tiles");

    const auto pixels = render ("rect = rect (0, 0, 64, 32); fillPattern = tiles", [&] (ExpressionState& state)
    {
        state.set (tiles, PatternPaint { std::addressof (source), AffineTransform::translation (4.0f, 0.0f) });
    });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // The tiles start four pixels in: red for eight pixels, then empty for eight
    for (const auto x : { 6, 22, 38 })
        EXPECT_GT (alphaAt (pixels, x, 16), 200) << "at x = " << x;

    for (const auto x : { 16, 32, 48 })
        EXPECT_LT (alphaAt (pixels, x, 16), 50) << "at x = " << x;

    EXPECT_LT (alphaAt (pixels, 6, 48), 50);

    source.releaseResources();
}

TEST_F (ExpressionDrawingGpuTests, PatternWithoutASourceLeavesTheFillColor)
{
    addGraphicsExpressionTypes (context);

    const auto pixels = render ("rect = rect (0, 0, 64, 64); fill = #ff0000; fillPattern = pattern()");
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 32, 32), 200);
}

TEST_F (ExpressionDrawingGpuTests, FillAndStrokeOpacityFadeTheirOwnPaint)
{
    const auto pixels = render (R"(
        rect = rect (16, 16, 32, 32);
        fill = #ff0000;
        fillOpacity = 0.5;
        stroke = #0000ff;
        strokeWidth = 8;
        strokeOpacity = 0.25;
    )");
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // Half opaque inside, a quarter opaque on the outer half of the stroke
    EXPECT_NEAR (alphaAt (pixels, 32, 32), 128, 12);
    EXPECT_NEAR (alphaAt (pixels, 14, 32), 64, 12);
    EXPECT_LT (alphaAt (pixels, 4, 32), 10);
}

TEST_F (ExpressionDrawingGpuTests, ASeriesWithBasesFillsItsAreaAndStrokesOnlyItsLine)
{
    const auto bindRows = [this] (ExpressionState& state)
    {
        state.bindRowIndex (row);
    };

    // A line across the canvas at y = 16, closed down to y = 48
    const String area ("vertex = point (row * 32, 16); vertexBase = point (row * 32, 48);");

    const auto filled = render (area + " fill = rgb (255, 0, 0);", bindRows, 3);
    ASSERT_FALSE (filled.empty());
    EXPECT_GT (alphaAt (filled, 32, 32), 240);
    EXPECT_LT (alphaAt (filled, 32, 8), 10);
    EXPECT_LT (alphaAt (filled, 32, 56), 10);

    // The stroke follows the vertices, and leaves the bases and the sides alone
    const auto stroked = render (area + " stroke = rgb (255, 0, 0); strokeWidth = 4;", bindRows, 3);
    ASSERT_FALSE (stroked.empty());
    EXPECT_GT (alphaAt (stroked, 32, 16), 240);
    EXPECT_LT (alphaAt (stroked, 32, 32), 10);
    EXPECT_LT (alphaAt (stroked, 32, 48), 10);

    const auto both = render (area + " fill = rgb (0, 0, 255); stroke = rgb (255, 0, 0); strokeWidth = 4;", bindRows, 3);
    ASSERT_FALSE (both.empty());
    EXPECT_GT (alphaAt (both, 32, 16), 240);
    EXPECT_GT (alphaAt (both, 32, 32), 240);
    EXPECT_LT (alphaAt (both, 32, 56), 10);
}
#endif

TEST_F (ExpressionDrawingTests, LabelBoundsTellWhereTheLabelsLand)
{
    const auto font = loadTestFont (10.0f);

    auto fitted = ExpressionDrawing::create (R"(
        labelRect = rect (10 + row * 100, 20, 80, 40);
        label = "abc";
        labelJustification = justification.topLeft;
    )", context);
    ASSERT_TRUE (fitted.wasOk()) << fitted.getErrorMessage().toStdString();

    ExpressionState state (fitted.getReference().getProgram());
    state.bindRowIndex (row);

    // The rectangles the labels are fitted in, and the part of them their glyphs take
    EXPECT_EQ (fitted.getReference().getLabelBounds (state, 2, &font, false), Rectangle<float> (10.0f, 20.0f, 180.0f, 40.0f));

    const auto glyphs = fitted.getReference().getLabelBounds (state, 1, &font);
    EXPECT_FLOAT_EQ (glyphs.getX(), 10.0f);
    EXPECT_FLOAT_EQ (glyphs.getY(), 20.0f);
    EXPECT_GT (glyphs.getWidth(), 0.0f);
    EXPECT_LT (glyphs.getWidth(), 80.0f);
    EXPECT_LT (glyphs.getHeight(), 40.0f);

    // Without a font there is no label, and a drawing of shapes has none either
    EXPECT_TRUE (fitted.getReference().getLabelBounds (state, 1).isEmpty());
    EXPECT_TRUE (fitted.getReference().getLabelBounds (state, 0, &font).isEmpty());

    auto shapes = ExpressionDrawing::create ("rect = rect (0, 0, 10, 10);", context);
    ASSERT_TRUE (shapes.wasOk());

    ExpressionState shapesState (shapes.getReference().getProgram());
    EXPECT_TRUE (shapes.getReference().getLabelBounds (shapesState, 1, &font).isEmpty());
}

TEST_F (ExpressionDrawingTests, LabelBoundsFollowTheAnchorAndTheTransform)
{
    const auto font = loadTestFont (10.0f);

    auto anchored = ExpressionDrawing::create (R"(
        labelAnchor = point (50, 30);
        label = "abc";
        transform = transform.translation (100, 0);
    )", context);
    ASSERT_TRUE (anchored.wasOk()) << anchored.getErrorMessage().toStdString();

    ExpressionState state (anchored.getReference().getProgram());

    // The text starts at its anchor, which is on its baseline
    const auto glyphs = anchored.getReference().getLabelBounds (state, 1, &font);
    EXPECT_FLOAT_EQ (glyphs.getX(), 150.0f);
    EXPECT_LT (glyphs.getY(), 30.0f);
    EXPECT_GT (glyphs.getBottom(), 25.0f);
    EXPECT_GT (glyphs.getWidth(), 0.0f);
}
