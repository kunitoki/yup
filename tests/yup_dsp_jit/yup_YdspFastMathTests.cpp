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

#include <gtest/gtest.h>

#include <yup_dsp_jit/yup_dsp_jit.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace yup::test
{

//==============================================================================

class YdspFastMathTests : public ::testing::Test
{
protected:
    static constexpr float piValue = 3.14159265358979323846f;

    static YdspIrFunction makeUnaryFunction (YdspIrOp op, YdspValueType type, int lanes, bool fastMath)
    {
        YdspIrFunction fn;
        fn.fastMath = fastMath;

        // value 0 = loaded input, value 1 = transcendental result
        fn.valueTypes = { type, type };
        if (lanes > 1)
            fn.valueLanes = { lanes, lanes };

        YdspIrBlock block;
        block.insts.push_back ({ YdspIrOp::loadStateF, 0, 0 });
        block.insts.push_back ({ op, 1, 0 });
        block.insts.push_back ({ YdspIrOp::storeStateF, -1, 1, -1, -1, 0 });
        fn.blocks.push_back (std::move (block));

        return fn;
    }

    static YdspIrFunction makeBinaryFunction (YdspIrOp op, YdspValueType type, int lanes, bool fastMath)
    {
        YdspIrFunction fn;
        fn.fastMath = fastMath;

        fn.valueTypes = { type, type, type };
        if (lanes > 1)
            fn.valueLanes = { lanes, lanes, lanes };

        YdspIrBlock block;
        block.insts.push_back ({ YdspIrOp::loadStateF, 0, 0 });
        block.insts.push_back ({ YdspIrOp::loadStateF, 1, 1 });
        block.insts.push_back ({ op, 2, 0, 1 });
        block.insts.push_back ({ YdspIrOp::storeStateF, -1, 2, -1, -1, 0 });
        fn.blocks.push_back (std::move (block));

        return fn;
    }

    static int countInst (const YdspIrFunction& fn, YdspIrOp op)
    {
        int count = 0;

        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
                if (inst.op == op)
                    ++count;

        return count;
    }

    static int countTranscendentals (const YdspIrFunction& fn)
    {
        return countInst (fn, YdspIrOp::tanhF)
             + countInst (fn, YdspIrOp::sinF)
             + countInst (fn, YdspIrOp::cosF)
             + countInst (fn, YdspIrOp::tanF)
             + countInst (fn, YdspIrOp::atanF)
             + countInst (fn, YdspIrOp::atan2F)
             + countInst (fn, YdspIrOp::expF)
             + countInst (fn, YdspIrOp::logF)
             + countInst (fn, YdspIrOp::log10F)
             + countInst (fn, YdspIrOp::powF);
    }

    static void verifyVectorLanesWellFormed (const YdspIrFunction& fn, int expectedLanes)
    {
        EXPECT_EQ (fn.valueTypes.size(), fn.valueLanes.size());

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.op == YdspIrOp::constF || inst.op == YdspIrOp::constI)
                {
                    EXPECT_EQ (1, fn.laneCountOf (inst.result));
                    continue;
                }

                if (inst.op == YdspIrOp::vsplat)
                {
                    EXPECT_EQ (expectedLanes, fn.laneCountOf (inst.result));
                    EXPECT_EQ (1, fn.laneCountOf (inst.a));
                    continue;
                }

                if (inst.op == YdspIrOp::shlI || inst.op == YdspIrOp::shrI)
                {
                    EXPECT_EQ (expectedLanes, fn.laneCountOf (inst.result));
                    EXPECT_EQ (expectedLanes, fn.laneCountOf (inst.a));
                    EXPECT_EQ (1, fn.laneCountOf (inst.b));
                    continue;
                }

                if (inst.result >= 0 && inst.op != YdspIrOp::loadStateF)
                    EXPECT_EQ (expectedLanes, fn.laneCountOf (inst.result));

                if (inst.op == YdspIrOp::bitcastFtoI)
                {
                    EXPECT_EQ (YdspValueType::int32Type, fn.valueTypes[static_cast<size_t> (inst.result)]);
                    EXPECT_EQ (YdspValueType::float32Type, fn.valueTypes[static_cast<size_t> (inst.a)]);
                }
                else if (inst.op == YdspIrOp::bitcastItoF)
                {
                    EXPECT_EQ (YdspValueType::float32Type, fn.valueTypes[static_cast<size_t> (inst.result)]);
                    EXPECT_EQ (YdspValueType::int32Type, fn.valueTypes[static_cast<size_t> (inst.a)]);
                }
            }
        }
    }

    static std::unique_ptr<YdspIrProgram> buildIr (StringRef source, YdspDiagnostics& diagnostics, bool fastMath)
    {
        YdspLexer lexer (source, diagnostics);
        auto tokens = lexer.tokenize();

        YdspParser parser (std::move (tokens), diagnostics);
        auto program = parser.parseProgram();
        if (program == nullptr)
            return nullptr;

        YdspSemanticAnalyzer analyzer (diagnostics);
        auto analyzed = analyzer.analyze (std::move (program));
        if (analyzed == nullptr)
            return nullptr;

        YdspOptimizer optimizer (diagnostics);
        optimizer.setFastMath (fastMath);

        return optimizer.build (*analyzed);
    }

    static std::vector<float> runUnary (StringRef expr, const std::vector<float>& input, const YdspCompileOptions& options)
    {
        const String source = "processor P { input stream in; output stream out; process { out = " + String (expr) + "; } }\n"
                              "graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";

        YdspCompiler compiler;
        auto result = compiler.compile (source, options);
        EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

        if (! result.wasOk())
            return {};

        auto graph = std::move (result).getValue();
        graph.prepare (48000.0, static_cast<int> (input.size()));

        std::vector<float> output (input.size(), 0.0f);
        std::vector<YdspInputBuffer> inputs { YdspInputBuffer (Span<const float> (input.data(), input.size())) };
        std::vector<YdspOutputBuffer> outputs { YdspOutputBuffer (Span<float> (output.data(), output.size())) };
        graph.process (YdspProcessRequest { inputs, outputs, static_cast<int> (input.size()) });

        return output;
    }

    static std::vector<float> runBinary (StringRef expr, const std::vector<float>& inputA, const std::vector<float>& inputB, const YdspCompileOptions& options)
    {
        const String source = "processor P { input stream a; input stream b; output stream out; process { out = " + String (expr) + "; } }\n"
                              "graph G { input stream x; input stream y; output stream z; node p = P; connection { x -> p.a; y -> p.b; p.out -> z; } }";

        YdspCompiler compiler;
        auto result = compiler.compile (source, options);
        EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

        if (! result.wasOk())
            return {};

        auto graph = std::move (result).getValue();
        graph.prepare (48000.0, static_cast<int> (inputA.size()));

        std::vector<float> output (inputA.size(), 0.0f);
        std::vector<YdspInputBuffer> inputs
        {
            YdspInputBuffer (Span<const float> (inputA.data(), inputA.size())),
            YdspInputBuffer (Span<const float> (inputB.data(), inputB.size()))
        };
        std::vector<YdspOutputBuffer> outputs { YdspOutputBuffer (Span<float> (output.data(), output.size())) };
        graph.process (YdspProcessRequest { inputs, outputs, static_cast<int> (inputA.size()) });

        return output;
    }

    static YdspCompileOptions scalarFastMathOptions()
    {
        YdspCompileOptions options;
        options.targetPolicy = YdspTargetPolicy::baseline;
        options.baselineTarget = YdspNativeTarget::scalar;
        options.optimizationTier = YdspOptimizationTier::baseline;
        options.fastMath = true;
        return options;
    }

    static YdspCompileOptions hostFastMathOptions()
    {
        YdspCompileOptions options;
        options.targetPolicy = YdspTargetPolicy::host;
        options.optimizationTier = YdspOptimizationTier::automatic;
        options.fastMath = true;
        return options;
    }

    static YdspCompileOptions hostStrictOptions()
    {
        YdspCompileOptions options;
        options.targetPolicy = YdspTargetPolicy::host;
        options.optimizationTier = YdspOptimizationTier::automatic;
        options.fastMath = false;
        return options;
    }
};

