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

#include <yup_expression/yup_expression.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class ExpressionContextTests : public ::testing::Test
{
protected:
    CompiledExpression compile (StringRef text)
    {
        auto result = CompiledExpression::compile (text, context);
        EXPECT_TRUE (result.wasOk()) << (result.failed() ? result.getErrorMessage() : String()).toStdString();
        return result.wasOk() ? result.getValue() : CompiledExpression();
    }

    String compileError (StringRef text)
    {
        auto result = CompiledExpression::compile (text, context);
        EXPECT_TRUE (result.failed());
        return result.failed() ? result.getErrorMessage() : String();
    }

    var evaluate (StringRef text)
    {
        ExpressionState state (compile (text));
        state.evaluate();
        return state.getVar (0);
    }

    ExpressionContext context;
};

//==============================================================================
TEST_F (ExpressionContextTests, DeclaresInputs)
{
    const auto a = context.addInput<float> ("a");
    const auto b = context.addInput<int> ("b");
    const auto values = context.addArray<double> ("values");

    EXPECT_EQ (0, a.index);
    EXPECT_EQ (1, b.index);
    EXPECT_EQ (2, values.index);
    EXPECT_EQ (3, context.getNumInputs());
    EXPECT_EQ (1, context.findInput ("b"));
    EXPECT_EQ (-1, context.findInput ("missing"));
    EXPECT_EQ (String ("a"), context.getInputName (0));
    EXPECT_EQ (&ExpressionType::floatType(), context.getInputType (0));
    EXPECT_EQ (String ("double[]"), context.getInputType (2)->getName());
    EXPECT_EQ (&ExpressionType::doubleType(), context.getInputType (2)->getElementType());
    EXPECT_EQ (context.getInputType (2), context.getType<ExpressionArrayView<double>>());
}

TEST_F (ExpressionContextTests, FindsTypes)
{
    EXPECT_EQ (&ExpressionType::intType(), context.findType ("int"));
    EXPECT_EQ (&ExpressionType::stringType(), context.getType<String>());
    EXPECT_EQ (nullptr, context.findType ("missing"));
    EXPECT_TRUE (ExpressionType::floatType().isNumeric());
    EXPECT_FALSE (ExpressionType::stringType().isNumeric());
    EXPECT_TRUE (ExpressionType::doubleType().isTriviallyCopyable());
    EXPECT_FALSE (ExpressionType::stringType().isTriviallyCopyable());
}

TEST_F (ExpressionContextTests, ConstantsAreFolded)
{
    context.addConstant ("answer", 42);
    context.addConstant ("label", String ("value"));

    const auto program = compile ("a = answer * 2; b = label + \"!\"");
    EXPECT_TRUE (program.isConstant());
    EXPECT_EQ (0, program.getNumInstructions());

    ExpressionState state (program);
    state.evaluate();
    EXPECT_EQ (84, (int) state.getVar (0));
    EXPECT_EQ (String ("value!"), state.getVar (1).toString());
}

TEST_F (ExpressionContextTests, NamedExpressionsAreInlined)
{
    const auto x = context.addInput<float> ("x");
    context.addExpression ("twice", Expression::parse ("x * 2").getValue());
    EXPECT_TRUE (context.addExpression ("area", "let side = twice + 1; side * side").wasOk());
    EXPECT_TRUE (context.addExpression ("broken", "1 +").failed());

    const auto program = compile ("y = area + twice");
    ExpressionState state (program);
    state.set (x, 1.0f);
    state.evaluate();
    EXPECT_DOUBLE_EQ (11.0, (double) state.getVar (0));
}

TEST_F (ExpressionContextTests, NamedExpressionCyclesFail)
{
    context.addExpression ("a", "b + 1");
    context.addExpression ("b", "a + 1");

    EXPECT_TRUE (compileError ("a").contains ("references itself"));
}

