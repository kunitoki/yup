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
#include <memory>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================

String loopDistPatch (StringRef processorBody)
{
    return String (processorBody)
         + "\ngraph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";
}

std::unique_ptr<YdspIrProgram> loopDistBuildIr (StringRef source,
                                                YdspDiagnostics& diagnostics,
                                                bool vectorMath = true,
                                                bool fastMath = true)
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
    optimizer.setContractionEnabled (fastMath);
    optimizer.setTargetHasVectorMath (vectorMath);
    optimizer.setVectorizationEnabled (true);
    optimizer.setVectorWidth (4);
    optimizer.setUnrollingEnabled (true);

    return optimizer.build (*analyzed);
}

int loopDistCountWidenedOp (const YdspIrFunction& fn, YdspIrOp op)
{
    int count = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == op && inst.result >= 0 && fn.laneCountOf (inst.result) > 1)
                ++count;

    return count;
}

YdspAudioGraph loopDistCompileGraph (StringRef source, YdspCompiler& compiler, const YdspCompileOptions& options = {})
{
    auto result = compiler.compile (source, options);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

void loopDistRunBlock (YdspAudioGraph& graph, const float* input, float* output, int numSamples)
{
    std::vector<YdspInputBuffer> inputs;
    inputs.emplace_back (Span<const float> (input, static_cast<size_t> (numSamples)));

    std::vector<YdspOutputBuffer> outputs;
    outputs.emplace_back (Span<float> (output, static_cast<size_t> (numSamples)));

    ASSERT_EQ (YdspProcessResult::ok, graph.process (YdspProcessRequest { inputs, outputs, numSamples }));
}

std::vector<float> loopDistMakeSignal (int numSamples)
{
    std::vector<float> data (static_cast<size_t> (numSamples));

    for (int i = 0; i < numSamples; ++i)
        data[static_cast<size_t> (i)] = 0.6f * std::sin (0.13f * static_cast<float> (i))
                                       + 0.3f * std::cos (0.37f * static_cast<float> (i));

    return data;
}

constexpr auto loopDistDistortionSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;

        input parameter float drive = 1.8;
        input parameter float tone = 0.5;
        input parameter float mix = 0.7;

        state float z1;

        process {
            float shaped = tanh (in * drive);

            float k = 0.05 + 0.95 * tone;
            z1 = z1 + k * (shaped - z1);

            out = in + mix * (z1 - in);
        }
    }
)YDSP";

constexpr auto loopDistCompressorSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;

        input parameter float threshold = -18.0;
        input parameter float ratio = 4.0;
        input parameter float attack = 0.005;
        input parameter float release = 0.15;
        input parameter float makeup = 1.0;

        state float env;

        process {
            float attackCoeff = 1.0 - exp (-1.0 / (attack * sampleRate));
            float releaseCoeff = 1.0 - exp (-1.0 / (release * sampleRate));

            float level = abs (in);
            float coeff = level > env ? attackCoeff : releaseCoeff;
            env = env + coeff * (level - env);

            float envDb = 20.0 * log10 (env + 1e-6);
            float gainDb = 0.0;
            if (envDb > threshold) { gainDb = (threshold - envDb) * (1.0 - 1.0 / ratio); }

            float gain = pow (10.0, gainDb / 20.0);
            out = in * gain * makeup;
        }
    }
)YDSP";

} // namespace

//==============================================================================

