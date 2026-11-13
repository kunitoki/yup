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
class GraphicsExpressionTypesTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        addGraphicsExpressionTypes (context);
        bounds = context.addInput<Rectangle<float>> ("bounds");
    }

    template <class T>
    T evaluate (StringRef text, Rectangle<float> boundsValue = { 0.0f, 0.0f, 100.0f, 50.0f })
    {
        auto program = CompiledExpression::compile (text, context);
        EXPECT_TRUE (program.wasOk()) << (program.failed() ? program.getErrorMessage() : String()).toStdString();

        if (program.failed())
            return {};

        const auto output = program.getReference().getOutput<T> ("result");
        EXPECT_TRUE (output.wasOk()) << (output.failed() ? output.getErrorMessage() : String()).toStdString();

        if (output.failed())
            return {};

        ExpressionState state (program.getReference());
        state.set (bounds, boundsValue);
        state.evaluate();
        return state.get (output.getValue());
    }

    bool compiles (StringRef text)
    {
        return CompiledExpression::compile (text, context).wasOk();
    }

    ExpressionContext context;
    ExpressionInput<Rectangle<float>> bounds;
};

//==============================================================================
TEST_F (GraphicsExpressionTypesTests, RegistersTypes)
{
    EXPECT_EQ (context.findType ("point"), context.getType<Point<float>>());
    EXPECT_EQ (context.findType ("size"), context.getType<Size<float>>());
    EXPECT_EQ (context.findType ("rect"), context.getType<Rectangle<float>>());
    EXPECT_EQ (context.findType ("line"), context.getType<Line<float>>());
    EXPECT_EQ (context.findType ("color"), context.getType<Color>());
    EXPECT_EQ (context.findType ("transform"), context.getType<AffineTransform>());
    EXPECT_EQ (context.findType ("path"), context.getType<Path>());
    EXPECT_EQ (context.findType ("font"), context.getType<Font>());
}

TEST_F (GraphicsExpressionTypesTests, RegisteringTwiceKeepsOverloadsUnambiguous)
{
    addGraphicsExpressionTypes (context);

    EXPECT_EQ (Point<float> (4.0f, 6.0f), evaluate<Point<float>> ("point (1, 2) + point (3, 4)"));
    EXPECT_EQ (Color (0xffff0000), evaluate<Color> ("rgb (255, 0, 0)"));
}

TEST_F (GraphicsExpressionTypesTests, PointOperations)
{
    EXPECT_EQ (Point<float> (-2.0f, -2.0f), evaluate<Point<float>> ("point (1, 2) - point (3, 4)"));
    EXPECT_EQ (Point<float> (2.0f, 4.0f), evaluate<Point<float>> ("2 * point (1, 2)"));
    EXPECT_EQ (Point<float> (0.5f, 1.0f), evaluate<Point<float>> ("point (1, 2) / 2"));
    EXPECT_EQ (Point<float> (-1.0f, -2.0f), evaluate<Point<float>> ("-point (1, 2)"));
    EXPECT_EQ (Point<float> (2.0f, 1.0f), evaluate<Point<float>> ("point (1, 2).yx"));
    EXPECT_EQ (Point<float> (11.0f, 22.0f), evaluate<Point<float>> ("point (1, 2).translated (10, 20)"));
    EXPECT_EQ (Point<float> (2.0f, 3.0f), evaluate<Point<float>> ("point (0, 2).lerp (point (4, 4), 0.5)"));
    EXPECT_FLOAT_EQ (5.0f, evaluate<float> ("point (3, 4).length"));
    EXPECT_FLOAT_EQ (5.0f, evaluate<float> ("point (0, 0).distanceTo (point (3, 4))"));
    EXPECT_FLOAT_EQ (11.0f, evaluate<float> ("point (1, 2).dot (point (3, 4))"));
    EXPECT_TRUE (evaluate<bool> ("point (1, 2) == point (1, 2)"));
    EXPECT_TRUE (evaluate<bool> ("point (1, 2) != point (2, 1)"));
}

