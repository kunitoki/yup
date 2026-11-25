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
class CompiledExpressionTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        x = context.addInput<float> ("x");
        n = context.addInput<int> ("n");
        values = context.addArray<float> ("values");
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

    var evaluate (StringRef text, float xValue = 0.0f, int nValue = 0)
    {
        ExpressionState state (compile (text));
        state.set (x, xValue);
        state.set (n, nValue);
        state.evaluate();
        return state.getVar (0);
    }

    const ExpressionType* resultType (StringRef text)
    {
        return compile (text).getOutputType (0);
    }

    ExpressionContext context;
    ExpressionInput<float> x;
    ExpressionInput<int> n;
    ExpressionArray<float> values;
};

//==============================================================================
TEST_F (CompiledExpressionTests, DefaultProgramIsEmpty)
{
    CompiledExpression program;

    EXPECT_EQ (0, program.getNumOutputs());
    EXPECT_EQ (0, program.getNumInputs());
    EXPECT_EQ (0, program.getNumInstructions());
    EXPECT_EQ (nullptr, program.getOutputType (0));
    EXPECT_TRUE (program.getOutput<float> ("result").failed());

    ExpressionState state (program);
    state.evaluate();
    state.evaluateRows (100);
    EXPECT_TRUE (state.getVar (0).isVoid());
}

TEST_F (CompiledExpressionTests, IntegerArithmetic)
{
    EXPECT_EQ (3, (int) evaluate ("7 / 2"));
    EXPECT_EQ (1, (int) evaluate ("7 % 3"));
    EXPECT_EQ (-1, (int) evaluate ("-7 % 3"));
    EXPECT_EQ (0, (int) evaluate ("7 / n"));
    EXPECT_EQ (0, (int) evaluate ("7 % n"));
    EXPECT_EQ (1024, (int) evaluate ("2 ^ 10"));
    EXPECT_EQ (-4, (int) evaluate ("-2 ^ 2"));
    EXPECT_EQ (0, (int) evaluate ("2 ^ -1"));
    EXPECT_EQ (std::numeric_limits<int>::min(), (int) evaluate ("2147483647 + n", 0.0f, 1));
    EXPECT_EQ (std::numeric_limits<int>::min(), (int) evaluate ("n / -1", 0.0f, std::numeric_limits<int>::min()));
    EXPECT_EQ (5, (int) evaluate ("abs (n)", 0.0f, -5));
    EXPECT_EQ (&ExpressionType::intType(), resultType ("n * 2 + 1"));
}

TEST_F (CompiledExpressionTests, CastsSaturate)
{
    EXPECT_EQ (std::numeric_limits<int>::max(), (int) evaluate ("int (1e20)"));
    EXPECT_EQ (std::numeric_limits<int>::min(), (int) evaluate ("int (-1e20)"));
    EXPECT_EQ (0, (int) evaluate ("int (sqrt (-1.0))"));
    EXPECT_EQ (-2, (int) evaluate ("int (x)", -2.75f));
    EXPECT_EQ (42, (int) evaluate ("int (\"42\")"));
    EXPECT_TRUE ((bool) evaluate ("bool (x)", 0.5f));
    EXPECT_DOUBLE_EQ (1.5, (double) evaluate ("double (\"1.5\")"));
}

TEST_F (CompiledExpressionTests, LiteralsAdoptTheFloatingTypeOfOtherOperands)
{
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("x * 0.5"));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("x + 1"));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("sin (x) * pi"));
    EXPECT_EQ (&ExpressionType::doubleType(), resultType ("0.5 * 2"));
    EXPECT_EQ (&ExpressionType::doubleType(), resultType ("n * 0.5"));
    EXPECT_EQ (&ExpressionType::doubleType(), resultType ("double (x) * 0.5"));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("1.5f"));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("n > 0 ? x : 0.5"));
    EXPECT_EQ (&ExpressionType::doubleType(), resultType ("n > 0 ? 1.5 : 0.5"));

    EXPECT_DOUBLE_EQ (1.75, (double) evaluate ("x * 0.5 + 0.25", 3.0f));
    EXPECT_DOUBLE_EQ (3.5, (double) evaluate ("7.0 / 2"));
}