TEST (YdspLoopDistributionTests, SkipsTrivialStatefulLoopsWithoutHeavyStatelessWork)
{
    const char* trivialSources[] = {
        R"YDSP(
            processor P {
                input stream in;
                output stream out;
                state float z1;
                process {
                    z1 = z1 * 0.9 + in * 0.1;
                    out = z1;
                }
            }
        )YDSP",
        R"YDSP(
            processor P {
                input stream in;
                output stream out;
                state float s1, s2, s3, s4;
                process {
                    let x = in - 0.5 * s4;
                    s1 = s1 + 0.25 * (x - s1);
                    s2 = s2 + 0.25 * (s1 - s2);
                    s3 = s3 + 0.25 * (s2 - s3);
                    s4 = s4 + 0.25 * (s3 - s4);
                    out = s4;
                }
            }
        )YDSP"
    };

    for (const auto* body : trivialSources)
    {
        YdspDiagnostics diagnostics;
        auto ir = loopDistBuildIr (loopDistPatch (body), diagnostics);

        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        ASSERT_FALSE (ir->kernels.empty());

        const auto& fn = *ir->kernels[0];

        EXPECT_FALSE (fn.vectorized);
        EXPECT_EQ (1u, fn.loops.size());
        EXPECT_EQ (0, fn.float32ArrayElements);
    }
}

TEST (YdspLoopDistributionTests, SkipsPureStatelessLoopSoVectorizerWidensDirectly)
{
    const auto source = loopDistPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                out = tanh (in * 3.0);
            }
        }
    )YDSP");

    {
        YdspDiagnostics scalarDiag;
        YdspLexer lexer (source, scalarDiag);
        auto tokens = lexer.tokenize();
        YdspParser parser (std::move (tokens), scalarDiag);
        auto program = parser.parseProgram();
        ASSERT_NE (nullptr, program);

        YdspSemanticAnalyzer analyzer (scalarDiag);
        auto analyzed = analyzer.analyze (std::move (program));
        ASSERT_NE (nullptr, analyzed);

        YdspOptimizer scalarOpt (scalarDiag);
        scalarOpt.setFastMath (true);
        scalarOpt.setTargetHasVectorMath (true);
        scalarOpt.setVectorizationEnabled (false);

        auto scalarIr = scalarOpt.build (*analyzed);
        ASSERT_NE (nullptr, scalarIr);
        ASSERT_FALSE (scalarIr->kernels.empty());
        EXPECT_FALSE (scalarOpt.distributeSampleLoop (*scalarIr->kernels[0]));
    }

    YdspDiagnostics diagnostics;
    auto ir = loopDistBuildIr (source, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (0, fn.float32ArrayElements);
    EXPECT_EQ (0, loopDistCountWidenedOp (fn, YdspIrOp::tanhF));
    EXPECT_GT (loopDistCountWidenedOp (fn, YdspIrOp::divF), 0);
}