TEST_F (GraphicsExpressionTypesTests, RectangleOperations)
{
    EXPECT_EQ (Rectangle<float> (1.0f, 2.0f, 3.0f, 4.0f), evaluate<Rectangle<float>> ("rect (point (1, 2), size (3, 4))"));
    EXPECT_EQ (Rectangle<float> (10.0f, 10.0f, 80.0f, 30.0f), evaluate<Rectangle<float>> ("bounds.reduced (10)"));
    EXPECT_EQ (Point<float> (50.0f, 25.0f), evaluate<Point<float>> ("bounds.reduced (10).center"));
    EXPECT_EQ (Size<float> (100.0f, 50.0f), evaluate<Size<float>> ("bounds.size"));
    EXPECT_FLOAT_EQ (40.0f, evaluate<float> ("rect (10, 20, 30, 40).right"));
    EXPECT_FLOAT_EQ (60.0f, evaluate<float> ("rect (10, 20, 30, 40).bottom"));
    EXPECT_FLOAT_EQ (25.0f, evaluate<float> ("bounds.withWidth (bounds.width / 2).centerX"));
    EXPECT_TRUE (evaluate<bool> ("bounds.contains (point (50, 25))"));
    EXPECT_FALSE (evaluate<bool> ("bounds.contains (point (150, 25))"));
    EXPECT_TRUE (evaluate<bool> ("bounds.intersects (rect (90, 40, 20, 20))"));
    EXPECT_EQ (Rectangle<float> (90.0f, 40.0f, 10.0f, 10.0f), evaluate<Rectangle<float>> ("bounds.intersection (rect (90, 40, 20, 20))"));
    EXPECT_EQ (Rectangle<float> (25.0f, 0.0f, 50.0f, 50.0f), evaluate<Rectangle<float>> ("bounds.largestFittingSquare()"));
}

TEST_F (GraphicsExpressionTypesTests, LineOperations)
{
    EXPECT_FLOAT_EQ (5.0f, evaluate<float> ("line (point (0, 0), point (3, 4)).length"));
    EXPECT_EQ (Point<float> (2.5f, 0.0f), evaluate<Point<float>> ("line (0, 0, 10, 0).pointAt (0.25)"));
    EXPECT_EQ (Point<float> (10.0f, 0.0f), evaluate<Point<float>> ("line (0, 0, 10, 0).reversed().start"));
    EXPECT_EQ (Line<float> (1.0f, 1.0f, 11.0f, 1.0f), evaluate<Line<float>> ("line (0, 0, 10, 0).translated (1, 1)"));
}

TEST_F (GraphicsExpressionTypesTests, ColorConstructors)
{
    EXPECT_EQ (Color (0xffff8000), evaluate<Color> ("rgb (255, 128, 0)"));
    EXPECT_EQ (Color (0xffff8000), evaluate<Color> ("rgb (1.0, 0.5, 0.0)"));
    EXPECT_EQ (Color (0x80ff0000), evaluate<Color> ("rgba (255, 0, 0, 128)"));
    EXPECT_EQ (Color (0x80ff0000), evaluate<Color> ("rgba (255, 0, 0, 0.5)"));
    EXPECT_EQ (Color (0x80ff0000), evaluate<Color> ("rgba (1.0, 0.0, 0.0, 0.5)"));
    EXPECT_EQ (Color (0xff00ff00), evaluate<Color> ("color (0xff00ff00)"));
    EXPECT_EQ (Color (0xffff0000), evaluate<Color> ("rgb (300, -5, 0)"));
    EXPECT_EQ (Color::fromHSL (0.5f, 1.0f, 0.5f), evaluate<Color> ("hsl (0.5, 1, 0.5)"));
}

TEST_F (GraphicsExpressionTypesTests, ColorLiterals)
{
    EXPECT_EQ (Color (0xffff8800), evaluate<Color> ("#ff8800"));
    EXPECT_EQ (Color (0xffff8800), evaluate<Color> ("#F80"));
    EXPECT_EQ (Color (0x80ff8800), evaluate<Color> ("#ff880080"));
    EXPECT_EQ (Color (0x88ff8800), evaluate<Color> ("#f808"));
    EXPECT_EQ (Color (0xff0000ff), evaluate<Color> ("bounds.width > 50 ? #0000ff : #ff0000"));
    EXPECT_FLOAT_EQ (1.0f, evaluate<float> ("#ff8800.r"));
    EXPECT_TRUE (CompiledExpression::compile ("#ff8800", context).getReference().isConstant());
}

TEST_F (GraphicsExpressionTypesTests, PolarPoints)
{
    const auto p = evaluate<Point<float>> ("polar (bounds.center, 10, 90deg)");
    EXPECT_NEAR (50.0f, p.getX(), 1.0e-4f);
    EXPECT_NEAR (35.0f, p.getY(), 1.0e-4f);
}

TEST_F (GraphicsExpressionTypesTests, MixesPointsAndColors)
{
    EXPECT_EQ (Point<float> (5.0f, 10.0f), evaluate<Point<float>> ("mix (point (0, 0), point (10, 20), 0.5)"));
    EXPECT_EQ (Color (0xff000000).mixedWith (Color (0xffffffff), 0.25f, ColorSpace::RGB), evaluate<Color> ("mix (#000, #fff, 0.25)"));
    EXPECT_EQ (Color (0xff000000).mixedWith (Color (0xffffffff), 1.0f, ColorSpace::RGB), evaluate<Color> ("mix (#000, #fff, 2)"));
}

