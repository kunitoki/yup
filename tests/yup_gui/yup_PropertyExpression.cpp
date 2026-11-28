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

using namespace yup;

class PropertyExpressionTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    var evaluate (Component& component, const String& text, std::optional<ComponentProperty::Type> type = std::nullopt)
    {
        auto compiled = PropertyExpression::compile (component, text, type);
        EXPECT_TRUE (compiled.wasOk()) << text << ": " << compiled.getErrorMessage();

        return compiled.wasOk() ? compiled.getReference()->evaluate() : var();
    }

    String error (Component& component, const String& text, std::optional<ComponentProperty::Type> type = std::nullopt)
    {
        auto compiled = PropertyExpression::compile (component, text, type);
        return compiled.failed() ? compiled.getErrorMessage() : String();
    }

    static String heldAs (const var& value)
    {
        if (value.isVoid())
            return "void";

        if (value.isBool())
            return "bool";

        if (value.isInt())
            return "int";

        if (value.isInt64())
            return "int64";

        if (value.isDouble())
            return "double";

        return value.isString() ? "string" : "other";
    }

    /** Tells all there is to know of the expression a text makes: its type, its value and what the value is
        held as, and whether it reads anything. */
    String describe (Component& component, const String& text, std::optional<ComponentProperty::Type> type)
    {
        auto compiled = PropertyExpression::compile (component, text, type);

        if (compiled.failed())
            return "fails";

        auto& expression = *compiled.getReference();
        const auto value = expression.evaluate();

        String description;
        description << "type " << static_cast<int> (expression.getType()) << ", " << heldAs (value) << " " << value.toString();

        if (expression.hasInputs() || expression.readsPalette() || ! expression.getDependencies().empty())
            description << ", reads its component";

        return description;
    }

    static String split (const String& text, int from = 0)
    {
        StringArray elements;
        auto offset = from;

        const auto result = PropertyExpression::splitArrayLiteral (text, elements, offset);

        if (result.failed())
            return String (offset) + ": " + result.getErrorMessage();

        return String (offset) + ": " + elements.joinIntoString ("|");
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (PropertyExpressionTests, SplitsArrayLiterals)
{
    EXPECT_EQ (split ("[]"), "2: ");
    EXPECT_EQ (split ("  [ ]  "), "5: ");
    EXPECT_EQ (split ("[1, a.b * 2]"), "12: 1|a.b * 2");
    EXPECT_EQ (split ("[[1, 2], [3]] trailing"), "13: [1, 2]|[3]");
    EXPECT_EQ (split ("x = [1]", 4), "7: 1");

    EXPECT_EQ (split ("1"), "0: Expected '['");
    EXPECT_EQ (split ("[1 2]"), "3: Expected ',' or ']'");
    EXPECT_EQ (split ("[1, [2]"), "7: Expected ',' or ']'");
    EXPECT_EQ (split ("[1, ]"), "4: Unexpected character ']'");
}

TEST_F (PropertyExpressionTests, ProbesTheTypeOfAValue)
{
    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 40, 20);

    EXPECT_EQ (PropertyExpression::compile (slider, "0.5", std::nullopt).getReference()->getType(), ComponentProperty::Type::number);
    EXPECT_EQ (PropertyExpression::compile (slider, "true", std::nullopt).getReference()->getType(), ComponentProperty::Type::boolean);
    EXPECT_EQ (PropertyExpression::compile (slider, "\"hi\"", std::nullopt).getReference()->getType(), ComponentProperty::Type::string);
    EXPECT_EQ (PropertyExpression::compile (slider, "#ff0000", std::nullopt).getReference()->getType(), ComponentProperty::Type::color);
    EXPECT_EQ (PropertyExpression::compile (slider, "point (1, 2)", std::nullopt).getReference()->getType(), ComponentProperty::Type::point);
    EXPECT_EQ (PropertyExpression::compile (slider, "rect (1, 2, 3, 4)", std::nullopt).getReference()->getType(), ComponentProperty::Type::rectangle);
    EXPECT_EQ (PropertyExpression::compile (slider, "[1]", std::nullopt).getReference()->getType(), ComponentProperty::Type::array);
}

