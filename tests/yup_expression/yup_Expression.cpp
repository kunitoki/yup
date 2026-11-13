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
class ExpressionTests : public ::testing::Test
{
protected:
    static Expression parse (StringRef text)
    {
        auto result = Expression::parse (text);
        EXPECT_TRUE (result.wasOk()) << (result.failed() ? result.getErrorMessage() : String()).toStdString();
        return result.wasOk() ? result.getValue() : Expression();
    }

    static String parseError (StringRef text)
    {
        auto result = Expression::parse (text);
        EXPECT_TRUE (result.failed());
        return result.failed() ? result.getErrorMessage() : String();
    }

    static double evaluate (StringRef text)
    {
        auto result = parse (text).evaluate();
        EXPECT_TRUE (result.wasOk()) << (result.failed() ? result.getErrorMessage() : String()).toStdString();
        return result.valueOr (0.0);
    }
};

//==============================================================================
TEST_F (ExpressionTests, ParsesLiterals)
{
    auto integer = parse ("42");
    EXPECT_EQ (Expression::Kind::constant, integer.getKind());
    EXPECT_EQ (42, (int) integer.getConstantValue());

    EXPECT_EQ (255, (int) parse ("0xFF").getConstantValue());
    EXPECT_DOUBLE_EQ (1000.0, (double) parse ("1e3").getConstantValue());
    EXPECT_DOUBLE_EQ (0.25, (double) parse (".25").getConstantValue());
    EXPECT_FLOAT_EQ (1.5f, (float) (double) parse ("1.5f").getConstantValue());
    EXPECT_TRUE ((bool) parse ("true").getConstantValue());
    EXPECT_EQ (String ("a\"b\n"), parse ("\"a\\\"b\\n\"").getConstantValue().toString());
}

TEST_F (ExpressionTests, NegativeLiteralsAreFolded)
{
    auto e = parse ("-3");
    EXPECT_EQ (Expression::Kind::constant, e.getKind());
    EXPECT_EQ (-3, (int) e.getConstantValue());
}

TEST_F (ExpressionTests, ParsesStructure)
{
    auto e = parse ("a + b * c");
    ASSERT_EQ (Expression::Kind::binary, e.getKind());
    EXPECT_EQ (ExpressionOperator::add, e.getOperator());
    EXPECT_EQ (2, e.getNumInputs());
    EXPECT_EQ (String ("a"), e.getInput (0).getName());
    EXPECT_EQ (ExpressionOperator::multiply, e.getInput (1).getOperator());

    auto method = parse ("p.distanceTo (q)");
    ASSERT_EQ (Expression::Kind::method, method.getKind());
    EXPECT_EQ (String ("distanceTo"), method.getName());
    EXPECT_EQ (String ("p"), method.getInput (0).getName());

    auto program = parse ("let w = 2; y = w");
    ASSERT_EQ (Expression::Kind::block, program.getKind());
    EXPECT_EQ (Expression::Kind::let, program.getInput (0).getKind());
    EXPECT_EQ (Expression::Kind::output, program.getInput (1).getKind());
}

TEST_F (ExpressionTests, OperatorPrecedence)
{
    EXPECT_DOUBLE_EQ (14.0, evaluate ("2 + 3 * 4"));
    EXPECT_DOUBLE_EQ (20.0, evaluate ("(2 + 3) * 4"));
    EXPECT_DOUBLE_EQ (-4.0, evaluate ("-2 ^ 2"));
    EXPECT_DOUBLE_EQ (512.0, evaluate ("2 ^ 3 ^ 2"));
    EXPECT_DOUBLE_EQ (1.0, evaluate ("1 < 2 && 3 > 2 || false"));
    EXPECT_DOUBLE_EQ (3.0, evaluate ("1 /* two */ + // comment\n 2"));
}

TEST_F (ExpressionTests, ToStringRoundTrips)
{
    for (const auto* text : { "a + b * c",
                              "(a + b) * c",
                              "a - (b - c)",
                              "a ? b : c ? d : e",
                              "(a ? b : c) ? d : e",
                              "-a.b",
                              "f (x, y).z",
                              "values[i + 1]",
                              "p.distanceTo (q)",
                              "!(a && b) || c",
                              "\"text\\n\"",
                              "(-2) ^ 2",
                              "2 ^ 3 ^ 2",
                              "(2 ^ 3) ^ 2",
                              "1.5f + 0.1 + 2.0 + 1e+20",
                              "a ? #ff8800 : #ff880080.withAlpha (0.5)",
                              "let x = 1; y = x * 2" })
    {
        const auto printed = parse (text).toString();
        EXPECT_EQ (String (text), printed);
        EXPECT_EQ (printed, parse (printed).toString());
    }
}