TEST (YdspLoopDistributionTests, SkipsDistributionWhenBoundaryExceedsTwoScalars)
{
    YdspDiagnostics diagnostics;
    auto ir = loopDistBuildIr (loopDistPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z1, z2, z3;
            process {
                let a = tanh (in * 1.5);
                let b = tanh (in * 2.5);
                let c = tanh (in * 3.5);
                z1 = z1 * 0.8 + a * 0.05;
                z2 = z2 * 0.7 + b * 0.05;
                z3 = z3 * 0.6 + c * 0.05;
                out = z1 + z2 + z3;
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_FALSE (fn.vectorized);
    EXPECT_EQ (1u, fn.loops.size());
    EXPECT_EQ (0, fn.float32ArrayElements);
}

TEST (YdspLoopDistributionTests, DistributesAndVectorizesDistortionAcrossBlockSizes)
{
    YdspDiagnostics diagnostics;
    auto ir = loopDistBuildIr (loopDistPatch (loopDistDistortionSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_GE (fn.float32ArrayElements, 64);
    EXPECT_EQ (0, loopDistCountWidenedOp (fn, YdspIrOp::tanhF));
    EXPECT_GT (loopDistCountWidenedOp (fn, YdspIrOp::divF), 0);

    const auto source = loopDistPatch (loopDistDistortionSource);

    for (const int blockSize : { 1, 7, 63, 64, 65, 129, 512 })
    {
        SCOPED_TRACE (blockSize);

        YdspCompiler compiler;
        auto graph = loopDistCompileGraph (source, compiler);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, blockSize).wasOk());

#if ! YUP_WASM
        const auto& kernels = graph.getExecutionReport().getKernels();
        ASSERT_FALSE (kernels.empty());
        EXPECT_TRUE (kernels[0].vectorized);
#endif

        const int totalSamples = blockSize * 3;
        const auto input = loopDistMakeSignal (totalSamples);
        std::vector<float> actual (static_cast<size_t> (totalSamples), 0.0f);
        std::vector<float> expected (static_cast<size_t> (totalSamples), 0.0f);

        float z1 = 0.0f;
        const float k = 0.05f + 0.95f * 0.5f;

        for (int i = 0; i < totalSamples; ++i)
        {
            const float x = input[static_cast<size_t> (i)];
            const float shaped = std::tanh (x * 1.8f);
            z1 = z1 + k * (shaped - z1);
            expected[static_cast<size_t> (i)] = x + 0.7f * (z1 - x);
        }

        for (int b = 0; b < 3; ++b)
        {
            const auto offset = static_cast<size_t> (b * blockSize);
            loopDistRunBlock (graph, input.data() + offset, actual.data() + offset, blockSize);
        }

        for (int i = 0; i < totalSamples; ++i)
            EXPECT_NEAR (expected[static_cast<size_t> (i)], actual[static_cast<size_t> (i)], 1.0e-5f)
                << "sample " << i;
    }
}

TEST (YdspLoopDistributionTests, DistributesAndVectorizesCompressorAcrossBlockSizes)
{
    YdspDiagnostics diagnostics;
    auto ir = loopDistBuildIr (loopDistPatch (loopDistCompressorSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_GE (fn.float32ArrayElements, 64);
    EXPECT_EQ (0, loopDistCountWidenedOp (fn, YdspIrOp::log10F));
    EXPECT_EQ (0, loopDistCountWidenedOp (fn, YdspIrOp::powF));
    EXPECT_GT (loopDistCountWidenedOp (fn, YdspIrOp::divF), 0);

    const auto source = loopDistPatch (loopDistCompressorSource);

    for (const int blockSize : { 1, 7, 63, 64, 65, 129, 512 })
    {
        SCOPED_TRACE (blockSize);

        YdspCompiler compiler;
        auto graph = loopDistCompileGraph (source, compiler);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, blockSize).wasOk());

#if ! YUP_WASM
        const auto& kernels = graph.getExecutionReport().getKernels();
        ASSERT_FALSE (kernels.empty());
        EXPECT_TRUE (kernels[0].vectorized);
#endif

        const int totalSamples = blockSize * 3;
        const auto input = loopDistMakeSignal (totalSamples);
        std::vector<float> actual (static_cast<size_t> (totalSamples), 0.0f);
        std::vector<float> expected (static_cast<size_t> (totalSamples), 0.0f);

        float env = 0.0f;
        const float attackCoeff = 1.0f - std::exp (-1.0f / (0.005f * 48000.0f));
        const float releaseCoeff = 1.0f - std::exp (-1.0f / (0.15f * 48000.0f));

        for (int i = 0; i < totalSamples; ++i)
        {
            const float x = input[static_cast<size_t> (i)];
            const float level = std::fabs (x);
            const float coeff = level > env ? attackCoeff : releaseCoeff;
            env = env + coeff * (level - env);

            const float envDb = 20.0f * std::log10 (env + 1.0e-6f);
            float gainDb = 0.0f;
            if (envDb > -18.0f)
                gainDb = (-18.0f - envDb) * (1.0f - 1.0f / 4.0f);

            const float gain = std::pow (10.0f, gainDb / 20.0f);
            expected[static_cast<size_t> (i)] = x * gain;
        }

        for (int b = 0; b < 3; ++b)
        {
            const auto offset = static_cast<size_t> (b * blockSize);
            loopDistRunBlock (graph, input.data() + offset, actual.data() + offset, blockSize);
        }

        for (int i = 0; i < totalSamples; ++i)
            EXPECT_NEAR (expected[static_cast<size_t> (i)], actual[static_cast<size_t> (i)], 1.0e-4f)
                << "sample " << i;
    }
}

TEST (YdspLoopDistributionTests, DistributesArithmeticHeavyStatelessStageWithoutVectorMath)
{
    const auto source = loopDistPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            state float z1;

            process {
                let x = in * 1.5;
                let shaped = clamp (x - x * x * x * 0.3333333, -1.0, 1.0);
                z1 = z1 * 0.9 + shaped * 0.1;
                out = z1;
            }
        }
    )YDSP");

    YdspDiagnostics diagnostics;
    auto ir = loopDistBuildIr (source, diagnostics, false, true);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_GE (fn.float32ArrayElements, 64);
    EXPECT_GT (loopDistCountWidenedOp (fn, YdspIrOp::clampF), 0);

    for (const int blockSize : { 1, 7, 63, 64, 65, 129, 512 })
    {
        SCOPED_TRACE (blockSize);

        YdspCompiler compiler;
        auto graph = loopDistCompileGraph (source, compiler);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, blockSize).wasOk());

        const auto input = loopDistMakeSignal (blockSize);
        std::vector<float> actual (static_cast<size_t> (blockSize), 0.0f);
        std::vector<float> expected (static_cast<size_t> (blockSize), 0.0f);

        float z1 = 0.0f;

        for (int i = 0; i < blockSize; ++i)
        {
            const float x = input[static_cast<size_t> (i)] * 1.5f;
            const float poly = x - x * x * x * 0.3333333f;
            const float shaped = std::min (std::max (poly, -1.0f), 1.0f);
            z1 = z1 * 0.9f + shaped * 0.1f;
            expected[static_cast<size_t> (i)] = z1;
        }

        loopDistRunBlock (graph, input.data(), actual.data(), blockSize);

        for (int i = 0; i < blockSize; ++i)
            EXPECT_NEAR (expected[static_cast<size_t> (i)], actual[static_cast<size_t> (i)], 1.0e-5f)
                << "sample " << i;
    }
}

