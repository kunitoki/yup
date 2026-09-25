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

#include <yup_dsp_jit/yup_dsp_jit.h>
#include <gtest/gtest.h>
#include <thread>

#include "yup_YdspTestPatches.h"
#include "yup_YdspAllocationCounter.h"

using namespace yup;

class YdspRuntimeOptimizationTests : public ::testing::Test
{
protected:
    YdspCompiler compiler;

    YdspCompileOptions strictOptions()
    {
        YdspCompileOptions options;
        options.optimizationTier = YdspOptimizationTier::baseline;
        options.targetPolicy = YdspTargetPolicy::baseline;
        options.fastMath = false;
        return options;
    }

    YdspAudioGraph chain (bool doublePrecision, bool readsOutput = false)
    {
        const String type = doublePrecision ? "float64" : "float";
        const auto expression = readsOutput ? "in + out" : "in + 1.0";

        // A second input prevents chain fusion from removing the scratch under test.
        const auto source = String ("processor Step { input stream ") + type
            + " in; input stream " + type + " bias; output stream " + type + " out; process { out = " + expression + "; } } "
            + "graph G { input stream " + type + " x; output stream " + type + " y; "
              "node a = Step; node b = Step; node c = Step; node d = Step; "
              "connection { x -> a.in; x -> a.bias; x -> b.bias; x -> c.bias; x -> d.bias; "
              "a.out -> b.in; b.out -> c.in; c.out -> d.in; d.out -> y; } }";

        return yup::test::patches::compilePatch (source, compiler, strictOptions());
    }
};

TEST_F (YdspRuntimeOptimizationTests, IntegerLiteralsRemainExactThroughDefaultsStateAndConstantSubstitution)
{
    const auto source = R"YDSP(
        let exact = 9007199254740993;
        processor P {
            input parameter int64 defaultValue = 9223372036854775807;
            input parameter int64 forwarded = 0;
            output stream out;
            output parameter int64 literal, constant, storedValue, minimum, defaultMeter, forwardedMeter;
            state int64 stored = 9007199254740993;
            process {
                int64 value = 9007199254740993;
                literal = value;
                constant = exact;
                storedValue = stored;
                minimum = -9223372036854775808;
                defaultMeter = defaultValue;
                forwardedMeter = forwarded;
                out = 0.0;
            }
        }
        graph G {
            input parameter int64 forwarded = 9007199254740993;
            output stream y;
            node p = P;
            connection { p.out -> y; forwarded -> p.forwarded; }
        }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
    {
        YdspCompileOptions options;
        options.optimizationTier = tier;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
        std::array<float, 1> output {};
        YdspOutputBuffer outputs[] { Span<float> (output) };
        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
        for (const auto* meter : { "p.literal", "p.constant", "p.storedValue", "p.forwardedMeter" })
            EXPECT_EQ (9007199254740993LL, graph.getIntOutputValue (meter)) << meter;
        EXPECT_EQ (std::numeric_limits<int64_t>::min(), graph.getIntOutputValue ("p.minimum"));
        EXPECT_EQ (std::numeric_limits<int64_t>::max(), graph.getIntOutputValue ("p.defaultMeter"));
    }
}

