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
    EXPECT_TRUE (createError ("label = \"a\"").contains ("'label' and 'labelRect'"));
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

    auto other = ExpressionDrawing::create ("rect = rect (row * 12, 0, 10, 10)", context);
    ASSERT_TRUE (other.wasOk()) << other.getErrorMessage().toStdString();

    ExpressionState otherState (other.getReference().getProgram());
    otherState.bindRowIndex (row);

    other.getReference().draw (g, otherState, 3, nullptr, &cache);
    EXPECT_TRUE (cache.wasRebuilt());
    other.getReference().draw (g, otherState, 3, nullptr, &cache);
    EXPECT_FALSE (cache.wasRebuilt());
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