//==============================================================================
// IR pass tests

TEST_F (YdspFastMathTests, ReturnsFalseWhenFastMathDisabled)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    auto fn = makeUnaryFunction (YdspIrOp::sinF, YdspValueType::float32Type, 1, false);
    EXPECT_FALSE (optimizer.lowerFastMathTranscendentals (fn));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::sinF));
}

TEST_F (YdspFastMathTests, LeavesFloat64Untouched)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    const YdspIrOp unaryOps[] { YdspIrOp::tanhF, YdspIrOp::sinF, YdspIrOp::cosF, YdspIrOp::tanF, YdspIrOp::atanF,
                                YdspIrOp::expF, YdspIrOp::logF, YdspIrOp::log10F };

    for (const auto op : unaryOps)
    {
        auto fn = makeUnaryFunction (op, YdspValueType::float64Type, 1, true);
        EXPECT_FALSE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (1, countInst (fn, op));
    }

    for (const auto op : { YdspIrOp::powF, YdspIrOp::atan2F })
    {
        auto fn = makeBinaryFunction (op, YdspValueType::float64Type, 1, true);
        EXPECT_FALSE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (1, countInst (fn, op));
    }
}

TEST_F (YdspFastMathTests, LowersScalarFloat32Transcendentals)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    const YdspIrOp unaryOps[] { YdspIrOp::tanhF, YdspIrOp::sinF, YdspIrOp::cosF, YdspIrOp::tanF, YdspIrOp::atanF,
                                YdspIrOp::expF, YdspIrOp::logF, YdspIrOp::log10F };

    for (const auto op : unaryOps)
    {
        auto fn = makeUnaryFunction (op, YdspValueType::float32Type, 1, true);
        EXPECT_TRUE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (0, countTranscendentals (fn));
        EXPECT_TRUE (fn.valueLanes.empty());
        EXPECT_GT (fn.valueTypes.size(), 2u);

        if (op != YdspIrOp::tanhF && op != YdspIrOp::tanF)
        {
            EXPECT_GT (countInst (fn, YdspIrOp::bitcastFtoI) + countInst (fn, YdspIrOp::bitcastItoF), 0);
        }
    }

    for (const auto op : { YdspIrOp::powF, YdspIrOp::atan2F })
    {
        auto fn = makeBinaryFunction (op, YdspValueType::float32Type, 1, true);
        EXPECT_TRUE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (0, countTranscendentals (fn));
        EXPECT_TRUE (fn.valueLanes.empty());
        EXPECT_GT (countInst (fn, YdspIrOp::bitcastFtoI), 0);
        EXPECT_GT (countInst (fn, YdspIrOp::bitcastItoF), 0);
    }
}