TEST_F (YdspRuntimeOptimizationTests, IntegerNegationAbsoluteValueAndDivisionSaturateAtBothWidths)
{
    for (const bool wide : { false, true })
    {
        const int64_t minimum = wide ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int32_t>::min();
        const int64_t maximum = wide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();
        const auto source = String (R"YDSP(
            processor P {
                input parameter int64 inputA = 0, inputB = 0;
                input parameter int64 negativeMinimum = -(-9223372036854775808);
                output parameter int64 defaultNeg;
                output parameter int64 negative, magnitude, quotient, remainder, aliased;
                output parameter int64 foldedNeg, foldedAbs, foldedDiv, foldedRem, foldedZero;
                output stream out;
                process {
                    defaultNeg = negativeMinimum;
                    TYPE a = TYPE (inputA);
                    TYPE b = TYPE (inputB);
                    negative = int64 (-a);
                    magnitude = int64 (abs (a));
                    quotient = int64 (a / b);
                    remainder = int64 (a % b);
                    TYPE copy = a;
                    copy = -copy;
                    aliased = int64 (copy);
                    TYPE low = MINIMUM;
                    TYPE minusOne = -1;
                    TYPE zero = 0;
                    foldedNeg = int64 (-low);
                    foldedAbs = int64 (abs (low));
                    foldedDiv = int64 (low / minusOne);
                    foldedRem = int64 (low % minusOne);
                    foldedZero = int64 (low / zero);
                    out = 0.0;
                }
            }
            graph G { output stream y; node p = P; connection { p.out -> y; } }
        )YDSP").replace ("TYPE", wide ? "int64" : "int32")
                   .replace ("MINIMUM", wide ? "-9223372036854775808" : "-2147483648");

        for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        {
            for (const bool fastMath : { false, true })
            {
                auto options = strictOptions();
                SCOPED_TRACE (::testing::Message() << "wide=" << wide << ", tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
                options.optimizationTier = tier;
                options.fastMath = fastMath;
                auto graph = yup::test::patches::compilePatch (source, compiler, options);
                ASSERT_TRUE (graph.isValid());
                ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
                float output = 0.0f;
                YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
                for (const int64_t a : { minimum, minimum + 1, int64_t { -7 }, int64_t { 0 }, int64_t { 7 }, maximum })
                {
                    for (const int64_t b : { int64_t { -1 }, int64_t { 0 }, int64_t { 1 }, int64_t { 3 } })
                    {
                        SCOPED_TRACE (::testing::Message() << "a=" << a << ", b=" << b);
                        graph.setIntParameter ("p.inputA", a);
                        graph.setIntParameter ("p.inputB", b);
                        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
                        EXPECT_EQ (std::numeric_limits<int64_t>::max(), graph.getIntOutputValue ("p.defaultNeg"));
                        const auto negated = a == minimum ? maximum : -a;
                        const auto quotient = b == 0 ? 0 : (a == minimum && b == -1 ? maximum : a / b);
                        const auto remainder = b == 0 || (a == minimum && b == -1) ? 0 : a % b;
                        EXPECT_EQ (negated, graph.getIntOutputValue ("p.negative"));
                        EXPECT_EQ (negated, graph.getIntOutputValue ("p.aliased"));
                        EXPECT_EQ (a < 0 ? negated : a, graph.getIntOutputValue ("p.magnitude"));
                        EXPECT_EQ (quotient, graph.getIntOutputValue ("p.quotient"));
                        EXPECT_EQ (remainder, graph.getIntOutputValue ("p.remainder"));
                        for (const auto* meter : { "p.foldedNeg", "p.foldedAbs", "p.foldedDiv" })
                            EXPECT_EQ (maximum, graph.getIntOutputValue (meter));
                        EXPECT_EQ (0, graph.getIntOutputValue ("p.foldedRem"));
                        EXPECT_EQ (0, graph.getIntOutputValue ("p.foldedZero"));
                    }
                }
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, IntegerBinaryArithmeticSaturatesAtBothWidths)
{
    for (const bool wide : { false, true })
    {
        const int64_t lo = wide ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int32_t>::min();
        const int64_t hi = wide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();
        struct Case { int64_t a, b, sum, difference, product; };
        const Case cases[] {
            { hi, 1, hi, hi - 1, hi }, { lo, -1, lo, lo + 1, hi },
            { lo, 1, lo + 1, lo, lo }, { hi, -1, hi - 1, hi, -hi },
            { hi, hi, hi, 0, hi }, { lo, lo, lo, 0, hi },
            { hi, lo, -1, hi, lo }, { lo, hi, -1, lo, lo },
            { lo, 0, lo, lo, 0 }, { 0, lo, lo, hi, 0 },
            { 7, -3, 4, 10, -21 }, { -7, -3, -10, -4, 21 },
            { hi / 2, 2, hi / 2 + 2, hi / 2 - 2, hi - 1 },
            { lo / 2, 2, lo / 2 + 2, lo / 2 - 2, lo },
        };
        const auto source = String (R"YDSP(
            processor P {
                input parameter int64 inputA = 0, inputB = 0;
                output parameter int64 sum, difference, product, aliased;
                output parameter int64 foldedSum, foldedDifference, foldedProduct;
                output stream out;
                process {
                    TYPE a = TYPE (inputA);
                    TYPE b = TYPE (inputB);
                    sum = int64 (a + b); difference = int64 (a - b); product = int64 (a * b);
                    TYPE copy = a; copy = copy * b; aliased = int64 (copy);
                    TYPE high = HIGH; TYPE low = LOW; TYPE one = 1; TYPE minusOne = -1;
                    foldedSum = int64 (high + one);
                    foldedDifference = int64 (low - one);
                    foldedProduct = int64 (low * minusOne);
                    out = 0.0;
                }
            }
            graph G { output stream y; node p = P; connection { p.out -> y; } }
        )YDSP").replace ("TYPE", wide ? "int64" : "int32")
                   .replace ("HIGH", String (hi)).replace ("LOW", String (lo));
        for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
            for (const bool fastMath : { false, true })
            {
                SCOPED_TRACE (::testing::Message() << "wide=" << wide << ", tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
                auto options = strictOptions();
                options.optimizationTier = tier;
                options.fastMath = fastMath;
                auto graph = yup::test::patches::compilePatch (source, compiler, options);
                ASSERT_TRUE (graph.isValid());
                ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
                float output = 0.0f;
                YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
                for (const auto& item : cases)
                {
                    SCOPED_TRACE (::testing::Message() << "a=" << item.a << ", b=" << item.b);
                    graph.setIntParameter ("p.inputA", item.a);
                    graph.setIntParameter ("p.inputB", item.b);
                    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
                    EXPECT_EQ (item.sum, graph.getIntOutputValue ("p.sum"));
                    EXPECT_EQ (item.difference, graph.getIntOutputValue ("p.difference"));
                    EXPECT_EQ (item.product, graph.getIntOutputValue ("p.product"));
                    EXPECT_EQ (item.product, graph.getIntOutputValue ("p.aliased"));
                    EXPECT_EQ (hi, graph.getIntOutputValue ("p.foldedSum"));
                    EXPECT_EQ (lo, graph.getIntOutputValue ("p.foldedDifference"));
                    EXPECT_EQ (hi, graph.getIntOutputValue ("p.foldedProduct"));
                }
            }
    }
}

TEST_F (YdspRuntimeOptimizationTests, FloatToIntegerConversionsSaturateAndMapNaNToZero)
{
    struct Case { double value; int64_t narrow, wide; };
    const Case cases[] {
        { 0.0, 0, 0 }, { -0.0, 0, 0 }, { 7.75, 7, 7 }, { -7.75, -7, -7 },
        { 2147483648.0, 2147483647, 2147483648 },
        { -2147483648.0, -2147483648, -2147483648 },
        { 9223372036854775808.0, 2147483647, std::numeric_limits<int64_t>::max() },
        { -9223372036854775808.0, -2147483648, std::numeric_limits<int64_t>::min() },
        { std::numeric_limits<double>::infinity(), 2147483647, std::numeric_limits<int64_t>::max() },
        { -std::numeric_limits<double>::infinity(), -2147483648, std::numeric_limits<int64_t>::min() },
        { std::numeric_limits<double>::quiet_NaN(), 0, 0 },
    };
    for (const bool doublePrecision : { false, true })
        for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
            for (const bool fastMath : { false, true })
            {
                const auto source = String (R"YDSP(
                    processor P {
                        input parameter TYPE value = 0.0;
                        output parameter int64 narrow, wide;
                        output stream out;
                        process { narrow = int64 (int32 (value)); wide = int64 (value); out = 0.0; }
                    }
                    graph G { output stream y; node p = P; connection { p.out -> y; } }
                )YDSP").replace ("TYPE", doublePrecision ? "float64" : "float32");
                auto options = strictOptions();
                options.optimizationTier = tier;
                options.fastMath = fastMath;
                auto graph = yup::test::patches::compilePatch (source, compiler, options);
                ASSERT_TRUE (graph.isValid());
                ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
                float output = 0.0f;
                YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
                for (const auto& item : cases)
                {
                    SCOPED_TRACE (::testing::Message() << "value=" << item.value << ", double=" << doublePrecision << ", tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
                    if (doublePrecision) graph.setDoubleParameter ("p.value", item.value);
                    else graph.setParameter ("p.value", static_cast<float> (item.value));
                    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
                    EXPECT_EQ (item.narrow, graph.getIntOutputValue ("p.narrow"));
                    EXPECT_EQ (item.wide, graph.getIntOutputValue ("p.wide"));
                }
            }
}

TEST_F (YdspRuntimeOptimizationTests, FoldedAndRuntimeIntegerOverflowComparisonsAgree)
{
    const auto source = R"YDSP(
        processor P {
            input parameter int32 largest = 2147483647;
            input parameter int32 smallest = -2147483648;
            input parameter int32 factor = 65536;
            output parameter int64 live, folded;
            output stream out;
            process {
                int32 a = 2147483647;
                int32 b = -2147483648;
                int32 c = 65536;
                live = ((largest + 1) < 0 ? 1 : 0)
                     + ((smallest - 1) > 0 ? 2 : 0)
                     + ((factor * factor) == 0 ? 4 : 0);
                folded = ((a + 1) < 0 ? 1 : 0)
                       + ((b - 1) > 0 ? 2 : 0)
                       + ((c * c) == 0 ? 4 : 0);
                out = 0.0;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        for (const bool fastMath : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
            auto options = strictOptions();
            options.optimizationTier = tier;
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
            float output = 0.0f;
            YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
            EXPECT_EQ (0, graph.getIntOutputValue ("p.live"));
            EXPECT_EQ (0, graph.getIntOutputValue ("p.folded"));
        }
}

TEST_F (YdspRuntimeOptimizationTests, FoldedAndRuntimeShiftsAgreeAtBothWidths)
{
    for (const bool wide : { false, true })
        for (const int count : { -1, 0, 1, 31, 32, 33, 63, 64, 65 })
        {
            const auto source = String (R"YDSP(
                processor P {
                    input parameter TYPE inputA = -7;
                    input parameter TYPE inputB = COUNT;
                    output parameter int64 liveLeft, liveRight, foldedLeft, foldedRight;
                    output stream out;
                    process {
                        TYPE a = -7;
                        TYPE b = COUNT;
                        liveLeft = int64 (inputA << inputB);
                        liveRight = int64 (inputA >> inputB);
                        foldedLeft = int64 (a << b);
                        foldedRight = int64 (a >> b);
                        out = 0.0;
                    }
                }
                graph G { output stream y; node p = P; connection { p.out -> y; } }
            )YDSP").replace ("TYPE", wide ? "int64" : "int32").replace ("COUNT", String (count));
            for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
                for (const bool fastMath : { false, true })
                {
                    SCOPED_TRACE (::testing::Message() << "wide=" << wide << ", count=" << count << ", tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
                    auto options = strictOptions();
                    options.optimizationTier = tier;
                    options.fastMath = fastMath;
                    auto graph = yup::test::patches::compilePatch (source, compiler, options);
                    ASSERT_TRUE (graph.isValid());
                    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
                    float output = 0.0f;
                    YdspOutputBuffer outputs[] { Span<float> (&output, 1) };
                    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
                    EXPECT_EQ (graph.getIntOutputValue ("p.liveLeft"), graph.getIntOutputValue ("p.foldedLeft"));
                    EXPECT_EQ (graph.getIntOutputValue ("p.liveRight"), graph.getIntOutputValue ("p.foldedRight"));
                }
        }
}

TEST_F (YdspRuntimeOptimizationTests, ValueIdentifierAndOutputParameterPreserveMeterBehavior)
{
    const auto source = R"YDSP(
        processor P {
            input stream in;
            input parameter float gain = 2.0;
            output stream out;
            output parameter float level;
            process {
                float value = in * gain;
                out = value;
                level = value;
            }
        }
        graph G {
            input stream x; output stream y;
            input parameter float gain = 3.0;
            output parameter float level;
            node p = P;
            connection { x -> p.in; p.out -> y; gain -> p.gain; p.level -> level; }
        }
    )YDSP";
    auto graph = yup::test::patches::compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    const std::array<float, 1> input { 0.25f };
    std::array<float, 1> output {};
    const YdspInputBuffer inputs[] { Span<const float> (input) };
    YdspOutputBuffer outputs[] { Span<float> (output) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 1 }));
    EXPECT_FLOAT_EQ (0.75f, output[0]);
    EXPECT_FLOAT_EQ (0.75f, graph.getOutputValue ("level"));
}

TEST_F (YdspRuntimeOptimizationTests, ConditionalExpressionsEvaluateOnlyTheSelectedBranch)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            state int calls;
            func touch (result: bool): bool { calls = calls + 1; return result; }
            func number (value: float): float { calls = calls + 1; return value; }
            process {
                calls = 0;
                let positive = in > 0.0;
                let a = positive && touch (true);
                let b = positive || touch (false);
                let c = positive ? number (2.0) : number (3.0);
                let d = positive ? (a ? 4.0 : number (100.0)) : (b ? number (100.0) : 5.0);
                let eager = select (positive, number (6.0), number (7.0));
                out = float (calls) * 1000.0 + c * 100.0 + d * 10.0 + eager;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
    {
        for (const bool fastMath : { false, true })
        {
            YdspCompileOptions options;
            options.optimizationTier = tier;
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 5).wasOk());
            const std::array<float, 5> input { 1.0f, -1.0f, 1.0f, 0.0f, -1.0f };
            std::array<float, 5> output {};
            const YdspInputBuffer inputs[] { Span<const float> (input) };
            YdspOutputBuffer outputs[] { Span<float> (output) };
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 5 }));
            for (size_t i = 0; i < input.size(); ++i)
                EXPECT_FLOAT_EQ (input[i] > 0.0f ? 4246.0f : 4357.0f, output[i]);
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, InitializationPreservesShortCircuitSideEffects)
{
    const auto source = R"YDSP(
        processor P {
            output stream out;
            state int calls;
            state float result;
            func touch (): bool { calls = calls + 1; return true; }
            func number (value: float64): float64 { calls = calls + 1; return value; }
            init {
                let a = false && touch();
                let b = true || touch();
                let c = b ? number (2.0) : 0;
                let d = a ? 0 : number (3.0);
                result = float (calls) * 100.0 + float (c + d);
            }
            process { out = result; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    auto graph = yup::test::patches::compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    std::array<float, 1> output {};
    YdspOutputBuffer outputs[] { Span<float> (output) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    EXPECT_FLOAT_EQ (205.0f, output[0]);
}

TEST_F (YdspRuntimeOptimizationTests, InvalidArrayAndStructIndicesReadZeroAndDiscardWrites)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            output parameter float sentinel;
            state float table[2] = { 10.0, 20.0 };
            struct Voice { float taps[2]; float level; }
            state Voice voices[2];
            init {
                voices[0].taps[0] = 1.0; voices[0].taps[1] = 2.0;
                voices[1].taps[0] = 30.0; voices[1].taps[1] = 40.0;
                voices[0].level = 100.0; voices[1].level = 200.0;
            }
            process {
                let index = int (in);
                out = table[index] + voices[0].taps[index] + voices[index].level;
                if (index < 0 || index >= 2) {
                    table[index] = 999.0;
                    voices[0].taps[index] = 999.0;
                    voices[index].level = 999.0;
                    voices[index].taps[0] = 999.0;
                }
                sentinel = voices[1].taps[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
    {
        YdspCompileOptions options;
        options.optimizationTier = tier;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 7).wasOk());
        const std::array<float, 7> input { -1.0f, 2.0f, -2147483648.0f, 2147483520.0f, 3.0f, 0.0f, 1.0f };
        std::array<float, 7> output {};
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 7 }));
        for (size_t i = 0; i < 5; ++i)
            EXPECT_FLOAT_EQ (0.0f, output[i]);
        EXPECT_FLOAT_EQ (111.0f, output[5]);
        EXPECT_FLOAT_EQ (222.0f, output[6]);
        EXPECT_FLOAT_EQ (30.0f, graph.getOutputValue ("p.sentinel"));
    }
}

TEST_F (YdspRuntimeOptimizationTests, BranchlessArrayReadsHandleExtremeIndicesAtEveryNumericWidth)
{
    for (const auto* type : { "float32", "float64", "int32", "int64" })
    {
        const auto source = String (R"YDSP(
            processor P {
                input parameter int64 index;
                output stream out;
                state TYPE data[2] = { 10, 20 };
                process { out = float (data[int32 (index)]); }
            }
            graph G { output stream out; node p = P; connection { p.out -> out; } }
        )YDSP").replace ("TYPE", type);
        for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        {
            YdspCompileOptions options;
            options.optimizationTier = tier;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
            std::array<float, 1> output {};
            YdspOutputBuffer outputs[] { Span<float> (output) };
            for (const int index : { std::numeric_limits<int32_t>::min(), -1, 0, 1, 2, std::numeric_limits<int32_t>::max() })
            {
                graph.setIntParameter ("p.index", index);
                ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
                EXPECT_FLOAT_EQ (index == 0 ? 10.0f : index == 1 ? 20.0f : 0.0f, output[0]) << type << " index=" << index;
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, ProvenMaskedAndClampedIndicesPreserveReadsAndWrites)
{
    for (const bool masked : { false, true })
        for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        {
            const auto source = String (R"YDSP(
                processor P {
                    input parameter int64 index;
                    output stream out;
                    state float data[8];
                    process {
                        let raw = int32 (index);
                        let bounded = EXPRESSION;
                        out = data[bounded];
                        data[bounded] = float (bounded + 1);
                    }
                }
                graph G { output stream out; node p = P; connection { p.out -> out; } }
            )YDSP").replace ("EXPRESSION", masked ? "raw & 7" : "clamp (raw, 0, 7)");
            YdspCompileOptions options;
            options.optimizationTier = tier;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
            std::array<float, 8> expectedState {};
            std::array<float, 1> output {};
            YdspOutputBuffer outputs[] { Span<float> (output) };
            for (const int index : { std::numeric_limits<int32_t>::min(), -1, 0, 7, 8, std::numeric_limits<int32_t>::max() })
            {
                graph.setIntParameter ("p.index", index);
                ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
                const auto bounded = static_cast<size_t> (masked ? index & 7 : std::clamp (index, 0, 7));
                EXPECT_FLOAT_EQ (expectedState[bounded], output[0]);
                expectedState[bounded] = static_cast<float> (bounded + 1);
            }
        }
}

TEST_F (YdspRuntimeOptimizationTests, ProvenStridedArrayAccessMatchesReferenceAcrossReset)
{
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float table[16];
            process {
                int32 index = (int32 (in) & 3) * 4 + 3;
                out = table[index];
                table[index] = in;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        for (const bool fastMath : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
            auto options = strictOptions();
            options.optimizationTier = tier;
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 8).wasOk());
            const float input[] { -1, 0, 1, 2, 3, -5, 1000000, -1000000 };
            float output[8] {};
            const YdspInputBuffer inputs[] { Span<const float> (input) };
            YdspOutputBuffer outputs[] { Span<float> (output) };
            for (int reset = 0; reset < 2; ++reset)
            {
                graph.reset();
                float reference[16] {};
                for (int block = 0; block < 2; ++block)
                {
                    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 8 }));
                    for (size_t i = 0; i < 8; ++i)
                    {
                        const auto index = (static_cast<int32_t> (input[i]) & 3) * 4 + 3;
                        EXPECT_FLOAT_EQ (reference[index], output[i]);
                        reference[index] = input[i];
                    }
                }
            }
        }
}

TEST_F (YdspRuntimeOptimizationTests, InvalidConstantRingUpdateReadsZeroAndDiscardsWrites)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            state float data[8]; state int wp;
            init { data[0] = 37.0; }
            process {
                wp = -1;
                float invalid = data[wp];
                data[wp] = in;
                out = invalid + data[0];
                wp = wp + 1; if (wp >= 8) { wp = 0; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        for (const bool fastMath : { false, true })
        {
            YdspCompileOptions options;
            options.optimizationTier = tier;
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 4).wasOk());
            const std::array<float, 4> input { 11, 22, 33, 44 };
            std::array<float, 4> output {};
            const YdspInputBuffer inputs[] { Span<const float> (input) };
            YdspOutputBuffer outputs[] { Span<float> (output) };
            for (int block = 0; block < 3; ++block)
            {
                ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 4 }));
                for (const auto value : output)
                    EXPECT_FLOAT_EQ (37.0f, value);
            }
        }
}

