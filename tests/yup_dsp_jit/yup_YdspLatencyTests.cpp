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

#include <cmath>
#include <vector>

namespace yup::test
{

namespace
{

constexpr int latencyOversamplerDelay = 16;

constexpr const char* latencyProcessors =
    "processor Ident { input stream in; output stream out; process { out = in; } }\n"
    "processor Diff { input stream a; input stream b; output stream out; process { out = a - b; } }\n";

DspJitGraph latencyCompile (StringRef body, DspJitCompiler& compiler)
{
    auto result = compiler.compile (String (latencyProcessors) + body);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

String latencyCompileError (StringRef body)
{
    DspJitCompiler compiler;
    auto result = compiler.compile (String (latencyProcessors) + body);

    EXPECT_FALSE (result.wasOk());

    return compiler.getDiagnostics().toString();
}

std::vector<float> latencySine (int size, double frequency, double sampleRate)
{
    std::vector<float> data (static_cast<size_t> (size));

    for (int i = 0; i < size; ++i)
        data[static_cast<size_t> (i)] = static_cast<float> (std::sin (2.0 * 3.14159265358979323846 * frequency * static_cast<double> (i) / sampleRate));

    return data;
}

double latencyRms (const float* data, int count)
{
    double sum = 0.0;

    for (int i = 0; i < count; ++i)
        sum += static_cast<double> (data[i]) * static_cast<double> (data[i]);

    return std::sqrt (sum / static_cast<double> (count));
}

std::vector<std::vector<float>> latencyRunBlocks (DspJitGraph& graph,
                                                  const std::vector<float>& input,
                                                  int blockSize,
                                                  int blockCount,
                                                  int numOutputs)
{
    std::vector<std::vector<float>> outputs (static_cast<size_t> (numOutputs),
                                             std::vector<float> (static_cast<size_t> (blockSize), 0.0f));

    for (int block = 0; block < blockCount; ++block)
    {
        const auto offset = static_cast<size_t> (block * blockSize);

        std::vector<DspJitInputBuffer> inputs {
            DspJitInputBuffer (Span<const float> (input.data() + offset, static_cast<size_t> (blockSize)))
        };

        std::vector<DspJitOutputBuffer> outputBuffers;
        for (auto& channel : outputs)
            outputBuffers.emplace_back (Span<float> (channel.data(), channel.size()));

        graph.process (inputs, outputBuffers, blockSize);
    }

    return outputs;
}

std::vector<std::vector<float>> latencyRunOnce (DspJitGraph& graph, const std::vector<float>& input, int numOutputs)
{
    return latencyRunBlocks (graph, input, static_cast<int> (input.size()), 1, numOutputs);
}

bool latencyGraphHasFusedKernel (const DspJitGraph& graph)
{
    for (const auto& kernel : graph.getExecutionReport().getKernels())
        if (kernel.name.startsWith ("fused("))
            return true;

    return false;
}

} // namespace

//==============================================================================

TEST (YdspLatencyTests, CancelsAnOversampledBranchAgainstADryOne)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node os = Ident * 4;
            node mix = Diff;
            connection { x -> mix.a; x -> os.in; os.out -> mix.b; mix.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (latencyOversamplerDelay, graph.getLatencySamples());

    constexpr int blockSize = 256;
    constexpr int blockCount = 8;
    constexpr double sampleRate = 48000.0;

    graph.prepare (sampleRate, blockSize);

    const auto input = latencySine (blockSize * blockCount, 200.0, sampleRate);
    const auto out = latencyRunBlocks (graph, input, blockSize, blockCount, 1);

    const auto residual = latencyRms (out[0].data(), blockSize);
    const auto reference = latencyRms (input.data() + blockSize * (blockCount - 1), blockSize);

    ASSERT_GT (reference, 0.5); // the test would pass vacuously on silence
    EXPECT_LT (residual, reference * 0.05) << "residual RMS " << residual << " against input RMS " << reference;
}

TEST (YdspLatencyTests, AlignsAnImpulseAcrossTwoGraphOutputs)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream left;
            output stream right;
            node os = Ident * 4;
            connection { x -> left; x -> os.in; os.out -> right; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (latencyOversamplerDelay, graph.getLatencySamples());

    constexpr int blockSize = 64;
    graph.prepare (48000.0, blockSize);

    std::vector<float> impulse (static_cast<size_t> (blockSize), 0.0f);
    impulse[0] = 1.0f;

    const auto out = latencyRunOnce (graph, impulse, 2);

    const auto peakIndex = [] (const std::vector<float>& data)
    {
        int best = 0;

        for (size_t i = 1; i < data.size(); ++i)
            if (std::fabs (data[i]) > std::fabs (data[static_cast<size_t> (best)]))
                best = static_cast<int> (i);

        return best;
    };

    EXPECT_EQ (latencyOversamplerDelay, peakIndex (out[0]));
    EXPECT_EQ (latencyOversamplerDelay, peakIndex (out[1]));
}

//==============================================================================

TEST (YdspLatencyTests, PreservesAHaasSkewAndReportsNoLatency)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream left;
            output stream right;
            node g = Ident;
            connection { x -> g.in; g.out -> left; g.out -> [400] -> right; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (0, graph.getLatencySamples());

    constexpr int blockSize = 512;
    graph.prepare (48000.0, blockSize);

    std::vector<float> ramp (static_cast<size_t> (blockSize));
    for (int i = 0; i < blockSize; ++i)
        ramp[static_cast<size_t> (i)] = static_cast<float> (i + 1) * 0.001f;

    const auto out = latencyRunOnce (graph, ramp, 2);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (ramp[static_cast<size_t> (i)], out[0][static_cast<size_t> (i)], 1e-6f) << "left sample " << i;

        const auto expected = i >= 400 ? ramp[static_cast<size_t> (i - 400)] : 0.0f;
        EXPECT_NEAR (expected, out[1][static_cast<size_t> (i)], 1e-6f) << "right sample " << i;
    }
}

TEST (YdspLatencyTests, ADelayEffectReportsNoLatency)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        processor Echo {
            input stream in;
            output stream out;
            process { out = in + 0.5 * (in @ 400); }
        }
        graph G {
            input stream x;
            output stream y;
            node e = Echo;
            connection { x -> e.in; e.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (0, graph.getLatencySamples());
}

TEST (YdspLatencyTests, APlainChainReportsNoLatency)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Ident;
            node b = Ident;
            connection { x -> a.in; a.out -> b.in; b.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (0, graph.getLatencySamples());
}

//==============================================================================

TEST (YdspLatencyTests, ReportsADeclaredProcessorLatency)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        processor Slow [[ latency: 64 ]] {
            input stream in;
            output stream out;
            process { out = in; }
        }
        graph G {
            input stream x;
            output stream y;
            node s = Slow;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (64, graph.getLatencySamples());
}

TEST (YdspLatencyTests, DividesADeclaredLatencyByTheOversamplingFactor)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        processor Slow [[ latency: 64 ]] {
            input stream in;
            output stream out;
            process { out = in; }
        }
        graph G {
            input stream x;
            output stream y;
            node s = Slow * 4;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (latencyOversamplerDelay + 64 / 4, graph.getLatencySamples());
}

TEST (YdspLatencyTests, RejectsADeclaredLatencyThatTheFactorDoesNotDivide)
{
    const auto errors = latencyCompileError (R"YDSP(
        processor Slow [[ latency: 65 ]] {
            input stream in;
            output stream out;
            process { out = in; }
        }
        graph G {
            input stream x;
            output stream y;
            node s = Slow * 4;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (errors.contains ("[[ latency: 65 ]]"));
    EXPECT_TRUE (errors.contains ("not divisible by this instance's oversampling factor of 4"));
}

TEST (YdspLatencyTests, RejectsAnUnknownProcessorAnnotation)
{
    const auto errors = latencyCompileError (R"YDSP(
        processor Slow [[ lateness: 64 ]] {
            input stream in;
            output stream out;
            process { out = in; }
        }
        graph G {
            input stream x;
            output stream y;
            node s = Slow;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (errors.contains ("Unknown processor annotation 'lateness'"));
}

TEST (YdspLatencyTests, RejectsANonIntegerDeclaredLatency)
{
    const auto errors = latencyCompileError (R"YDSP(
        processor Slow [[ latency: "soon" ]] {
            input stream in;
            output stream out;
            process { out = in; }
        }
        graph G {
            input stream x;
            output stream y;
            node s = Slow;
            connection { x -> s.in; s.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (errors.contains ("must be a non-negative integer number of samples"));
}

//==============================================================================

TEST (YdspLatencyTests, SumsDeclaredLatencyAcrossAFusedChainAndStillFuses)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        processor Stage [[ latency: 8 ]] {
            input stream in;
            output stream out;
            input value float g = 1.0;
            process { out = in * g; }
        }
        graph G {
            input stream x;
            output stream y;
            node a = Stage (g = 1.0);
            node b = Stage (g = 1.0);
            node c = Stage (g = 1.0);
            connection { x -> a.in; a.out -> b.in; b.out -> c.in; c.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (24, graph.getLatencySamples());
    EXPECT_TRUE (latencyGraphHasFusedKernel (graph)) << "the chain stopped fusing";
}

//==============================================================================

TEST (YdspLatencyTests, CompensatesEachBranchOfAThreeWaySplitToTheLongest)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node one = Ident * 4;
            node two = Ident * 4;
            connection { x -> y; x -> one.in; one.out -> y; one.out -> two.in; two.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (2 * latencyOversamplerDelay, graph.getLatencySamples());

    constexpr int blockSize = 256;
    graph.prepare (48000.0, blockSize);

    std::vector<float> impulse (static_cast<size_t> (blockSize), 0.0f);
    impulse[0] = 1.0f;

    const auto out = latencyRunOnce (graph, impulse, 1);

    int peak = 0;

    for (int i = 1; i < blockSize; ++i)
        if (std::fabs (out[0][static_cast<size_t> (i)]) > std::fabs (out[0][static_cast<size_t> (peak)]))
            peak = i;

    EXPECT_EQ (2 * latencyOversamplerDelay, peak);
    EXPECT_GT (out[0][static_cast<size_t> (peak)], 2.0f);
}

TEST (YdspLatencyTests, AccumulatesOversamplingAndDeclaredLatencyInSeries)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        processor Look [[ latency: 32 ]] {
            input stream in;
            output stream out;
            process { out = in @ 32; }
        }
        graph G {
            input stream x;
            output stream y;
            node os = Ident * 4;
            node look = Look;
            connection { x -> y; x -> os.in; os.out -> look.in; look.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (latencyOversamplerDelay + 32, graph.getLatencySamples());

    constexpr int blockSize = 256;
    graph.prepare (48000.0, blockSize);

    std::vector<float> impulse (static_cast<size_t> (blockSize), 0.0f);
    impulse[0] = 1.0f;

    const auto out = latencyRunOnce (graph, impulse, 1);

    int peak = 0;

    for (int i = 1; i < blockSize; ++i)
        if (std::fabs (out[0][static_cast<size_t> (i)]) > std::fabs (out[0][static_cast<size_t> (peak)]))
            peak = i;

    EXPECT_EQ (latencyOversamplerDelay + 32, peak);
    EXPECT_GT (out[0][static_cast<size_t> (peak)], 1.5f);
}

//==============================================================================

TEST (YdspLatencyTests, UndersampledNodePassesLowFrequencyContentThrough)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node slow = Ident / 4;
            connection { x -> slow.in; slow.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());

    const auto expectedLatency = latencyOversamplerDelay * 4 + 3;
    EXPECT_EQ (expectedLatency, graph.getLatencySamples());

    constexpr int blockSize = 256;
    constexpr int blockCount = 8;
    constexpr double sampleRate = 48000.0;

    graph.prepare (sampleRate, blockSize);

    const auto input = latencySine (blockSize * blockCount, 300.0, sampleRate);
    const auto out = latencyRunBlocks (graph, input, blockSize, blockCount, 1);

    const auto* expected = input.data() + blockSize * (blockCount - 1) - expectedLatency;

    double error = 0.0;
    double reference = 0.0;

    for (int i = 0; i < blockSize; ++i)
    {
        const auto diff = static_cast<double> (out[0][static_cast<size_t> (i)]) - static_cast<double> (expected[i]);
        error += diff * diff;
        reference += static_cast<double> (expected[i]) * static_cast<double> (expected[i]);
    }

    ASSERT_GT (reference, 1.0); // not vacuously passing on silence
    EXPECT_LT (std::sqrt (error / reference), 0.1) << "decimated round trip did not reproduce the input";
}

TEST (YdspLatencyTests, UndersampledNodeSurvivesABlockSizeThatIsNotAMultipleOfTheFactor)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node slow = Ident / 4;
            connection { x -> slow.in; slow.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());

    constexpr int blockSize = 100; // deliberately not a multiple of 4
    constexpr int blockCount = 16;
    constexpr double sampleRate = 48000.0;

    graph.prepare (sampleRate, blockSize);

    const auto input = latencySine (blockSize * blockCount, 200.0, sampleRate);
    const auto out = latencyRunBlocks (graph, input, blockSize, blockCount, 1);

    for (int i = 1; i < blockSize; ++i)
    {
        const auto sample = out[0][static_cast<size_t> (i)];
        ASSERT_TRUE (std::isfinite (sample)) << "sample " << i;
        EXPECT_LT (std::fabs (sample - out[0][static_cast<size_t> (i - 1)]), 0.2f) << "discontinuity at sample " << i;
    }
}

TEST (YdspLatencyTests, CompensatesAnUndersampledBranchAgainstADryOne)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node slow = Ident / 2;
            node mix = Diff;
            connection { x -> mix.a; x -> slow.in; slow.out -> mix.b; mix.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (latencyOversamplerDelay * 2 + 1, graph.getLatencySamples());

    constexpr int blockSize = 256;
    constexpr int blockCount = 8;
    constexpr double sampleRate = 48000.0;

    graph.prepare (sampleRate, blockSize);

    const auto input = latencySine (blockSize * blockCount, 200.0, sampleRate);
    const auto out = latencyRunBlocks (graph, input, blockSize, blockCount, 1);

    const auto residual = latencyRms (out[0].data(), blockSize);
    const auto reference = latencyRms (input.data() + blockSize * (blockCount - 1), blockSize);

    ASSERT_GT (reference, 0.5);
    EXPECT_LT (residual, reference * 0.15) << "residual RMS " << residual << " against input RMS " << reference;
}

TEST (YdspLatencyTests, ARateChangedKernelReportsItsOwnSampleRate)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        processor Report { input stream in; output stream out; process { out = sampleRate; } }
        graph G {
            input stream x;
            output stream fast;
            output stream slow;
            node up = Report * 4;
            node down = Report / 4;
            connection { x -> up.in; x -> down.in; up.out -> fast; down.out -> slow; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());

    constexpr int blockSize = 128;
    constexpr double sampleRate = 48000.0;

    graph.prepare (sampleRate, blockSize);

    const std::vector<float> input (static_cast<size_t> (blockSize), 0.0f);

    const auto out = latencyRunBlocks (graph, std::vector<float> (static_cast<size_t> (blockSize * 6), 0.0f), blockSize, 6, 2);

    EXPECT_NEAR (sampleRate * 4.0, out[0][static_cast<size_t> (blockSize - 1)], sampleRate * 0.02)
        << "an oversampled kernel should see 4x the graph rate";

    EXPECT_NEAR (sampleRate / 4.0, out[1][static_cast<size_t> (blockSize - 1)], sampleRate * 0.02)
        << "an undersampled kernel should see a quarter of the graph rate";
}

TEST (YdspLatencyTests, RejectsAnUnsupportedUndersamplingFactor)
{
    const auto errors = latencyCompileError (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node slow = Ident / 3;
            connection { x -> slow.in; slow.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (errors.contains ("undersampling (/N) supports a factor of 2, 4 or 8"));
}

TEST (YdspLatencyTests, LeavesAPlainFanInUncompensated)
{
    DspJitCompiler compiler;

    auto graph = latencyCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Ident;
            node b = Ident;
            connection { x -> a.in; x -> b.in; a.out -> y; b.out -> y; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (0, graph.getLatencySamples());

    constexpr int blockSize = 8;
    graph.prepare (48000.0, blockSize);

    std::vector<float> input { 1.0f, 2.0f, -0.5f, 0.25f, 4.0f, 0.0f, -3.0f, 1.5f };
    const auto out = latencyRunOnce (graph, input, 1);

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR (input[i] * 2.0f, out[0][i], 1e-6f) << "sample " << i;
}

} // namespace yup::test