TEST_F (YdspFastMathTests, LowersPackedVectorFloat32Transcendentals4Lanes)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    const YdspIrOp unaryOps[] { YdspIrOp::tanhF, YdspIrOp::sinF, YdspIrOp::cosF, YdspIrOp::tanF, YdspIrOp::atanF,
                                YdspIrOp::expF, YdspIrOp::logF, YdspIrOp::log10F };

    for (const auto op : unaryOps)
    {
        auto fn = makeUnaryFunction (op, YdspValueType::float32Type, 4, true);
        EXPECT_TRUE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (0, countTranscendentals (fn));
        EXPECT_GT (countInst (fn, YdspIrOp::vsplat), 0);
        verifyVectorLanesWellFormed (fn, 4);
    }

    for (const auto op : { YdspIrOp::powF, YdspIrOp::atan2F })
    {
        auto fn = makeBinaryFunction (op, YdspValueType::float32Type, 4, true);
        EXPECT_TRUE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (0, countTranscendentals (fn));
        EXPECT_GT (countInst (fn, YdspIrOp::vsplat), 0);
        verifyVectorLanesWellFormed (fn, 4);
    }
}

TEST_F (YdspFastMathTests, LowersPackedVectorFloat32Transcendentals8Lanes)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    const YdspIrOp unaryOps[] { YdspIrOp::tanhF, YdspIrOp::sinF, YdspIrOp::cosF, YdspIrOp::tanF, YdspIrOp::atanF,
                                YdspIrOp::expF, YdspIrOp::logF, YdspIrOp::log10F };

    for (const auto op : unaryOps)
    {
        auto fn = makeUnaryFunction (op, YdspValueType::float32Type, 8, true);
        EXPECT_TRUE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (0, countTranscendentals (fn));
        EXPECT_GT (countInst (fn, YdspIrOp::vsplat), 0);
        verifyVectorLanesWellFormed (fn, 8);
    }

    for (const auto op : { YdspIrOp::powF, YdspIrOp::atan2F })
    {
        auto fn = makeBinaryFunction (op, YdspValueType::float32Type, 8, true);
        EXPECT_TRUE (optimizer.lowerFastMathTranscendentals (fn));
        EXPECT_EQ (0, countTranscendentals (fn));
        EXPECT_GT (countInst (fn, YdspIrOp::vsplat), 0);
        verifyVectorLanesWellFormed (fn, 8);
    }
}