TEST_F (PropertyExpressionTests, AnIntegerTakesANumberCutToAWholeOne)
{
    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 45, 20);

    const auto integer = ComponentProperty::Type::integer;

    // A number that is not whole is cut as `int (...)` cuts it, so the numbers of a model need no cast
    EXPECT_TRUE (evaluate (slider, "2.75", integer).isInt());
    EXPECT_EQ (static_cast<int> (evaluate (slider, "2.75", integer)), 2);
    EXPECT_EQ (static_cast<int> (evaluate (slider, "-2.75", integer)), -2);
    EXPECT_EQ (static_cast<int> (evaluate (slider, "width / 10", integer)), 4);
    EXPECT_EQ (static_cast<int> (evaluate (slider, "width / 10", integer)), static_cast<int> (evaluate (slider, "int (width / 10)", integer)));

    // Whole numbers are kept as they are, however large
    EXPECT_EQ (static_cast<int> (evaluate (slider, "7 / 2", integer)), 3);
    EXPECT_EQ (static_cast<int> (evaluate (slider, "16777217", integer)), 16777217);
    EXPECT_EQ (static_cast<int> (evaluate (slider, "true", integer)), 1);
    EXPECT_EQ (static_cast<int> (evaluate (slider, "1e12", integer)), std::numeric_limits<int>::max());

    // What is not a number is still refused
    EXPECT_FALSE (error (slider, "\"12\"", integer).isEmpty());
    EXPECT_FALSE (error (slider, "point (1, 2)", integer).isEmpty());
}

TEST_F (PropertyExpressionTests, EvaluatesPointsAndRectangles)
{
    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 40, 20);

    EXPECT_EQ (VariantConverter<Point<float>>::fromVar (evaluate (slider, "point (1, width)")), Point<float> (1.0f, 40.0f));
    EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (evaluate (slider, "rect (1, 2, width, height)", ComponentProperty::Type::rectangle)),
               Rectangle<float> (1.0f, 2.0f, 40.0f, 20.0f));
}

TEST_F (PropertyExpressionTests, AnArrayMakesThePointOrTheRectangleOfAProperty)
{
    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 40, 20);

    EXPECT_EQ (VariantConverter<Point<float>>::fromVar (evaluate (slider, "[1, width]", ComponentProperty::Type::point)), Point<float> (1.0f, 40.0f));
    EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (evaluate (slider, "[1, 2, width, height]", ComponentProperty::Type::rectangle)),
               Rectangle<float> (1.0f, 2.0f, 40.0f, 20.0f));

    // An array has no members of what its numbers make until it is given to it
    EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (evaluate (slider, "rect ([1, 2, 10, 10]).enlarged (1)", ComponentProperty::Type::rectangle)),
               Rectangle<float> (0.0f, 1.0f, 12.0f, 12.0f));

    // Two numbers make a point and four a rectangle, and not the other way round
    EXPECT_TRUE (error (slider, "[1, 2, 3]", ComponentProperty::Type::point).contains ("float[3]"));
    EXPECT_FALSE (error (slider, "[1, 2, 3, 4]", ComponentProperty::Type::point).isEmpty());

    // With no type to be of, it is the var holding its elements
    EXPECT_EQ (PropertyExpression::compile (slider, "[1, 2]", std::nullopt).getReference()->getType(), ComponentProperty::Type::array);
}

