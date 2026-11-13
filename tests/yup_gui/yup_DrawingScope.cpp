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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

#include <memory>

using namespace yup;

class DrawingScopeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        addGraphicsExpressionTypes (context);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    var evaluate (DrawingScope& scope, const String& text)
    {
        scope.install();

        auto compiled = CompiledExpression::compile (text, context);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        if (compiled.failed())
            return {};

        const auto& program = compiled.getReference();
        const int outputIndex = program.findOutput ("out");
        EXPECT_GE (outputIndex, 0);

        ExpressionState state (program);
        scope.updateInputs (program, state);
        state.evaluate();

        return state.getVar (outputIndex);
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    ExpressionContext context;
};

TEST_F (DrawingScopeTest, ResolvesNumberPropertyByBareName)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (3.0, dontSendNotification);

    DrawingScope scope (slider, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = value * 2 + 1")), 7.0);
}

TEST_F (DrawingScopeTest, ResolvesNumberPropertyBySelfPrefix)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (4.0, dontSendNotification);

    DrawingScope scope (slider, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = self.value")), 4.0);
}

TEST_F (DrawingScopeTest, ResolvesBooleanProperty)
{
    Slider slider (Slider::LinearHorizontal);

    DrawingScope scope (slider, context);

    // A slider that is not being dragged reports dragging == false.
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = dragging ? 1 : 0")), 0.0);
}

TEST_F (DrawingScopeTest, ResolvesColorPropertyChannels)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setColor (Slider::Style::thumbColorId, Color (128, 64, 32)); // r, g, b (opaque)

    DrawingScope scope (slider, context);

    EXPECT_NEAR (static_cast<double> (evaluate (scope, "out = thumbColor.g")), 64.0 / 255.0, 1.0e-4);
}

TEST_F (DrawingScopeTest, ResolvesParentProperty)
{
    Slider parent (Slider::LinearHorizontal);
    parent.setRange (0.0, 20.0, 0.0);
    parent.setValue (8.0, dontSendNotification);

    Slider child (Slider::LinearHorizontal);
    parent.addChildComponent (child);

    DrawingScope scope (child, context);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (scope, "out = parent.value")), 8.0);
}

TEST_F (DrawingScopeTest, UnknownSymbolFailsToCompile)
{
    Slider slider (Slider::LinearHorizontal);

    DrawingScope scope (slider, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("out = notAProperty", context);
    EXPECT_TRUE (compiled.failed());
}

TEST_F (DrawingScopeTest, DestroyedComponentFeedsNothing)
{
    auto slider = std::make_unique<Slider> (Slider::LinearHorizontal);
    slider->setRange (0.0, 10.0, 0.0);
    slider->setValue (5.0, dontSendNotification);

    DrawingScope scope (*slider, context);
    scope.install();

    auto compiled = CompiledExpression::compile ("out = value + 1", context);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto& program = compiled.getReference();
    const int outputIndex = program.findOutput ("out");

    slider.reset();

    ExpressionState state (program);
    scope.updateInputs (program, state); // must not touch the destroyed component
    state.evaluate();

    // value defaults to 0 when the component is gone, so the result is just the constant.
    EXPECT_DOUBLE_EQ (static_cast<double> (state.getVar (outputIndex)), 1.0);
}
