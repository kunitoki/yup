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

#include <chrono>
#include <iostream>

using namespace yup;

//==============================================================================
/** Compares the expression engine with native C++ and the JavascriptEngine.

    Disabled by default, run it in Release with:
    GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "*ExpressionBenchmark*" yup_expression
*/
class ExpressionBenchmarkTests : public ::testing::Test
{
protected:
    static constexpr int numRows = 20000;
    static constexpr int numRuns = 7;

    void SetUp() override
    {
        for (int i = 0; i < numRows; ++i)
            data.push_back ((float) (0.5 + 0.5 * std::sin (i * 0.013)));
    }

    template <class F>
    static double measureSeconds (F&& run)
    {
        auto best = std::numeric_limits<double>::max();

        for (int i = 0; i < numRuns; ++i)
        {
            const auto start = std::chrono::steady_clock::now();
            run();
            best = jmin (best, std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count());
        }

        return best;
    }

    static void report (const String& name, double seconds, double reference, const String& unit, double divisor)
    {
        std::cout << "  " << name.paddedRight (' ', 24)
                  << String (seconds * 1.0e9 / divisor, 2).paddedLeft (' ', 12) << " " << unit
                  << String (reference / seconds, 1).paddedLeft (' ', 10) << "x vs javascript\n";
    }

    void expectResultsMatch (const std::vector<float>& expected, const std::vector<float>& actual, const char* name) const
    {
        for (size_t i = 0; i < expected.size(); i += 97)
            EXPECT_NEAR (expected[i], actual[i], 1.0e-4f) << name << " row " << i;
    }

    std::vector<float> data;
};

//==============================================================================
TEST_F (ExpressionBenchmarkTests, DISABLED_AgainstJavascriptAndNative)
{
    constexpr float scale = 3.0f, offset = 10.0f;

    const String expressionCode = "let t = value * scale; y = sin (t) * 0.5 + offset + (value > 0.5 ? value * 2 : value / 2)";
    const String javascriptCode = "function f (value, scale, offset) { var t = value * scale; return Math.sin (t) * 0.5 + offset + (value > 0.5 ? value * 2 : value / 2); }";

    std::vector<float> nativeResults (numRows), scalarResults (numRows), batchResults (numRows), javascriptResults (numRows);

    // Compilation
    ExpressionContext context;
    const auto valueInput = context.addInput<float> ("value");
    const auto scaleInput = context.addInput<float> ("scale");
    const auto offsetInput = context.addInput<float> ("offset");

    CompiledExpression program;
    const auto expressionCompile = measureSeconds ([&]
    {
        program = CompiledExpression::compile (expressionCode, context).getValue();
    });

    JavascriptEngine engine;
    engine.maximumExecutionTime = RelativeTime::seconds (3600);

    const auto javascriptCompile = measureSeconds ([&]
    {
        EXPECT_TRUE (engine.execute (javascriptCode).wasOk());
    });

    const auto yOutput = program.getOutput<float> ("y").getValue();

    // Native C++
    const auto native = measureSeconds ([&]
    {
        for (int i = 0; i < numRows; ++i)
        {
            const auto value = data[(size_t) i];
            nativeResults[(size_t) i] = std::sin (value * scale) * 0.5f + offset + (value > 0.5f ? value * 2.0f : value / 2.0f);
        }
    });

    // JavascriptEngine, one call per row
    const Identifier functionName ("f");
    var arguments[] = { var(), var ((double) scale), var ((double) offset) };

    const auto javascript = measureSeconds ([&]
    {
        for (int i = 0; i < numRows; ++i)
        {
            arguments[0] = (double) data[(size_t) i];
            javascriptResults[(size_t) i] = (float) (double) engine.callFunction (functionName, var::NativeFunctionArgs (var(), arguments, 3));
        }
    });

    // Expression, one row at a time
    ExpressionState scalarState (program);
    scalarState.set (scaleInput, scale);
    scalarState.set (offsetInput, offset);

    const auto scalar = measureSeconds ([&]
    {
        for (int i = 0; i < numRows; ++i)
        {
            scalarState.set (valueInput, data[(size_t) i]);
            scalarState.evaluate();
            scalarResults[(size_t) i] = scalarState.get (yOutput);
        }
    });

    // Expression, batch over columns
    ExpressionState batchState (program);
    batchState.set (scaleInput, scale);
    batchState.set (offsetInput, offset);
    batchState.bindColumn (valueInput, data.data());
    batchState.bindOutputColumn (yOutput, batchResults.data());

    const auto batch = measureSeconds ([&]
    {
        batchState.evaluateRows (numRows);
    });

    expectResultsMatch (nativeResults, javascriptResults, "javascript");
    expectResultsMatch (nativeResults, scalarResults, "scalar");
    expectResultsMatch (nativeResults, batchResults, "batch");

    std::cout << "\n  |==== BENCH ==== (expression vs javascript, " << numRows << " rows, best of " << numRuns << ")\n";
    report ("javascript compile", javascriptCompile, javascriptCompile, "ns", 1.0);
    report ("expression compile", expressionCompile, javascriptCompile, "ns", 1.0);
    report ("javascript per row", javascript, javascript, "ns/row", numRows);
    report ("expression scalar", scalar, javascript, "ns/row", numRows);
    report ("expression batch", batch, javascript, "ns/row", numRows);
    report ("native c++", native, javascript, "ns/row", numRows);
}