TEST (YdspLoopDistributionTests, DistributesBothPreAndPostStatefulStages)
{
    const auto source = loopDistPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            state float z1;

            process {
                let pre = tanh (in * 2.0);
                z1 = z1 * 0.8 + pre * 0.2;
                out = tanh (z1 * 1.5);
            }
        }
    )YDSP");

    YdspDiagnostics diagnostics;
    auto ir = loopDistBuildIr (source, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_GE (fn.float32ArrayElements, 128);
    EXPECT_EQ (0, loopDistCountWidenedOp (fn, YdspIrOp::tanhF));
    EXPECT_GE (loopDistCountWidenedOp (fn, YdspIrOp::divF), 2);

    for (const int blockSize : { 1, 7, 63, 64, 65, 129, 512 })
    {
        SCOPED_TRACE (blockSize);

        YdspCompiler compiler;
        auto graph = loopDistCompileGraph (source, compiler);
        ASSERT_TRUE (graph.isValid());
        ASSERT_TRUE (graph.prepare (48000.0, blockSize).wasOk());

        const auto input = loopDistMakeSignal (blockSize);
        std::vector<float> actual (static_cast<size_t> (blockSize), 0.0f);
        std::vector<float> expected (static_cast<size_t> (blockSize), 0.0f);

        float z1 = 0.0f;

        for (int i = 0; i < blockSize; ++i)
        {
            const float pre = std::tanh (input[static_cast<size_t> (i)] * 2.0f);
            z1 = z1 * 0.8f + pre * 0.2f;
            expected[static_cast<size_t> (i)] = std::tanh (z1 * 1.5f);
        }

        loopDistRunBlock (graph, input.data(), actual.data(), blockSize);

        for (int i = 0; i < blockSize; ++i)
            EXPECT_NEAR (expected[static_cast<size_t> (i)], actual[static_cast<size_t> (i)], 1.0e-5f)
                << "sample " << i;
    }
}