TEST_F (CompiledExpressionTests, ComparisonsAndLogic)
{
    EXPECT_TRUE ((bool) evaluate ("x > 1 && n < 3", 2.0f, 2));
    EXPECT_FALSE ((bool) evaluate ("x > 1 && n < 3", 2.0f, 5));
    EXPECT_TRUE ((bool) evaluate ("!(x > 1) || n == 5", 2.0f, 5));
    EXPECT_TRUE ((bool) evaluate ("n && x", 1.0f, 1));
    EXPECT_FALSE ((bool) evaluate ("!n", 0.0f, 3));
    EXPECT_TRUE ((bool) evaluate ("\"abc\" < \"abd\""));
    EXPECT_TRUE ((bool) evaluate ("true != false"));
    EXPECT_TRUE ((bool) evaluate ("n == 2.0", 0.0f, 2));
}

TEST_F (CompiledExpressionTests, Ternary)
{
    EXPECT_EQ (10, (int) evaluate ("n > 0 ? 10 : 20", 0.0f, 1));
    EXPECT_EQ (20, (int) evaluate ("n > 0 ? 10 : 20", 0.0f, 0));
    EXPECT_EQ (3, (int) evaluate ("n == 0 ? 1 : n == 1 ? 2 : 3", 0.0f, 7));
    EXPECT_DOUBLE_EQ (2.5, (double) evaluate ("n ? x : 1", 2.5f, 1));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("n ? x : 1"));
    EXPECT_EQ (String ("yes"), evaluate ("x > 0 ? \"yes\" : \"no\"", 1.0f).toString());

    const auto program = compile ("true ? x : 2");
    EXPECT_EQ (0, program.getNumInstructions());
    EXPECT_FALSE (program.usesInput (n.index));

    EXPECT_TRUE (compileError ("n ? 1 : \"no\"").contains ("The branches of '?:' have incompatible types 'int' and 'string'"));
    EXPECT_TRUE (compileError ("\"s\" ? 1 : 2").contains ("The type 'string' can't be used as a condition"));
}

TEST_F (CompiledExpressionTests, LocalsAndOutputs)
{
    const auto program = compile ("let w = x * 2; a = w + 1; let h = a * 2; b = h + n; c = a");

    ASSERT_EQ (3, program.getNumOutputs());
    EXPECT_EQ (String ("a"), program.getOutputName (0));
    EXPECT_EQ (String ("b"), program.getOutputName (1));
    EXPECT_EQ (String ("c"), program.getOutputName (2));
    EXPECT_EQ (2, program.findOutput ("c"));
    EXPECT_EQ (-1, program.findOutput ("w"));

    ExpressionState state (program);
    state.set (x, 1.0f);
    state.set (n, 4);
    state.evaluate();

    EXPECT_FLOAT_EQ (3.0f, state.get (program.getOutput<float> ("a").getValue()));
    EXPECT_FLOAT_EQ (10.0f, state.get (program.getOutput<float> ("b").getValue()));
    EXPECT_FLOAT_EQ (3.0f, state.get (program.getOutput<float> ("c").getValue()));
}

TEST_F (CompiledExpressionTests, TypedOutputHandles)
{
    const auto program = compile ("a = 1; b = x");

    EXPECT_TRUE (program.getOutput<int> ("a").wasOk());
    EXPECT_EQ (String ("Output 'a' has type 'int'"), program.getOutput<float> ("a").getErrorMessage());
    EXPECT_EQ (String ("Unknown output 'missing'"), program.getOutput<float> ("missing").getErrorMessage());
}

TEST_F (CompiledExpressionTests, ReportsErrorsWithPositions)
{
    EXPECT_EQ (String ("1:8: The output 'a' is already assigned"), compileError ("a = 1; a = 2"));
    EXPECT_EQ (String ("1:5: Unknown symbol 'foo'"), compileError ("1 + foo"));
    EXPECT_EQ (String ("1:1: Cannot assign to the input 'x'"), compileError ("x = 1"));
    EXPECT_EQ (String ("1:12: 'a' is already defined"), compileError ("let a = 1; let a = 2; a"));
    EXPECT_EQ (String ("1:1: The expression has no outputs"), compileError ("let a = 1"));
    EXPECT_EQ (String ("1:1: Unknown function 'bar'"), compileError ("bar (1)"));
    EXPECT_EQ (String ("1:6: No matching overload for '+' with 'bool' and 'string'"), compileError ("true + \"a\""));
    EXPECT_TRUE (compileError ("1 +").startsWith ("1:4:"));

    const auto overloads = compileError ("clamp (x)");
    EXPECT_TRUE (overloads.startsWith ("1:1: No matching overload for 'clamp (float)', candidates are: "));
    EXPECT_TRUE (overloads.contains ("clamp (float, float, float) -> float"));
}