TEST_F (YdspRuntimeOptimizationTests, ProvenRingTapsSurviveWrapsParameterChangesAndReset)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            input parameter float delay = 1.0;
            state float data[8]; state int wp;
            process {
                float d = delay;
                if (d > 8.0) { d = 8.0; }
                if (d < 1.0) { d = 1.0; }
                int a = wp - int (d);
                if (a < 0) { a = a + 8; }
                int b = a - 1;
                if (b < 0) { b = b + 8; }
                out = data[a] + data[b];
                data[wp] = in;
                wp = wp + 1;
                if (wp >= 8) { wp = 0; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        for (const bool fastMath : { false, true })
        {
            YdspCompileOptions options;
            options.optimizationTier = tier;
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
            for (int reset = 0; reset < 2; ++reset)
            {
                graph.reset();
                std::array<float, 8> ring {};
                int wp = 0;
                for (const float delay : { 1.0f, 8.0f, 3.5f, -100.0f, 100.0f,
                                          std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                                          std::numeric_limits<float>::quiet_NaN() })
                {
                    graph.setParameter ("p.delay", delay);
                    for (const int length : { 1, 3, 17 })
                    {
                        std::vector<float> input (static_cast<size_t> (length)), output (input.size());
                        for (int i = 0; i < length; ++i) input[static_cast<size_t> (i)] = static_cast<float> (i + 1);
                        const YdspInputBuffer inputs[] { Span<const float> (input) };
                        YdspOutputBuffer outputs[] { Span<float> (output) };
                        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, length }));
                        for (int i = 0; i < length; ++i)
                        {
                            const int d = std::isnan (delay) ? 0 : static_cast<int> (std::clamp (delay, 1.0f, 8.0f));
                            const int a = (wp - d + 8) % 8;
                            const float expected = ring[static_cast<size_t> (a)] + ring[static_cast<size_t> ((a + 7) % 8)];
                            if (std::isnan (delay))
                            {
                                // Existing native float comparisons may clamp NaN to
                                // the lower endpoint before it reaches the conversion.
                                const float lowerClamped = ring[static_cast<size_t> ((wp + 7) % 8)] + ring[static_cast<size_t> ((wp + 6) % 8)];
                                EXPECT_TRUE (output[static_cast<size_t> (i)] == expected || output[static_cast<size_t> (i)] == lowerClamped);
                            }
                            else
                                EXPECT_FLOAT_EQ (expected, output[static_cast<size_t> (i)]);
                            ring[static_cast<size_t> (wp)] = input[static_cast<size_t> (i)];
                            wp = (wp + 1) % 8;
                        }
                    }
                }
            }
        }
}

