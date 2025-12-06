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

#include <yup_core/yup_core.h>

using namespace yup;

namespace
{
// Helper scope for testing custom symbols and functions
class TestScope : public Expression::Scope
{
public:
    TestScope()
    {
        symbols.set ("x", 5.0);
        symbols.set ("y", 10.0);
        symbols.set ("pi", 3.14159265358979323846);
    }

    String getScopeUID() const override
    {
        return "TestScope";
    }

    Expression getSymbolValue (const String& symbol) const override
    {
        if (symbols.contains (symbol))
            return Expression (symbols[symbol]);

        return Expression::Scope::getSymbolValue (symbol);
    }

    double evaluateFunction (const String& functionName, const double* parameters, int numParams) const override
    {
        if (functionName == "square" && numParams == 1)
            return parameters[0] * parameters[0];

        if (functionName == "add" && numParams == 2)
            return parameters[0] + parameters[1];

        return Expression::Scope::evaluateFunction (functionName, parameters, numParams);
    }

    HashMap<String, double> symbols;
};

// Helper scope for testing dot operator
class NestedScope : public Expression::Scope
{
public:
    NestedScope()
    {
        innerValue = 42.0;
    }

    String getScopeUID() const override
    {
        return "NestedScope";
    }

    Expression getSymbolValue (const String& symbol) const override
    {
        if (symbol == "value")
            return Expression (innerValue);

        return Expression::Scope::getSymbolValue (symbol);
    }

    double innerValue;
};

class OuterScope : public Expression::Scope
{
public:
    String getScopeUID() const override
    {
        return "OuterScope";
    }

    void visitRelativeScope (const String& scopeName, Visitor& visitor) const override
    {
        if (scopeName == "inner")
        {
            NestedScope nested;
            visitor.visit (nested);
        }
        else
        {
            Expression::Scope::visitRelativeScope (scopeName, visitor);
        }
    }
};
} // namespace

// ==============================================================================
// Constructor Tests
// ==============================================================================

TEST (ExpressionTests, DefaultConstructorCreatesZero)
{
    Expression e;
    EXPECT_EQ (0.0, e.evaluate());
    EXPECT_EQ (Expression::constantType, e.getType());
}

TEST (ExpressionTests, ConstantConstructorCreatesCorrectValue)
{
    Expression e (42.5);
    EXPECT_EQ (42.5, e.evaluate());
    EXPECT_EQ (Expression::constantType, e.getType());
}

TEST (ExpressionTests, NegativeConstantConstructor)
{
    Expression e (-123.45);
    EXPECT_EQ (-123.45, e.evaluate());
}

TEST (ExpressionTests, CopyConstructorCreatesIndependentCopy)
{
    Expression e1 (100.0);
    Expression e2 (e1);

    EXPECT_EQ (100.0, e2.evaluate());
    EXPECT_EQ (e1.evaluate(), e2.evaluate());
}

TEST (ExpressionTests, CopyAssignmentOperator)
{
    Expression e1 (200.0);
    Expression e2;

    e2 = e1;
    EXPECT_EQ (200.0, e2.evaluate());
}

TEST (ExpressionTests, MoveConstructor)
{
    Expression e1 (300.0);
    Expression e2 (std::move (e1));

    EXPECT_EQ (300.0, e2.evaluate());
}

TEST (ExpressionTests, MoveAssignmentOperator)
{
    Expression e1 (400.0);
    Expression e2;

    e2 = std::move (e1);
    EXPECT_EQ (400.0, e2.evaluate());
}

// ==============================================================================
// String Parsing Tests
// ==============================================================================