TEST_F (ExpressionContextTests, NamedExpressionsCantSeeProgramLocals)
{
    context.addExpression ("usesLocal", "local * 2");

    EXPECT_TRUE (compileError ("let local = 1; y = usesLocal").contains ("Unknown symbol 'local'"));
}

TEST_F (ExpressionContextTests, AddsFunctions)
{
    context.addFunction ("square", [] (float x) { return x * x; });

    const auto program = compile ("square (3.0f)");
    EXPECT_TRUE (program.isConstant());
    EXPECT_EQ (&ExpressionType::floatType(), program.getOutputType (0));
    EXPECT_DOUBLE_EQ (9.0, (double) evaluate ("square (3.0f)"));
    EXPECT_DOUBLE_EQ (6.25, (double) evaluate ("square (2.5)"));
}

TEST_F (ExpressionContextTests, ImpureFunctionsAreNotFolded)
{
    int counter = 0;
    context.addFunction ("tick", [&counter] { return ++counter; }, false);

    const auto program = compile ("tick () + tick ()");
    EXPECT_FALSE (program.isConstant());
    EXPECT_EQ (3, program.getNumInstructions());

    ExpressionState state (program);
    state.evaluate();
    EXPECT_EQ (3, (int) state.getVar (0));
}

TEST_F (ExpressionContextTests, FunctionsReplaceBuiltinsWithSameSignature)
{
    context.addFunction ("sin", [] (double) { return 42.0; });

    EXPECT_DOUBLE_EQ (42.0, (double) evaluate ("sin (1.0)"));
    EXPECT_NEAR (std::sin (1.0f), (double) evaluate ("sin (1.0f)"), 1.0e-6);
}

TEST_F (ExpressionContextTests, OverloadsPickTheCheapestConversion)
{
    context.addFunction ("pick", [] (int) { return 1; });
    context.addFunction ("pick", [] (float) { return 2; });
    context.addFunction ("pick", [] (const String&) { return 3; });

    EXPECT_EQ (1, (int) evaluate ("pick (3)"));
    EXPECT_EQ (2, (int) evaluate ("pick (3.0f)"));
    EXPECT_EQ (2, (int) evaluate ("pick (3.5)"));
    EXPECT_EQ (1, (int) evaluate ("pick (true)"));
    EXPECT_EQ (3, (int) evaluate ("pick (\"x\")"));
}

TEST_F (ExpressionContextTests, AmbiguousOverloadsFail)
{
    context.addFunction ("amb", [] (float, double) { return 0; });
    context.addFunction ("amb", [] (double, float) { return 1; });

    EXPECT_TRUE (compileError ("amb (1.0f, 1.0f)").contains ("ambiguous"));
    EXPECT_EQ (1, (int) evaluate ("amb (double (1), 1.0f)"));
}

TEST_F (ExpressionContextTests, QualifiedFunctionNames)
{
    context.addFunction ("math.twice", [] (int x) { return x * 2; });

    EXPECT_EQ (8, (int) evaluate ("math.twice (4)"));
}

TEST_F (ExpressionContextTests, DottedInputNames)
{
    const auto value = context.addInput<float> ("row.value");

    ExpressionState state (compile ("row.value * 2"));
    state.set (value, 1.5f);
    state.evaluate();
    EXPECT_DOUBLE_EQ (3.0, (double) state.getVar (0));
}

TEST_F (ExpressionContextTests, DeclaredOutputsConvert)
{
    context.declareOutput<float> ("y");
    context.declareOutput<bool> ("flag");

    const auto program = compile ("y = 1; z = 1");
    EXPECT_EQ (&ExpressionType::floatType(), program.getOutputType (0));
    EXPECT_EQ (&ExpressionType::intType(), program.getOutputType (1));

    EXPECT_TRUE (compileError ("flag = 1").contains ("Cannot convert 'int' to 'bool'"));
}