TEST_F (YdspRuntimeOptimizationTests, IndexedStreamsCheckTheActualBlockLength)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i + 1]; }
                out[blockSize] = 999.0;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    auto graph = yup::test::patches::compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 7).wasOk());
    const std::array<float, 7> input { 1, 2, 3, 4, 5, 6, 7 };
    std::array<float, 7> output {};
    const YdspInputBuffer inputs[] { Span<const float> (input) };
    YdspOutputBuffer outputs[] { Span<float> (output) };
    for (const int length : { 1, 3, 7 })
    {
        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, length }));
        for (int i = 0; i + 1 < length; ++i)
            EXPECT_FLOAT_EQ (input[static_cast<size_t> (i + 1)], output[static_cast<size_t> (i)]);
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (length - 1)]);
    }
}

TEST_F (YdspRuntimeOptimizationTests, MismatchedLoopAndBufferLengthsRetainBoundsGuards)
{
    for (const bool fixedStreamLoop : { false, true })
    {
        const auto source = String (R"YDSP(
            processor P {
                input stream in; output stream out;
                state float z[8] = { 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0 };
                process block {
        )YDSP") + (fixedStreamLoop
                       ? "for i in 0..blockSize { out[i] = 0.0; } for i in 0..10 { out[i] = in[i] * 2.0; }"
                       : "for i in 0..blockSize { out[i] = in[i] + z[i]; }")
            + "} } graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";

        for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        {
            YdspCompileOptions options;
            options.optimizationTier = tier;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 13).wasOk());
            std::array<float, 13> input;
            std::array<float, 14> output;
            for (size_t i = 0; i < input.size(); ++i)
                input[i] = static_cast<float> (i + 1);

            for (const int length : { 1, 3, 7, 8, 9, 10, 13 })
            {
                output.fill (-999.0f);
                const YdspInputBuffer inputs[] { Span<const float> (input.data(), static_cast<size_t> (length)) };
                YdspOutputBuffer outputs[] { Span<float> (output.data(), static_cast<size_t> (length)) };
                ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, length }));
                for (int i = 0; i < length; ++i)
                {
                    const auto expected = fixedStreamLoop ? (i < 10 ? input[static_cast<size_t> (i)] * 2.0f : 0.0f)
                                                          : input[static_cast<size_t> (i)] + (i < 8 ? 2.0f : 0.0f);
                    EXPECT_FLOAT_EQ (expected, output[static_cast<size_t> (i)]);
                }
                for (size_t i = static_cast<size_t> (length); i < output.size(); ++i)
                    EXPECT_FLOAT_EQ (-999.0f, output[i]);
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, RejectsOverflowingStructStateLayouts)
{
    const auto source = R"YDSP(
        processor P {
            struct Voice { float samples[65536]; }
            state Voice voices[65536];
            output stream out;
            process { out = 0.0; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    auto result = compiler.compile (source);
    ASSERT_TRUE (result.failed());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("State layout exceeds"));
}

TEST_F (YdspRuntimeOptimizationTests, VectorFmaPreservesSharedAddendsAcrossBlocks)
{
    for (const bool shared : { false, true })
    {
        const auto source = String (R"YDSP(
            processor P {
                input stream in; output stream out;
                state float z[16];
                process {
                    float sum = 0.0;
                    for i in 0..16 {
                        let product = z[i] * 0.25;
                        z[i] = fma (in, 0.5, product);
                        sum = sum + z[i];
        )YDSP") + (shared ? "sum = sum + product;" : "") + R"YDSP(
                    }
                    out = sum;
                }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP";
        auto graph = yup::test::patches::compilePatch (source, compiler);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
        std::vector<float> input (17), output (17);
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        float state = 0.0f;
        for (const int size : { 1, 17, 3 })
        {
            for (int i = 0; i < size; ++i)
                input[static_cast<size_t> (i)] = (i - 8) * 0.0625f;
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));
            for (int i = 0; i < size; ++i)
            {
                const float product = state * 0.25f;
                state = std::fma (input[static_cast<size_t> (i)], 0.5f, product);
                EXPECT_NEAR (16.0f * (state + (shared ? product : 0.0f)), output[static_cast<size_t> (i)], 0.00001f);
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, UnrolledVectorBankPreservesEveryElementAcrossBlocks)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            state float z[16];
            process {
                float sum = 0.0;
                for i in 0..16 {
                    z[i] = z[i] * 0.5 + in;
                    sum = sum + z[i];
                }
                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const bool fast : { false, true })
    {
        YdspCompileOptions options;
        options.fastMath = fast;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
        std::vector<float> input (17, 0.125f), output (17);
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        float state = 0.0f;
        for (const int size : { 1, 17, 3, 16 })
        {
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));
            for (int i = 0; i < size; ++i)
            {
                state = state * 0.5f + 0.125f;
                EXPECT_NEAR (16.0f * state, output[static_cast<size_t> (i)], 0.00001f);
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, UnrolledLoopsPreserveCarriedValuesAndArrayStateAcrossBlocks)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            state float z[8];
            state float carry = 0.0;
            process {
                float sum = 0.0;
                for i in 1..8 {
                    let previous = carry;
                    carry = carry * 0.5 + in;
                    z[i] = z[i] * 0.25 + previous;
                    sum = sum + z[i];
                }
                out = sum + carry;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const bool fast : { false, true })
    {
        YdspCompileOptions options;
        options.fastMath = fast;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
        std::vector<float> input (17), output (17);
        float state[8] {};
        float carry = 0.0f;
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        for (const int size : { 1, 17, 3, 16 })
        {
            for (int i = 0; i < size; ++i)
                input[static_cast<size_t> (i)] = (i - 8) * 0.0625f;
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));
            for (int i = 0; i < size; ++i)
            {
                float sum = 0.0f;
                for (int j = 1; j < 8; ++j)
                {
                    const float previous = carry;
                    carry = carry * 0.5f + input[static_cast<size_t> (i)];
                    state[j] = state[j] * 0.25f + previous;
                    sum += state[j];
                }
                EXPECT_NEAR (sum + carry, output[static_cast<size_t> (i)], 0.00001f);
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, RematerializedDryWetAndConstantProductsMatchReference)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            input parameter float wet = 0.25;
            process {
                let shaped = sin (in);
                out = ((1.0 - wet) * in + wet * shaped) * 0.05 * 2.302585092994046;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const bool fast : { false, true })
    {
        auto options = strictOptions();
        options.fastMath = fast;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 65).wasOk());
        std::vector<float> input (65), output (65);
        for (size_t i = 0; i < input.size(); ++i)
            input[i] = (static_cast<float> (i) - 32.0f) * 0.125f;
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        for (int block = 0; block < 2; ++block)
        {
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 65 }));
            for (size_t i = 0; i < input.size(); ++i)
                EXPECT_NEAR ((0.75f * input[i] + 0.25f * std::sin (input[i])) * 0.05f * 2.302585092994046f,
                             output[i], 0.000001f);
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, HoistedLocalClampsPreserveOutputAcrossBlocks)
{
    for (const auto* delay : { "-2.0", "3.5", "20.0" })
    {
        const auto source = String (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                input parameter float delay = )YDSP") + delay + R"YDSP(;
                process {
                    float d = delay;
                    if (d > 8.0) d = 8.0;
                    if (d < 1.0) d = 1.0;
                    out = in * (d - float (int32 (d)));
                }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP";
        auto graph = yup::test::patches::compilePatch (source, compiler, strictOptions());
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
        std::vector<float> input (17, 3.0f), output (17);
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        for (int block = 0; block < 2; ++block)
        {
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 17 }));
            for (const auto sample : output)
                EXPECT_FLOAT_EQ (String (delay) == "3.5" ? 1.5f : 0.0f, sample);
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, GuardedClampHoistingTracksParameterChangesAcrossBlocks)
{
    const auto source = R"YDSP(
        processor P {
            input stream in; output stream out;
            input parameter float delay = 3.5;
            state float data[2] = { 10.0, 20.0 };
            process {
                float d = delay;
                if (d > 8.0) d = 8.0;
                if (d < 1.0) d = 1.0;
                let whole = int32 (d);
                out = data[int32 (in)] + d - float (whole);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";
    for (const bool fastMath : { false, true })
    {
        auto options = strictOptions();
        options.fastMath = fastMath;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 4).wasOk());
        const std::array<float, 4> input { -1.0f, 0.0f, 1.0f, 2.0f };
        std::array<float, 4> output {};
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };
        for (const float delay : { 3.5f, -2.0f, 20.0f, 2.25f })
        {
            graph.setParameter ("p.delay", delay);
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 4 }));
            const float clamped = std::clamp (delay, 1.0f, 8.0f);
            const float fraction = clamped - std::floor (clamped);
            for (size_t i = 0; i < input.size(); ++i)
                EXPECT_FLOAT_EQ ((i == 1 ? 10.0f : i == 2 ? 20.0f : 0.0f) + fraction, output[i]);
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, ConstantBasePowerMatchesReferenceAcrossGainRange)
{
    for (const auto* base : { "10.0", "0.5" })
    {
        const auto source = String ("processor P { input stream in; output stream out; process { out = pow (")
            + base + ", in); } } graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";
        for (const bool fastMath : { false, true })
        {
            auto options = strictOptions();
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            std::vector<float> input (97), output (97);
            for (size_t i = 0; i < input.size(); ++i)
                input[i] = -12.0f + static_cast<float> (i) * 0.25f;
            ASSERT_TRUE (graph.prepare (48000.0, static_cast<int> (input.size())).wasOk());
            const YdspInputBuffer inputs[] { Span<const float> (input) };
            YdspOutputBuffer outputs[] { Span<float> (output) };
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, static_cast<int> (input.size()) }));
            for (size_t i = 0; i < input.size(); ++i)
            {
                const float expected = std::pow (String (base).getFloatValue(), input[i]);
                EXPECT_NEAR (expected, output[i], std::abs (expected) * 0.00002f) << "exponent " << input[i];
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, SharedDelayTapsPreserveSamplesAcrossWrapsBlocksAndReset)
{
    const auto source = R"YDSP(
        processor Taps {
            input stream in; output stream a; output stream b; output stream c;
            process { a = in @ 509; b = in @ 1; c = in @ 251; }
        }
        graph G {
            input stream x; output stream a; output stream b; output stream c;
            node p = Taps;
            connection { x -> p.in; p.a -> a; p.b -> b; p.c -> c; }
        }
    )YDSP";

    for (const bool fastMath : { false, true })
    {
        auto options = strictOptions();
        options.fastMath = fastMath;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 257).wasOk());

        for (int pass = 0; pass < 2; ++pass)
        {
            graph.reset();
            int position = 0;

            for (const int size : { 1, 17, 257, 3, 251, 257, 257, 17 })
            {
                std::vector<float> input (static_cast<size_t> (size));
                std::vector<float> a (input.size()), b (input.size()), c (input.size());
                for (int i = 0; i < size; ++i)
                    input[static_cast<size_t> (i)] = static_cast<float> (position + i + 1);

                const YdspInputBuffer inputs[] { Span<const float> (input.data(), input.size()) };
                YdspOutputBuffer outputs[] {
                    Span<float> (a.data(), a.size()),
                    Span<float> (b.data(), b.size()),
                    Span<float> (c.data(), c.size())
                };
                ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));

                for (int i = 0; i < size; ++i)
                {
                    const auto expected = [position, i] (int delay)
                    {
                        return position + i < delay ? 0.0f : static_cast<float> (position + i - delay + 1);
                    };
                    EXPECT_FLOAT_EQ (expected (509), a[static_cast<size_t> (i)]);
                    EXPECT_FLOAT_EQ (expected (1), b[static_cast<size_t> (i)]);
                    EXPECT_FLOAT_EQ (expected (251), c[static_cast<size_t> (i)]);
                }
                position += size;
            }
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, StateCopyChainsPreserveSampleHistoryAcrossBlocks)
{
    const auto source = R"YDSP(
        processor History {
            input stream in; output stream a; output stream b;
            state float x1, x2;
            state int i1, i2;
            process {
                let previous = x2;
                let previousInt = i2;
                x2 = x1;
                x1 = in;
                i2 = i1;
                i1 = int (in);
                a = previous;
                b = float (previousInt);
            }
        }
        graph G {
            input stream x; output stream a; output stream b;
            node p = History;
            connection { x -> p.in; p.a -> a; p.b -> b; }
        }
    )YDSP";

    for (const bool fastMath : { false, true })
    {
        auto options = strictOptions();
        options.fastMath = fastMath;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
        int position = 0;

        for (const int size : { 1, 1, 17, 3, 9 })
        {
            std::vector<float> input (static_cast<size_t> (size));
            std::vector<float> a (input.size()), b (input.size());
            for (int i = 0; i < size; ++i)
                input[static_cast<size_t> (i)] = static_cast<float> (position + i + 1);

            const YdspInputBuffer inputs[] { Span<const float> (input) };
            YdspOutputBuffer outputs[] { Span<float> (a), Span<float> (b) };
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));

            for (int i = 0; i < size; ++i)
            {
                const float expected = position + i < 2 ? 0.0f : static_cast<float> (position + i - 1);
                EXPECT_FLOAT_EQ (expected, a[static_cast<size_t> (i)]);
                EXPECT_FLOAT_EQ (expected, b[static_cast<size_t> (i)]);
            }
            position += size;
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, LocalInitializersRemainIndependentOfLaterAssignments)
{
    const auto source = R"YDSP(
        processor Copies {
            input stream in; output stream a; output stream b;
            process {
                float original = in;
                let saved = original;
                float copy = original;
                copy = copy + 1.0;
                original = original + 2.0;
                a = saved;
                b = copy + original;
            }
        }
        graph G {
            input stream x; output stream a; output stream b;
            node p = Copies;
            connection { x -> p.in; p.a -> a; p.b -> b; }
        }
    )YDSP";

    for (const bool fastMath : { false, true })
    {
        auto options = strictOptions();
        options.fastMath = fastMath;
        auto graph = yup::test::patches::compilePatch (source, compiler, options);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 4).wasOk());
        const std::vector<float> input { -2.0f, 0.0f, 3.0f, 7.0f };
        std::vector<float> a (input.size()), b (input.size());
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (a), Span<float> (b) };
        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 4 }));

        for (size_t i = 0; i < input.size(); ++i)
        {
            EXPECT_FLOAT_EQ (input[i], a[i]);
            EXPECT_FLOAT_EQ (2.0f * input[i] + 3.0f, b[i]);
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, ReusesIntermediateBuffersAndSupportsRepeatedPreparation)
{
    auto graph = chain (false);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (0u, graph.getScratchMemorySizeBytes());

    for (const int size : { 17, 64, 9 })
    {
        ASSERT_TRUE (graph.prepare (48000.0, size).wasOk());

        const auto regionBytes = (static_cast<size_t> (size) * sizeof (float) + 7) & ~size_t (7);
        EXPECT_EQ (2 * regionBytes, graph.getScratchMemorySizeBytes());

        std::vector<float> input (static_cast<size_t> (size), 2.0f);
        std::vector<float> output (static_cast<size_t> (size));
        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };

        for (int block = 0; block < 3; ++block)
        {
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));

            for (const auto sample : output)
                EXPECT_FLOAT_EQ (6.0f, sample);
        }
    }
}