//==============================================================================
// Optimizer pipeline integration

TEST_F (YdspFastMathTests, OptimizerPipelineLowersTranscendentalsOnlyUnderFastMath)
{
    const char* source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float a = sin (in) + cos (in) + tanh (in) + atan (in);
                float b = exp (in * 0.25) + log (abs (in) + 1.0) + log10 (abs (in) + 1.0);
                out = a + b + pow (abs (in) + 1.0, 1.5) + atan2 (in, 1.0);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    {
        YdspDiagnostics diagnostics;
        auto ir = buildIr (source, diagnostics, true);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        ASSERT_FALSE (ir->kernels.empty());
        EXPECT_EQ (0, countTranscendentals (*ir->kernels[0]));
    }

    {
        YdspDiagnostics diagnostics;
        auto ir = buildIr (source, diagnostics, false);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        ASSERT_FALSE (ir->kernels.empty());
        EXPECT_EQ (9, countTranscendentals (*ir->kernels[0]));
    }
}

//==============================================================================
// End-to-end accuracy and parity tests

TEST_F (YdspFastMathTests, SinAndCosMatchLibm)
{
    std::vector<float> input;
    for (int i = -250; i <= 250; ++i)
        input.push_back (static_cast<float> (i) * 0.2f);

    input.push_back (0.0f);
    input.push_back (-0.0f);
    input.push_back (piValue * 0.5f);
    input.push_back (-piValue * 0.5f);
    input.push_back (piValue);
    input.push_back (piValue * 1.5f);

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto sinOut = runUnary ("sin (in)", input, options);
        const auto cosOut = runUnary ("cos (in)", input, options);
        ASSERT_EQ (input.size(), sinOut.size());
        ASSERT_EQ (input.size(), cosOut.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            EXPECT_NEAR (std::sin (input[i]), sinOut[i], 1.0e-5f) << "x=" << input[i];
            EXPECT_NEAR (std::cos (input[i]), cosOut[i], 1.0e-5f) << "x=" << input[i];
        }
    }
}

TEST_F (YdspFastMathTests, TanhMatchesLibmAcrossFullRange)
{
    std::vector<float> input;
    for (int i = -150; i <= 150; ++i)
        input.push_back (static_cast<float> (i) * 0.1f);

    input.push_back (0.0f);
    input.push_back (1.0e-4f);
    input.push_back (-1.0e-4f);
    input.push_back (100.0f);
    input.push_back (-100.0f);

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto out = runUnary ("tanh (in)", input, options);
        ASSERT_EQ (input.size(), out.size());

        for (size_t i = 0; i < input.size(); ++i)
            EXPECT_NEAR (std::tanh (input[i]), out[i], 1.0e-5f) << "x=" << input[i];
    }
}