TEST_F (ExpressionTests, ParsesUnits)
{
    EXPECT_DOUBLE_EQ (MathConstants<double>::halfPi, (double) parse ("90deg").getConstantValue());
    EXPECT_DOUBLE_EQ (-0.75 * MathConstants<double>::pi, (double) parse ("-135deg").getConstantValue());
    EXPECT_DOUBLE_EQ (0.15, (double) parse ("150ms").getConstantValue());
    EXPECT_DOUBLE_EQ (0.5, (double) parse ("50%").getConstantValue());
    EXPECT_DOUBLE_EQ (1.5, (double) parse ("1.5s").getConstantValue());
    EXPECT_DOUBLE_EQ (0.5, (double) parse ("0.5rad").getConstantValue());
    EXPECT_TRUE (parse ("10px").getConstantValue().isInt());
    EXPECT_EQ (10, (int) parse ("10px").getConstantValue());
    EXPECT_EQ (2, (int) parse ("2s").getConstantValue());
    EXPECT_TRUE (parseError ("5em").contains ("Unknown unit 'em'"));
}

TEST_F (ExpressionTests, PercentFollowedByAnOperandIsModulo)
{
    EXPECT_EQ (ExpressionOperator::modulo, parse ("10%3").getOperator());
    EXPECT_EQ (ExpressionOperator::modulo, parse ("10 % 3").getOperator());
    EXPECT_EQ (ExpressionOperator::modulo, parse ("10%(x)").getOperator());
    EXPECT_DOUBLE_EQ (1.0, evaluate ("10%3"));
    EXPECT_DOUBLE_EQ (1.0, evaluate ("50% * 2"));
    EXPECT_DOUBLE_EQ (0.25, evaluate ("(25%)"));
}

TEST_F (ExpressionTests, ParsesColorLiterals)
{
    const auto color = parse ("#ff8800");
    ASSERT_EQ (Expression::Kind::call, color.getKind());
    EXPECT_EQ (String ("color"), color.getName());
    ASSERT_EQ (1, color.getNumInputs());
    EXPECT_EQ ((int) 0xffff8800u, (int) color.getInput (0).getConstantValue());

    EXPECT_EQ ((int) 0xffff8800u, (int) parse ("#F80").getInput (0).getConstantValue());
    EXPECT_EQ ((int) 0x80ff8800u, (int) parse ("#ff880080").getInput (0).getConstantValue());
    EXPECT_EQ ((int) 0x88ff8800u, (int) parse ("#f808").getInput (0).getConstantValue());
    EXPECT_EQ (String ("#ff8800"), parse ("#F80").toString());
    EXPECT_EQ (String ("#00000080"), Expression::call ("color", { Expression ((int) 0x80000000u) }).toString());

    EXPECT_TRUE (parseError ("#ff88000").contains ("Invalid color literal"));
    EXPECT_TRUE (parseError ("#ffgg00").contains ("Invalid color literal"));
    EXPECT_TRUE (parseError ("#").contains ("Invalid color literal"));
}

TEST_F (ExpressionTests, ParsesExpressionsEmbeddedInText)
{
    const String text ("fill: a > 1 ? #ff8800 : b; next: f (x, y) }");

    int offset = 6;
    const auto fill = Expression::parseExpression (text, offset);
    ASSERT_TRUE (fill.wasOk());
    EXPECT_EQ (Expression::Kind::ternary, fill.getValue().getKind());
    EXPECT_EQ (text.indexOfChar (';'), offset);

    offset = text.indexOf ("f (");
    const auto next = Expression::parseExpression (text, offset);
    ASSERT_TRUE (next.wasOk());
    EXPECT_EQ (Expression::Kind::call, next.getValue().getKind());
    EXPECT_EQ (text.indexOfChar ('}'), offset);

    const String arguments ("g (a + 1, b)");
    offset = 3;
    EXPECT_TRUE (Expression::parseExpression (arguments, offset).wasOk());
    EXPECT_EQ (arguments.indexOfChar (','), offset);

    offset = arguments.indexOfChar ('b');
    EXPECT_TRUE (Expression::parseExpression (arguments, offset).wasOk());
    EXPECT_EQ (arguments.indexOfChar (')'), offset);
}