TEST_F (YdspRuntimeOptimizationTests, ReusesFloat64RegionsAtOddBlockSizes)
{
    auto graph = chain (true);

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());
    EXPECT_EQ (2u * 17u * sizeof (double), graph.getScratchMemorySizeBytes());

    std::array<double, 17> input {}, output {};
    input.fill (0.25);
    const YdspInputBuffer inputs[] { Span<const double> (input) };
    YdspOutputBuffer outputs[] { Span<double> (output) };

    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 17 }));

    for (const auto sample : output)
        EXPECT_DOUBLE_EQ (4.25, sample);
}

TEST_F (YdspRuntimeOptimizationTests, PinsOutputStorageWhenKernelsReadItsPreviousContents)
{
    auto graph = chain (false, true);

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 16).wasOk());
    EXPECT_EQ (3u * 16u * sizeof (float), graph.getScratchMemorySizeBytes());
}

TEST_F (YdspRuntimeOptimizationTests, FanOutKeepsSourcesAliveUntilTheirFinalConsumer)
{
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor Step {
            input stream in; output stream out;
            process { out = in + 1.0; }
        }
        graph G {
            input stream x; output stream y;
            node a = Step; node b = Step; node c = Step; node d = Step;
            connection {
                x -> a.in; a.out -> b.in; a.out -> c.in;
                b.out -> d.in; c.out -> d.in; d.out -> y;
            }
        }
    )YDSP", compiler, strictOptions());

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());

    std::array<float, 17> input {}, output {};
    input.fill (2.0f);
    const YdspInputBuffer inputs[] { Span<const float> (input) };
    YdspOutputBuffer outputs[] { Span<float> (output) };

    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 17 }));

    for (const auto sample : output)
        EXPECT_FLOAT_EQ (9.0f, sample);
}