TEST_F (YdspFastMathTests, ExpMatchesLibmAndHandlesOverflowUnderflow)
{
    std::vector<float> input;
    for (int i = -100; i <= 100; ++i)
        input.push_back (static_cast<float> (i) * 0.2f);

    input.push_back (0.0f);
    input.push_back (-87.3f);
    input.push_back (-100.0f);
    input.push_back (88.5f);
    input.push_back (89.5f);

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto out = runUnary ("exp (in)", input, options);
        ASSERT_EQ (input.size(), out.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            const float x = input[i];

            if (x > 89.0f)
            {
                EXPECT_TRUE (std::isinf (out[i]) && out[i] > 0.0f) << "x=" << x;
                continue;
            }

            if (x < -87.34f)
            {
                EXPECT_EQ (0.0f, out[i]) << "x=" << x;
                continue;
            }

            const float ref = std::exp (x);
            EXPECT_NEAR (ref, out[i], std::max (1.0e-6f, ref * 1.0e-5f)) << "x=" << x;
        }
    }
}

TEST_F (YdspFastMathTests, LogAndLog10MatchLibmAndHandleZeroAndNegative)
{
    std::vector<float> input { 1.0e-6f, 1.0e-4f, 0.01f, 0.25f, 0.5f, 0.70710678f, 1.0f, 1.41421356f,
                               2.0f, 3.5f, 10.0f, 100.0f, 1000.0f, 1.0e6f };

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto logOut = runUnary ("log (in)", input, options);
        const auto log10Out = runUnary ("log10 (in)", input, options);
        ASSERT_EQ (input.size(), logOut.size());
        ASSERT_EQ (input.size(), log10Out.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            EXPECT_NEAR (std::log (input[i]), logOut[i], 1.0e-5f) << "x=" << input[i];
            EXPECT_NEAR (std::log10 (input[i]), log10Out[i], 1.0e-5f) << "x=" << input[i];
        }

        const std::vector<float> edge { 0.0f, -1.0f };
        const auto edgeLog = runUnary ("log (in)", edge, options);
        const auto edgeLog10 = runUnary ("log10 (in)", edge, options);
        ASSERT_EQ (2u, edgeLog.size());
        ASSERT_EQ (2u, edgeLog10.size());
        EXPECT_TRUE (std::isinf (edgeLog[0]) && edgeLog[0] < 0.0f);
        EXPECT_TRUE (std::isnan (edgeLog[1]));
        EXPECT_TRUE (std::isinf (edgeLog10[0]) && edgeLog10[0] < 0.0f);
        EXPECT_TRUE (std::isnan (edgeLog10[1]));
    }
}

TEST_F (YdspFastMathTests, PowMatchesLibmAndHandlesZeroBase)
{
    const std::vector<float> bases { 0.5f, 2.0f, 10.0f, 10.0f, 4.0f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f };
    const std::vector<float> exps  { 2.0f, 3.5f, -2.0f, 1.2f, 0.5f, -0.5f, 7.0f, 2.0f, 0.5f, 0.0f };

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto out = runBinary ("pow (a, b)", bases, exps, options);
        ASSERT_EQ (bases.size(), out.size());

        for (size_t i = 0; i < bases.size(); ++i)
        {
            const float ref = std::pow (bases[i], exps[i]);
            EXPECT_NEAR (ref, out[i], std::max (1.0e-6f, std::abs (ref) * 1.0e-5f))
                << "a=" << bases[i] << ", b=" << exps[i];
        }
    }
}

TEST_F (YdspFastMathTests, AtanAndAtan2MatchLibmAcrossAllQuadrants)
{
    std::vector<float> input;
    for (int i = -100; i <= 100; ++i)
        input.push_back (static_cast<float> (i) * 0.2f);

    const std::vector<float> ys { 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.3f, -2.7f };
    const std::vector<float> xs { 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.5f, 0.4f };

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto atanOut = runUnary ("atan (in)", input, options);
        ASSERT_EQ (input.size(), atanOut.size());

        for (size_t i = 0; i < input.size(); ++i)
            EXPECT_NEAR (std::atan (input[i]), atanOut[i], 1.0e-5f) << "x=" << input[i];

        const auto atan2Out = runBinary ("atan2 (a, b)", ys, xs, options);
        ASSERT_EQ (ys.size(), atan2Out.size());

        for (size_t i = 0; i < ys.size(); ++i)
            EXPECT_NEAR (std::atan2 (ys[i], xs[i]), atan2Out[i], 1.0e-5f)
                << "y=" << ys[i] << ", x=" << xs[i];
    }
}