TEST_F (ExpressionContextTests, ProgramsOutliveTheirContext)
{
    CompiledExpression program;

    {
        ExpressionContext temporary;
        const int offset = 10;
        temporary.addInput<int> ("v");
        temporary.addFunction ("addOffset", [offset] (int v) { return v + offset; });
        program = CompiledExpression::compile ("addOffset (v)", temporary).getValue();
    }

    ExpressionState state (program);
    EXPECT_TRUE (state.setVar ("v", 5).wasOk());
    state.evaluate();
    EXPECT_EQ (15, (int) state.getVar (0));
}

//==============================================================================
class ExpressionTypeBuilderTests : public ::testing::Test
{
protected:
    struct Vec2
    {
        float x = 0.0f, y = 0.0f;

        bool operator== (const Vec2& other) const { return x == other.x && y == other.y; }

        bool operator!= (const Vec2& other) const { return ! (*this == other); }
    };

    struct Tag
    {
        String text;
        int weight = 0;
    };

    void SetUp() override
    {
        context.addType<Vec2> ("vec2")
            .constructor ([] (float x, float y) { return Vec2 { x, y }; })
            .constructor ("splat", [] (float v) { return Vec2 { v, v }; })
            .member ("x", &Vec2::x)
            .member ("y", &Vec2::y)
            .member ("length", [] (const Vec2& v) { return std::sqrt (v.x * v.x + v.y * v.y); })
            .method ("dot", [] (const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; })
            .method ("scaled", [] (const Vec2& v, float s) { return Vec2 { v.x * s, v.y * s }; })
            .operation (ExpressionOperator::add, [] (const Vec2& a, const Vec2& b) { return Vec2 { a.x + b.x, a.y + b.y }; })
            .operation (ExpressionOperator::multiply, [] (const Vec2& v, float s) { return Vec2 { v.x * s, v.y * s }; })
            .operation (ExpressionOperator::negate, [] (const Vec2& v) { return Vec2 { -v.x, -v.y }; })
            .equality()
            .components ({ "x", "y" });

        context.addType<Tag> ("tag")
            .constructor ([] (const String& text, int weight) { return Tag { text, weight }; })
            .member ("text", &Tag::text)
            .member ("weight", &Tag::weight)
            .conversion ([] (const Tag& t) { return t.text; });

        input = context.addInput<Vec2> ("a");
    }

    CompiledExpression compile (StringRef text)
    {
        auto result = CompiledExpression::compile (text, context);
        EXPECT_TRUE (result.wasOk()) << (result.failed() ? result.getErrorMessage() : String()).toStdString();
        return result.wasOk() ? result.getValue() : CompiledExpression();
    }

    String compileError (StringRef text)
    {
        auto result = CompiledExpression::compile (text, context);
        EXPECT_TRUE (result.failed());
        return result.failed() ? result.getErrorMessage() : String();
    }

    template <class T>
    T evaluate (StringRef text, Vec2 a = { 2.0f, 3.0f })
    {
        const auto program = compile (text);
        const auto output = program.getOutput<T> ("result");
        EXPECT_TRUE (output.wasOk());

        if (output.failed())
            return {};

        ExpressionState state (program);
        state.set (input, a);
        state.evaluate();
        return state.get (output.getValue());
    }

    ExpressionContext context;
    ExpressionInput<Vec2> input;
};

//==============================================================================
TEST_F (ExpressionTypeBuilderTests, RegistersTheType)
{
    const auto* type = context.getType<Vec2>();
    ASSERT_NE (nullptr, type);
    EXPECT_EQ (String ("vec2"), type->getName());
    EXPECT_EQ (type, context.findType ("vec2"));
    EXPECT_EQ (StringArray ({ "x", "y" }), type->getComponentNames());
    EXPECT_TRUE (type->isTriviallyCopyable());
    EXPECT_FALSE (context.getType<Tag>()->isTriviallyCopyable());
}