TEST_F (PropertyExpressionTests, EvaluatesArrayLiterals)
{
    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 40, 20);
    slider.setRange (0.0, 1.0, 0.0);
    slider.setValue (0.5, dontSendNotification);

    const auto elementsOf = [] (const var& value)
    {
        StringArray texts;

        if (const auto* elements = value.getArray())
            for (const auto& element : *elements)
                texts.add (element.toString());

        return texts.joinIntoString ("|");
    };

    EXPECT_EQ (evaluate (slider, "[]").getArray()->size(), 0);
    EXPECT_EQ (elementsOf (evaluate (slider, "[1, value * 2, \"a\", true]")), "1.0|1.0|a|1");

    // Read at an index, a literal is part of an expression
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "[10, 20, 30][1]", ComponentProperty::Type::number)), 20.0);

    // A nested literal is one element, an array of its own
    const auto nested = evaluate (slider, "[[1, 2], 3]", ComponentProperty::Type::array);
    ASSERT_NE (nested.getArray(), nullptr);
    ASSERT_EQ (nested.getArray()->size(), 2);
    EXPECT_EQ (elementsOf ((*nested.getArray())[0]), "1.0|2.0");

    // An array of points keeps each element in its var form
    const auto points = evaluate (slider, "[point (1, 2), point (3, 4)]");
    ASSERT_NE (points.getArray(), nullptr);
    ASSERT_EQ (points.getArray()->size(), 2);
    EXPECT_EQ (VariantConverter<Point<float>>::fromVar ((*points.getArray())[1]), Point<float> (3.0f, 4.0f));
}

TEST_F (PropertyExpressionTests, EvaluatesAnArrayValueOfAComponent)
{
    const ComponentTypeInfo extended (Identifier ("Knob"),
                                      &Slider::getClassTypeInfo(),
                                      { ComponentProperty { .name = Identifier ("lanes"), .type = ComponentProperty::Type::array, .elementType = ComponentProperty::Type::number } });

    Slider slider { Slider::LinearHorizontal };
    slider.setTypeInfo (&extended);
    slider.setProperty (Identifier ("lanes"), var (Array<var> { 2.0, 4.0 }));

    auto compiled = PropertyExpression::compile (slider, "lanes", ComponentProperty::Type::array);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
    EXPECT_TRUE (compiled.getReference()->hasInputs());

    const auto value = compiled.getReference()->evaluate();
    ASSERT_NE (value.getArray(), nullptr);
    ASSERT_EQ (value.getArray()->size(), 2);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*value.getArray())[1]), 4.0);

    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "len (lanes)")), 2.0);
}

TEST_F (PropertyExpressionTests, ReportsTheDependenciesAndPaletteUseOfTheElements)
{
    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 40, 20);

    auto constant = PropertyExpression::compile (slider, "[1, 2]", std::nullopt);
    ASSERT_TRUE (constant.wasOk());
    EXPECT_FALSE (constant.getReference()->hasInputs());
    EXPECT_FALSE (constant.getReference()->readsPalette());
    EXPECT_TRUE (constant.getReference()->getDependencies().empty());

    auto reactive = PropertyExpression::compile (slider, "[width, palette.accent]", std::nullopt);
    ASSERT_TRUE (reactive.wasOk()) << reactive.getErrorMessage();
    EXPECT_TRUE (reactive.getReference()->hasInputs());
    EXPECT_TRUE (reactive.getReference()->readsPalette());

    const auto dependencies = reactive.getReference()->getDependencies();
    ASSERT_EQ (dependencies.size(), 1u);
    EXPECT_EQ (dependencies.front().property, Identifier ("width"));
}

TEST_F (PropertyExpressionTests, ReportsTheProblemsOfAnArrayValue)
{
    Slider slider { Slider::LinearHorizontal };

    EXPECT_EQ (error (slider, "[1]", ComponentProperty::Type::number), "An array can't be the value of this property");
    EXPECT_EQ (error (slider, "[1 2]"), "Expected ',' or ']'");
    EXPECT_EQ (error (slider, "[1] 2"), "Expected the end of the array");
    EXPECT_TRUE (error (slider, "[1, nope]").contains ("nope"));
}