TEST_F (YdspRuntimeOptimizationTests, DelayWrappingMatchesTheGlobalTimelineAcrossVariableBlocks)
{
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor Pass { input stream in; output stream out; process { out = in; } }
        graph G {
            input stream x; output stream y; node p = Pass;
            connection { x -> [5] -> p.in; p.out -> y; }
        }
    )YDSP", compiler, strictOptions());

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 17).wasOk());

    int position = 0;

    for (const int size : { 1, 17, 3, 9, 17, 2, 17 })
    {
        std::vector<float> input (static_cast<size_t> (size)), output (static_cast<size_t> (size));
        for (int i = 0; i < size; ++i)
            input[static_cast<size_t> (i)] = static_cast<float> (position + i + 1);

        const YdspInputBuffer inputs[] { Span<const float> (input) };
        YdspOutputBuffer outputs[] { Span<float> (output) };

        ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, size }));

        for (int i = 0; i < size; ++i)
            EXPECT_FLOAT_EQ (position + i < 5 ? 0.0f : static_cast<float> (position + i - 4), output[static_cast<size_t> (i)]);

        position += size;
    }
}

TEST_F (YdspRuntimeOptimizationTests, StrictSubtractionPreservesSignedZeroAndSpecialValues)
{
    for (const bool useDouble : { false, true })
    {
        const String type = useDouble ? "float64" : "float";
        const auto source = String ("processor Sub { input stream ") + type + " in; output stream " + type
            + " out; process { out = in - (-0.0); } } graph G { input stream " + type
            + " x; output stream " + type + " y; node p = Sub; connection { x -> p.in; p.out -> y; } }";
        auto graph = yup::test::patches::compilePatch (source, compiler, strictOptions());

        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, 6).wasOk());

        const auto verify = [&]<typename Float>()
        {
            const std::array<Float, 6> input { Float (-0.0), Float (0.0),
                std::numeric_limits<Float>::infinity(), -std::numeric_limits<Float>::infinity(),
                std::numeric_limits<Float>::quiet_NaN(), std::numeric_limits<Float>::denorm_min() };
            std::array<Float, 6> output {};
            const YdspInputBuffer inputs[] { Span<const Float> (input) };
            YdspOutputBuffer outputs[] { Span<Float> (output) };

            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inputs, outputs, 6 }));

            EXPECT_FALSE (std::signbit (output[0]));
            EXPECT_FALSE (std::signbit (output[1]));
            EXPECT_EQ (input[2], output[2]);
            EXPECT_EQ (input[3], output[3]);
            EXPECT_TRUE (std::isnan (output[4]));

            // Match the calling thread's current flush-to-zero policy.
            volatile Float tiny = input[5];
            EXPECT_EQ (tiny - Float (-0.0), output[5]);
        };

        if (useDouble)
            verify.template operator()<double>();
        else
            verify.template operator()<float>();
    }
}

TEST_F (YdspRuntimeOptimizationTests, AllocationInstrumentationDetectsCppAndHeapBlockGrowth)
{
#if ! YUP_ENABLE_ALLOCATION_HOOKS
    GTEST_SKIP() << "Enable YUP_TEST_ALLOCATION_HOOKS to measure allocations";
#else
    yup::test::YdspAllocationCounter allocations;
    std::vector<int> values;

    allocations.start();
    values.resize (256);
    const auto cppCount = allocations.stop();

    EXPECT_GT (cppCount, 0u);

    HeapBlock<int> memory;

    allocations.start();
    memory.malloc (256);
    const auto heapCount = allocations.stop();

    EXPECT_GT (heapCount, 0u);
#endif
}