TEST_F (ExpressionTypeBuilderTests, ConstructorsAndMembers)
{
    EXPECT_EQ ((Vec2 { 1.0f, 2.0f }), evaluate<Vec2> ("vec2 (1, 2)"));
    EXPECT_EQ ((Vec2 { 4.0f, 4.0f }), evaluate<Vec2> ("splat (4)"));
    EXPECT_FLOAT_EQ (5.0f, evaluate<float> ("a.x + a.y"));
    EXPECT_FLOAT_EQ (5.0f, evaluate<float> ("vec2 (3, 4).length"));
}

TEST_F (ExpressionTypeBuilderTests, MethodsAndOperators)
{
    EXPECT_EQ ((Vec2 { 6.0f, 8.0f }), evaluate<Vec2> ("(a + vec2 (1, 1)) * 2"));
    EXPECT_EQ ((Vec2 { -2.0f, -3.0f }), evaluate<Vec2> ("-a"));
    EXPECT_EQ ((Vec2 { 1.0f, 1.5f }), evaluate<Vec2> ("a.scaled (0.5)"));
    EXPECT_FLOAT_EQ (2.0f, evaluate<float> ("a.dot (vec2 (1, 0))"));
    EXPECT_TRUE (evaluate<bool> ("a == vec2 (2, 3)"));
    EXPECT_FALSE (evaluate<bool> ("a != a"));
}

TEST_F (ExpressionTypeBuilderTests, Swizzles)
{
    EXPECT_EQ ((Vec2 { 3.0f, 2.0f }), evaluate<Vec2> ("a.yx"));
    EXPECT_EQ ((Vec2 { 2.0f, 2.0f }), evaluate<Vec2> ("a.xx"));
    EXPECT_EQ ((Vec2 { 3.0f, 2.0f }), evaluate<Vec2> ("let p = a; p.yx"));
}

TEST_F (ExpressionTypeBuilderTests, TernarySelectsCustomValues)
{
    EXPECT_EQ ((Vec2 { 2.0f, 3.0f }), evaluate<Vec2> ("a.x > 1 ? a : splat (0)"));
    EXPECT_EQ ((Vec2 { 0.0f, 0.0f }), evaluate<Vec2> ("a.x > 1 ? a : splat (0)", { 0.5f, 1.0f }));
}

TEST_F (ExpressionTypeBuilderTests, CustomConstants)
{
    context.addConstant ("origin", Vec2 { 1.0f, 1.0f });

    EXPECT_EQ ((Vec2 { 3.0f, 4.0f }), evaluate<Vec2> ("a + origin"));
}

TEST_F (ExpressionTypeBuilderTests, NonTrivialTypesAndConversions)
{
    context.declareOutput<String> ("label");

    const auto program = compile ("let t = tag (\"hello\", 3); w = t.weight; label = t; size = len (t); text = string (t) + \"!\"");
    EXPECT_FALSE (program.isAllocationFree());

    ExpressionState state (program);
    state.evaluate();
    EXPECT_EQ (3, (int) state.getVar (program.findOutput ("w")));
    EXPECT_EQ (String ("hello"), state.getVar (program.findOutput ("label")).toString());
    EXPECT_EQ (5, (int) state.getVar (program.findOutput ("size")));
    EXPECT_EQ (String ("hello!"), state.getVar (program.findOutput ("text")).toString());
}

TEST_F (ExpressionTypeBuilderTests, ReportsUnknownMembersAndMethods)
{
    EXPECT_EQ (String ("1:1: The type 'vec2' has no member 'z'"), compileError ("a.z"));
    EXPECT_TRUE (compileError ("a.foo (1)").contains ("The type 'vec2' has no method 'foo'"));
    EXPECT_TRUE (compileError ("a * a").contains ("No matching overload for '*' with 'vec2' and 'vec2'"));
    EXPECT_TRUE (compileError ("a ? 1 : 2").contains ("can't be used as a condition"));
}