TEST_F (CompiledExpressionTests, FoldsConstants)
{
    const auto program = compile ("a = sin (pi / 2) * 4 + max (1, 2, 3); b = string (2) + \"x\"");

    EXPECT_TRUE (program.isConstant());
    EXPECT_EQ (0, program.getNumInstructions());
    EXPECT_FALSE (program.isAllocationFree());

    ExpressionState state (program);
    state.evaluate();
    EXPECT_DOUBLE_EQ (7.0, (double) state.getVar (0));
    EXPECT_EQ (String ("2x"), state.getVar (1).toString());
}

TEST_F (CompiledExpressionTests, EliminatesCommonSubexpressions)
{
    EXPECT_EQ (2, compile ("x * x + x * x").getNumInstructions());
    EXPECT_EQ (3, compile ("sin (x) + sin (x) * cos (x)").getNumInstructions());
}

TEST_F (CompiledExpressionTests, FusesMultiplyAdd)
{
    EXPECT_EQ (1, compile ("x * 2 + 1").getNumInstructions());
    EXPECT_EQ (1, compile ("1 + x * 2").getNumInstructions());
    EXPECT_EQ (1, compile ("x * 2 - 1").getNumInstructions());
    EXPECT_EQ (1, compile ("1 - x * 2").getNumInstructions());
    EXPECT_EQ (2, compile ("double (x) * 2.0 + 1.0").getNumInstructions());

    EXPECT_FLOAT_EQ (7.0f, (float) evaluate ("x * 2 + 1", 3.0f));
    EXPECT_FLOAT_EQ (7.0f, (float) evaluate ("1 + x * 2", 3.0f));
    EXPECT_FLOAT_EQ (5.0f, (float) evaluate ("x * 2 - 1", 3.0f));
    EXPECT_FLOAT_EQ (-5.0f, (float) evaluate ("1 - x * 2", 3.0f));
    EXPECT_DOUBLE_EQ (7.0, (double) evaluate ("double (x) * 2.0 + 1.0", 3.0f));
}

TEST_F (CompiledExpressionTests, DoesNotFuseSharedOrIntegerProducts)
{
    EXPECT_EQ (3, compile ("let p = x * 2; a = p + 1; b = p + 2").getNumInstructions());
    EXPECT_EQ (2, compile ("p = x * 2; q = p + 1").getNumInstructions());
    EXPECT_EQ (2, compile ("n * 2 + 1").getNumInstructions());

    ExpressionState state (compile ("p = x * 2; q = p + 1"));
    state.set (x, 3.0f);
    state.evaluate();
    EXPECT_FLOAT_EQ (6.0f, (float) state.getVar (0));
    EXPECT_FLOAT_EQ (7.0f, (float) state.getVar (1));
}

TEST_F (CompiledExpressionTests, FusesCompareAndSelect)
{
    EXPECT_EQ (1, compile ("x > 1 ? n : 2").getNumInstructions());
    EXPECT_EQ (1, compile ("n <= 3 ? x : 2.0f").getNumInstructions());
    EXPECT_EQ (1, compile ("x != 0 ? 1.0 : 2.0").getNumInstructions());
    EXPECT_EQ (1, compile ("n == 3 ? true : false").getNumInstructions());
    EXPECT_EQ (3, compile ("let c = x > 1; a = c ? 1 : 2; b = c ? 3 : 4").getNumInstructions());
    EXPECT_EQ (2, compile ("c = x > 1; a = c ? 1 : 2").getNumInstructions());

    EXPECT_EQ (5, (int) evaluate ("x > 1 ? n : 2", 3.0f, 5));
    EXPECT_EQ (2, (int) evaluate ("x > 1 ? n : 2", 1.0f, 5));
    EXPECT_FLOAT_EQ (0.5f, (float) evaluate ("n <= 3 ? x : 2.0f", 0.5f, 3));
    EXPECT_FLOAT_EQ (2.0f, (float) evaluate ("n <= 3 ? x : 2.0f", 0.5f, 4));
    EXPECT_DOUBLE_EQ (2.0, (double) evaluate ("x != 0 ? 1.0 : 2.0", 0.0f));
    EXPECT_TRUE ((bool) evaluate ("n == 3 ? true : false", 0.0f, 3));
}