TEST_F (ExpressionTests, EmbeddedParseErrorsReportOffsets)
{
    const String text ("a: 1;\nb: 2 +* 3");

    int offset = text.indexOfChar ('2');
    const auto result = Expression::parseExpression (text, offset);
    ASSERT_TRUE (result.failed());
    EXPECT_EQ (String ("2:7: Unexpected character '*'"), result.getErrorMessage());
    EXPECT_EQ (text.indexOfChar ('*'), offset);

    offset = text.length();
    EXPECT_TRUE (Expression::parseExpression (text, offset).getErrorMessage().contains ("Unexpected end of expression"));
    EXPECT_EQ (text.length(), offset);
}

TEST_F (ExpressionTests, ParseErrorsReportPositions)
{
    EXPECT_EQ (String ("1:4: Unexpected end of expression"), parseError ("1 +"));
    EXPECT_EQ (String ("2:8: Unexpected character '*'"), parseError ("let a = 1;\nb = a +* 2"));
    EXPECT_EQ (String ("1:1: Unterminated string literal"), parseError ("\"abc"));
    EXPECT_TRUE (parseError ("1; 2").startsWith ("1:1: Only the last statement"));
    EXPECT_TRUE (parseError ("(1 + 2").contains ("Expected ')'"));
    EXPECT_TRUE (parseError ("99999999999").contains ("too large"));
    EXPECT_TRUE (parseError ("12abc").contains ("Unknown unit 'abc'"));
    EXPECT_TRUE (parseError ("/* open").contains ("Unterminated comment"));
    EXPECT_TRUE (parseError ("").contains ("Expected an expression"));
}

TEST_F (ExpressionTests, DeepNestingFailsGracefully)
{
    EXPECT_TRUE (parseError (String::repeatedString ("(", 1000) + "1" + String::repeatedString (")", 1000)).contains ("too deeply nested"));
}

TEST_F (ExpressionTests, ReferencedSymbolsExcludeLocals)
{
    const auto symbols = parse ("let k = a * 2; out = k + b.c + sin (d); z = out").getReferencedSymbols();
    EXPECT_EQ (StringArray ({ "a", "b.c", "d" }), symbols);

    EXPECT_TRUE (parse ("x + 1").referencesSymbol ("x"));
    EXPECT_FALSE (parse ("let x = 1; y = x").referencesSymbol ("x"));
}

TEST_F (ExpressionTests, RenamesFreeSymbols)
{
    EXPECT_EQ (String ("z + z.x + b"), parse ("a + a.x + b").withRenamedSymbol ("a", "z").toString());
    EXPECT_EQ (String ("let a = 1; y = a + b"), parse ("let a = 1; y = a + b").withRenamedSymbol ("a", "q").toString());
}

TEST_F (ExpressionTests, BuildsExpressions)
{
    const auto e = Expression::symbol ("x") * Expression (2) + Expression (1.5f);
    EXPECT_EQ (String ("x * 2 + 1.5f"), e.toString());
    EXPECT_EQ (String ("-x"), (-Expression::symbol ("x")).toString());
    EXPECT_EQ (String ("max (1, 2)"), Expression::call ("max", { Expression (1), Expression (2) }).toString());
    EXPECT_EQ (String ("\"hi\""), Expression::text ("hi").toString());
    EXPECT_EQ (String ("(1 + 2) * 3"), ((Expression (1) + Expression (2)) * Expression (3)).toString());
}

TEST_F (ExpressionTests, EvaluatesWithoutContext)
{
    EXPECT_DOUBLE_EQ (5.0, evaluate ("max (1, 2.5) * 2"));
    EXPECT_NEAR (1.0, evaluate ("sin (pi / 2)"), 1.0e-12);

    const auto failed = parse ("\"a\" + \"b\"").evaluate();
    ASSERT_TRUE (failed.failed());
    EXPECT_TRUE (failed.getErrorMessage().contains ("not numeric"));

    EXPECT_TRUE (parse ("unknown + 1").evaluate().failed());
}

TEST_F (ExpressionTests, EvaluatesWithContext)
{
    ExpressionContext context;
    context.addConstant ("answer", 42);

    const auto result = parse ("answer / 2").evaluate (context);
    ASSERT_TRUE (result.wasOk());
    EXPECT_DOUBLE_EQ (21.0, result.getValue());
}