TEST_F (PropertyExpressionTests, KeepsEvaluatingAfterItsComponentIsDestroyed)
{
    auto slider = std::make_unique<Slider> (Slider::LinearHorizontal);
    slider->setBounds (0, 0, 40, 20);

    auto compiled = PropertyExpression::compile (*slider, "[1, width]", std::nullopt);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    const auto before = compiled.getReference()->evaluate();
    ASSERT_NE (before.getArray(), nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*before.getArray())[1]), 40.0);

    slider.reset();

    // The elements keep the values they last read, nothing reads the destroyed component
    const auto after = compiled.getReference()->evaluate();
    ASSERT_NE (after.getArray(), nullptr);
    ASSERT_EQ (after.getArray()->size(), 2);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*after.getArray())[1]), 40.0);
}

TEST_F (PropertyExpressionTests, LoadsFontFilesRelativeToTheBaseDirectory)
{
    Component component;
    const auto fonts = File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts");

    auto sized = PropertyExpression::compile (component, "font (\"YupColrTest.ttf\", 18)", ComponentProperty::Type::font, nullptr, nullptr, fonts);
    ASSERT_TRUE (sized.wasOk()) << sized.getErrorMessage();

    const auto font = VariantConverter<Font>::fromVar (sized.getReference()->evaluate());
    EXPECT_FALSE (font.isEmpty());
    EXPECT_FLOAT_EQ (font.getHeight(), 18.0f);

    auto inferred = PropertyExpression::compile (component, "font (\"YupColrTest.ttf\").withHeight (9)", std::nullopt, nullptr, nullptr, fonts);
    ASSERT_TRUE (inferred.wasOk()) << inferred.getErrorMessage();
    EXPECT_EQ (inferred.getReference()->getType(), ComponentProperty::Type::font);
    EXPECT_EQ (VariantConverter<Font>::fromVar (inferred.getReference()->evaluate()).getFont().get(), font.getFont().get());
}

TEST_F (PropertyExpressionTests, FontFileThatCannotBeLoadedGivesTheThemeFont)
{
    const auto fonts = File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts");
    const auto themeFont = Font::loadFontFromFile (fonts.getChildFile ("YupColrTest.ttf"));
    ASSERT_TRUE (themeFont.wasOk()) << themeFont.getErrorMessage();
    theme->setDefaultFont (themeFont.getValue());

    Component component;
    const auto font = VariantConverter<Font>::fromVar (evaluate (component, "font (\"missing.ttf\", 11)", ComponentProperty::Type::font));

    EXPECT_EQ (font.getFont().get(), themeFont.getValue().getFont().get());
    EXPECT_FLOAT_EQ (font.getHeight(), 11.0f);
}

TEST_F (PropertyExpressionTests, CreatesTheContextExpressionsAreCompiledWith)
{
    StringArray signatures;

    for (const auto& function : PropertyExpression::createContext().getFunctions())
        signatures.add (function.getSignature());

    const auto hasFunction = [&signatures] (const String& start)
    {
        return std::any_of (signatures.begin(), signatures.end(), [&start] (const String& signature)
        {
            return signature.startsWith (start);
        });
    };

    EXPECT_TRUE (hasFunction ("font (string"));
    EXPECT_TRUE (hasFunction ("rect ("));
    EXPECT_TRUE (hasFunction ("clamp ("));
    EXPECT_TRUE (hasFunction (".withAlpha ("));
}