TEST_F (CompiledExpressionTests, FusedInstructionsMatchInBatches)
{
    const auto program = compile ("y = x * 3 - 1; k = x < 2 ? n : -n");
    ASSERT_EQ (3, program.getNumInstructions());

    constexpr int numRows = 100;
    std::vector<float> xs (numRows), ys (numRows);
    std::vector<int> ks (numRows);

    for (int i = 0; i < numRows; ++i)
        xs[(size_t) i] = (float) i * 0.05f;

    ExpressionState state (program);
    state.set (n, 7);
    state.bindColumn (x, xs.data());
    state.bindOutputColumn (program.getOutput<float> ("y").getValue(), ys.data());
    state.bindOutputColumn (program.getOutput<int> ("k").getValue(), ks.data());
    state.evaluateRows (numRows);

    for (int i = 0; i < numRows; ++i)
    {
        // Absolute tolerance, as the compiler may contract the reference into a fused multiply-add
        EXPECT_NEAR (xs[(size_t) i] * 3.0f - 1.0f, ys[(size_t) i], 1.0e-6f);
        EXPECT_EQ (xs[(size_t) i] < 2.0f ? 7 : -7, ks[(size_t) i]);
    }
}

TEST_F (CompiledExpressionTests, RemovesUnusedCode)
{
    const auto program = compile ("let unused = sin (x) * n; y = x * 2");

    EXPECT_EQ (1, program.getNumInstructions());
    EXPECT_TRUE (program.usesInput (x.index));
    EXPECT_FALSE (program.usesInput (n.index));
    EXPECT_FALSE (program.usesInput (values.index));
    EXPECT_FALSE (program.isConstant());
    EXPECT_TRUE (program.isAllocationFree());
}

TEST_F (CompiledExpressionTests, MathBuiltins)
{
    EXPECT_NEAR (1.0, (double) evaluate ("sin (rad (90))"), 1.0e-6);
    EXPECT_NEAR (180.0, (double) evaluate ("deg (pi)"), 1.0e-12);
    EXPECT_DOUBLE_EQ (5.0, (double) evaluate ("hypot (3, 4)"));
    EXPECT_DOUBLE_EQ (0.5, (double) evaluate ("lerp (0, 1, 0.5)"));
    EXPECT_DOUBLE_EQ (2.5, (double) evaluate ("mix (2, 4, 0.25)"));
    EXPECT_DOUBLE_EQ (50.0, (double) evaluate ("map (5, 0, 10, 0, 100)"));
    EXPECT_DOUBLE_EQ (0.5, (double) evaluate ("smoothstep (0, 1, 0.5)"));
    EXPECT_EQ (3, (int) evaluate ("clamp (7, 0, 3)"));
    EXPECT_EQ (-1, (int) evaluate ("min (3, -1, 2)"));
    EXPECT_DOUBLE_EQ (-1.0, (double) evaluate ("sign (x)", -3.0f));
    EXPECT_DOUBLE_EQ (2.0, (double) evaluate ("floor (2.7)"));
    EXPECT_NEAR (MathConstants<double>::twoPi, (double) evaluate ("tau"), 1.0e-12);
}

TEST_F (CompiledExpressionTests, Strings)
{
    EXPECT_EQ (String ("3.14"), evaluate ("string (3.14159, 2)").toString());
    EXPECT_EQ (String ("n=5"), evaluate ("\"n=\" + string (n)", 0.0f, 5).toString());
    EXPECT_EQ (String ("true"), evaluate ("string (n > 1)", 0.0f, 5).toString());
    EXPECT_EQ (5, (int) evaluate ("len (\"hello\")"));
    EXPECT_EQ (String ("a\"b\n"), evaluate ("\"a\\\"b\\n\"").toString());
}

TEST_F (CompiledExpressionTests, FormatsNumbers)
{
    EXPECT_EQ (String ("3.14"), evaluate ("format (\"%.2f\", 3.14159)").toString());
    EXPECT_EQ (String ("5 items"), evaluate ("format (\"%d items\", n)", 0.0f, 5).toString());
    EXPECT_EQ (String ("  2.5|7   |%"), evaluate ("format (\"%5.1f|%-4d|%%\", x, n)", 2.5f, 7).toString());
    EXPECT_EQ (String ("1 0"), evaluate ("format (\"%d %d\", 1)").toString());
    EXPECT_EQ (String ("0"), evaluate ("format (\"%d\", -0.4)").toString());
    EXPECT_EQ (String ("%s 1"), evaluate ("format (\"%s %d\", 1)").toString());
    EXPECT_EQ (String ("1, 2, 3"), evaluate ("format (\"%g, %g, %g\", 1, 2, 3)").toString());
    EXPECT_TRUE (compile ("format (\"%.1f\", 2)").isConstant());
}