TEST_F (GraphicsExpressionTypesTests, ResolvesDottedSymbolsLazily)
{
    context.setSymbolResolver ([this] (const String& name) -> const ExpressionType*
    {
        return name == "theme.font" ? context.getType<Font>() : nullptr;
    });

    const auto program = CompiledExpression::compile ("theme.font.withHeight (12).height + theme.font.height", context);
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage().toStdString();

    const auto font = program.getReference().getInput<Font> ("theme.font");
    const auto result = program.getReference().getOutput<float> ("result");
    ASSERT_TRUE (font.wasOk());
    ASSERT_TRUE (result.wasOk());

    ExpressionState state (program.getReference());
    state.set (font.getValue(), Font().withHeight (3.0f));
    state.evaluate();
    EXPECT_FLOAT_EQ (15.0f, state.get (result.getValue()));
}

TEST_F (GraphicsExpressionTypesTests, ResolvedTypesMustBelongToTheContext)
{
    ExpressionContext other;
    addGraphicsExpressionTypes (other);

    context.setSymbolResolver ([&other] (const String&) { return other.getType<Font>(); });

    const auto program = CompiledExpression::compile ("unknown", context);
    ASSERT_TRUE (program.failed());
    EXPECT_TRUE (program.getErrorMessage().contains ("isn't registered in the context"));
}

TEST_F (GraphicsExpressionTypesTests, ColorOperations)
{
    EXPECT_FLOAT_EQ (1.0f, evaluate<float> ("rgb (255, 0, 0).r"));
    EXPECT_FLOAT_EQ (0.0f, evaluate<float> ("rgb (255, 0, 0).g"));
    EXPECT_EQ (Color (0x00ff0000), evaluate<Color> ("rgb (255, 0, 0).withAlpha (0)"));
    EXPECT_EQ (Color (0xff0000ff), evaluate<Color> ("bounds.width > 50 ? rgb (0, 0, 255) : rgb (255, 0, 0)"));
    EXPECT_EQ (Color (0xffff0000), evaluate<Color> ("bounds.width > 50 ? rgb (0, 0, 255) : rgb (255, 0, 0)", { 0.0f, 0.0f, 10.0f, 10.0f }));
    EXPECT_EQ (Color (0xff00ffff), evaluate<Color> ("rgb (255, 0, 0).inverted()"));
    EXPECT_EQ (Color (0xff000000), evaluate<Color> ("rgb (0, 0, 0).mix (rgb (255, 255, 255), 0)"));
    EXPECT_TRUE (evaluate<bool> ("rgb (255, 255, 255) == color (0xffffffff)"));
}

TEST_F (GraphicsExpressionTypesTests, TransformOperations)
{
    EXPECT_EQ (Point<float> (11.0f, 22.0f), evaluate<Point<float>> ("point (1, 2).transformed (transform.translation (10, 20))"));
    EXPECT_EQ (Point<float> (1.0f, 2.0f), evaluate<Point<float>> ("point (1, 2).transformed (transform())"));
    EXPECT_EQ (Point<float> (3.0f, 2.0f), evaluate<Point<float>> ("point (1, 1).transformed (transform.scaling (2).followedBy (transform.translation (1, 0)))"));
    EXPECT_EQ (Rectangle<float> (2.0f, 4.0f, 6.0f, 8.0f), evaluate<Rectangle<float>> ("rect (1, 2, 3, 4).transformed (transform.scaling (2))"));

    const auto rotated = evaluate<Point<float>> ("point (1, 0).transformed (transform.rotation (pi / 2))");
    EXPECT_NEAR (0.0f, rotated.getX(), 1.0e-6f);
    EXPECT_NEAR (1.0f, rotated.getY(), 1.0e-6f);

    EXPECT_TRUE (evaluate<bool> ("transform.translation (1, 2).inverted() == transform.translation (-1, -2)"));
}

TEST_F (GraphicsExpressionTypesTests, PathOperations)
{
    EXPECT_TRUE (evaluate<bool> ("path().isEmpty"));
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 10.0f, 20.0f), evaluate<Rectangle<float>> ("path().moveTo (0, 0).lineTo (10, 0).lineTo (point (10, 20)).close().bounds"));
    EXPECT_EQ (Rectangle<float> (5.0f, 5.0f, 10.0f, 10.0f), evaluate<Rectangle<float>> ("path().addRectangle (rect (5, 5, 10, 10)).bounds"));
    EXPECT_EQ (Rectangle<float> (15.0f, 5.0f, 10.0f, 10.0f), evaluate<Rectangle<float>> ("path().addRectangle (rect (5, 5, 10, 10)).transformed (transform.translation (10, 0)).bounds"));
    EXPECT_NEAR (50.0f, evaluate<float> ("path (\"M 0 0 L 30 40\").length"), 1.0e-3f);

    const auto halfway = evaluate<Point<float>> ("path (\"M 0 0 L 30 40\").pointAt (0.5)");
    EXPECT_NEAR (15.0f, halfway.getX(), 1.0e-3f);
    EXPECT_NEAR (20.0f, halfway.getY(), 1.0e-3f);

    const auto star = evaluate<Path> ("path().addStar (point (0, 0), 5, 4, 10).withNonZeroWinding (false).appendPath (path().addRectangle (bounds))");
    EXPECT_FALSE (star.isUsingNonZeroWinding());
    EXPECT_FLOAT_EQ (100.0f, star.getBounds().getRight());
    EXPECT_FLOAT_EQ (50.0f, star.getBounds().getBottom());
}