TEST_F (YdspRuntimeOptimizationTests, TraceCapturesScalarsInOrderAndCanBeDisabled)
{
    const auto source = R"YDSP(
        processor P {
            output stream out;
            state int count;
            init { trace("ready {{noise}}"); }
            process {
                int x = count;
                int64 exact = 9007199254740993;
                float value = 0.5;
                float64 precise = 0.25;
                bool odd = (x & 1) == 1;
                trace("x={x}, exact={exact}, value={value}, precise={precise}, odd={odd}, again={x}");
                count = count + 1;
                if (odd) { trace("odd {x}"); }
                out = float (count);
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        for (const bool enabled : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "tier=" << static_cast<int> (tier) << ", enabled=" << enabled);
            YdspCompileOptions options;
            options.optimizationTier = tier;
            options.enableTracing = enabled;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid());
            ASSERT_TRUE (graph.prepare (48000.0, 2).wasOk());
            auto messages = graph.drainTraceMessages();
            ASSERT_EQ (enabled ? 1 : 0, messages.size());
            if (enabled)
                EXPECT_EQ (String ("ready {noise}"), messages[0]);
            float samples[2] {};
            YdspOutputBuffer outputs[] { Span<float> (samples, 2) };
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2 }));
            EXPECT_EQ (1.0f, samples[0]);
            EXPECT_EQ (2.0f, samples[1]);
            messages = graph.drainTraceMessages();
            ASSERT_EQ (enabled ? 3 : 0, messages.size());
            if (enabled)
            {
                EXPECT_EQ (String ("x=0, exact=9007199254740993, value=") + String (0.5) + ", precise=" + String (0.25) + ", odd=0, again=0", messages[0]);
                EXPECT_EQ (String ("x=1, exact=9007199254740993, value=") + String (0.5) + ", precise=" + String (0.25) + ", odd=1, again=1", messages[1]);
                EXPECT_EQ (String ("odd 1"), messages[2]);
            }
            EXPECT_TRUE (graph.drainTraceMessages().isEmpty());
            graph.setTracingEnabled (false);
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2 }));
            EXPECT_TRUE (graph.drainTraceMessages().isEmpty());
            graph.setTracingEnabled (true);
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2 }));
            EXPECT_EQ (enabled ? 3 : 0, graph.drainTraceMessages().size());
        }
}

TEST_F (YdspRuntimeOptimizationTests, TraceQueueDropsWholeMessagesAndReusesDrainedSlots)
{
    YdspCompileOptions options;
    options.enableTracing = true;
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            output stream out; state int count;
            process { trace("{count}"); count = count + 1; out = 0.0; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", compiler, options);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 300).wasOk());
    float samples[300] {};
    YdspOutputBuffer outputs[] { Span<float> (samples, 300) };
#if YUP_ENABLE_ALLOCATION_HOOKS
    yup::test::YdspAllocationCounter allocations;
    allocations.start();
#endif
    const auto result = graph.process ({ {}, outputs, 300 });
#if YUP_ENABLE_ALLOCATION_HOOKS
    const auto count = allocations.stop();
    EXPECT_EQ (0u, count);
#endif
    ASSERT_EQ (YdspProcessResult::ok, result);
    EXPECT_EQ (44u, graph.getDroppedTraceCount());
    auto messages = graph.drainTraceMessages();
    ASSERT_EQ (256, messages.size());
    for (int i = 0; i < messages.size(); ++i)
        EXPECT_EQ (String (i), messages[i]);
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2 }));
    messages = graph.drainTraceMessages();
    ASSERT_EQ (2, messages.size());
    EXPECT_EQ (String ("300"), messages[0]);
    EXPECT_EQ (String ("301"), messages[1]);
}

TEST_F (YdspRuntimeOptimizationTests, TraceRejectsMalformedAndOutOfScopePlaceholdersEvenWhenDisabled)
{
    struct Case { const char* statement; const char* diagnostic; };
    const Case cases[] {
        { "trace(1);", "string literal" },
        { "trace(\"{x\");", "Invalid trace placeholder" },
        { "trace(\"}\");", "Unmatched '}'" },
        { "trace(\"{}\");", "Invalid trace placeholder" },
        { "trace(\"{x + 1}\");", "Invalid trace placeholder" },
        { "trace(\"{missing}\");", "Unknown symbol 'missing'" },
        { "trace(\"{x}{x}{x}{x}{x}{x}{x}{x}{x}\");", "at most 8" },
        { "{ int hidden = 1; } trace(\"{hidden}\");", "Unknown symbol 'hidden'" },
        { "trace(\"text\", x);", "')' after the trace string" },
    };
    for (const auto& item : cases)
        for (const bool enabled : { false, true })
        {
            SCOPED_TRACE (item.statement);
            YdspCompileOptions options;
            options.enableTracing = enabled;
            const auto source = String ("processor P { output stream out; process { int x = 1; ") + item.statement
                              + " out = 0.0; } } graph G { output stream y; node p = P; connection { p.out -> y; } }";
            const auto result = compiler.compile (source, options);
            EXPECT_FALSE (result.wasOk());
            EXPECT_TRUE (compiler.getDiagnostics().toString().contains (item.diagnostic)) << compiler.getDiagnostics().toString();
        }
}

TEST_F (YdspRuntimeOptimizationTests, TraceCapturesEventAndFunctionLocals)
{
    YdspCompileOptions options;
    options.enableTracing = true;
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            input event midi; output stream out;
            func twice (x: float) : float { trace("function {x}"); return x * 2.0; }
            event midi (e: noteOn) { int note = int (e.pitch); trace("note {note}"); }
            process { out = twice (0.5); }
        }
        graph G { input event midi; output stream y; node p = P;
            connection { midi -> p.midi; p.out -> y; } }
    )YDSP", compiler, options);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float sample = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&sample, 1) };
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 1.0f), 0);
    const MidiBuffer* events[] { &midi };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1, Span<const MidiBuffer*> (events, 1), {} }));
    const auto messages = graph.drainTraceMessages();
    ASSERT_EQ (2, messages.size());
    EXPECT_EQ (String ("note 60"), messages[0]);
    EXPECT_EQ (String ("function ") + String (0.5), messages[1]);
    EXPECT_EQ (1.0f, sample);
}

TEST_F (YdspRuntimeOptimizationTests, TraceOnlyCapturesTakenFunctionReturnBranches)
{
    YdspCompileOptions options;
    options.enableTracing = true;
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            output stream out;
            func record (x: int) : int { trace("chosen {x}"); return x; }
            func choose (x: int) : int {
                if (x > 0) { return record (1); }
                return record (2);
            }
            func fallback (x: int) : int {
                if (x > 0) { return record (3); }
                trace("fallback");
                return record (4);
            }
            process { out = float (choose (1) + choose (0) + fallback (0)); }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", compiler, options);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float sample = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&sample, 1) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    const auto messages = graph.drainTraceMessages();
    ASSERT_EQ (4, messages.size());
    EXPECT_EQ (String ("chosen 1"), messages[0]);
    EXPECT_EQ (String ("chosen 2"), messages[1]);
    EXPECT_EQ (String ("fallback"), messages[2]);
    EXPECT_EQ (String ("chosen 4"), messages[3]);
    EXPECT_EQ (7.0f, sample);
}

TEST_F (YdspRuntimeOptimizationTests, TraceFormatsBooleansAndIntegerCastsAsIntegers)
{
    YdspCompileOptions options;
    options.enableTracing = true;
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            output stream out; state bool flag;
            process {
                let alias = flag;
                int numeric = int (flag);
                bool local = true;
                trace("{flag} {alias} {numeric} {local}");
                int narrow = int (true);
                int64 wide = int64 (true);
                float single = float (true);
                float64 precise = float64 (true);
                trace("{narrow} {wide} {single} {precise}");
                out = 0.0;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", compiler, options);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 1).wasOk());
    float sample = 0.0f;
    YdspOutputBuffer outputs[] { Span<float> (&sample, 1) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 1 }));
    const auto messages = graph.drainTraceMessages();
    ASSERT_EQ (2, messages.size());
    EXPECT_EQ (String ("0 0 0 1"), messages[0]);
    EXPECT_EQ (String ("1 1 ") + String (1.0) + " " + String (1.0), messages[1]);
}

#if ! YUP_WASM
TEST_F (YdspRuntimeOptimizationTests, TraceCanBeDrainedConcurrentlyWithProcessing)
{
    YdspCompileOptions options;
    options.enableTracing = true;
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            output stream out; state int count;
            process { trace("{count}"); count = count + 1; out = 0.0; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", compiler, options);
    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.prepare (48000.0, 64).wasOk());
    std::atomic<bool> done { false };
    bool success = true;
    std::thread producer ([&]
    {
        float samples[64] {};
        YdspOutputBuffer outputs[] { Span<float> (samples, 64) };
        for (int block = 0; block < 64; ++block)
            if (graph.process ({ {}, outputs, 64 }) != YdspProcessResult::ok)
                success = false;
        done.store (true, std::memory_order_release);
    });
    StringArray captured;
    while (! done.load (std::memory_order_acquire))
    {
        captured.addArray (graph.drainTraceMessages());
        std::this_thread::yield();
    }
    producer.join();
    captured.addArray (graph.drainTraceMessages());
    EXPECT_TRUE (success);
    EXPECT_EQ (4096u, static_cast<uint64_t> (captured.size()) + graph.getDroppedTraceCount());
    int previous = -1;
    for (const auto& message : captured)
    {
        const int current = message.getIntValue();
        EXPECT_GT (current, previous);
        EXPECT_LT (current, 4096);
        EXPECT_EQ (String (current), message);
        previous = current;
    }
}
#endif