TEST_F (CompiledExpressionTests, UnitsAreFloatingLiterals)
{
    EXPECT_NEAR (MathConstants<double>::halfPi, (double) evaluate ("90deg"), 1.0e-12);
    EXPECT_FLOAT_EQ (2.0f, (float) (double) evaluate ("x * 50%", 4.0f));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("x * 50%"));
    EXPECT_EQ (&ExpressionType::intType(), resultType ("4px"));
}

TEST_F (CompiledExpressionTests, ColorLiteralsNeedAColorType)
{
    EXPECT_TRUE (compileError ("#ff8800").contains ("color literals need a 'color' type"));
}

TEST_F (CompiledExpressionTests, ArraysWrittenInPlaceAreArrays)
{
    EXPECT_EQ (context.getType<ExpressionArrayView<float>>(), resultType ("[1, 2.5, n]"));
    EXPECT_EQ (3, (int) evaluate ("len ([1, 2, 3])"));
    EXPECT_DOUBLE_EQ (2.0, (double) evaluate ("[1, 2, 3][1]"));
    EXPECT_DOUBLE_EQ (3.0, (double) evaluate ("[1, 2, 3][7]"));

    // One stands for an array given as an input
    const float data[] = { 1.0f, 2.0f, 4.0f };
    ExpressionState either (compile ("(n > 0 ? [5, 6] : values)[1]"));
    either.bindArray (values, data, 3);
    either.set (n, 1);
    either.evaluate();
    EXPECT_DOUBLE_EQ (6.0, (double) either.getVar (0));

    either.set (n, 0);
    either.evaluate();
    EXPECT_DOUBLE_EQ (2.0, (double) either.getVar (0));

    // What a function makes of a constant one when compiling still views its values when evaluating
    context.addFunction ("same", [] (ExpressionArrayView<float> array) { return array; });
    EXPECT_DOUBLE_EQ (3.0, (double) evaluate ("same ([1, 2, 3])[n]", 0.0f, 2));

    // A context with no array of numbers has the ones written in place
    ExpressionContext bare;
    EXPECT_TRUE (CompiledExpression::compile ("[1, 2, 3][1] + len ([4, 5])", bare).wasOk());

    EXPECT_TRUE (compileError ("[1, \"a\"]").contains ("must be of one type"));
    EXPECT_TRUE (compileError ("[\"a\", \"b\"]").contains ("no array type for 'string'"));
}

TEST_F (CompiledExpressionTests, ArraysWrittenInPlaceCostWhatIsReadOfThem)
{
    // Its length and the values at known indices are the ones it was written with: no array is made
    const auto known = compile ("let a = [x, x * 2, 3]; first = a[0]; count = len (a)");
    EXPECT_EQ (0, known.getNumInstructions());

    ExpressionState knownState (known);
    knownState.set (x, 7.0f);
    knownState.evaluate();
    EXPECT_DOUBLE_EQ (7.0, (double) knownState.getVar (0));
    EXPECT_EQ (3, (int) knownState.getVar (1));

    // Read at an index only known when evaluating, it holds its values
    const auto program = compile ("y = [x, x * 2, 10][n]");
    ExpressionState state (program);

    state.set (x, 3.0f);
    state.set (n, 1);
    state.evaluate();
    EXPECT_DOUBLE_EQ (6.0, (double) state.getVar (0));

    // The values change behind an array that stays the same
    state.set (x, 4.0f);
    state.evaluate();
    EXPECT_DOUBLE_EQ (8.0, (double) state.getVar (0));
    EXPECT_TRUE (state.hasOutputChanged (0));

    state.set (n, 2);
    state.evaluate();
    EXPECT_DOUBLE_EQ (10.0, (double) state.getVar (0));

    state.evaluate();
    EXPECT_FALSE (state.hasOutputChanged (0));

    // Every row has the array of its own values
    const float xs[] = { 1.0f, 2.0f, 3.0f };
    float ys[3] = {};

    state.bindColumn (x, xs);
    state.bindOutputColumn (program.getOutput<float> ("y").getValue(), ys);
    state.set (n, 1);
    state.evaluateRows (3);

    EXPECT_FLOAT_EQ (2.0f, ys[0]);
    EXPECT_FLOAT_EQ (4.0f, ys[1]);
    EXPECT_FLOAT_EQ (6.0f, ys[2]);
}