TEST_F (YdspFastMathTests, TanMatchesLibmAcrossSvfAndQuadrants)
{
    std::vector<float> input;
    for (int i = -140; i <= 140; ++i)
    {
        const float x = static_cast<float> (i) * 0.02f;
        if (std::abs (std::abs (x) - piValue * 0.5f) > 0.05f)
            input.push_back (x);
    }

    for (const auto& options : { scalarFastMathOptions(), hostFastMathOptions() })
    {
        const auto tanOut = runUnary ("tan (in)", input, options);
        ASSERT_EQ (input.size(), tanOut.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            const float ref = std::tan (input[i]);
            EXPECT_NEAR (ref, tanOut[i], std::max (1.0e-6f, std::abs (ref) * 2.0e-6f)) << "x=" << input[i];
        }
    }
}

TEST_F (YdspFastMathTests, VectorizedBankLoopMatchesStrictLibm)
{
    const char* source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];

            process {
                float sum = 0.0;

                for i in 0..8 {
                    float phase = in + float (i) * 0.125;
                    z[i] = sin (phase) * 0.5 + cos (phase) * 0.25 + tanh (phase) * 0.25;
                    sum = sum + z[i];
                }

                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    std::vector<float> input (64);
    for (size_t i = 0; i < input.size(); ++i)
        input[i] = -4.0f + static_cast<float> (i) * 0.125f;

    const auto runPatch = [&] (const YdspCompileOptions& options)
    {
        YdspCompiler compiler;
        auto result = compiler.compile (source, options);
        EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();
        auto graph = std::move (result).getValue();
        graph.prepare (48000.0, static_cast<int> (input.size()));
        std::vector<float> output (input.size(), 0.0f);
        std::vector<YdspInputBuffer> inputs { YdspInputBuffer (Span<const float> (input.data(), input.size())) };
        std::vector<YdspOutputBuffer> outputs { YdspOutputBuffer (Span<float> (output.data(), output.size())) };
        graph.process (YdspProcessRequest { inputs, outputs, static_cast<int> (input.size()) });
        return output;
    };

    const auto fastOut = runPatch (hostFastMathOptions());
    const auto strictOut = runPatch (hostStrictOptions());
    ASSERT_EQ (strictOut.size(), fastOut.size());

    for (size_t i = 0; i < fastOut.size(); ++i)
        EXPECT_NEAR (strictOut[i], fastOut[i], 2.0e-5f) << "sample=" << i;
}