TEST (ExpressionTests, ParseSimpleNumber)
{
    String error;
    Expression e ("42", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (42.0, e.evaluate());
}

TEST (ExpressionTests, ParseDecimalNumber)
{
    String error;
    Expression e ("3.14159", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_NEAR (3.14159, e.evaluate(), 0.00001);
}

TEST (ExpressionTests, ParseNegativeNumber)
{
    String error;
    Expression e ("-99.5", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (-99.5, e.evaluate());
}

TEST (ExpressionTests, ParseAddition)
{
    String error;
    Expression e ("10 + 20", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (30.0, e.evaluate());
}

TEST (ExpressionTests, ParseSubtraction)
{
    String error;
    Expression e ("50 - 30", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (20.0, e.evaluate());
}

TEST (ExpressionTests, ParseMultiplication)
{
    String error;
    Expression e ("6 * 7", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (42.0, e.evaluate());
}

TEST (ExpressionTests, ParseDivision)
{
    String error;
    Expression e ("100 / 4", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (25.0, e.evaluate());
}

TEST (ExpressionTests, ParseComplexExpression)
{
    String error;
    Expression e ("2 + 3 * 4", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (14.0, e.evaluate()); // Respects operator precedence
}

TEST (ExpressionTests, ParseExpressionWithParentheses)
{
    String error;
    Expression e ("(2 + 3) * 4", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (20.0, e.evaluate());
}

TEST (ExpressionTests, ParseNestedParentheses)
{
    String error;
    Expression e ("((2 + 3) * (4 + 1))", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (25.0, e.evaluate());
}

TEST (ExpressionTests, ParseUnaryMinus)
{
    String error;
    Expression e ("-(5 + 3)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (-8.0, e.evaluate());
}

TEST (ExpressionTests, ParseUnaryPlus)
{
    String error;
    Expression e ("+42", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (42.0, e.evaluate());
}

TEST (ExpressionTests, ParseInvalidSyntaxReturnsError)
{
    String error;
    Expression e ("10 +", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseInvalidCharactersReturnsError)
{
    String error;
    Expression e ("10 $ 20", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseEmptyString)
{
    String error;
    Expression e ("", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (0.0, e.evaluate());
}

TEST (ExpressionTests, ParseWhitespaceOnly)
{
    String error;
    Expression e ("   ", error);

    EXPECT_FALSE (error.isEmpty());
    EXPECT_EQ (0.0, e.evaluate());
}

// ==============================================================================
// Arithmetic Operator Tests
// ==============================================================================

TEST (ExpressionTests, AdditionOperator)
{
    Expression e1 (10.0);
    Expression e2 (20.0);
    Expression result = e1 + e2;

    EXPECT_EQ (30.0, result.evaluate());
    EXPECT_EQ (Expression::operatorType, result.getType());
}

TEST (ExpressionTests, SubtractionOperator)
{
    Expression e1 (50.0);
    Expression e2 (20.0);
    Expression result = e1 - e2;

    EXPECT_EQ (30.0, result.evaluate());
}

TEST (ExpressionTests, MultiplicationOperator)
{
    Expression e1 (6.0);
    Expression e2 (7.0);
    Expression result = e1 * e2;

    EXPECT_EQ (42.0, result.evaluate());
}

TEST (ExpressionTests, DivisionOperator)
{
    Expression e1 (100.0);
    Expression e2 (4.0);
    Expression result = e1 / e2;

    EXPECT_EQ (25.0, result.evaluate());
}

TEST (ExpressionTests, UnaryNegationOperator)
{
    Expression e (42.0);
    Expression result = -e;

    EXPECT_EQ (-42.0, result.evaluate());
}

TEST (ExpressionTests, ChainedOperations)
{
    Expression a (2.0);
    Expression b (3.0);
    Expression c (4.0);
    Expression result = a + b * c;

    EXPECT_EQ (14.0, result.evaluate());
}

TEST (ExpressionTests, DivisionByZeroReturnsInfinity)
{
    Expression e1 (10.0);
    Expression e2 (0.0);
    Expression result = e1 / e2;

    double value = result.evaluate();
    EXPECT_TRUE (std::isinf (value));
}

// ==============================================================================
// Built-in Function Tests
// ==============================================================================

TEST (ExpressionTests, ParseSinFunction)
{
    String error;
    Expression e ("sin(0)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_NEAR (0.0, e.evaluate(), 0.0001);
}

TEST (ExpressionTests, ParseCosFunction)
{
    String error;
    Expression e ("cos(0)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_NEAR (1.0, e.evaluate(), 0.0001);
}

TEST (ExpressionTests, ParseTanFunction)
{
    String error;
    Expression e ("tan(0)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_NEAR (0.0, e.evaluate(), 0.0001);
}

TEST (ExpressionTests, ParseAbsFunction)
{
    String error;
    Expression e ("abs(-42)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (42.0, e.evaluate());
}

TEST (ExpressionTests, ParseMinFunction)
{
    String error;
    Expression e ("min(10, 20, 5, 30)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (5.0, e.evaluate());
}

TEST (ExpressionTests, ParseMaxFunction)
{
    String error;
    Expression e ("max(10, 20, 5, 30)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (30.0, e.evaluate());
}

TEST (ExpressionTests, ParseMinFunctionWithTwoArgs)
{
    String error;
    Expression e ("min(10, 5)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (5.0, e.evaluate());
}

TEST (ExpressionTests, ParseFunctionWithExpressionAsArgument)
{
    String error;
    Expression e ("abs(5 - 10)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (5.0, e.evaluate());
}

TEST (ExpressionTests, ParseNestedFunctions)
{
    String error;
    Expression e ("abs(sin(0) - 1)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_NEAR (1.0, e.evaluate(), 0.0001);
}

TEST (ExpressionTests, ParseUnknownFunctionReturnsErrorOnEvaluation)
{
    String error;
    Expression e ("unknownFunc(42)", error);

    EXPECT_TRUE (error.isEmpty()); // Parsing succeeds
    EXPECT_EQ (Expression::functionType, e.getType());

    String evalError;
    e.evaluate (Expression::Scope(), evalError);
    EXPECT_FALSE (evalError.isEmpty());
}

TEST (ExpressionTests, ParseEmptyFunctionCall)
{
    String error;
    Expression e ("cos()", error);

    EXPECT_TRUE (error.isEmpty());
}

// ==============================================================================
// Symbol Tests
// ==============================================================================

TEST (ExpressionTests, CreateSymbolExpression)
{
    Expression e = Expression::symbol ("x");

    EXPECT_EQ (Expression::symbolType, e.getType());
    EXPECT_EQ ("x", e.getSymbolOrFunction());
    EXPECT_TRUE (e.usesAnySymbols());
}

TEST (ExpressionTests, ParseSymbolExpression)
{
    String error;
    Expression e ("x + 10", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_TRUE (e.usesAnySymbols());
}

TEST (ExpressionTests, EvaluateSymbolWithCustomScope)
{
    String error;
    Expression e ("x + y", error);
    TestScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (15.0, e.evaluate (scope)); // x=5, y=10
}

TEST (ExpressionTests, EvaluateUnknownSymbolReturnsError)
{
    String error;
    Expression e ("unknownSymbol", error);

    EXPECT_TRUE (error.isEmpty()); // Parsing succeeds

    String evalError;
    e.evaluate (Expression::Scope(), evalError);
    EXPECT_FALSE (evalError.isEmpty());
}

TEST (ExpressionTests, SymbolsInComplexExpression)
{
    String error;
    Expression e ("x * 2 + y / 5", error);
    TestScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (12.0, e.evaluate (scope)); // (5*2) + (10/5) = 12
}

TEST (ExpressionTests, UsesAnySymbolsReturnsFalseForConstants)
{
    Expression e (42.0);
    EXPECT_FALSE (e.usesAnySymbols());
}

TEST (ExpressionTests, UsesAnySymbolsReturnsTrueForSymbols)
{
    Expression e = Expression::symbol ("x");
    EXPECT_TRUE (e.usesAnySymbols());
}

TEST (ExpressionTests, ReferencesSymbolFindsSymbol)
{
    String error;
    Expression e ("x + y", error);
    TestScope scope;

    Expression::Symbol symbolX ("TestScope", "x");
    Expression::Symbol symbolY ("TestScope", "y");
    Expression::Symbol symbolZ ("TestScope", "z");

    EXPECT_TRUE (e.referencesSymbol (symbolX, scope));
    EXPECT_TRUE (e.referencesSymbol (symbolY, scope));
    EXPECT_FALSE (e.referencesSymbol (symbolZ, scope));
}

TEST (ExpressionTests, FindReferencedSymbols)
{
    String error;
    Expression e ("x + y * 2", error);
    TestScope scope;

    Array<Expression::Symbol> symbols;
    e.findReferencedSymbols (symbols, scope);

    EXPECT_EQ (2, symbols.size());
}

TEST (ExpressionTests, WithRenamedSymbol)
{
    String error;
    Expression e ("x + 10", error);
    TestScope scope;

    Expression::Symbol symbolX ("TestScope", "x");
    Expression renamed = e.withRenamedSymbol (symbolX, "newX", scope);

    EXPECT_EQ ("newX + 10", renamed.toString());
}

TEST (ExpressionTests, WithRenamedSymbolDoesNotChangeOriginal)
{
    String error;
    Expression e ("x + 10", error);
    TestScope scope;

    Expression::Symbol symbolX ("TestScope", "x");
    Expression renamed = e.withRenamedSymbol (symbolX, "newX", scope);

    EXPECT_EQ ("x + 10", e.toString());
    EXPECT_EQ ("newX + 10", renamed.toString());
}

// ==============================================================================
// Function Tests
// ==============================================================================

TEST (ExpressionTests, CreateFunctionExpression)
{
    Array<Expression> params;
    params.add (Expression (5.0));

    Expression e = Expression::function ("square", params);

    EXPECT_EQ (Expression::functionType, e.getType());
    EXPECT_EQ ("square", e.getSymbolOrFunction());
}

TEST (ExpressionTests, EvaluateFunctionWithCustomScope)
{
    String error;
    Expression e ("square(5)", error);
    TestScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (25.0, e.evaluate (scope));
}

TEST (ExpressionTests, EvaluateFunctionWithMultipleParameters)
{
    String error;
    Expression e ("add(10, 20)", error);
    TestScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (30.0, e.evaluate (scope));
}

TEST (ExpressionTests, FunctionWithSymbolArguments)
{
    String error;
    Expression e ("square(x)", error);
    TestScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (25.0, e.evaluate (scope)); // x=5, so square(5)=25
}

// ==============================================================================
// ToString Tests
// ==============================================================================

TEST (ExpressionTests, ToStringForConstant)
{
    Expression e (42.5);
    EXPECT_EQ ("42.5", e.toString());
}

TEST (ExpressionTests, ToStringForSymbol)
{
    Expression e = Expression::symbol ("myVar");
    EXPECT_EQ ("myVar", e.toString());
}

TEST (ExpressionTests, ToStringForAddition)
{
    Expression e1 (10.0);
    Expression e2 (20.0);
    Expression result = e1 + e2;

    EXPECT_EQ ("10 + 20", result.toString());
}

TEST (ExpressionTests, ToStringForSubtraction)
{
    Expression e1 (50.0);
    Expression e2 (30.0);
    Expression result = e1 - e2;

    EXPECT_EQ ("50 - 30", result.toString());
}

TEST (ExpressionTests, ToStringForMultiplication)
{
    Expression e1 (6.0);
    Expression e2 (7.0);
    Expression result = e1 * e2;

    EXPECT_EQ ("6 * 7", result.toString());
}

TEST (ExpressionTests, ToStringForDivision)
{
    Expression e1 (100.0);
    Expression e2 (4.0);
    Expression result = e1 / e2;

    EXPECT_EQ ("100 / 4", result.toString());
}

TEST (ExpressionTests, ToStringForNegation)
{
    Expression e (42.0);
    Expression result = -e;

    EXPECT_EQ ("-42", result.toString());
}

TEST (ExpressionTests, ToStringRespectsOperatorPrecedence)
{
    String error;
    Expression e ("2 + 3 * 4", error);

    EXPECT_EQ ("2 + 3 * 4", e.toString());
}

TEST (ExpressionTests, ToStringWithParentheses)
{
    String error;
    Expression e ("(2 + 3) * 4", error);

    EXPECT_EQ ("(2 + 3) * 4", e.toString());
}

TEST (ExpressionTests, ToStringForFunction)
{
    Array<Expression> params;
    params.add (Expression (5.0));
    params.add (Expression (10.0));

    Expression e = Expression::function ("myFunc", params);
    EXPECT_EQ ("myFunc (5, 10)", e.toString());
}

TEST (ExpressionTests, ToStringForFunctionWithNoParams)
{
    Array<Expression> params;
    Expression e = Expression::function ("myFunc", params);

    EXPECT_EQ ("myFunc()", e.toString());
}

// ==============================================================================
// Type and Input Tests
// ==============================================================================

TEST (ExpressionTests, GetTypeForConstant)
{
    Expression e (42.0);
    EXPECT_EQ (Expression::constantType, e.getType());
}

TEST (ExpressionTests, GetTypeForSymbol)
{
    Expression e = Expression::symbol ("x");
    EXPECT_EQ (Expression::symbolType, e.getType());
}

TEST (ExpressionTests, GetTypeForOperator)
{
    Expression e1 (10.0);
    Expression e2 (20.0);
    Expression result = e1 + e2;

    EXPECT_EQ (Expression::operatorType, result.getType());
}

TEST (ExpressionTests, GetTypeForFunction)
{
    Array<Expression> params;
    params.add (Expression (5.0));

    Expression e = Expression::function ("func", params);
    EXPECT_EQ (Expression::functionType, e.getType());
}

TEST (ExpressionTests, GetSymbolOrFunctionForSymbol)
{
    Expression e = Expression::symbol ("mySymbol");
    EXPECT_EQ ("mySymbol", e.getSymbolOrFunction());
}

TEST (ExpressionTests, GetSymbolOrFunctionForFunction)
{
    Array<Expression> params;
    Expression e = Expression::function ("myFunc", params);

    EXPECT_EQ ("myFunc", e.getSymbolOrFunction());
}

TEST (ExpressionTests, GetSymbolOrFunctionForOperator)
{
    Expression e1 (10.0);
    Expression e2 (20.0);
    Expression result = e1 + e2;

    EXPECT_EQ ("+", result.getSymbolOrFunction());
}

TEST (ExpressionTests, GetNumInputsForConstant)
{
    Expression e (42.0);
    EXPECT_EQ (0, e.getNumInputs());
}

TEST (ExpressionTests, GetNumInputsForBinaryOperator)
{
    Expression e1 (10.0);
    Expression e2 (20.0);
    Expression result = e1 + e2;

    EXPECT_EQ (2, result.getNumInputs());
}

TEST (ExpressionTests, GetNumInputsForUnaryOperator)
{
    Expression e (42.0);
    Expression result = -e;

    EXPECT_EQ (0, result.getNumInputs());
}

TEST (ExpressionTests, GetNumInputsForFunction)
{
    Array<Expression> params;
    params.add (Expression (5.0));
    params.add (Expression (10.0));
    params.add (Expression (15.0));

    Expression e = Expression::function ("func", params);
    EXPECT_EQ (3, e.getNumInputs());
}

TEST (ExpressionTests, GetInputForBinaryOperator)
{
    Expression e1 (10.0);
    Expression e2 (20.0);
    Expression result = e1 + e2;

    Expression input0 = result.getInput (0);
    Expression input1 = result.getInput (1);

    EXPECT_EQ (10.0, input0.evaluate());
    EXPECT_EQ (20.0, input1.evaluate());
}

TEST (ExpressionTests, GetInputForFunction)
{
    Array<Expression> params;
    params.add (Expression (5.0));
    params.add (Expression (10.0));

    Expression e = Expression::function ("func", params);

    Expression input0 = e.getInput (0);
    Expression input1 = e.getInput (1);

    EXPECT_EQ (5.0, input0.evaluate());
    EXPECT_EQ (10.0, input1.evaluate());
}

// ==============================================================================
// Parse Static Method Tests
// ==============================================================================

TEST (ExpressionTests, ParseStaticMethodAdvancesPointer)
{
    String input = "10 + 20, 30";
    auto ptr = input.getCharPointer();
    String error;

    Expression e = Expression::parse (ptr, error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (30.0, e.evaluate());
    EXPECT_EQ (' ', *ptr); // Should be pointing at the space before '30'
}

TEST (ExpressionTests, ParseStaticMethodWithError)
{
    String input = "10 +";
    auto ptr = input.getCharPointer();
    String error;

    Expression e = Expression::parse (ptr, error);

    EXPECT_FALSE (error.isEmpty());
}

// ==============================================================================
// AdjustedToGiveNewResult Tests
// ==============================================================================

TEST (ExpressionTests, AdjustedToGiveNewResultForSimpleAddition)
{
    String error;
    Expression e ("x + 10", error);
    TestScope scope;

    // x=5, so x+10=15. We want it to equal 20.
    Expression adjusted = e.adjustedToGiveNewResult (20.0, scope);

    EXPECT_EQ (20.0, adjusted.evaluate (scope));
}

TEST (ExpressionTests, AdjustedToGiveNewResultForConstant)
{
    Expression e (42.0);
    Expression::Scope scope;

    Expression adjusted = e.adjustedToGiveNewResult (100.0, scope);

    EXPECT_EQ (100.0, adjusted.evaluate (scope));
}

TEST (ExpressionTests, AdjustedToGiveNewResultForMultiplication)
{
    String error;
    Expression e ("x * 2", error);
    TestScope scope;

    // x=5, so x*2=10. We want it to equal 20.
    Expression adjusted = e.adjustedToGiveNewResult (20.0, scope);

    EXPECT_EQ (20.0, adjusted.evaluate (scope));
}

TEST (ExpressionTests, AdjustedToGiveNewResultWithResolutionTarget)
{
    String error;
    Expression e ("x + @10", error);
    TestScope scope;

    // The @10 is a resolution target, so it should be adjusted
    Expression adjusted = e.adjustedToGiveNewResult (20.0, scope);

    EXPECT_EQ (20.0, adjusted.evaluate (scope));
}

// ==============================================================================
// Dot Operator Tests
// ==============================================================================

TEST (ExpressionTests, ParseDotOperator)
{
    String error;
    Expression e ("inner.value", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_TRUE (e.usesAnySymbols());
}

TEST (ExpressionTests, EvaluateDotOperator)
{
    String error;
    Expression e ("inner.value", error);
    OuterScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (42.0, e.evaluate (scope));
}

TEST (ExpressionTests, ParseThisKeyword)
{
    String error;
    Expression e ("this.value", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ ("value", e.toString());
}

TEST (ExpressionTests, DotOperatorInComplexExpression)
{
    String error;
    Expression e ("inner.value * 2", error);
    OuterScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (84.0, e.evaluate (scope));
}

// ==============================================================================
// Edge Cases and Error Handling
// ==============================================================================

TEST (ExpressionTests, ComplexNestedExpression)
{
    String error;
    Expression e ("((10 + 5) * (20 - 8)) / (3 + 1)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (45.0, e.evaluate());
}

TEST (ExpressionTests, ExpressionWithMultipleSpaces)
{
    String error;
    Expression e ("  10   +   20  ", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (30.0, e.evaluate());
}

TEST (ExpressionTests, ParseMissingClosingParenthesis)
{
    String error;
    Expression e ("(10 + 20", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseMissingOpeningParenthesis)
{
    String error;
    Expression e ("10 + 20)", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseIncompleteExpression)
{
    String error;
    Expression e ("10 + * 20", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseFunctionMissingClosingParenthesis)
{
    String error;
    Expression e ("sin(10", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseFunctionMissingComma)
{
    String error;
    Expression e ("min(10 20)", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ParseTrailingComma)
{
    String error;
    Expression e ("min(10, 20,)", error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, EvaluateWithVeryLargeNumbers)
{
    Expression e1 (1e100);
    Expression e2 (1e100);
    Expression result = e1 + e2;

    EXPECT_EQ (2e100, result.evaluate());
}

TEST (ExpressionTests, EvaluateWithVerySmallNumbers)
{
    Expression e1 (1e-100);
    Expression e2 (1e-100);
    Expression result = e1 + e2;

    EXPECT_NEAR (2e-100, result.evaluate(), 1e-110);
}

TEST (ExpressionTests, DoubleNegation)
{
    Expression e (42.0);
    Expression result = -(-e);

    EXPECT_EQ (42.0, result.evaluate());
}

TEST (ExpressionTests, ChainedNegations)
{
    String error;
    Expression e ("---42", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (-42.0, e.evaluate());
}

TEST (ExpressionTests, MultipleOperationsWithSamePrecedence)
{
    String error;
    Expression e ("10 - 5 - 2", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (3.0, e.evaluate());
}

TEST (ExpressionTests, DivisionAndMultiplicationChained)
{
    String error;
    Expression e ("100 / 5 * 2", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (40.0, e.evaluate());
}

// ==============================================================================
// Symbol Equality Tests
// ==============================================================================

TEST (ExpressionTests, SymbolEqualityOperator)
{
    Expression::Symbol s1 ("scope1", "symbol1");
    Expression::Symbol s2 ("scope1", "symbol1");
    Expression::Symbol s3 ("scope2", "symbol1");
    Expression::Symbol s4 ("scope1", "symbol2");

    EXPECT_TRUE (s1 == s2);
    EXPECT_FALSE (s1 == s3);
    EXPECT_FALSE (s1 == s4);
}

TEST (ExpressionTests, SymbolInequalityOperator)
{
    Expression::Symbol s1 ("scope1", "symbol1");
    Expression::Symbol s2 ("scope2", "symbol2");

    EXPECT_TRUE (s1 != s2);
    EXPECT_FALSE (s1 != s1);
}

// ==============================================================================
// Scope Tests
// ==============================================================================

TEST (ExpressionTests, DefaultScopeHasEmptyUID)
{
    Expression::Scope scope;
    EXPECT_TRUE (scope.getScopeUID().isEmpty());
}

TEST (ExpressionTests, CustomScopeUID)
{
    TestScope scope;
    EXPECT_EQ ("TestScope", scope.getScopeUID());
}

TEST (ExpressionTests, ScopeThrowsOnUnknownSymbol)
{
    Expression::Scope scope;
    String error;

    Expression e = Expression::symbol ("unknown");
    e.evaluate (scope, error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ScopeThrowsOnUnknownFunction)
{
    Expression::Scope scope;
    String error;

    Array<Expression> params;
    params.add (Expression (42.0));

    Expression e = Expression::function ("unknownFunc", params);
    e.evaluate (scope, error);

    EXPECT_FALSE (error.isEmpty());
}

TEST (ExpressionTests, ScopeThrowsOnUnknownRelativeScope)
{
    Expression::Scope scope;
    String error;

    Expression e = Expression::symbol ("unknown");
    e.evaluate (scope, error);

    EXPECT_FALSE (error.isEmpty());
}

// ==============================================================================
// Resolution Target Tests
// ==============================================================================

TEST (ExpressionTests, ParseResolutionTarget)
{
    String error;
    Expression e ("@10", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (10.0, e.evaluate());
    EXPECT_EQ ("@10", e.toString());
}

TEST (ExpressionTests, ResolutionTargetInExpression)
{
    String error;
    Expression e ("x + @5", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ ("x + @5", e.toString());
}

// ==============================================================================
// Identifier Validation Tests
// ==============================================================================

TEST (ExpressionTests, ParseIdentifierStartingWithUnderscore)
{
    String error;
    Expression e ("_myVar + 10", error);
    TestScope scope;
    scope.symbols.set ("_myVar", 100.0);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (110.0, e.evaluate (scope));
}

TEST (ExpressionTests, ParseIdentifierWithNumbers)
{
    String error;
    Expression e ("var123 + 10", error);
    TestScope scope;
    scope.symbols.set ("var123", 50.0);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (60.0, e.evaluate (scope));
}

TEST (ExpressionTests, ParseIdentifierWithUnderscores)
{
    String error;
    Expression e ("my_var_name + 10", error);
    TestScope scope;
    scope.symbols.set ("my_var_name", 25.0);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (35.0, e.evaluate (scope));
}

// ==============================================================================
// Comprehensive Integration Tests
// ==============================================================================

TEST (ExpressionTests, ComplexMathematicalExpression)
{
    String error;
    Expression e ("(sin(0) + cos(0)) * (abs(-10) + min(5, 3, 7))", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_NEAR (13.0, e.evaluate(), 0.0001); // (0 + 1) * (10 + 3) = 13
}

TEST (ExpressionTests, ExpressionWithSymbolsAndFunctions)
{
    String error;
    Expression e ("square(x) + square(y)", error);
    TestScope scope;

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (125.0, e.evaluate (scope)); // 25 + 100
}

TEST (ExpressionTests, DeeplyNestedExpression)
{
    String error;
    Expression e ("((((10 + 5) * 2) - 3) / 3)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (9.0, e.evaluate());
}

TEST (ExpressionTests, MixedOperatorsAndFunctions)
{
    String error;
    Expression e ("max(10, 20) + min(5, 3) * abs(-2)", error);

    EXPECT_TRUE (error.isEmpty());
    EXPECT_EQ (26.0, e.evaluate()); // 20 + (3 * 2) = 26
}