TEST_F (CompiledExpressionTests, FusesInPlaceMethodChainsWithoutIntermediateClones)
{
    struct BufferHandle
    {
        std::shared_ptr<std::vector<float>> items = std::make_shared<std::vector<float>>();

        BufferHandle& push (float value)
        {
            items->push_back (value);
            return *this;
        }

        bool operator== (const BufferHandle& other) const noexcept { return items == other.items; }
        bool operator!= (const BufferHandle& other) const noexcept { return items != other.items; }
    };

    int cloneCount = 0;

    context.addType<BufferHandle> ("buf")
        .clone ([&cloneCount] (const BufferHandle& source)
        {
            ++cloneCount;
            BufferHandle copy;
            *copy.items = *source.items;
            return copy;
        })
        .constructor ([] { return BufferHandle(); })
        .member ("size", [] (const BufferHandle& b) { return (int) b.items->size(); })
        .method ("push", [] (BufferHandle& b, float value) -> BufferHandle& { return b.push (value); });

    const auto y = context.addInput<float> ("y");

    // 4-step chain from empty buf() fuses into 1 instruction with 1 clone from the empty constant seed
    const auto chained = compile ("buf().push (x).push (1.0f).push (y).push (3.0f)");
    EXPECT_EQ (1, chained.getNumInstructions());

    cloneCount = 0;
    ExpressionState chainedState (chained);
    const auto chainedOut = chained.getOutput<BufferHandle> ("result").getValue();

    chainedState.set (x, 10.0f);
    chainedState.set (y, 20.0f);
    chainedState.evaluate();
    EXPECT_EQ (1, cloneCount);
    EXPECT_EQ ((std::vector<float> { 10.0f, 1.0f, 20.0f, 3.0f }), *chainedState.get (chainedOut).items);

    // Updating only a later step's input re-runs the fused chain cleanly from the seed without accumulating
    chainedState.set (y, 99.0f);
    chainedState.evaluate();
    EXPECT_EQ (2, cloneCount);
    EXPECT_EQ ((std::vector<float> { 10.0f, 1.0f, 99.0f, 3.0f }), *chainedState.get (chainedOut).items);

    // Branching off a shared local leaves the shared local untouched and clones once per branch
    const auto branched = compile ("let base = buf().push (x).push (2.0f); a = base; b = base.push (y).push (4.0f)");
    EXPECT_EQ (2, branched.getNumInstructions());

    cloneCount = 0;
    ExpressionState branchedState (branched);
    branchedState.set (x, 5.0f);
    branchedState.set (y, 6.0f);
    branchedState.evaluate();

    EXPECT_EQ (2, cloneCount);
    EXPECT_EQ ((std::vector<float> { 5.0f, 2.0f }), *branchedState.get (branched.getOutput<BufferHandle> ("a").getValue()).items);
    EXPECT_EQ ((std::vector<float> { 5.0f, 2.0f, 6.0f, 4.0f }), *branchedState.get (branched.getOutput<BufferHandle> ("b").getValue()).items);
}

TEST_F (CompiledExpressionTests, ResolvesUnknownSymbolsLazily)
{
    StringArray asked;

    context.setSymbolResolver ([&] (const String& name) -> const ExpressionType*
    {
        asked.add (name);
        return name.startsWith ("model.") ? context.getType<float>() : nullptr;
    });

    const auto program = compile ("model.gain * 2 + model.gain + x");
    EXPECT_EQ (context.getNumInputs() + 1, program.getNumInputs());
    ASSERT_EQ (1, asked.size());
    EXPECT_EQ (String ("model.gain"), asked[0]);
    EXPECT_TRUE (program.getInput<int> ("model.gain").failed());
    EXPECT_TRUE (program.getInput<float> ("missing").failed());

    const auto gain = program.getInput<float> ("model.gain");
    ASSERT_TRUE (gain.wasOk());
    EXPECT_TRUE (program.usesInput (gain.getValue().index));

    ExpressionState state (program);
    state.set (gain.getValue(), 3.0f);
    state.set (x, 1.0f);
    state.evaluate();
    EXPECT_DOUBLE_EQ (10.0, (double) state.getVar (0));

    asked.clear();
    EXPECT_TRUE (compileError ("other + 1").contains ("Unknown symbol 'other'"));
    ASSERT_EQ (1, asked.size());
    EXPECT_EQ (String ("other"), asked[0]);
}