TEST_F (YdspFastMathTests, LowerStrictVectorTranscendentalsPreservesOneUlpOnTanhAndBoundedSin)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setTargetHasVectorMath (true);

    // 1. Widened strict tanh lowers unconditionally (<= 1 ULP across all float32).
    {
        auto fn = makeUnaryFunction (YdspIrOp::tanhF, YdspValueType::float32Type, 4, false);
        fn.vectorMathEnabled = true;
        EXPECT_TRUE (optimizer.lowerStrictVectorTranscendentals (fn));
        EXPECT_EQ (0, countInst (fn, YdspIrOp::tanhF));
        verifyVectorLanesWellFormed (fn, 4);
    }

    const auto buildNativeStrict = [&] (StringRef source) -> std::unique_ptr<YdspIrProgram>
    {
        YdspLexer lexer (source, diagnostics);
        auto program = YdspParser (lexer.tokenize(), diagnostics).parseProgram();
        if (program == nullptr)
            return nullptr;
        auto analyzed = YdspSemanticAnalyzer (diagnostics).analyze (std::move (program));
        if (analyzed == nullptr)
            return nullptr;
        YdspOptimizer opt (diagnostics);
        opt.setFastMath (false);
        opt.setVectorizationEnabled (true);
        opt.setVectorWidth (4);
        opt.setTargetHasVectorMath (true);
        opt.setTargetHasVectorRamp (true);
        opt.setUnrollingEnabled (true);
        return opt.build (*analyzed);
    };

    // 2. Widened strict sin lowers inline when input is bounded via x - floor(x),
    //    and leaves unbounded inputs on Sleef u10.
    {
        const auto boundedSource = R"YDSP(
            processor P {
                output stream out;
                state float phase[8];
                process {
                    float acc = 0.0;
                    for k in 0..8 {
                        float p = phase[k] + 0.01 * float (k + 1);
                        p = p - floor (p);
                        phase[k] = p;
                        acc = acc + sin (2.0 * pi * p);
                    }
                    out = acc;
                }
            }
            graph G { output stream y; node p = P; connection { p.out -> y; } }
        )YDSP";

        auto ir = buildNativeStrict (boundedSource);
        ASSERT_NE (nullptr, ir) << diagnostics.toString();
        EXPECT_TRUE (ir->kernels[0]->vectorized);
        EXPECT_EQ (0, countInst (*ir->kernels[0], YdspIrOp::sinF));

        const auto unboundedSource = R"YDSP(
            processor P {
                input stream in;
                output stream out;
                state float z[8];
                process {
                    float acc = 0.0;
                    for k in 0..8 {
                        z[k] = sin (in * 200.0 + float (k));
                        acc = acc + z[k];
                    }
                    out = acc;
                }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP";

        auto unboundedIr = buildNativeStrict (unboundedSource);
        ASSERT_NE (nullptr, unboundedIr) << diagnostics.toString();
        EXPECT_GT (countInst (*unboundedIr->kernels[0], YdspIrOp::sinF), 0);
    }

    // 3. Verify strict tanh runtime accuracy (<= 1 ULP) against std::tanh across [-12, 12] and -0.0/NaN.
    const auto tanhSource = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = tanh (in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspCompiler compiler;
    auto strictResult = compiler.compile (tanhSource, hostStrictOptions());
    ASSERT_TRUE (strictResult.wasOk()) << compiler.getDiagnostics().toString();
    auto graph = std::move (strictResult).getValue();

    std::vector<float> input;
    for (int i = -1200; i <= 1200; ++i)
        input.push_back (static_cast<float> (i) * 0.01f);
    input.push_back (-0.0f);
    input.push_back (std::numeric_limits<float>::quiet_NaN());

    ASSERT_TRUE (graph.prepare (48000.0, static_cast<int> (input.size())).wasOk());
    std::vector<float> output (input.size(), 0.0f);
    YdspInputBuffer inBuf[] { Span<const float> (input) };
    YdspOutputBuffer outBuf[] { Span<float> (output) };
    ASSERT_EQ (YdspProcessResult::ok, graph.process ({ inBuf, outBuf, static_cast<int> (input.size()) }));

    for (size_t i = 0; i < input.size(); ++i)
    {
        if (std::isnan (input[i]))
        {
            EXPECT_TRUE (std::isnan (output[i]));
            continue;
        }
        if (input[i] == 0.0f)
        {
            EXPECT_EQ (0.0f, output[i]);
            EXPECT_EQ (std::signbit (input[i]), std::signbit (output[i]));
            continue;
        }

        const float ref = std::tanh (input[i]);
        auto toOrdered = [] (float v) -> int32_t
        {
            const auto bits = std::bit_cast<int32_t> (v);
            return bits < 0 ? (std::numeric_limits<int32_t>::min() - bits) : bits;
        };
        const int32_t ulp = std::abs (toOrdered (output[i]) - toOrdered (ref));
        EXPECT_LE (ulp, 1) << "x=" << input[i] << " got=" << output[i] << " ref=" << ref;
    }
}

} // namespace yup::test