TEST (YdspLoopDistributionTests, OverlappingHostBuffersMatchScalarOrder)
{
    constexpr int numSamples = 131;
    constexpr int padding = 16;

    for (const auto* body : { loopDistDistortionSource, loopDistCompressorSource })
    {
        const auto source = loopDistPatch (body);

        for (const int offset : { 0, 1, 3, 16, -1, -5 })
        {
            SCOPED_TRACE (offset);

            YdspCompileOptions baselineOptions;
            baselineOptions.optimizationTier = YdspOptimizationTier::baseline;

            YdspCompiler compiler;
            auto scalarGraph = loopDistCompileGraph (source, compiler, baselineOptions);
            auto optGraph = loopDistCompileGraph (source, compiler);

            ASSERT_TRUE (scalarGraph.isValid());
            ASSERT_TRUE (optGraph.isValid());
            ASSERT_TRUE (scalarGraph.prepare (48000.0, 256).wasOk());
            ASSERT_TRUE (optGraph.prepare (48000.0, 256).wasOk());

            std::vector<float> expectedStorage (static_cast<size_t> (numSamples + 2 * padding));

            for (size_t i = 0; i < expectedStorage.size(); ++i)
                expectedStorage[i] = 0.3f * std::sin (0.17f * static_cast<float> (i));

            auto actualStorage = expectedStorage;

            loopDistRunBlock (scalarGraph,
                              expectedStorage.data() + padding,
                              expectedStorage.data() + padding + offset,
                              numSamples);

            loopDistRunBlock (optGraph,
                              actualStorage.data() + padding,
                              actualStorage.data() + padding + offset,
                              numSamples);

            for (size_t i = 0; i < expectedStorage.size(); ++i)
                EXPECT_NEAR (expectedStorage[i], actualStorage[i], 1.0e-4f) << "index " << i;
        }
    }
}

TEST (YdspLoopDistributionTests, DistributesThreeStageModulatedFilterAndMatchesReference)
{
    const auto source = loopDistPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float base = 800.0;
            input parameter float depth = 2.0;
            input parameter float rate = 0.5;
            input parameter float q = 2.0;

            state float ph, ic1, ic2;

            process {
                let k = 1.0 / q;
                ph = ph + rate / sampleRate;
                ph = ph - floor (ph);
                let lfo = sin (2.0 * pi * ph);
                let fc = base * pow (2.0, depth * lfo);
                let g = tan (pi * fc / sampleRate);
                let a1 = 1.0 / (1.0 + g * (g + k));
                let a2 = g * a1;
                let a3 = g * a2;
                let v3 = in - ic2;
                let v1 = a1 * ic1 + a2 * v3;
                let v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0 * v1 - ic1;
                ic2 = 2.0 * v2 - ic2;
                out = v2;
            }
        }
    )YDSP");

    for (const bool fastMath : { false, true })
    {
        SCOPED_TRACE (fastMath);
        YdspDiagnostics diagnostics;
        auto ir = loopDistBuildIr (source, diagnostics, true, fastMath);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        ASSERT_FALSE (ir->kernels.empty());

        const auto& fn = *ir->kernels[0];
        EXPECT_TRUE (fn.vectorized);
        EXPECT_GE (fn.float32ArrayElements, 128);
        if (fastMath)
            EXPECT_GT (loopDistCountWidenedOp (fn, YdspIrOp::divF), 0);
        else
            EXPECT_GT (loopDistCountWidenedOp (fn, YdspIrOp::tanF), 0);
    }

    for (const int blockSize : { 1, 7, 63, 64, 65, 129, 512 })
    {
        SCOPED_TRACE (blockSize);
        YdspCompileOptions baselineOptions;
        baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
        baselineOptions.fastMath = false;

        YdspCompiler compiler;
        auto refGraph = loopDistCompileGraph (source, compiler, baselineOptions);
        auto optGraph = loopDistCompileGraph (source, compiler);
        ASSERT_TRUE (refGraph.isValid());
        ASSERT_TRUE (optGraph.isValid());
        ASSERT_TRUE (refGraph.prepare (48000.0, blockSize).wasOk());
        ASSERT_TRUE (optGraph.prepare (48000.0, blockSize).wasOk());

        const int totalSamples = blockSize * 3;
        const auto input = loopDistMakeSignal (totalSamples);
        std::vector<float> expected (static_cast<size_t> (totalSamples), 0.0f);
        std::vector<float> actual (static_cast<size_t> (totalSamples), 0.0f);

        for (int b = 0; b < 3; ++b)
        {
            const auto offset = static_cast<size_t> (b * blockSize);
            loopDistRunBlock (refGraph, input.data() + offset, expected.data() + offset, blockSize);
            loopDistRunBlock (optGraph, input.data() + offset, actual.data() + offset, blockSize);
        }

        for (int i = 0; i < totalSamples; ++i)
            EXPECT_NEAR (expected[static_cast<size_t> (i)], actual[static_cast<size_t> (i)], 1.0e-5f) << "sample=" << i;
    }
}