TEST_F (PropertyExpressionTests, ALiteralIsWhatTheCompilerMakesOfIt)
{
    using Type = ComponentProperty::Type;

    Component component;

    const std::optional<Type> types[] = {
        std::nullopt,
        Type::any,
        Type::number,
        Type::integer,
        Type::boolean,
        Type::string,
        Type::color,
        Type::enumeration,
        Type::array
    };

    const StringArray literals {
        "0", "200", "-7", "-0", "007", "999999999", "1234567890", "2147483648", "12345678901", "16777217",
        "0.1", "-2.75", "-0.0", "0.30000000000000004", "123456789012.5", "1e3", "1E-2", "2.5e+3", "1e999", " 42\n",
        "true", "false", " true ",
        "\"Reset\"", "\"\"", "\" a  b \"", "\"font\"", "\"100% of it's\"", "\"true\"", "\"12\""
    };

    for (const auto& literal : literals)
    {
        for (const auto& type : types)
        {
            // Between parentheses the same text is an expression, which only the compiler reads
            EXPECT_EQ (describe (component, literal, type), describe (component, "(" + literal + ")", type))
                << "'" << literal << "' for the type " << (type.has_value() ? static_cast<int> (*type) : -1);
        }
    }

    // What is told of them is what is told of an expression giving the same value
    EXPECT_EQ (describe (component, "200", std::nullopt), describe (component, "100 + 100", std::nullopt));
    EXPECT_EQ (describe (component, "200", Type::number), describe (component, "100 + 100", Type::number));
    EXPECT_EQ (describe (component, "200", Type::integer), describe (component, "100 + 100", Type::integer));
    EXPECT_EQ (describe (component, "true", Type::boolean), describe (component, "! false", Type::boolean));
    EXPECT_EQ (describe (component, "\"Reset\"", Type::string), describe (component, "\"Re\" + \"set\"", Type::string));

    // A number is a number, an integer of a property that is one, and nothing reads the component
    EXPECT_EQ (describe (component, "200", std::nullopt), "type " + String (static_cast<int> (Type::number)) + ", double 200.0");
    EXPECT_TRUE (evaluate (component, "200", Type::integer).isInt());
    EXPECT_TRUE (evaluate (component, "true").isBool());
    EXPECT_EQ (evaluate (component, "\"Reset\"").toString(), "Reset");
    EXPECT_EQ (PropertyExpression::compile (component, " 200 ", std::nullopt).getReference()->getSource(), " 200 ");
}

TEST_F (PropertyExpressionTests, WhatOnlyLooksLikeALiteralIsLeftToTheCompiler)
{
    using Type = ComponentProperty::Type;

    Slider slider { Slider::LinearHorizontal };
    slider.setBounds (0, 0, 40, 20);

    // Units, suffixes, signs, other bases and comments
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "50%")), 0.5);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "250ms")), 0.25);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "10px")), 10.0);
    EXPECT_NEAR (static_cast<double> (evaluate (slider, "180deg")), MathConstants<double>::pi, 1.0e-9);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "1.5f")), 1.5);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "0x10")), 16.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, ".5")), 0.5);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "+3")), 3.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "- 3")), -3.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "2 // two")), 2.0);
    EXPECT_TRUE (error (slider, "10em").contains ("em"));
    EXPECT_FALSE (error (slider, "1.").isEmpty());
    EXPECT_FALSE (error (slider, "1e").isEmpty());

    // A string with an escape, and what only starts and ends with a quote
    EXPECT_EQ (evaluate (slider, "\"a\\\"b\"").toString(), "a\"b");
    EXPECT_EQ (evaluate (slider, "\"a\\nb\"").toString(), "a\nb");
    EXPECT_EQ (evaluate (slider, "\"a\" + \"b\"").toString(), "ab");
    EXPECT_FALSE (error (slider, "\"a\nb\"").isEmpty());

    // A name: a property in scope, a constant, the value of an enumeration, and what only starts as a boolean does
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "width")), 40.0);
    EXPECT_TRUE (PropertyExpression::compile (slider, "width", std::nullopt).getReference()->hasInputs());
    EXPECT_NEAR (static_cast<double> (evaluate (slider, "pi")), MathConstants<double>::pi, 1.0e-6);
    EXPECT_TRUE (error (slider, "round").contains ("ambiguous"));
    EXPECT_FALSE (error (slider, "none").isEmpty());
    EXPECT_TRUE (VariantConverter<AffineTransform>::fromVar (evaluate (slider, "none", Type::transform)).isIdentity());
    EXPECT_TRUE (error (slider, "truthy").contains ("truthy"));

    // A literal of another type than the property is converted, or refused, by the compiler
    EXPECT_DOUBLE_EQ (static_cast<double> (evaluate (slider, "true", Type::number)), 1.0);
    EXPECT_FALSE (error (slider, "\"12\"", Type::number).isEmpty());
    EXPECT_FALSE (error (slider, "12", Type::array).isEmpty());
}