TEST_F (YdspRuntimeOptimizationTests, MatchEvaluatesSelectorOnceAndScopesSelectedArms)
{
    const auto source = R"YDSP(
        processor P {
            output stream out;
            state int count = -1;
            func next () : int { int previous = count; count = count + 1; return previous; }
            process {
                int result = 7;
                int outer = 5;
                match (next()) {
                    -1 => { int local = 10; result = local; },
                    0 => { int local = 20; result = local; },
                    1 => {
                        match (true) { false => { result = 99; }, true => { result = 30; } }
                    },
                    _ => { result = 40; },
                }
                match (count) { -100 => { result = 99; } }
                out = float (result * 10 + count + outer);
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";
    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic, YdspOptimizationTier::aggressive })
        for (const bool fastMath : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "tier=" << static_cast<int> (tier) << ", fastMath=" << fastMath);
            YdspCompileOptions options;
            options.optimizationTier = tier;
            options.fastMath = fastMath;
            auto graph = yup::test::patches::compilePatch (source, compiler, options);
            ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
            ASSERT_TRUE (graph.prepare (48000.0, 5).wasOk());
            float samples[5] {};
            YdspOutputBuffer outputs[] { Span<float> (samples, 5) };
            ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 5 }));
            const float expected[] { 105.0f, 206.0f, 307.0f, 408.0f, 409.0f };
            for (int i = 0; i < 5; ++i)
                EXPECT_EQ (expected[i], samples[i]);
        }
}

TEST_F (YdspRuntimeOptimizationTests, MatchPreservesWidePatternsAndFunctionReturns)
{
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            output stream out; state int64 value = 9007199254740992;
            func classify (x: int64) : float {
                match (x) {
                    -9223372036854775808 => { return -1.0; },
                    9007199254740992 => { return 1.0; },
                    9007199254740993 => { return 2.0; },
                    _ => { return 3.0; }
                }
            }
            process { out = classify (value); value = value + 1; }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP", compiler);
    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (graph.prepare (48000.0, 3).wasOk());
    float samples[3] {};
    YdspOutputBuffer outputs[] { Span<float> (samples, 3) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 3 }));
    EXPECT_EQ (1.0f, samples[0]);
    EXPECT_EQ (2.0f, samples[1]);
    EXPECT_EQ (3.0f, samples[2]);
}

TEST_F (YdspRuntimeOptimizationTests, MatchRejectsInvalidPatternsSelectorsAndArmSyntax)
{
    const struct { const char* statement; const char* diagnostic; } cases[] {
        { "match (1) {}", "at least one arm" },
        { "match (1) { 1 => {}, 1 => {} }", "Duplicate match pattern" },
        { "match (1) { 0 => {}, -0 => {} }", "Duplicate match pattern" },
        { "match (1) { _ => {}, 1 => {} }", "must be last" },
        { "match (1) { _ => {}, _ => {} }", "must be last" },
        { "match (1) { 1.0 => {} }", "integer or boolean literal" },
        { "match (1) { x => {} }", "integer or boolean literal" },
        { "match (1) { -x => {} }", "integer literal after '-'" },
        { "match (1) { --9223372036854775808 => {} }", "integer literal after '-'" },
        { "match (true) { false => {}, false => {} }", "Duplicate match pattern" },
        { "match (1) { 1 -> {} }", "'=>'" },
        { "match (1) { 1 => out = 1.0; }", "match arm body" },
        { "match (1) { 1 => {} 2 => {} }", "separate arms" },
        { "match (0.5) { _ => {} }", "selector must be an integer or bool" },
        { "match (1) { 1 => { int hidden = 2; } } out = float (hidden);", "hidden" },
    };
    for (const auto& item : cases)
    {
        SCOPED_TRACE (item.statement);
        const auto source = String ("processor P { output stream out; process { out = 0.0; ")
                          + item.statement + " } } graph G { output stream y; node p = P; connection { p.out -> y; } }";
        const auto result = compiler.compile (source);
        EXPECT_FALSE (result.wasOk());
        EXPECT_TRUE (compiler.getDiagnostics().toString().contains (item.diagnostic)) << compiler.getDiagnostics().toString();
    }
}

TEST_F (YdspRuntimeOptimizationTests, MatchWorksInInitAndEventsAndWithDynamicBooleans)
{
    auto graph = yup::test::patches::compilePatch (R"YDSP(
        processor P {
            input event midi; output stream out; state int note; state int count;
            init { match (1) { _ => { note = 5; } } }
            event midi (e: noteOn) {
                match (int (e.pitch)) { 60 => { note = 60; }, _ => { note = 1; } }
            }
            process {
                match ((count & 1) == 0) {
                    true => { out = float (note); }, false => { out = -float (note); }
                }
                count = count + 1;
            }
        }
        graph G { input event midi; output stream y; node p = P;
            connection { midi -> p.midi; p.out -> y; } }
    )YDSP", compiler);
    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_TRUE (graph.prepare (48000.0, 2).wasOk());
    float samples[2] {};
    YdspOutputBuffer outputs[] { Span<float> (samples, 2) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2 }));
    EXPECT_EQ (5.0f, samples[0]);
    EXPECT_EQ (-5.0f, samples[1]);
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 1.0f), 0);
    const MidiBuffer* events[] { &midi };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ {}, outputs, 2, Span<const MidiBuffer*> (events, 1), {} }));
    EXPECT_EQ (60.0f, samples[0]);
    EXPECT_EQ (-60.0f, samples[1]);
}

TEST_F (YdspRuntimeOptimizationTests, ConstantMatchRemovesUnselectedArmsAndTheirLoops)
{
    for (const auto* selector : { "2", "1 + 1", "9" })
    {
        SCOPED_TRACE (selector);
        const auto source = String (R"YDSP(
            processor P {
                output stream out; state float result;
                init {
                    match ()YDSP") + selector + R"YDSP() {
                        1 => { for i in 0..4 { result = float (i); trace("discarded {i}"); } },
                        2 => { result = 0.5; },
                        _ => { result = 0.75; }
                    }
                }
                process { out = result; }
            }
            graph G { output stream y; node p = P; connection { p.out -> y; } }
        )YDSP";
        YdspDiagnostics diagnostics;
        YdspLexer lexer (source, diagnostics);
        YdspParser parser (lexer.tokenize(), diagnostics);
        auto program = parser.parseProgram();
        ASSERT_NE (nullptr, program);
        YdspSemanticAnalyzer analyzer (diagnostics);
        auto analyzed = analyzer.analyze (std::move (program));
        ASSERT_NE (nullptr, analyzed);
        YdspOptimizer optimizer (diagnostics);
        optimizer.setTracingEnabled (true);
        auto ir = optimizer.build (*analyzed);
        ASSERT_NE (nullptr, ir);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        bool foundInit = false;
        for (const auto& kernel : ir->kernels)
        {
            if (! kernel->isInit)
                continue;
            foundInit = true;
            EXPECT_TRUE (kernel->loops.empty());
            int stores = 0;
            for (size_t index = 0; index < kernel->blocks.size(); ++index)
            {
                const auto& block = kernel->blocks[index];
                EXPECT_NE (YdspIrTerm::branchIf, block.term);
                if (block.term == YdspIrTerm::branch)
                    EXPECT_EQ (static_cast<int> (index + 1), block.termTarget);
                for (const auto& inst : block.insts)
                {
                    EXPECT_NE (YdspIrOp::traceCommit, inst.op);
                    if (inst.op == YdspIrOp::storeStateF)
                        ++stores;
                }
            }
            EXPECT_EQ (1, stores);
        }
        EXPECT_TRUE (foundInit);
    }
}