TEST_F (GraphicsExpressionTypesTests, PathMethodsDontModifyTheirSource)
{
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 10.0f, 10.0f), evaluate<Rectangle<float>> (R"(
        let a = path().addRectangle (rect (0, 0, 10, 10));
        let b = a.addRectangle (rect (20, 20, 10, 10));
        b.isEmpty ? rect (0, 0, 0, 0) : a.bounds
    )"));
}

TEST_F (GraphicsExpressionTypesTests, GradientConstructors)
{
    const auto linear = evaluate<ColorGradient> ("linearGradient (rgb (255, 0, 0), point (0, 0), rgb (0, 0, 255), bounds.bottomRight)");
    EXPECT_EQ (ColorGradient::Linear, linear.getType());
    EXPECT_EQ (Color (0xffff0000), linear.getStartColor());
    EXPECT_EQ (Color (0xff0000ff), linear.getFinishColor());
    EXPECT_FLOAT_EQ (100.0f, linear.getFinishX());
    EXPECT_FLOAT_EQ (50.0f, linear.getFinishY());

    const auto radial = evaluate<ColorGradient> ("radialGradient (rgb (255, 0, 0), point (10, 10), rgb (0, 0, 255), 20)");
    EXPECT_EQ (ColorGradient::Radial, radial.getType());
    EXPECT_FLOAT_EQ (20.0f, radial.getRadius());

    EXPECT_EQ (ColorGradient::Angular, evaluate<ColorGradient> ("angularGradient (rgb (255, 0, 0), point (10, 10), rgb (0, 0, 255), point (20, 10))").getType());
}

TEST_F (GraphicsExpressionTypesTests, GradientOperations)
{
    const auto stopped = evaluate<ColorGradient> ("linearGradient (rgb (255, 0, 0), point (0, 0), rgb (0, 0, 255), point (100, 0)).withStop (rgb (0, 255, 0), 0.5)");
    ASSERT_EQ (3u, stopped.getNumStops());
    EXPECT_EQ (Color (0xff00ff00), stopped.getStop (1).color);
    EXPECT_FLOAT_EQ (50.0f, stopped.getStop (1).x);

    const auto faded = evaluate<ColorGradient> ("linearGradient (rgb (255, 0, 0), point (0, 0), rgb (0, 0, 255), point (100, 0)).withAlpha (2)");
    EXPECT_EQ (255, faded.getStartColor().getAlpha());

    EXPECT_TRUE (evaluate<bool> (R"(
        let a = linearGradient (rgb (255, 0, 0), point (0, 0), rgb (0, 0, 255), point (100, 0));
        a == a.withTransform (transform()) && a != a.withMultipliedAlpha (0.5)
    )"));
}

TEST_F (GraphicsExpressionTypesTests, Enumerations)
{
    EXPECT_EQ (StrokeCap::Square, evaluate<StrokeCap> ("square"));
    EXPECT_EQ (StrokeJoin::Bevel, evaluate<StrokeJoin> ("bevel"));
    EXPECT_EQ (StrokePosition::Outside, evaluate<StrokePosition> ("outside"));
    EXPECT_EQ (BlendMode::Multiply, evaluate<BlendMode> ("bounds.width > 50 ? multiply : screen"));
    EXPECT_EQ (Justification::topLeft, evaluate<Justification::Flags> ("topLeft"));

    EXPECT_FALSE (compiles ("round"));

    ExpressionContext drawingContext (context);
    ExpressionDrawing::declareOutputs (drawingContext);
    EXPECT_TRUE (CompiledExpression::compile ("strokeCap = round; strokeJoin = round; strokePosition = center; labelJustification = center", drawingContext).wasOk());
}

TEST_F (GraphicsExpressionTypesTests, ReportsInvalidUsage)
{
    EXPECT_FALSE (compiles ("rgb (1, 2)"));
    EXPECT_FALSE (compiles ("point (1, 2).z"));
    EXPECT_FALSE (compiles ("point (1, 2) + rect (0, 0, 1, 1)"));
    EXPECT_FALSE (compiles ("true ? point (1, 2) : rgb (0, 0, 0)"));
}