TEST (YdspLoopDistributionTests, DistributesFourAccumulatorFmPadWhenStatelessHasHeavyTranscendentals)
{
    const auto source = R"YDSP(
        processor P {
            output stream out;
            input parameter float freq = 220.0;
            input parameter float ratio = 1.5;
            input parameter float index = 3.0;
            input parameter float drive = 2.0;
            input parameter float detune = 1.003;
            state float pc1, pm1, pc2, pm2;
            process {
                pc1 = pc1 + freq / sampleRate;
                pc1 = pc1 - floor (pc1);
                pm1 = pm1 + freq * ratio / sampleRate;
                pm1 = pm1 - floor (pm1);
                pc2 = pc2 + freq * detune / sampleRate;
                pc2 = pc2 - floor (pc2);
                pm2 = pm2 + freq * detune * ratio / sampleRate;
                pm2 = pm2 - floor (pm2);
                let v1 = tanh (drive * sin (2.0 * pi * pc1 + index * sin (2.0 * pi * pm1)));
                let v2 = tanh (drive * sin (2.0 * pi * pc2 + index * sin (2.0 * pi * pm2)));
                out = (v1 + v2) * 0.25;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP";

    for (const bool fastMath : { false, true })
    {
        SCOPED_TRACE (fastMath);
        YdspDiagnostics diagnostics;
        auto ir = loopDistBuildIr (source, diagnostics, true, fastMath);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        ASSERT_FALSE (ir->kernels.empty());

        const auto& fn = *ir->kernels[0];
        EXPECT_TRUE (fn.vectorized);
        EXPECT_GE (fn.float32ArrayElements, 256);
    }

    for (const int blockSize : { 1, 7, 64, 65, 256 })
    {
        SCOPED_TRACE (blockSize);
        YdspCompileOptions baselineOptions;
        baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
        baselineOptions.fastMath = false;

        YdspCompiler compiler;
        auto refGraph = loopDistCompileGraph (source, compiler, baselineOptions);
        auto optGraph = loopDistCompileGraph (source, compiler);
        ASSERT_TRUE (refGraph.isValid());
        ASSERT_TRUE (optGraph.isValid());
        ASSERT_TRUE (refGraph.prepare (48000.0, blockSize).wasOk());
        ASSERT_TRUE (optGraph.prepare (48000.0, blockSize).wasOk());

        std::vector<float> expected (static_cast<size_t> (blockSize), 0.0f);
        std::vector<float> actual (static_cast<size_t> (blockSize), 0.0f);
        YdspOutputBuffer refOut[] { Span<float> (expected) };
        YdspOutputBuffer optOut[] { Span<float> (actual) };
        ASSERT_EQ (YdspProcessResult::ok, refGraph.process ({ {}, refOut, blockSize }));
        ASSERT_EQ (YdspProcessResult::ok, optGraph.process ({ {}, optOut, blockSize }));

        for (int i = 0; i < blockSize; ++i)
            EXPECT_NEAR (expected[static_cast<size_t> (i)], actual[static_cast<size_t> (i)], 1.0e-4f) << "sample=" << i;
    }
}