TEST_F (CompiledExpressionTests, EnumValues)
{
    enum class Cap { butt, round, square };
    enum class Align { left, center };
    enum class Justify { center, fill };

    context.addEnum<Cap> ("cap", { { "butt", Cap::butt }, { "round", Cap::round }, { "square", Cap::square } });
    context.addEnum<Align> ("align", { { "left", Align::left }, { "center", Align::center } });
    context.addEnum<Justify> ("justify", { { "center", Justify::center }, { "fill", Justify::fill } });
    context.addInput<float> ("square");
    context.declareOutput<Cap> ("strokeCap");
    context.declareOutput<Justify> ("justification");

    EXPECT_TRUE (compile ("justification = n > 0 ? center : fill").getOutput<Justify> ("justification").wasOk());

    const auto program = compile ("strokeCap = n > 0 ? round : cap.butt");
    const auto strokeCap = program.getOutput<Cap> ("strokeCap");
    ASSERT_TRUE (strokeCap.wasOk());

    ExpressionState state (program);
    state.set (n, 1);
    state.evaluate();
    EXPECT_EQ (Cap::round, state.get (strokeCap.getValue()));

    state.set (n, 0);
    state.evaluate();
    EXPECT_EQ (Cap::butt, state.get (strokeCap.getValue()));

    EXPECT_TRUE ((bool) evaluate ("cap.square == cap.square && left != align.center"));
    EXPECT_TRUE (compile ("round == cap.round").isConstant());
    EXPECT_DOUBLE_EQ (2.0, (double) evaluate ("round (1.6)"));
    EXPECT_EQ (&ExpressionType::floatType(), resultType ("square * 2"));
    EXPECT_TRUE (compileError ("center").contains ("'center' is ambiguous, use one of align.center, justify.center"));
    EXPECT_TRUE (compileError ("strokeCap = center").contains ("ambiguous"));
    EXPECT_TRUE (compileError ("round == 1").isNotEmpty());

    context.setSymbolResolver ([this] (const String& name) -> const ExpressionType*
    {
        return name == "fill" ? context.getType<float>() : nullptr;
    });

    EXPECT_EQ (&ExpressionType::floatType(), resultType ("fill * 2"));
}

TEST_F (CompiledExpressionTests, Arrays)
{
    const float data[] = { 1.0f, 2.0f, 4.0f };

    const auto program = compile ("first = values[0]; last = values[n]; count = len (values); sum = values[0] + values[1] + values[2]");
    ExpressionState state (program);
    state.bindArray (values, data, 3);

    state.set (n, 2);
    state.evaluate();
    EXPECT_DOUBLE_EQ (1.0, (double) state.getVar (0));
    EXPECT_DOUBLE_EQ (4.0, (double) state.getVar (1));
    EXPECT_EQ (3, (int) state.getVar (2));
    EXPECT_DOUBLE_EQ (7.0, (double) state.getVar (3));

    state.set (n, 100);
    state.evaluate();
    EXPECT_DOUBLE_EQ (4.0, (double) state.getVar (1));

    state.set (n, -5);
    state.evaluate();
    EXPECT_DOUBLE_EQ (1.0, (double) state.getVar (1));

    state.bindArray (values, data, 0);
    state.evaluate();
    EXPECT_DOUBLE_EQ (0.0, (double) state.getVar (1));
    EXPECT_EQ (0, (int) state.getVar (2));
}

TEST_F (CompiledExpressionTests, ArrayErrors)
{
    EXPECT_EQ (String ("1:8: An array index must be an 'int', not 'float'"), compileError ("values[x]"));
    EXPECT_TRUE (compileError ("x[0]").contains ("The type 'float' can't be indexed"));
}

TEST_F (CompiledExpressionTests, CompilesExpressionObjects)
{
    const auto expression = Expression::symbol ("x") * Expression (2.0f) + Expression (1);
    const auto program = CompiledExpression::compile (expression, context);

    ASSERT_TRUE (program.wasOk());
    ExpressionState state (program.getValue());
    state.set (x, 3.0f);
    state.evaluate();
    EXPECT_DOUBLE_EQ (7.0, (double) state.getVar (0));
}

TEST_F (CompiledExpressionTests, CompilesBuiltProgramsLikeTheParsedOnes)
{
    const auto built = Expression::block ({
        Expression::let ("w", Expression::symbol ("x") * Expression (2)),
        Expression::output ("a", Expression::symbol ("w") + Expression (1)),
        Expression::output ("b", Expression::ternary (Expression::symbol ("n"), Expression::symbol ("a"), Expression (0.0f)))
    });

    const auto fromTree = CompiledExpression::compile (built, context);
    ASSERT_TRUE (fromTree.wasOk());

    const auto fromText = compile (built.toString());
    ASSERT_EQ (fromText.getNumOutputs(), fromTree.getValue().getNumOutputs());
    EXPECT_EQ (fromText.getNumInstructions(), fromTree.getValue().getNumInstructions());

    ExpressionState treeState (fromTree.getValue()), textState (fromText);

    for (auto* state : { &treeState, &textState })
    {
        state->set (x, 1.5f);
        state->set (n, 1);
        state->evaluate();
    }

    for (int i = 0; i < fromText.getNumOutputs(); ++i)
    {
        EXPECT_EQ (fromText.getOutputName (i), fromTree.getValue().getOutputName (i));
        EXPECT_EQ (fromText.getOutputType (i), fromTree.getValue().getOutputType (i));
        EXPECT_DOUBLE_EQ ((double) textState.getVar (i), (double) treeState.getVar (i));
    }

    EXPECT_DOUBLE_EQ (4.0, (double) treeState.getVar (0));
}

TEST_F (CompiledExpressionTests, ReportsTheStatementThatFailed)
{
    const auto failedStatement = [this] (const Expression& program, String* message = nullptr)
    {
        int index = 100;
        const auto result = CompiledExpression::compile (program, context, index);
        EXPECT_TRUE (result.failed());

        if (message != nullptr && result.failed())
            *message = result.getErrorMessage();

        return index;
    };

    const auto missing = Expression::symbol ("missing");

    // A let, an output and the last plain expression
    EXPECT_EQ (1, failedStatement (Expression::block ({ Expression::let ("a", Expression (1)), Expression::let ("b", missing), Expression::output ("c", Expression (2)) })));
    EXPECT_EQ (2, failedStatement (Expression::block ({ Expression::let ("a", Expression (1)), Expression::output ("b", Expression (2)), Expression::output ("c", missing) })));
    EXPECT_EQ (1, failedStatement (Expression::block ({ Expression::output ("a", Expression (1)), missing })));
    EXPECT_EQ (0, failedStatement (missing));

    // Built nodes have no position, so the message has none either
    String message;
    EXPECT_EQ (1, failedStatement (Expression::block ({ Expression::output ("a", Expression (1)), Expression::output ("a", Expression (2)) }), &message));
    EXPECT_EQ (String ("The output 'a' is already assigned"), message);

    // Parsed programs report the statement as well as the position
    EXPECT_EQ (2, failedStatement (Expression::parse ("let a = 1;\nb = a;\nc = a + missing").getValue(), &message));
    EXPECT_EQ (String ("3:9: Unknown symbol 'missing'"), message);

    // A failure of the program as a whole is not the one of a statement
    EXPECT_EQ (-1, failedStatement (Expression::block ({ Expression::let ("a", Expression (1)) }), &message));
    EXPECT_EQ (String ("The expression has no outputs"), message);

    int index = 100;
    EXPECT_TRUE (CompiledExpression::compile (Expression::parse ("a = x + 1").getValue(), context, index).wasOk());
    EXPECT_EQ (-1, index);
}

TEST_F (CompiledExpressionTests, FindsTypedOutputsWithoutFailing)
{
    const auto program = compile ("a = 1; b = x");

    EXPECT_EQ (0, program.findTypedOutput<int> ("a").index);
    EXPECT_EQ (1, program.findTypedOutput<float> ("b").index);
    EXPECT_FALSE (program.findTypedOutput<float> ("a").isValid());
    EXPECT_FALSE (program.findTypedOutput<float> ("missing").isValid());
}

TEST_F (CompiledExpressionTests, DeepNestingFailsGracefully)
{
    const auto text = String::repeatedString ("(", 100) + "x" + String::repeatedString (" + 1)", 100);
    EXPECT_TRUE (CompiledExpression::compile (text, context).wasOk());

    const auto tooDeep = String::repeatedString ("-", 600) + "x";
    EXPECT_TRUE (CompiledExpression::compile (tooDeep, context).failed());
}
