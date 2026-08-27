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

#include "yup_YdspTestPatches.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <tuple>
#include <atomic>

namespace yup::test
{

namespace
{

using patches::compilePatch;
using patches::makeRamp;

void runProcess32 (yup::DspJitGraph& graph,
                   const float* const* inputs,
                   float* const* outputs,
                   int numSamples)
{
    std::vector<yup::DspJitInputBuffer> inputBuffers;
    inputBuffers.reserve (static_cast<size_t> (graph.getInputStreamCount()));

    for (int i = 0; i < graph.getInputStreamCount(); ++i)
        inputBuffers.emplace_back (yup::Span<const float> (inputs[i], static_cast<size_t> (numSamples)));

    std::vector<yup::DspJitOutputBuffer> outputBuffers;
    outputBuffers.reserve (static_cast<size_t> (graph.getOutputStreamCount()));

    for (int i = 0; i < graph.getOutputStreamCount(); ++i)
        outputBuffers.emplace_back (yup::Span<float> (outputs[i], static_cast<size_t> (numSamples)));

    graph.process (inputBuffers, outputBuffers, numSamples);
}

void runProcess (yup::DspJitGraph& graph,
                 const float* const* inputs,
                 int numInputs,
                 float* const* outputs,
                 int numOutputs,
                 int numSamples,
                 const yup::MidiBuffer* midi = nullptr,
                 const yup::DspJitAutomationEvent* automation = nullptr,
                 int numAutomationEvents = 0)
{
    std::vector<yup::DspJitInputBuffer> inputBuffers;
    inputBuffers.reserve (static_cast<size_t> (numInputs));

    for (int i = 0; i < numInputs; ++i)
        inputBuffers.emplace_back (yup::Span<const float> (inputs[i], static_cast<size_t> (numSamples)));

    std::vector<yup::DspJitOutputBuffer> outputBuffers;
    outputBuffers.reserve (static_cast<size_t> (numOutputs));

    for (int i = 0; i < numOutputs; ++i)
        outputBuffers.emplace_back (yup::Span<float> (outputs[i], static_cast<size_t> (numSamples)));

    graph.process (inputBuffers, outputBuffers, numSamples, midi, automation, numAutomationEvents);
}

void dumpAsmOnFailure (const DspJitGraph& graph)
{
    if (::testing::Test::HasFailure())
    {
        const auto asmText = graph.getDiagnostics().toString();

        if (! asmText.isEmpty())
            std::cout << "\n[AsmJit] graph kernels:\n"
                      << asmText << std::endl;
    }
}

} // namespace

//==============================================================================

TEST (YdspJitGraphTests, ArrayReadBackAfterWriteMatchesMemory)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        let size = 4;

        processor P {
            input stream in;
            output stream out;

            state float bank[size];
            state float other[size];

            process {
                float sum = 0.0;

                for i in 0..size {
                    let j = size - 1 - i;

                    bank[i] = in + float (i);
                    sum = sum + bank[i];

                    other[i] = in * 2.0;
                    sum = sum + bank[i] + other[i];

                    bank[j] = in * 3.0;
                    sum = sum + bank[i];
                }

                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 4);

    std::vector<float> input { 1.0f, 2.0f, -0.5f, 0.25f };
    std::vector<float> output (4, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 4);

    const auto expected = [] (float v)
    {
        float bank[4] {}, other[4] {};
        auto sum = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            const auto j = 3 - i;

            bank[i] = v + static_cast<float> (i);
            sum += bank[i];

            other[i] = v * 2.0f;
            sum += bank[i] + other[i];

            bank[j] = v * 3.0f;
            sum += bank[i]; // j == i never happens for size 4, but the compiler cannot know
        }

        return sum;
    };

    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR (expected (input[static_cast<size_t> (i)]), output[static_cast<size_t> (i)], 1e-4f)
            << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SelectsSharingAndReusingComparisons)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Selects {
            input stream in;
            output stream out;

            process {
                let wide = in > 0.25;
                let a = select (wide, 1.0, 2.0);
                let b = select (wide, 10.0, 20.0);

                float x = in;
                let isBig = x > 0.5;
                x = x * 100.0;
                let moved = select (isBig, x, -x);

                let plain = select (in < 0.0, -1.0, 1.0);

                out = a + b + moved + plain;
            }
        }
        graph G { input stream x; output stream y; node s = Selects; connection { x -> s.in; s.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 4);

    std::vector<float> input { -1.0f, 0.3f, 0.75f, 0.0f };
    std::vector<float> output (4, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 4);

    const auto expected = [] (float v)
    {
        const auto a = v > 0.25f ? 1.0f : 2.0f;
        const auto b = v > 0.25f ? 10.0f : 20.0f;
        const auto isBig = v > 0.5f;
        const auto x = v * 100.0f;
        const auto moved = isBig ? x : -x;
        const auto plain = v < 0.0f ? -1.0f : 1.0f;

        return a + b + moved + plain;
    };

    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR (expected (input[static_cast<size_t> (i)]), output[static_cast<size_t> (i)], 1e-4f)
            << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, NegatingZeroProducesNegativeZero)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Neg {
            input stream in;
            output stream out;
            process { out = -in; }
        }
        graph G { input stream x; output stream y; node n = Neg; connection { x -> n.in; n.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    std::vector<float> input { 0.0f, -0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::vector<float> output (8, 1.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 8);

    EXPECT_TRUE (std::signbit (output[0])) << "-(+0.0) must be -0.0";
    EXPECT_FALSE (std::signbit (output[1])) << "-(-0.0) must be +0.0";
    EXPECT_EQ (-1.0f, output[2]);
    EXPECT_EQ (1.0f, output[3]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsGainKernel)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            process { out = in * 2; }
        }
        graph G { input stream x; output stream y; node g = Gain; connection { x -> g.in; g.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsOversampledNodeThroughResampler)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Pass {
            input stream in;
            output stream out;
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = Pass * 2; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> input (64, 1.0f);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    EXPECT_NEAR (0.0f, output[0], 0.1f);
    EXPECT_NEAR (1.0f, output[63], 1e-3f);

    std::vector<float> steadyOutput (64, 0.0f);
    float* steadyPtrs[] = { steadyOutput.data() };

    runProcess32 (graph, inPtrs, steadyPtrs, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (1.0f, steadyOutput[static_cast<size_t> (i)], 1e-3f);

    graph.reset();

    std::vector<float> resetOutput (64, 0.0f);
    float* resetPtrs[] = { resetOutput.data() };

    runProcess32 (graph, inPtrs, resetPtrs, 64);

    EXPECT_NEAR (0.0f, resetOutput[0], 0.1f);
    EXPECT_NEAR (1.0f, resetOutput[63], 1e-3f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsOnePoleWithPrevState)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            input value float a = 0.5;
            process { out = (1 - a) * in + a * out'; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    float previous = 0.0f;

    for (int i = 0; i < 32; ++i)
    {
        const auto expected = 0.5f * input[static_cast<size_t> (i)] + 0.5f * previous;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
        previous = output[static_cast<size_t> (i)];
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, StatePersistsAcrossBlocks)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            process { out = 0.5 * in + 0.5 * out'; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    std::vector<float> fullInput (32);
    std::vector<float> fullOutput (32);

    for (int i = 0; i < 32; ++i)
        fullInput[static_cast<size_t> (i)] = 0.1f;

    const float* inA[] = { fullInput.data() };
    float* outA[] = { fullOutput.data() };
    const float* inB[] = { fullInput.data() + 16 };
    float* outB[] = { fullOutput.data() + 16 };

    runProcess32 (graph, inA, outA, 16);
    runProcess32 (graph, inB, outB, 16);

    float previous = 0.0f;

    for (int i = 0; i < 32; ++i)
    {
        const auto expected = 0.5f * 0.1f + 0.5f * previous;
        EXPECT_NEAR (expected, fullOutput[static_cast<size_t> (i)], 1e-5f);
        previous = fullOutput[static_cast<size_t> (i)];
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsTanhAndMathIntrinsics)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor SoftClip {
            input stream in;
            output stream out;
            process { out = tanh (in) * sqrt (abs (in) + 1); }
        }
        graph G { input stream x; output stream y; node s = SoftClip; connection { x -> s.in; s.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (48000.0, 64);

    auto input = makeRamp (64, -0.5f);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    for (int i = 0; i < 64; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];
        const auto expected = tanhf (x) * sqrtf (fabsf (x) + 1.0f);
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-4f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsFixedDelayPrimitive)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Delay3 {
            input stream in;
            output stream out;
            process { out = in @ 3; }
        }
        graph G { input stream x; output stream y; node d = Delay3; connection { x -> d.in; d.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
    {
        const auto expected = i >= 3 ? input[static_cast<size_t> (i - 3)] : 0.0f;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-6f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, GraphParameterDrivesSeveralNodeParameters)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain { input stream in; output stream out; input value float g = 1.0; process { out = in * g; } }
        graph G {
            input stream x;
            output stream y;
            input value float master = 2.0 [[ name: "Master", min: 0.0, max: 8.0 ]];
            node a = Gain;
            node b = Gain;
            connection { x -> a.in; a.out -> b.in; b.out -> y; master -> a.g; master -> b.g; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    EXPECT_EQ (graph.getParameterSlot ("master"), graph.getParameterSlot ("a.g"));
    EXPECT_EQ (graph.getParameterSlot ("master"), graph.getParameterSlot ("b.g"));

    graph.setParameter ("master", 3.0f);

    auto input = makeRamp (16, 1.0f);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 16);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 9.0f, output[static_cast<size_t> (i)], 1e-4f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AutomationReachesEveryNodeAGraphParameterDrives)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain { input stream in; output stream out; input value float g = 1.0; process { out = in * g; } }
        graph G {
            input stream x;
            output stream y;
            input value float master = 1.0;
            node a = Gain;
            node b = Gain;
            connection { x -> a.in; a.out -> b.in; b.out -> y; master -> a.g; master -> b.g; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    const auto slot = graph.getParameterSlot ("master");
    ASSERT_GE (slot, 0);

    const DspJitAutomationEvent events[] = { { slot, 8, 2.0f } };

    std::vector<float> input (16, 1.0f);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess (graph, inPtrs, 1, outPtrs, 1, 16, nullptr, events, 1);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (i < 8 ? 1.0f : 4.0f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsInlineConnectionDelay)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Pass { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = Pass; connection { x -> [3] -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32, 1.0f);
    const auto original = input;

    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
    {
        const auto expected = i >= 3 ? original[static_cast<size_t> (i - 3)] : 0.0f;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-6f);
    }

    EXPECT_EQ (original, input);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RejectsAnInlineDelayOnANonFloat32Stream)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor Pass { input stream float64 in; output stream float64 out; process { out = in; } }
        graph G { input stream float64 x; output stream float64 y; node p = Pass; connection { x -> [3] -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("inline delay is only supported on a float32 stream"));
}

TEST (YdspJitGraphTests, PreparesTwiceAtDifferentBlockSizesWithAnInlineDelay)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Pass { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = Pass; connection { x -> [3] -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    for (const int blockSize : { 16, 64, 32 })
    {
        graph.prepare (44100.0, blockSize);

        auto input = makeRamp (blockSize, 1.0f);
        const auto original = input;

        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);
        const float* inPtrs[] = { input.data() };
        float* outPtrs[] = { output.data() };

        runProcess32 (graph, inPtrs, outPtrs, blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            const auto expected = i >= 3 ? original[static_cast<size_t> (i - 3)] : 0.0f;
            EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-6f) << "block " << blockSize << " sample " << i;
        }
    }
}

TEST (YdspJitGraphTests, ResetClearsTheInlineDelayRing)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Pass { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = Pass; connection { x -> [3] -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    std::vector<float> loud (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    {
        const float* inPtrs[] = { loud.data() };
        float* outPtrs[] = { output.data() };
        runProcess32 (graph, inPtrs, outPtrs, 8);
    }

    // The ring now holds the last three 1.0 samples of that block.
    graph.reset();

    std::vector<float> silence (8, 0.0f);
    std::fill (output.begin(), output.end(), -1.0f);

    {
        const float* inPtrs[] = { silence.data() };
        float* outPtrs[] = { output.data() };
        runProcess32 (graph, inPtrs, outPtrs, 8);
    }

    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR (0.0f, output[static_cast<size_t> (i)], 1e-6f) << "sample " << i;
}

TEST (YdspJitGraphTests, ResolvesImportsRelativeToBasePath)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_import_test");

    tempDir.deleteRecursively();
    auto effectsDir = tempDir.getChildFile ("fx");
    effectsDir.createDirectory();

    effectsDir.getChildFile ("Trim.ydsp")
        .replaceWithText (R"YDSP(
            processor Trim { input stream in; output stream out; process { out = in * 0.5; } }
        )YDSP");

    effectsDir.getChildFile ("Gain.ydsp")
        .replaceWithText (R"YDSP(
            import Trim as trim;
            processor Gain { input stream in; output stream out; input value float gain = 2.0; process { out = in * gain; } }
        )YDSP");

    const auto patch = R"YDSP(
        import fx.Gain as fx;
        graph G { input stream x; output stream y;
                  node g = fx.Gain (gain = 3.0);
                  node t = fx.trim.Trim;
                  connection { x -> t.in; t.out -> g.in; g.out -> y; } }
    )YDSP";

    // Without a base path the import resolves against the CWD and must fail.
    {
        DspJitCompiler compiler;
        auto result = compiler.compile (patch);
        EXPECT_FALSE (result.wasOk());
    }

    // With the base path (given as a file path inside the folder) the import
    // and its nested import resolve and the graph runs.
    {
        DspJitCompiler compiler;
        auto result = compiler.compile (patch, tempDir.getChildFile ("Patch.ydsp").getFullPathName());
        ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

        auto graph = std::move (result).getValue();
        ASSERT_TRUE (graph.isValid());
        graph.prepare (44100.0, 32);

        // The imported node's parameter is exposed on the graph.
        EXPECT_TRUE (graph.hasParameter ("g.gain"));
        EXPECT_NEAR (3.0, graph.getParameter ("g.gain"), 1e-6f);

        // x -> Trim (0.5x) -> Gain (3x) -> y, so y = 1.5x.
        auto input = makeRamp (32);
        std::vector<float> output (32, 0.0f);
        const float* inPtrs[] = { input.data() };
        float* outPtrs[] = { output.data() };

        runProcess32 (graph, inPtrs, outPtrs, 32);

        for (int i = 0; i < 32; ++i)
            EXPECT_NEAR (input[static_cast<size_t> (i)] * 1.5f, output[static_cast<size_t> (i)], 1e-4f);

        dumpAsmOnFailure (graph);
    }

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, CallsTopLevelFunctionFromProcessorBody)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        func scale (x: float) : float { return x * 2.0; }
        func addOne (x: float) : float { return scale (x) + 1.0; }
        processor P { input stream in; output stream out; process { out = addOne (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f + 1.0f, output[static_cast<size_t> (i)], 1e-4f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, CallsImportedLibraryFunctions)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_libfunc_test");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("lib").createDirectory();

    tempDir.getChildFile ("lib/Util.ydsp").replaceWithText (R"YDSP(
        func scale (v: float) : float { return v * 2.0; }
        func wrap (v: float) : float { return scale (v) + 1.0; }
    )YDSP");

    const auto patch = R"YDSP(
        import lib.Util as u;
        processor P { input stream in; output stream out; process { out = u.wrap (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f + 1.0f, output[static_cast<size_t> (i)], 1e-4f);

    dumpAsmOnFailure (graph);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ImportedProcessorCallsItsLibraryFunctions)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_libproc_test");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("lib").createDirectory();

    tempDir.getChildFile ("lib/Osc.ydsp")
        .replaceWithText (R"YDSP(
            func scale (v: float) : float { return v * 2.0; }
            processor P { input stream in; output stream out; process { out = scale (in); } }
        )YDSP");

    const auto patch = R"YDSP(
        import lib.Osc as u;
        graph G { input stream x; output stream y; node p = u.P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f, output[static_cast<size_t> (i)], 1e-4f);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ImportedProcessorLocalFunctionCallsItsLibraryFunctions)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_libproclocal_test");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("lib").createDirectory();

    tempDir.getChildFile ("lib/Osc.ydsp")
        .replaceWithText (R"YDSP(
            func scale (v: float) : float { return v * 2.0; }
            processor P {
                input stream in;
                output stream out;
                func local (v: float) : float { return scale (v) + 1.0; }
                process { out = local (in); }
            }
        )YDSP");

    const auto patch = R"YDSP(
        import lib.Osc as u;
        graph G { input stream x; output stream y; node p = u.P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f + 1.0f, output[static_cast<size_t> (i)], 1e-4f);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ProcessorFunctionShadowsTopLevelFunction)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        func scale (x: float) : float { return x * 2.0; }
        processor P {
            input stream in;
            output stream out;
            func scale (x: float) : float { return x * 3.0; }
            process { out = scale (in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 3.0f, output[static_cast<size_t> (i)], 1e-4f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ReportsUnknownNamespacedFunctionCall)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor P { input stream in; output stream out; process { out = fx.missing (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("Unknown function 'fx.missing'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspJitGraphTests, ReportsRecursiveTopLevelFunction)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        func loop (x: float) : float { return loop (x); }
        processor P { input stream in; output stream out; process { out = loop (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("Recursive call detected in function 'loop'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspJitGraphTests, ReportsRecursiveFunctionHiddenInAConditionalBody)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        func loop (x: float) : float {
            if (x > 0.0) { return loop (x - 1.0); }
            return x;
        }
        processor P { input stream in; output stream out; process { out = loop (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("Recursive call detected in function 'loop'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspJitGraphTests, ReportsRecursiveFunctionHiddenInANestedExpression)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        func loop (x: float) : float { return loop (x) + 1.0; }
        processor P { input stream in; output stream out; process { out = loop (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("Recursive call detected in function 'loop'"))
            found = true;

    EXPECT_TRUE (found);
}

TEST (YdspJitGraphTests, ImportDottedPathMapsToFileAndLastSegmentNamespace)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_dotted_import");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("fx").createDirectory();

    tempDir.getChildFile ("fx/Delay.ydsp")
        .replaceWithText ("processor Delay { input stream in; output stream out; process { out = in * 0.5; } }");

    const auto patch = R"YDSP(
        import fx.Delay;
        graph G { input stream x; output stream y; node d = Delay.Delay; connection { x -> d.in; d.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 0.5f, output[static_cast<size_t> (i)], 1e-4f);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ImportAliasOverridesDefaultNamespace)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_alias_import");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("fx").createDirectory();

    tempDir.getChildFile ("fx/Delay.ydsp")
        .replaceWithText ("processor Delay { input stream in; output stream out; process { out = in * 0.25; } }");

    const auto patch = R"YDSP(
        import fx.Delay as w;
        graph G { input stream x; output stream y; node d = w.Delay; connection { x -> d.in; d.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 0.25f, output[static_cast<size_t> (i)], 1e-4f);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ReportsImportNamespaceCollision)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_collision_import");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("a").createDirectory();
    tempDir.getChildFile ("b").createDirectory();

    tempDir.getChildFile ("a/A.ydsp")
        .replaceWithText ("processor A { input stream in; output stream out; process { out = in; } }\n");

    tempDir.getChildFile ("b/A.ydsp")
        .replaceWithText ("processor B { input stream in; output stream out; process { out = in; } }\n");

    const auto patch = R"YDSP(
        import a.A;
        import b.A;
        graph G { input stream x; output stream y; node a = A.A; connection { x -> a.in; a.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("would share the namespace 'A'")
            && compiler.getDiagnostics().getItem (i).message.contains ("use 'as' to disambiguate"))
            found = true;

    EXPECT_TRUE (found);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, SameFileImportedTwiceIsNotAnError)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_dup_import");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("fx").createDirectory();

    tempDir.getChildFile ("fx/Gain.ydsp")
        .replaceWithText ("processor Gain { input stream in; output stream out; process { out = in * 2.0; } }");

    const auto patch = R"YDSP(
        import fx.Gain;
        import fx.Gain;
        graph G { input stream x; output stream y; node g = Gain.Gain; connection { x -> g.in; g.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ReportsCircularImport)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_circular_import");

    tempDir.deleteRecursively();
    tempDir.createDirectory();

    tempDir.getChildFile ("a.ydsp")
        .replaceWithText ("import b; processor A { input stream in; output stream out; process { out = in; } }");

    tempDir.getChildFile ("b.ydsp")
        .replaceWithText ("import a; processor B { input stream in; output stream out; process { out = in; } }");

    const auto patch = R"YDSP(
        import a;
        graph G { input stream x; output stream y; node a = A.A; connection { x -> a.in; a.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("Circular import detected for 'a'"))
            found = true;

    EXPECT_TRUE (found);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ReportsSyntaxErrorsInImportedFiles)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_badimport_test");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("lib").createDirectory();

    tempDir.getChildFile ("lib/Broken.ydsp")
        .replaceWithText ("processor Broken { input stream in; output stream out; process { out = ; } }\n");

    const auto patch = R"YDSP(
        import lib.Broken as b;
        graph G { input stream x; output stream y; node p = b.Broken; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    EXPECT_FALSE (result.wasOk());

    bool found = false;
    for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
        if (compiler.getDiagnostics().getItem (i).message.contains ("Expected an expression"))
            found = true;

    EXPECT_TRUE (found);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ParallelImportCompilationMatchesSequential)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_parallel_import");

    tempDir.deleteRecursively();
    tempDir.getChildFile ("fx").createDirectory();
    tempDir.getChildFile ("lib").createDirectory();

    tempDir.getChildFile ("fx/Trim.ydsp")
        .replaceWithText ("processor Trim { input stream in; output stream out; process { out = in * 0.5; } }\n");

    tempDir.getChildFile ("fx/Gain.ydsp")
        .replaceWithText ("import Trim as trim;\nprocessor Gain { input stream in; output stream out; input value float g = 2.0; process { out = in * g; } }\n");

    tempDir.getChildFile ("lib/Math.ydsp")
        .replaceWithText ("func scale (v: float) : float { return v * 2.0; }\n");

    const auto patch = R"YDSP(
        import fx.Gain as fx;
        import lib.Math as m;
        processor P { input stream in; output stream out; process { out = m.scale (in); } }
        graph G {
            input stream x; output stream y;
            node p = P;
            node t = fx.trim.Trim;
            node g = fx.Gain (g = 3.0);
            connection { x -> p.in; p.out -> t.in; t.out -> g.in; g.out -> y; }
        }
    )YDSP";

    const auto input = makeRamp (32);

    const auto runPatch = [&] (ThreadPool* pool) -> std::vector<float>
    {
        DspJitCompiler compiler;
        auto result = compiler.compile (patch, tempDir.getFullPathName(), pool);
        EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

        if (! result.wasOk())
            return {};

        auto graph = std::move (result).getValue();
        graph.prepare (44100.0, 32);

        std::vector<float> output (32, 0.0f);
        const float* inPtrs[] = { input.data() };
        float* outPtrs[] = { output.data() };

        runProcess32 (graph, inPtrs, outPtrs, 32);

        return output;
    };

    const auto sequential = runPatch (nullptr);
    ThreadPool pool;
    const auto parallel = runPatch (&pool);

    ASSERT_EQ (32u, sequential.size());
    ASSERT_EQ (32u, parallel.size());

    for (int i = 0; i < 32; ++i)
    {
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 3.0f, sequential[static_cast<size_t> (i)], 1e-4f);
        EXPECT_EQ (sequential[static_cast<size_t> (i)], parallel[static_cast<size_t> (i)]);
    }

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, CallerOwnedPoolStillRunsUnrelatedJobs)
{
    struct MarkerJob : public ThreadPoolJob
    {
        MarkerJob()
            : ThreadPoolJob ("parallel-import-marker")
        {
        }

        JobStatus runJob() override
        {
            ran = true;
            return jobHasFinished;
        }

        std::atomic<bool> ran { false };
    };

    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_pool_marker");

    tempDir.deleteRecursively();
    tempDir.createDirectory();

    tempDir.getChildFile ("Gain.ydsp")
        .replaceWithText ("processor Gain { input stream in; output stream out; process { out = in * 2.0; } }\n");

    const auto patch = R"YDSP(
        import Gain;
        graph G { input stream x; output stream y; node g = Gain.Gain; connection { x -> g.in; g.out -> y; } }
    )YDSP";

    ThreadPool pool;
    MarkerJob marker;
    pool.addJob (&marker, false);

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName(), &pool);
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    EXPECT_TRUE (pool.waitForJobToFinish (&marker, 1000));
    EXPECT_TRUE (marker.ran.load());

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ImportsTheSameLibraryFromTwoFilesUnderDifferentNamespaces)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_diamond_import");

    tempDir.deleteRecursively();
    tempDir.createDirectory();

    tempDir.getChildFile ("z.ydsp")
        .replaceWithText ("processor Z { input stream in; output stream out; process { out = in * 2.0; } }\n");

    tempDir.getChildFile ("x.ydsp")
        .replaceWithText ("import z;\ngraph XGraph { input stream in; output stream out; node zz = z.Z; connection { in -> zz.in; zz.out -> out; } }\n");

    tempDir.getChildFile ("y.ydsp")
        .replaceWithText ("import z as zz;\ngraph YGraph { input stream in; output stream out; node zz = zz.Z; connection { in -> zz.in; zz.out -> out; } }\n");

    const auto patch = R"YDSP(
        import x;
        import y;
        graph G {
            input stream x;
            output stream y;
            node a = x.XGraph;
            node b = y.YGraph;
            connection { x -> a.in; a.out -> b.in; b.out -> y; }
        }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 4.0f, output[static_cast<size_t> (i)], 1e-4f);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, ParallelImportsMatchSequentialForDiamondsAndFailures)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_parallel_import_edges");

    tempDir.deleteRecursively();
    tempDir.createDirectory();

    tempDir.getChildFile ("z.ydsp")
        .replaceWithText ("processor Z { input stream in; output stream out; process { out = in * 2.0; } }\n");

    tempDir.getChildFile ("x.ydsp")
        .replaceWithText ("import z;\ngraph XGraph { input stream in; output stream out; node zz = z.Z; connection { in -> zz.in; zz.out -> out; } }\n");

    tempDir.getChildFile ("y.ydsp")
        .replaceWithText ("import z as zz;\ngraph YGraph { input stream in; output stream out; node zz = zz.Z; connection { in -> zz.in; zz.out -> out; } }\n");

    tempDir.getChildFile ("broken.ydsp")
        .replaceWithText ("processor Broken { input stream in; output stream out; process { out = ; } }\n");

    const auto diamondPatch = R"YDSP(
        import x;
        import y;
        graph G { input stream x; output stream y; node a = x.XGraph; node b = y.YGraph;
                  connection { x -> a.in; a.out -> b.in; b.out -> y; } }
    )YDSP";

    const auto missingPatch = R"YDSP(
        import Missing;
        graph G { input stream x; output stream y; node m = Missing.Missing; connection { x -> m.in; m.out -> y; } }
    )YDSP";

    const auto brokenPatch = R"YDSP(
        import broken as b;
        graph G { input stream x; output stream y; node p = b.Broken; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    for (const auto& patch : { diamondPatch, missingPatch, brokenPatch })
    {
        String sequentialMessages;
        bool sequentialOk = false;

        {
            DspJitCompiler compiler;
            auto sequential = compiler.compile (patch, tempDir.getFullPathName());
            sequentialOk = sequential.wasOk();

            for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
                sequentialMessages += compiler.getDiagnostics().getItem (i).message + "\n";

            if (patch == diamondPatch)
                ASSERT_TRUE (sequentialOk) << sequentialMessages;
            else
                ASSERT_FALSE (sequentialOk);
        }

        ThreadPool pool;
        DspJitCompiler compiler;
        auto parallel = compiler.compile (patch, tempDir.getFullPathName(), &pool);

        String parallelMessages;

        for (int i = 0; i < compiler.getDiagnostics().getCount(); ++i)
            parallelMessages += compiler.getDiagnostics().getItem (i).message + "\n";

        EXPECT_EQ (sequentialOk, parallel.wasOk());
        EXPECT_EQ (sequentialMessages, parallelMessages);

        if (patch == diamondPatch && parallel.wasOk())
        {
            auto graph = std::move (parallel).getValue();
            ASSERT_TRUE (graph.isValid());
            graph.prepare (44100.0, 32);

            auto input = makeRamp (32);
            std::vector<float> output (32, 0.0f);
            const float* inPtrs[] = { input.data() };
            float* outPtrs[] = { output.data() };

            runProcess32 (graph, inPtrs, outPtrs, 32);

            for (int i = 0; i < 32; ++i)
                EXPECT_NEAR (input[static_cast<size_t> (i)] * 4.0f, output[static_cast<size_t> (i)], 1e-4f);
        }
    }

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, RoutesSeparateEventInputsByName)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor A {
            input event midi1;
            output stream out;
            state float f;
            event midi1 (e: noteOn) { f = e.pitch; }
            process { out = f; }
        }
        processor B {
            input event midi2;
            output stream out;
            state float g;
            event midi2 (e: noteOn) { g = e.pitch * 2.0; }
            process { out = g; }
        }
        graph G {
            input event midi1;
            input event midi2;
            output stream y1;
            output stream y2;
            node a = A;
            node b = B;
            connection { midi1 -> a.midi1; midi2 -> b.midi2; a.out -> y1; b.out -> y2; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    EXPECT_EQ (2, graph.getEventInputCount());
    EXPECT_EQ ("midi1", graph.getEventInputName (0));
    EXPECT_EQ ("midi2", graph.getEventInputName (1));

    yup::MidiBuffer midi1;
    midi1.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);

    yup::MidiBuffer midi2;
    midi2.addEvent (yup::MidiMessage::noteOn (1, 72, static_cast<uint8> (100)), 0);

    const auto run = [&] (const yup::MidiBuffer* in1, const yup::MidiBuffer* in2, std::vector<float>& out1, std::vector<float>& out2)
    {
        std::vector<yup::DspJitOutputBuffer> outputBuffers;
        outputBuffers.emplace_back (yup::Span<float> (out1.data(), static_cast<size_t> (32)));
        outputBuffers.emplace_back (yup::Span<float> (out2.data(), static_cast<size_t> (32)));

        const yup::MidiBuffer* buffers[] = { in1, in2 };

        const auto result = graph.process (yup::Span<const yup::DspJitInputBuffer>(),
                                           yup::Span<yup::DspJitOutputBuffer> (outputBuffers.data(), outputBuffers.size()),
                                           32,
                                           yup::Span<const yup::MidiBuffer*> (buffers, 2),
                                           nullptr,
                                           0);

        EXPECT_EQ (yup::DspJitProcessResult::ok, result);
    };

    {
        std::vector<float> y1 (32, 0.0f), y2 (32, 0.0f);
        run (&midi1, &midi2, y1, y2);

        for (int i = 0; i < 32; ++i)
        {
            EXPECT_NEAR (60.0f, y1[static_cast<size_t> (i)], 1e-4f);
            EXPECT_NEAR (144.0f, y2[static_cast<size_t> (i)], 1e-4f);
        }
    }

    graph.reset();

    {
        std::vector<float> y1 (32, 0.0f), y2 (32, 0.0f);
        run (nullptr, &midi2, y1, y2);

        for (int i = 0; i < 32; ++i)
        {
            EXPECT_NEAR (0.0f, y1[static_cast<size_t> (i)], 1e-4f);
            EXPECT_NEAR (144.0f, y2[static_cast<size_t> (i)], 1e-4f);
        }
    }
}

TEST (YdspJitGraphTests, NoteOffOnOneEventInputDoesNotReleaseTheOtherInputsVoice)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input event inA;
            input event inB;
            state float pitch;
            event inA (e: noteOn) { pitch = e.pitch; }
            event inA (e: noteOff) { pitch = 0.0; }
            event inB (e: noteOn) { pitch = e.pitch + 100.0; }
            event inB (e: noteOff) { pitch = 0.0; }
            process { out = pitch; }
        }
        graph G {
            input event inA;
            input event inB;
            output stream y;
            node v = Voice[4];
            connection { inA -> v.inA; inB -> v.inB; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);

    yup::MidiBuffer inA;
    inA.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    inA.addEvent (yup::MidiMessage::noteOff (1, 60), 10);

    yup::MidiBuffer inB;
    inB.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 20);

    inA.addEvent (yup::MidiMessage::noteOff (1, 60), 30);

    const yup::MidiBuffer* buffers[] = { &inA, &inB };

    std::vector<yup::DspJitOutputBuffer> outputBuffers;
    outputBuffers.emplace_back (yup::Span<float> (output.data(), static_cast<size_t> (512)));

    EXPECT_EQ (yup::DspJitProcessResult::ok,
               graph.process (yup::Span<const yup::DspJitInputBuffer>(),
                              yup::Span<yup::DspJitOutputBuffer> (outputBuffers.data(), outputBuffers.size()),
                              512,
                              yup::Span<const yup::MidiBuffer*> (buffers, 2),
                              nullptr,
                              0));

    for (int i = 0; i < 10; ++i)
        EXPECT_FLOAT_EQ (60.0f, output[static_cast<size_t> (i)]) << "before the inA note-off at " << i;

    for (int i = 20; i < 512; ++i)
        EXPECT_FLOAT_EQ (160.0f, output[static_cast<size_t> (i)]) << "after the inB note-on at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MonoNoteOffOnOneEventInputDoesNotRemoveTheOtherInputsHeldNote)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input event inA;
            input event inB;
            state float pitch;
            event inA (e: noteOn) { pitch = e.pitch; }
            event inA (e: noteOff) { pitch = 0.0; }
            event inB (e: noteOn) { pitch = e.pitch + 100.0; }
            event inB (e: noteOff) { pitch = 0.0; }
            process { out = pitch; }
        }
        graph G {
            input event inA;
            input event inB;
            output stream y;
            node v = Voice [[ mode: mono ]];
            connection { inA -> v.inA; inB -> v.inB; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);

    const auto runBlock = [&] (const yup::MidiBuffer* a, const yup::MidiBuffer* b)
    {
        output.assign (512, 0.0f);

        const yup::MidiBuffer* buffers[] = { a, b };

        std::vector<yup::DspJitOutputBuffer> outputBuffers;
        outputBuffers.emplace_back (yup::Span<float> (output.data(), static_cast<size_t> (512)));

        return graph.process (yup::Span<const yup::DspJitInputBuffer>(),
                              yup::Span<yup::DspJitOutputBuffer> (outputBuffers.data(), outputBuffers.size()),
                              512,
                              yup::Span<const yup::MidiBuffer*> (buffers, 2),
                              nullptr,
                              0);
    };

    {
        yup::MidiBuffer inB;
        inB.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);

        EXPECT_EQ (yup::DspJitProcessResult::ok, runBlock (nullptr, &inB));

        for (int i = 0; i < 512; ++i)
            EXPECT_FLOAT_EQ (160.0f, output[static_cast<size_t> (i)]) << "B's note sounding at " << i;
    }

    {
        yup::MidiBuffer inA;
        inA.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
        inA.addEvent (yup::MidiMessage::noteOff (1, 60), 10);

        EXPECT_EQ (yup::DspJitProcessResult::ok, runBlock (&inA, nullptr));

        for (int i = 10; i < 512; ++i)
            EXPECT_FLOAT_EQ (160.0f, output[static_cast<size_t> (i)]) << "B's note survives A's release at " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AllSoundOffOnEachInputSilencesAtItsOwnOffset)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input event inA;
            input event inB;
            state float active;
            event inA (e: noteOn) { active = 1.0; }
            process { out = active; }
        }
        graph G {
            input event inA;
            input event inB;
            output stream y;
            node v = Voice;
            connection { inA -> v.inA; inB -> v.inB; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);

    yup::MidiBuffer inA;
    inA.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    inA.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 100);

    yup::MidiBuffer inB;
    inB.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 200);

    const yup::MidiBuffer* buffers[] = { &inA, &inB };

    std::vector<yup::DspJitOutputBuffer> outputBuffers;
    outputBuffers.emplace_back (yup::Span<float> (output.data(), static_cast<size_t> (512)));

    EXPECT_EQ (yup::DspJitProcessResult::ok,
               graph.process (yup::Span<const yup::DspJitInputBuffer>(),
                              yup::Span<yup::DspJitOutputBuffer> (outputBuffers.data(), outputBuffers.size()),
                              512,
                              yup::Span<const yup::MidiBuffer*> (buffers, 2),
                              nullptr,
                              0));

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "before the first all-sound-off at " << i;

    for (int i = 100; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "after the first all-sound-off at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AllSoundOffSilencesANoteRetriggeredAfterIt)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input event inA;
            input event inB;
            state float active;
            event inA (e: noteOn) { active = 1.0; }
            process { out = active; }
        }
        graph G {
            input event inA;
            input event inB;
            output stream y;
            node v = Voice;
            connection { inA -> v.inA; inB -> v.inB; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);

    yup::MidiBuffer inA;
    inA.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    inA.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 100);
    inA.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 150);
    inA.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 200);

    yup::MidiBuffer inB;

    const yup::MidiBuffer* buffers[] = { &inA, &inB };

    std::vector<yup::DspJitOutputBuffer> outputBuffers;
    outputBuffers.emplace_back (yup::Span<float> (output.data(), static_cast<size_t> (512)));

    EXPECT_EQ (yup::DspJitProcessResult::ok,
               graph.process (yup::Span<const yup::DspJitInputBuffer>(),
                              yup::Span<yup::DspJitOutputBuffer> (outputBuffers.data(), outputBuffers.size()),
                              512,
                              yup::Span<const yup::MidiBuffer*> (buffers, 2),
                              nullptr,
                              0));

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "first sounding segment at " << i;

    for (int i = 100; i < 150; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "silenced at " << i;

    for (int i = 150; i < 200; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "re-triggered at " << i;

    for (int i = 200; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "silenced again at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AssemblesKernelsWithLargeStateLayouts)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor DelayLine {
            input stream in;
            output stream out;
            state float buf[5000];
            state int wp;
            process {
                buf[wp] = in;
                out = in + 0.5 * buf[wp];
                wp = wp + 1;
                if (wp >= 5000) { wp = 0; }
            }
        }
        processor Rings {
            input stream in;
            output stream out;
            process {
                let a = in @ 2000;
                let b = in @ 2000;
                let c = in @ 2000;
                out = a + b + c;
            }
        }
        graph G { input stream x; output stream y; node d = DelayLine; node r = Rings; connection { x -> d.in; d.out -> r.in; r.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (0.0f, output[static_cast<size_t> (i)], 1e-4f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsReverbStyleMultipleDelays)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor ReverbLike {
            input stream in;
            output stream out;

            input value float mix = 0.3;
            input value float feedback = 0.7;
            input value float damping = 0.5;

            state float c1; state float c2; state float c3; state float c4;
            state float c5; state float c6; state float c7; state float c8;
            state float l1; state float l2; state float l3; state float l4;
            state float l5; state float l6; state float l7; state float l8;

            process {
                let d1 = c1 @ 10;  l1 = l1 * damping + d1 * (1.0 - damping);  c1 = l1 * feedback + in;
                let d2 = c2 @ 11;  l2 = l2 * damping + d2 * (1.0 - damping);  c2 = l2 * feedback + in;
                let d3 = c3 @ 12;  l3 = l3 * damping + d3 * (1.0 - damping);  c3 = l3 * feedback + in;
                let d4 = c4 @ 13;  l4 = l4 * damping + d4 * (1.0 - damping);  c4 = l4 * feedback + in;
                let d5 = c5 @ 14;  l5 = l5 * damping + d5 * (1.0 - damping);  c5 = l5 * feedback + in;
                let d6 = c6 @ 15;  l6 = l6 * damping + d6 * (1.0 - damping);  c6 = l6 * feedback + in;
                let d7 = c7 @ 16;  l7 = l7 * damping + d7 * (1.0 - damping);  c7 = l7 * feedback + in;
                let d8 = c8 @ 17;  l8 = l8 * damping + d8 * (1.0 - damping);  c8 = l8 * feedback + in;
                let d9 = c1 @ 18;  c1 = l1 * feedback + in;
                let d10 = c2 @ 19; c2 = l2 * feedback + in;
                let d11 = c3 @ 20; c3 = l3 * feedback + in;
                let d12 = c4 @ 21; c4 = l4 * feedback + in;
                out = (d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9 + d10 + d11 + d12) * mix;
            }
        }
        graph G { input stream x; output stream y; node p = ReverbLike; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    for (int block = 0; block < 16; ++block)
        runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[static_cast<size_t> (i)]));
        EXPECT_LT (std::fabs (output[static_cast<size_t> (i)]), 100.0f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsBlockModeWithLoop)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor BlockGain {
            input stream in;
            output stream out;
            input value float drive = 1;
            process block {
                for i in 0..blockSize { out[i] = in[i] * drive; }
            }
        }
        graph G { input stream x; output stream y; node b = BlockGain (drive = 3); connection { x -> b.in; b.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 3.0f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SetsAndReadsParameters)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float drive = 1;
            output value float level;
            process { out = in * drive; level = abs (out); }
        }
        graph G {
            input stream x;
            output stream y;
            input value float master = 0.5;
            node g = Gain;
            connection { x -> g.in; g.out -> y; master -> g.drive; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_TRUE (graph.hasParameter ("master"));
    EXPECT_TRUE (graph.hasParameter ("g.drive"));
    EXPECT_EQ (0.5f, graph.getParameter ("master"));

    graph.prepare (44100.0, 16);

    auto input = makeRamp (16);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    graph.setParameter ("master", 0.25f);
    runProcess32 (graph, inPtrs, outPtrs, 16);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 0.25f, output[static_cast<size_t> (i)], 1e-5f);

    EXPECT_NEAR (fabsf (output[15]), graph.getOutputValue ("g.level"), 1e-6f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsMultiNodeGraphWithSidechain)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Saturator {
            input stream in;
            input stream side;
            output stream out;
            process { out = tanh (in * (1 + 0.5 * side)); }
        }
        processor Gain {
            input stream in;
            output stream out;
            process { out = in * 0.5; }
        }
        graph G {
            input stream dry;
            input stream sc;
            output stream wet;
            node sat = Saturator;
            node gain = Gain;
            connection {
                dry -> sat.in;
                sc -> sat.side;
                sat.out -> gain.in;
                gain.out -> wet;
            }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto dry = makeRamp (32);
    auto sc = makeRamp (32, 0.1f);
    std::vector<float> wet (32, 0.0f);
    const float* inPtrs[] = { dry.data(), sc.data() };
    float* outPtrs[] = { wet.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
    {
        const auto x = dry[static_cast<size_t> (i)];
        const auto side = sc[static_cast<size_t> (i)];
        const auto expected = 0.5f * tanhf (x * (1.0f + 0.5f * side));
        EXPECT_NEAR (expected, wet[static_cast<size_t> (i)], 1e-4f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AlgebraAndConnectionsProduceSameResult)
{
    DspJitCompiler compilerA;
    DspJitCompiler compilerB;

    auto graphA = compilePatch (R"YDSP(
        processor Gain { input stream in; output stream out; input value float g = 1; process { out = in * g; } }
        graph G {
            input stream dry;
            output stream wet;
            node g1 = Gain (g = 2);
            node g2 = Gain (g = 3);
            connection { dry -> g1.in; g1.out -> g2.in; g2.out -> wet; }
        }
    )YDSP",
                                compilerA);

    auto graphB = compilePatch (R"YDSP(
        processor Gain { input stream in; output stream out; input value float g = 1; process { out = in * g; } }
        graph G {
            input stream dry;
            output stream wet;
            process = dry : Gain (g = 2) : Gain (g = 3) : wet;
        }
    )YDSP",
                                compilerB);

    ASSERT_TRUE (graphA.isValid());
    ASSERT_TRUE (graphB.isValid());

    graphA.prepare (44100.0, 32);
    graphB.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> outA (32), outB (32);
    const float* inPtrs[] = { input.data() };
    float* outAPtrs[] = { outA.data() };
    float* outBPtrs[] = { outB.data() };

    runProcess32 (graphA, inPtrs, outAPtrs, 32);
    runProcess32 (graphB, inPtrs, outBPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (outA[static_cast<size_t> (i)], outB[static_cast<size_t> (i)], 1e-6f);

    dumpAsmOnFailure (graphA);
    dumpAsmOnFailure (graphB);
}

TEST (YdspJitGraphTests, ReportsCompileErrors)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(processor P { input stream in; output stream out; process { out = missing; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_TRUE (result.failed());
    EXPECT_TRUE (compiler.getDiagnostics().hasErrors());
    EXPECT_EQ (1, compiler.getDiagnostics().getItem (0).line);
}

TEST (YdspJitGraphTests, ExposesExecutionReport)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Taps {
            input stream in;
            output stream out;
            process block {
                for i in 0..8 { out[i] = in[i]; }
            }
        }
        graph G { input stream x; output stream y; node t = Taps; connection { x -> t.in; t.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    const auto& report = graph.getExecutionReport();
    ASSERT_EQ (1u, report.getKernels().size());
    EXPECT_EQ ("Taps", report.getKernels()[0].name);
    EXPECT_GT (report.getKernels()[0].instructionCount, 0);
    EXPECT_EQ (8, report.getKernels()[0].boundedIterationCount);
    EXPECT_TRUE (report.isProvenRealtimeSafe());

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsBiquadLowpass)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor BiquadLP {
            input stream in;
            output stream out;
            state float z1;
            state float z2;
            process {
                let b0 = 0.0002414;
                let b1 = 0.0004827;
                let b2 = 0.0002414;
                let a1 = -1.9556;
                let a2 = 0.9565;
                let y = b0 * in + z1;
                z1 = b1 * in - a1 * y + z2;
                z2 = b2 * in - a2 * y;
                out = y;
            }
        }
        graph G { input stream x; output stream y; node f = BiquadLP; connection { x -> f.in; f.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    constexpr int kBlockSize = 512;
    constexpr float dcGain = (0.0002414f + 0.0004827f + 0.0002414f) / (1.0f + (-1.9556f) + 0.9565f);

    graph.prepare (44100.0, kBlockSize);

    std::vector<float> input (kBlockSize, 1.0f);
    std::vector<float> output (kBlockSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, kBlockSize);

    const auto steady = output[kBlockSize - 1];
    EXPECT_NEAR (dcGain, steady, 0.01f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsBiquadHighpass)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor BiquadHP {
            input stream in;
            output stream out;
            state float z1;
            state float z2;
            process {
                let b0 = 0.9780;
                let b1 = -1.9561;
                let b2 = 0.9780;
                let a1 = -1.9556;
                let a2 = 0.9565;
                let y = b0 * in + z1;
                z1 = b1 * in - a1 * y + z2;
                z2 = b2 * in - a2 * y;
                out = y;
            }
        }
        graph G { input stream x; output stream y; node f = BiquadHP; connection { x -> f.in; f.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    constexpr int kBlockSize = 512;
    constexpr float dcGain = (0.9780f + (-1.9561f) + 0.9780f) / (1.0f + (-1.9556f) + 0.9565f);

    graph.prepare (44100.0, kBlockSize);

    std::vector<float> input (kBlockSize, 1.0f);
    std::vector<float> output (kBlockSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, kBlockSize);

    const auto steady = output[kBlockSize - 1];
    EXPECT_NEAR (dcGain, steady, 0.01f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsEnvelopeFollower)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor EnvFollower {
            input stream in;
            output stream out;
            state float env;
            process {
                let attack = 0.01;
                let release = 0.0001;
                let rect = abs (in);
                let coeff = (rect > env) ? attack : release;
                env = env + coeff * (rect - env);
                out = env;
            }
        }
        graph G { input stream x; output stream y; node e = EnvFollower; connection { x -> e.in; e.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_GE (output[static_cast<size_t> (i)], 0.0f);

    float previous = -1.0f;
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_GE (output[static_cast<size_t> (i)], previous);
        previous = output[static_cast<size_t> (i)];
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsPeakDetector)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor PeakDet {
            input stream in;
            output stream out;
            state float peak;
            process {
                let rect = abs (in);
                peak = max (peak * 0.9995, rect);
                out = peak;
            }
        }
        graph G { input stream x; output stream y; node p = PeakDet; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    std::vector<float> input (32, 1.0f);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    EXPECT_NEAR (1.0f, output[31], 0.01f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsRingModulator)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor RingMod {
            input stream in;
            output stream out;
            state float phase;
            process {
                let carrier = sin (phase);
                out = in * carrier;
                phase = phase + 0.1;
            }
        }
        graph G { input stream x; output stream y; node r = RingMod; connection { x -> r.in; r.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> input (64, 1.0f);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    float phase = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        const auto expected = sinf (phase);
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-4f);
        phase += 0.1f;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ParameterModulationFromHost)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor ModGain {
            input stream in;
            output stream out;
            input value float g = 1;
            process { out = in * g; }
        }
        graph G {
            input stream x;
            output stream y;
            input value float master = 1;
            node m = ModGain;
            connection { x -> m.in; m.out -> y; master -> m.g; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    auto input = makeRamp (16);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    for (int block = 0; block < 4; ++block)
    {
        const float gain = 0.5f + static_cast<float> (block) * 0.25f; // 0.5, 0.75, 1.0, 1.25
        graph.setParameter ("master", gain);
        runProcess32 (graph, inPtrs, outPtrs, 16);

        for (int i = 0; i < 16; ++i)
            EXPECT_NEAR (input[static_cast<size_t> (i)] * gain, output[static_cast<size_t> (i)], 1e-5f)
                << "block " << block << " sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MeterOutputsReflectLastBlock)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor RMS {
            input stream in;
            output stream out;
            output value float accum;
            process { out = in; accum = accum + in * in; }
        }
        graph G {
            input stream x;
            output stream y;
            output value float meter;
            node r = RMS;
            connection { x -> r.in; r.out -> y; r.accum -> meter; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    auto input = makeRamp (16, 0.5f);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 16);

    const auto meterVal = graph.getOutputValue ("meter");
    EXPECT_GT (meterVal, 0.0f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MultiBlockAccumulation)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Acc {
            input stream in;
            output stream out;
            state float sum;
            process { sum = sum + in; out = sum; }
        }
        graph G { input stream x; output stream y; node a = Acc; connection { x -> a.in; a.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    std::vector<float> input (16, 2.0f);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 16);

    float runningSum = 32.0f; // block 1: 16 * 2 = 32

    runProcess32 (graph, inPtrs, outPtrs, 16);
    runningSum += 32.0f; // block 2 -> running sum = 64
    runProcess32 (graph, inPtrs, outPtrs, 16);
    runningSum += 32.0f; // block 3 -> running sum = 96

    EXPECT_NEAR (runningSum, output[15], 1e-4f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, GraphWithMultipleInputsAndOutputs)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Mixer {
            input stream a;
            input stream b;
            output stream sum;
            output stream diff;
            process { sum = a + b; diff = a - b; }
        }
        graph Mix {
            input stream left, right;
            output stream mix, delta;
            node m = Mixer;
            connection {
                left -> m.a;
                right -> m.b;
                m.sum -> mix;
                m.diff -> delta;
            }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto left = makeRamp (32, 0.1f);
    auto right = makeRamp (32, 0.2f);

    std::vector<float> mixOut (32, 0.0f);
    std::vector<float> deltaOut (32, 0.0f);
    const float* inPtrs[] = { left.data(), right.data() };
    float* outPtrs[] = { mixOut.data(), deltaOut.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
    {
        const auto a = left[static_cast<size_t> (i)];
        const auto b = right[static_cast<size_t> (i)];
        EXPECT_NEAR (a + b, mixOut[static_cast<size_t> (i)], 1e-5f);
        EXPECT_NEAR (a - b, deltaOut[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, BlockModeCircularBufferDelay)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor CircDelay {
            input stream in;
            output stream out;
            state float buf[128];
            state int wp;
            process block {
                for i in 0..blockSize {
                    buf[wp] = in[i];
                    let rp = (wp >= 8) ? (wp - 8) : (wp - 8 + 128);
                    out[i] = buf[rp];
                    wp = (wp + 1) % 128;
                }
            }
        }
        graph G { input stream x; output stream y; node d = CircDelay; connection { x -> d.in; d.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR (0.0f, output[static_cast<size_t> (i)], 1e-6f) << "sample " << i;

    for (int i = 8; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i - 8)], output[static_cast<size_t> (i)], 1e-6f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, IdenticalGraphsProduceIdenticalOutput)
{
    DspJitCompiler compiler;

    auto source = R"YDSP(
        processor P { input stream in; output stream out; input value float g = 1; process { out = tanh (in * g); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    auto graphA = compilePatch (source, compiler);
    DspJitCompiler compilerB;
    auto graphB = compilePatch (source, compilerB);

    ASSERT_TRUE (graphA.isValid());
    ASSERT_TRUE (graphB.isValid());
    graphA.prepare (44100.0, 128);
    graphB.prepare (44100.0, 128);

    auto input = makeRamp (128, -1.0f);
    std::vector<float> outA (128), outB (128);
    const float* inPtrs[] = { input.data() };
    float* outAPtrs[] = { outA.data() };
    float* outBPtrs[] = { outB.data() };

    runProcess32 (graphA, inPtrs, outAPtrs, 128);
    runProcess32 (graphB, inPtrs, outBPtrs, 128);

    for (int i = 0; i < 128; ++i)
        EXPECT_NEAR (outA[static_cast<size_t> (i)], outB[static_cast<size_t> (i)], 1e-6f);

    dumpAsmOnFailure (graphA);
    dumpAsmOnFailure (graphB);
}

TEST (YdspJitGraphTests, HandlesSmallBlockSize)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P { input stream in; output stream out; process { out = in * 2; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 1);

    std::vector<float> input = { 0.5f };
    std::vector<float> output (1, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 1);

    EXPECT_NEAR (1.0f, output[0], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, HandlesLargeBlockSize)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    auto input = makeRamp (512);
    std::vector<float> output (512, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 512);

    for (int i = 0; i < 512; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)], 1e-6f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, InvalidGraphReturnsFalse)
{
    DspJitGraph graph;
    EXPECT_FALSE (graph.isValid());
}

TEST (YdspJitGraphTests, GetParamOnNonexistentReturnsZero)
{
    DspJitGraph graph;
    EXPECT_EQ (0.0f, graph.getParameter ("nonexistent"));
    EXPECT_FALSE (graph.hasParameter ("nonexistent"));
}

TEST (YdspJitGraphTests, GetOutputValueOnNonexistentReturnsZero)
{
    DspJitGraph graph;
    EXPECT_EQ (0.0f, graph.getOutputValue ("nonexistent"));
}

TEST (YdspJitGraphTests, HandlesPatchWithoutGraph)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
    )YDSP");

    EXPECT_TRUE (result.failed());
}

TEST (YdspJitGraphTests, RunsProcessorWithFunctionCall)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func doubleIt(x: float) : float {
                return x * 2.0;
            }
            process { out = doubleIt(in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsProcessorWithMultiParamFunction)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func lerp(a: float, b: float, t: float) : float {
                return a + (b - a) * t;
            }
            process { out = lerp(in, out', 0.5); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    float previous = 0.0f;
    for (int i = 0; i < 32; ++i)
    {
        const auto expected = 0.5f * input[static_cast<size_t> (i)] + 0.5f * previous;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
        previous = output[static_cast<size_t> (i)];
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, FunctionParameterMutationDoesNotClobberCallerLocal)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func scaleThenZero(t: float) : float {
                t = t * 2.0;
                return t;
            }
            process {
                float x = in;
                out = scaleThenZero(x) - x;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    auto input = makeRamp (32);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, FunctionParameterMutationPreservesStateStore)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            output stream out;
            state float phase;
            func halve(t: float) : float {
                t = t * 0.5;
                return t;
            }
            process {
                phase = phase + 0.25;
                if (phase >= 1.0) { phase = phase - 1.0; }
                out = halve(phase) * 2.0 + phase;
            }
        }
        graph G { output stream y; node p = P; connection { p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    std::vector<float> output (32, 0.0f);
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, nullptr, outPtrs, 32);

    float phase = 0.0f;
    for (int i = 0; i < 32; ++i)
    {
        phase = phase + 0.25f;
        if (phase >= 1.0f)
            phase = phase - 1.0f;

        EXPECT_NEAR (2.0f * phase, output[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, CompoundAssignmentOnAnIndexedTargetComputesCorrectly)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float buf[4];
            process {
                let idx = 2;
                buf[idx] += in;
                out = buf[idx];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    auto input = makeRamp (8, 1.0f);
    std::vector<float> output (8, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 8);

    float expected = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        expected += input[static_cast<size_t> (i)];
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RejectsRecursionTheAnalyzerMissesInsteadOfCrashing)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func fact(n: float) : float {
                float r = 1.0;
                if (n > 0.0) { r = n * fact(n - 1.0); }
                return r;
            }
            process { out = fact(in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP");

    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().hasErrors());
}

TEST (YdspJitGraphTests, RunsFloat64StreamGraph)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor F64Gain {
            input stream float64 in;
            output stream float64 out;
            input value float64 gain = 0.5;
            process { out = in * gain; }
        }
        graph G {
            input stream float64 x;
            output stream float64 y;
            input value float64 master = 2.0;
            node g = F64Gain;
            connection { x -> g.in; g.out -> y; master -> g.gain; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    EXPECT_EQ (DspJitElementType::float64, graph.getInputStreamType (0));
    EXPECT_EQ (DspJitElementType::float64, graph.getOutputStreamType (0));
    EXPECT_EQ (DspJitElementType::float64, graph.getParameterType ("master"));
    EXPECT_EQ (DspJitElementType::float64, graph.getParameterType ("g.gain"));

    graph.prepare (44100.0, 32);

    // The graph-level default (2.0) is copied to the node parameter.
    EXPECT_NEAR (2.0, graph.getDoubleParameter ("master"), 1e-12);
    EXPECT_NEAR (2.0, graph.getDoubleParameter ("g.gain"), 1e-12);

    std::vector<double> input (32);
    for (int i = 0; i < 32; ++i)
        input[static_cast<size_t> (i)] = 0.01 * static_cast<double> (i) - 0.15;

    std::vector<double> output (32, 0.0);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const double> (input.data(), 32) };
    std::vector<yup::DspJitOutputBuffer> outputBuffers { yup::Span<double> (output.data(), 32) };

    graph.process (inputBuffers, outputBuffers, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.0, output[static_cast<size_t> (i)], 1e-9);

    graph.setDoubleParameter ("master", 0.25);
    graph.process (inputBuffers, outputBuffers, 32);

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 0.25, output[static_cast<size_t> (i)], 1e-9);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsInt64ParamAndState)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Counter {
            input stream in;
            output stream out;
            input value int64 start = 0;
            state int64 counter;
            process {
                counter = start + 1;
                out = float32 (float64 (counter));
            }
        }
        graph G {
            input stream x;
            output stream y;
            input value int64 seed = 100;
            node c = Counter;
            connection { x -> c.in; c.out -> y; seed -> c.start; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    EXPECT_EQ (DspJitElementType::int64, graph.getParameterType ("seed"));
    EXPECT_EQ (100, graph.getIntParameter ("seed"));

    graph.prepare (44100.0, 16);

    std::vector<float> input (16, 0.0f);
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 16);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (101.0f, output[static_cast<size_t> (i)], 1e-4f);

    graph.setIntParameter ("seed", 7);
    runProcess32 (graph, inPtrs, outPtrs, 16);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (8.0f, output[static_cast<size_t> (i)], 1e-4f);

    dumpAsmOnFailure (graph);
}

//==============================================================================

TEST (YdspJitGraphTests, RunsBitwiseInt32Ops)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Bit {
            output stream float out;
            process {
                let a = 60;          // 0b111100
                let b = 10;          // 0b001010
                let s = 3;
                let and_ = a & b;
                let or_  = a | b;
                let xor_ = a ^ b;
                let shl_ = a << s;
                let shr_ = a >> s;
                let not_ = ~a & 255; // ~60 = -61 (0xFFFFFFC3), & 0xFF = 195
                out = float (and_ + or_ + xor_ + shl_ + shr_ + not_);
            }
        }
        graph G { output stream y; node b = Bit; connection { b.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    constexpr float expected = 8.0f + 62.0f + 54.0f + 480.0f + 7.0f + 195.0f;

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsHexAndBinaryIntegerLiterals)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Bit {
            output stream float out;
            process {
                let masked = 0x1FF & 0xFF;   // 511 & 255 = 255
                let bits   = 0b1010 << 2;    // 10 << 2 = 40
                let big    = 1_000 + 0x10;   // 1000 + 16 = 1016
                out = float (masked + bits + big);
            }
        }
        graph G { output stream y; node b = Bit; connection { b.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 32);

    constexpr float expected = 255.0f + 40.0f + 1016.0f;

    for (int i = 0; i < 32; ++i)
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsBitwiseInt64Ops)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Bit64 {
            output stream float out;
            process {
                let x = int64(-1);
                let y = int64(255);
                let masked = x & y;            // 255
                let hi = (x >> 63) & int64(1); // arithmetic shift: -1 >> 63 = -1, & 1 = 1
                let low = (~int64(0)) & y;     // ~0 = -1, & 255 = 255
                // Regression: `int64(1) << 32` must stay 64-bit; a 32-bit
                // shift wraps to 1 and the mask collapses to 0.
                let mask32 = (int64(1) << 32) - int64(1); // 0xFFFFFFFF
                let maskLsb = mask32 & int64(1);          // 1 (0 if the mask broke)
                let shl32 = (int64(1) << 32) >> 32;       // 1 (0 if the shift wrapped)
                out = float (masked + hi + low + maskLsb + shl32);
            }
        }
        graph G { output stream y; node b = Bit64; connection { b.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    constexpr float expected = 255.0f + 1.0f + 255.0f + 1.0f + 1.0f;

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, BitwisePrecedenceMatchesC)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Pre {
            output stream float out;
            process {
                let a = 1 << 2 + 3; // shift binds looser than additive: 1 << (2 + 3) = 32
                let b = 2 + 3 << 1; // (2 + 3) << 1 = 10
                let c = 1 | 2 & 4;  // 1 | (2 & 4) = 1
                let d = 1 & 2 | 4;  // (1 & 2) | 4 = 4
                let ex = 1 ^ 2 & 3; // 1 ^ (2 & 3) = 3
                let f = 4 | 2 ^ 1;  // 4 | (2 ^ 1) = 7
                out = float (a + b + c + d + ex + f);
            }
        }
        graph G { output stream y; node p = Pre; connection { p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    constexpr float expected = 32.0f + 10.0f + 1.0f + 4.0f + 3.0f + 7.0f;

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsCompoundBitwiseAssignment)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Acc {
            output stream float out;
            state int acc;
            process {
                acc &= 15;  // 0 -> 0
                acc |= 240; // -> 0xF0
                acc ^= 85;  // ^ 0x55 -> 0xA5 = 165
                out = float (acc);
            }
        }
        graph G { output stream y; node a = Acc; connection { a.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    int acc = 0;

    for (int i = 0; i < 64; ++i)
    {
        acc &= 15;
        acc |= 240;
        acc ^= 85;
        EXPECT_NEAR (static_cast<float> (acc), output[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsLfsrNoiseGenerator)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Lfsr {
            output stream float out;
            state int64 mem;
            process {
                if (mem == 0)
                    mem = int64(1);

                let w = mem;
                let feedback = (w ^ (w >> 5) ^ (w >> 13)) << 31;
                let masked = feedback & ((int64(1) << 32) - int64(1)); // keep low 32 bits
                let next = (w >> 1) | masked;
                out = float ((next & 1) * 2 - 1);
                mem = next;
            }
        }
        graph G { output stream y; node l = Lfsr; connection { l.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    int64_t w = 1;

    for (int i = 0; i < 64; ++i)
    {
        int64_t feedback = static_cast<int64_t> (static_cast<uint64_t> (w ^ (w >> 5) ^ (w >> 13)) << 31);
        feedback &= 0xFFFFFFFFLL;
        w = (w >> 1) | feedback;

        const float expected = (w & 1) != 0 ? 1.0f : -1.0f;
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 0.0f);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsMaskedRingBuffer)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Ring {
            input stream in;
            output stream out;
            state int wp;
            state float buf[8];
            process {
                wp = (wp + 1) & 7;
                buf[wp] = in;
                out = buf[(wp - 3) & 7]; // mask handles negative indices
            }
        }
        graph G { input stream x; output stream y; node r = Ring; connection { x -> r.in; r.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    std::vector<float> buf (8, 0.0f);
    int wp = 0;

    for (int i = 0; i < 64; ++i)
    {
        wp = (wp + 1) & 7;
        buf[static_cast<size_t> (wp)] = input[static_cast<size_t> (i)];
        const float expected = buf[static_cast<size_t> ((wp - 3) & 7)];
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
    }

    dumpAsmOnFailure (graph);
}

//==============================================================================

TEST (YdspJitGraphTests, RunsStructState)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor S {
            input stream in;
            output stream out;
            struct Voice { float phase; float buf[8]; int idx; }
            Voice mono;
            process {
                mono.idx = (mono.idx + 1) & 7;
                mono.buf[mono.idx] = in;
                out = mono.phase + mono.buf[(mono.idx - 3) & 7];
                mono.phase = mono.phase + 0.25;
            }
        }
        graph G { input stream x; output stream y; node s = S; connection { x -> s.in; s.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    float phase = 0.0f;
    std::vector<float> refBuf (8, 0.0f);
    int idx = 0;

    for (int i = 0; i < 64; ++i)
    {
        idx = (idx + 1) & 7;
        refBuf[static_cast<size_t> (idx)] = input[static_cast<size_t> (i)];
        const float expected = phase + refBuf[static_cast<size_t> ((idx - 3) & 7)];
        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-5f);
        phase += 0.25f;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsStructArray)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor SA {
            output stream float out;
            struct Voice { float freq; float phase; }
            Voice voices[8];
            process {
                voices[3].freq = 440;
                out = voices[3].freq + voices[7].phase;
                voices[7].phase = voices[7].phase + 1;
            }
        }
        graph G { output stream y; node s = SA; connection { s.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (440.0f + static_cast<float> (i), output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsInitBlock)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor I {
            input stream in;
            output stream out;
            state float gain;
            init { gain = 2.5; }
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node i = I; connection { x -> i.in; i.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 2.5f, output[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RunsInitWithStructFieldsAndReset)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor R {
            output stream float out;
            struct C { float acc; }
            C c;
            init { c.acc = 10; }
            process { out = c.acc; c.acc = c.acc + 1; }
        }
        graph G { output stream y; node r = R; connection { r.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { nullptr };
    float* outPtrs[] = { output.data() };

    runProcess32 (graph, inPtrs, outPtrs, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (10.0f + static_cast<float> (i), output[static_cast<size_t> (i)], 1e-5f);

    graph.reset();

    std::vector<float> output2 (64, 0.0f);
    float* outPtrs2[] = { output2.data() };

    runProcess32 (graph, inPtrs, outPtrs2, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (10.0f + static_cast<float> (i), output2[static_cast<size_t> (i)], 1e-5f);

    dumpAsmOnFailure (graph);
}

//==============================================================================
// MIDI-driven events, voice banks and sample-accurate automation

namespace
{

constexpr const char* activeVoiceSource = R"YDSP(
    processor ActiveVoice {
        output stream out;
        input event midi;
        state float active;
        event midi (e: noteOn) {
            active = active + 1.0;
        }
        event midi (e: noteOff) {
            active = active - 1.0;
        }
        process { out = active; }
    }
)YDSP";

constexpr const char* activeVoiceGraph = R"YDSP(
    graph G {
        input event midi;
        output stream y;
        node v = ActiveVoice[4];
        connection { midi -> v.midi; v.out -> y; }
    }
)YDSP";

yup::MidiBuffer makeNoteBuffer (const std::vector<std::tuple<int, int, float>>& events)
{
    yup::MidiBuffer buffer;

    for (const auto& [offset, pitch, velocity] : events)
    {
        const auto velocityByte = static_cast<uint8> (std::clamp (velocity, 0.0f, 1.0f) * 127.0f);

        if (velocity > 0.0f)
            buffer.addEvent (yup::MidiMessage::noteOn (1, pitch, velocityByte), offset);
        else
            buffer.addEvent (yup::MidiMessage::noteOff (1, pitch), offset);
    }

    return buffer;
}

} // namespace

TEST (YdspJitGraphTests, MonosynthReceivesMidiNoteOn)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ActiveVoice[1];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, FourNoteOnsLandInFourDistinctVoices)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f }, { 0, 67, 1.0f }, { 0, 71, 1.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (4.0f, output[static_cast<size_t> (i)]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, HeldNotesSurviveEventFreeBlocks)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int b = 0; b < 4; ++b)
    {
        runProcess (graph, nullptr, 0, outPtrs, 1, 64);

        for (int i = 0; i < 64; ++i)
            EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "block " << b << " sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, FifthNoteOnStealsOldestTriggeredVoice)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f }, { 0, 67, 1.0f }, { 0, 71, 1.0f }, { 0, 74, 1.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (4.0f, output[static_cast<size_t> (i)]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SecondNoteOnAtSamePitchAndChannelRetriggers)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 60, 1.0f }, { 10, 60, 0.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int i = 0; i < 10; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "held at " << i;

    for (int i = 10; i < 64; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "released at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SamePitchOnTwoMpeMemberChannelsStaysIndependent)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());

    yup::MPEZoneLayout layout;
    layout.setLowerZone (15);
    graph.setMpeZoneLayout (layout);

    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (2, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::noteOn (3, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::noteOff (3, 60), 10);

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int i = 0; i < 10; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "both held at " << i;

    for (int i = 10; i < 64; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "one held at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, NoteOnAndNoteOffInSameBlockFireAtExactSampleOffsets)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 100, 60, 1.0f }, { 300, 60, 0.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "before note-on at " << i;

    for (int i = 100; i < 300; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "between note-on and note-off at " << i;

    for (int i = 300; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "after note-off at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AutomationEventChangesParamAtExactSampleOffset)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor GainVoice {
            output stream out;
            input event midi;
            input value float gain = 0.5;
            state float env;
            event midi (e: noteOn) {
                env = e.velocity;
            }
            process { out = env * gain; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = GainVoice;
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f } });

    const auto gainSlot = graph.getParameterSlot ("v.gain");
    ASSERT_GE (gainSlot, 0);

    DspJitAutomationEvent automation { gainSlot, 100, 1.0f };

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi, &automation, 1);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "before automation at " << i;

    for (int i = 100; i < 512; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "after automation at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, PolyphonicAutomationAppliesTheSameTimelineToEveryVoice)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor GainVoice {
            output stream out;
            input event midi;
            input value float gain = 0.5;
            state float env;
            event midi (e: noteOn) { env = e.velocity; }
            process { out = env * gain; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = GainVoice[4];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f } });

    const auto gainSlot = graph.getParameterSlot ("v.gain");
    ASSERT_GE (gainSlot, 0);

    const DspJitAutomationEvent automation { gainSlot, 100, 1.0f };

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi, &automation, 1);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "before automation at " << i;

    for (int i = 100; i < 512; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "after automation at " << i;

    EXPECT_FLOAT_EQ (1.0f, graph.getParameter ("v.gain"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SmoothedAutomationRampsInsteadOfStepping)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ name: "Gain", min: 0.0, max: 1.0, smoothing: 0.0005 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    const std::vector<float> input (512, 1.0f);
    const float* inPtrs[] = { input.data() };

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    runProcess (graph, inPtrs, 1, outPtrs, 1, 512);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "primed block at " << i;

    const auto gainSlot = graph.getParameterSlot ("p.gain");
    ASSERT_GE (gainSlot, 0);

    DspJitAutomationEvent automation { gainSlot, 100, 1.0f };

    std::fill (output.begin(), output.end(), 0.0f);
    runProcess (graph, inPtrs, 1, outPtrs, 1, 512, nullptr, &automation, 1);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "before automation at " << i;

    EXPECT_GT (output[100], 0.5f);
    EXPECT_LT (output[100], 0.6f);

    for (int i = 101; i < 512; ++i)
    {
        const auto previous = output[static_cast<size_t> (i - 1)];
        const auto current = output[static_cast<size_t> (i)];

        EXPECT_GE (current, previous) << "not monotone at " << i;

        if (previous < 1.0f)
            EXPECT_GT (current, previous) << "stalled short of the target at " << i;
    }

    EXPECT_FLOAT_EQ (1.0f, output[500]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ResetRePrimesSmoothedParameter)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ smoothing: 0.0005 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    const std::vector<float> input (512, 1.0f);
    const float* inPtrs[] = { input.data() };

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    graph.setParameter ("p.gain", 0.25f);

    graph.reset();

    runProcess (graph, inPtrs, 1, outPtrs, 1, 512);

    EXPECT_FLOAT_EQ (0.25f, output[0]);
    EXPECT_FLOAT_EQ (0.25f, output[511]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SmoothsAProcessorParameterDrivenByAGraphEndpoint)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ smoothing: 0.0005 ]];
            process { out = in * gain; }
        }
        graph G {
            input stream x;
            output stream y;
            input value float level = 0.5 [[ name: "Level", min: 0.0, max: 1.0 ]];
            node p = Gain;
            connection { x -> p.in; p.out -> y; level -> p.gain; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    const std::vector<float> input (512, 1.0f);
    const float* inPtrs[] = { input.data() };

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    runProcess (graph, inPtrs, 1, outPtrs, 1, 512);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "primed block at " << i;

    const auto levelSlot = graph.getParameterSlot ("level");
    ASSERT_GE (levelSlot, 0);

    DspJitAutomationEvent automation { levelSlot, 100, 1.0f };

    std::fill (output.begin(), output.end(), 0.0f);
    runProcess (graph, inPtrs, 1, outPtrs, 1, 512, nullptr, &automation, 1);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "before automation at " << i;

    EXPECT_GT (output[100], 0.5f);
    EXPECT_LT (output[100], 0.6f);
    EXPECT_FLOAT_EQ (1.0f, output[500]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ConstantSmoothedParameterMatchesUnsmoothedOutput)
{
    DspJitCompiler smoothedCompiler;

    auto smoothed = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.375 [[ smoothing: 0.02 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                  smoothedCompiler);

    DspJitCompiler steppedCompiler;

    auto stepped = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.375;
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 steppedCompiler);

    ASSERT_TRUE (smoothed.isValid());
    ASSERT_TRUE (stepped.isValid());

    smoothed.prepare (44100.0, 256);
    stepped.prepare (44100.0, 256);

    const auto input = makeRamp (256, -1.0f);
    const float* inPtrs[] = { input.data() };

    std::vector<float> smoothedOut (256, 0.0f);
    std::vector<float> steppedOut (256, 0.0f);
    float* smoothedPtrs[] = { smoothedOut.data() };
    float* steppedPtrs[] = { steppedOut.data() };

    runProcess (smoothed, inPtrs, 1, smoothedPtrs, 1, 256);
    runProcess (stepped, inPtrs, 1, steppedPtrs, 1, 256);

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (steppedOut[static_cast<size_t> (i)], smoothedOut[static_cast<size_t> (i)]) << "at " << i;

    dumpAsmOnFailure (smoothed);
}

TEST (YdspJitGraphTests, SmoothIntrinsicMatchesSmoothingAnnotation)
{
    DspJitCompiler sugarCompiler;

    auto sugar = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.5 [[ smoothing: 0.001 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               sugarCompiler);

    DspJitCompiler intrinsicCompiler;

    auto intrinsic = compilePatch (R"YDSP(
        processor Gain {
            input stream in;
            output stream out;
            input value float gain = 0.5;
            process {
                float gainSmoothed = smooth (gain, 0.001);
                out = in * gainSmoothed;
            }
        }
        graph G { input stream x; output stream y; node p = Gain; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                   intrinsicCompiler);

    ASSERT_TRUE (sugar.isValid());
    ASSERT_TRUE (intrinsic.isValid());

    sugar.prepare (44100.0, 512);
    intrinsic.prepare (44100.0, 512);

    const std::vector<float> input (512, 1.0f);
    const float* inPtrs[] = { input.data() };

    std::vector<float> sugarOut (512, 0.0f);
    std::vector<float> intrinsicOut (512, 0.0f);
    float* sugarPtrs[] = { sugarOut.data() };
    float* intrinsicPtrs[] = { intrinsicOut.data() };

    const auto sugarSlot = sugar.getParameterSlot ("p.gain");
    const auto intrinsicSlot = intrinsic.getParameterSlot ("p.gain");
    ASSERT_GE (sugarSlot, 0);
    ASSERT_GE (intrinsicSlot, 0);

    const DspJitAutomationEvent sugarAutomation { sugarSlot, 64, 1.0f };
    const DspJitAutomationEvent intrinsicAutomation { intrinsicSlot, 64, 1.0f };

    runProcess (sugar, inPtrs, 1, sugarPtrs, 1, 512, nullptr, &sugarAutomation, 1);
    runProcess (intrinsic, inPtrs, 1, intrinsicPtrs, 1, 512, nullptr, &intrinsicAutomation, 1);

    for (int i = 0; i < 512; ++i)
        EXPECT_EQ (intrinsicOut[static_cast<size_t> (i)], sugarOut[static_cast<size_t> (i)]) << "at " << i;

    dumpAsmOnFailure (sugar);
}

TEST (YdspJitGraphTests, BlockWithNoSplitPointsTakesSingleKernelCall)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Counter {
            output stream out;
            input event midi;
            state float calls;
            event midi (e: noteOn) { }
            process block {
                calls = calls + 1.0;
                out[0] = calls;
            }
        }
        graph G {
            input event midi;
            output stream y;
            node c = Counter;
            connection { midi -> c.midi; c.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    {
        std::vector<float> output (64, 0.0f);
        float* outPtrs[] = { output.data() };

        runProcess (graph, nullptr, 0, outPtrs, 1, 64);

        EXPECT_FLOAT_EQ (1.0f, output[0]);
        for (int i = 1; i < 64; ++i)
            EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]);
    }

    graph.reset();

    {
        std::vector<float> output (64, 0.0f);
        float* outPtrs[] = { output.data() };

        const auto midi = makeNoteBuffer ({ { 20, 60, 1.0f } });

        runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

        EXPECT_FLOAT_EQ (1.0f, output[0]);
        EXPECT_FLOAT_EQ (2.0f, output[20]);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ThreeArgProcessMatchesNewOverloadWithNullEvents)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            input value float a = 0.5;
            process { out = (1 - a) * in + a * out'; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);

    std::vector<float> outA (64, 0.0f);
    std::vector<float> outB (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (input.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outBuffersA { yup::Span<float> (outA.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outBuffersB { yup::Span<float> (outB.data(), 64) };

    for (int b = 0; b < 4; ++b)
    {
        graph.process (inputBuffers, outBuffersA, 64);
        graph.reset();

        graph.process (inputBuffers, outBuffersB, 64, nullptr, nullptr, 0);
        graph.reset();

        for (int i = 0; i < 64; ++i)
            EXPECT_FLOAT_EQ (outA[static_cast<size_t> (i)], outB[static_cast<size_t> (i)]);
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MidiOutOverloadsForwardAndLeaveBufferEmptyWithoutOutputEvents)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            input value float a = 0.5;
            process { out = (1 - a) * in + a * out'; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    auto input = makeRamp (64);

    std::vector<float> outA (64, 0.0f);
    std::vector<float> outB (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (input.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outBuffersA { yup::Span<float> (outA.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outBuffersB { yup::Span<float> (outB.data(), 64) };

    yup::MidiBuffer midiOut;

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outBuffersA, 64, nullptr, nullptr, 0));
    graph.reset();

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outBuffersB, 64, nullptr, nullptr, 0, &midiOut));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (outA[static_cast<size_t> (i)], outB[static_cast<size_t> (i)]);

    EXPECT_TRUE (midiOut.isEmpty());

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, IgnoresMismatchedStreamBuffers)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Gain { input stream in; output stream out; process { out = in * 2; } }
        graph G { input stream x; output stream y; node g = Gain; connection { x -> g.in; g.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> input (64, 0.5f);
    std::vector<float> output (64, 0.0f);

    std::vector<yup::DspJitOutputBuffer> outBuf { yup::Span<float> (output.data(), 64) };

    std::vector<double> wrongInput (64, 0.5);
    std::vector<yup::DspJitInputBuffer> wrongIn { yup::Span<const double> (wrongInput.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::bufferTypeMismatch, graph.process (wrongIn, outBuf, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]);

    std::vector<float> shortInput (32, 0.5f);
    std::vector<yup::DspJitInputBuffer> shortIn { yup::Span<const float> (shortInput.data(), 32) };

    EXPECT_EQ (yup::DspJitProcessResult::bufferTooShort, graph.process (shortIn, outBuf, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]);

    std::vector<yup::DspJitInputBuffer> extraIn {
        yup::Span<const float> (input.data(), 64),
        yup::Span<const float> (input.data(), 64)
    };

    EXPECT_EQ (yup::DspJitProcessResult::invalidBufferCount, graph.process (extraIn, outBuf, 64));

    std::vector<yup::DspJitInputBuffer> okIn { yup::Span<const float> (input.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (okIn, outBuf, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ExposesParamMetadataForUi)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input value float cutoff = 1500.0 [[ name: "Cutoff", min: 60.0, max: 12000.0 ]];
            input value float decay = 0.25;
            process { out = cutoff * decay; }
        }
        graph G {
            output stream y;
            input value float master = 0.8 [[ name: "Master Volume", min: 0.0, max: 1.0 ]];
            node v = Voice;
            connection { v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    ASSERT_EQ (0, graph.getInputStreamCount());
    ASSERT_EQ (1, graph.getOutputStreamCount());
    ASSERT_EQ (3, graph.getParameterCount());

    const auto master = graph.getParameterInfo (0);
    EXPECT_EQ ("master", master.name);
    EXPECT_EQ ("Master Volume", master.displayName);
    EXPECT_EQ (DspJitElementType::float32, master.type);
    EXPECT_NEAR (0.8, master.defaultValue, 1e-6);
    EXPECT_NEAR (0.0, master.minValue, 1e-6);
    EXPECT_NEAR (1.0, master.maxValue, 1e-6);

    EXPECT_EQ (1, graph.getParameterSlot ("v.cutoff"));

    const auto cutoff = graph.getParameterInfo (1);
    EXPECT_EQ ("v.cutoff", cutoff.name);
    EXPECT_EQ ("Cutoff", cutoff.displayName);
    EXPECT_NEAR (1500.0, cutoff.defaultValue, 1e-6);
    EXPECT_NEAR (60.0, cutoff.minValue, 1e-6);
    EXPECT_NEAR (12000.0, cutoff.maxValue, 1e-6);

    const auto decay = graph.getParameterInfo (2);
    EXPECT_EQ ("v.decay", decay.name);
    EXPECT_EQ ("decay", decay.displayName);
    EXPECT_NEAR (0.25, decay.defaultValue, 1e-6);
    EXPECT_NEAR (0.0, decay.minValue, 1e-6);
    EXPECT_NEAR (1.0, decay.maxValue, 1e-6);

    EXPECT_TRUE (graph.getParameterInfo (99).name.isEmpty());

    auto aliased = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input value float drive = 1.0;
            process { out = drive; }
        }
        graph G {
            output stream y;
            input value float master = 0.5;
            node v = Voice;
            connection { v.out -> y; master -> v.drive; }
        }
    )YDSP",
                                 compiler);

    ASSERT_TRUE (aliased.isValid());
    ASSERT_EQ (1, aliased.getParameterCount());
    EXPECT_EQ ("master", aliased.getParameterInfo (0).name);
    EXPECT_EQ (aliased.getParameterSlot ("master"), aliased.getParameterSlot ("v.drive"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ExposesUnitStepAndStyleForUi)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input value float cutoff = 1500.0 [[ name: "Cutoff", min: 60.0, max: 12000.0, unit: "Hz", step: 10.0, style: "knob" ]];
            input value float decay = 0.25;
            process { out = cutoff * decay; }
        }
        graph G { output stream y; node v = Voice; connection { v.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    const auto cutoff = graph.getParameterInfo (graph.getParameterSlot ("v.cutoff"));
    EXPECT_EQ ("Hz", cutoff.unit);
    EXPECT_NEAR (10.0, cutoff.stepSize, 1e-6);
    EXPECT_EQ ("knob", cutoff.style);

    const auto decay = graph.getParameterInfo (graph.getParameterSlot ("v.decay"));
    EXPECT_TRUE (decay.unit.isEmpty());
    EXPECT_NEAR (0.0, decay.stepSize, 1e-6);
    EXPECT_TRUE (decay.style.isEmpty());

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ExposesDiscreteValuesForUi)

{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input value float wave = 0.0 [[ name: "Waveform", min: 0.0, max: 3.0, values: { "Saw", "Square", "Triangle", "Pulse" } ]];
            process { out = wave; }
        }
        graph G {
            output stream y;
            node v = Voice;
            connection { v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    ASSERT_EQ (1, graph.getParameterCount());

    const auto info = graph.getParameterInfo (0);
    ASSERT_EQ (4, info.discreteValues.size());
    EXPECT_EQ ("Saw", info.discreteValues[0]);
    EXPECT_EQ ("Square", info.discreteValues[1]);
    EXPECT_EQ ("Triangle", info.discreteValues[2]);
    EXPECT_EQ ("Pulse", info.discreteValues[3]);
    EXPECT_TRUE (info.isDiscrete());

    EXPECT_NEAR (0.75, (info.maxValue - info.minValue) / 4.0, 1e-9);
    EXPECT_EQ ("Saw", info.labelForValue (0.0));
    EXPECT_EQ ("Triangle", info.labelForValue (2.1));
    EXPECT_EQ ("Pulse", info.labelForValue (3.0));
    EXPECT_EQ ("Saw", info.labelForValue (-5.0));   // clamps below the range
    EXPECT_EQ ("Pulse", info.labelForValue (99.0)); // clamps above the range

    auto continuous = compilePatch (R"YDSP(
        processor Voice {
            output stream out;
            input value float cutoff = 1500.0 [[ name: "Cutoff", min: 60.0, max: 12000.0 ]];
            process { out = cutoff; }
        }
        graph G {
            output stream y;
            node v = Voice;
            connection { v.out -> y; }
        }
    )YDSP",
                                    compiler);

    ASSERT_TRUE (continuous.isValid());

    const auto plain = continuous.getParameterInfo (0);
    EXPECT_TRUE (plain.discreteValues.isEmpty());
    EXPECT_FALSE (plain.isDiscrete());
    EXPECT_TRUE (plain.labelForValue (100.0).isEmpty());

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RejectsMalformedDiscreteValuesAnnotation)
{
    DspJitCompiler compiler;

    auto tooFew = compiler.compile (R"YDSP(
        processor Voice {
            output stream out;
            input value float wave = 0.0 [[ values: { "Only" } ]];
            process { out = wave; }
        }
        graph G {
            output stream y;
            node v = Voice;
            connection { v.out -> y; }
        }
    )YDSP");

    EXPECT_TRUE (tooFew.failed());
    EXPECT_TRUE (compiler.getDiagnostics().hasErrors());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("[[ values ]]"));

    auto tooFewOnGraph = compiler.compile (R"YDSP(
        processor Voice {
            output stream out;
            input value float wave = 0.0;
            process { out = wave; }
        }
        graph G {
            output stream y;
            input value float w = 0.0 [[ values: { "Only" } ]];
            node v = Voice;
            connection { v.out -> y; w -> v.wave; }
        }
    )YDSP");

    EXPECT_TRUE (tooFewOnGraph.failed());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("[[ values ]]"));
}

TEST (YdspJitGraphTests, ExposesDiscreteValuesOnAGraphParameterAliasedOntoNodes)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Band {
            input stream in;
            output stream out;
            input value float vowel = 0.0 [[ name: "Vowel", min: 0.0, max: 2.0,
                                             values: { "A", "E", "I" } ]];
            process { out = in * vowel; }
        }
        graph G {
            input stream x;
            output stream y;

            input value float vowel = 0.0 [[ name: "Vowel", min: 0.0, max: 2.0,
                                             values: { "A", "E", "I" } ]];

            node a = Band;
            node b = Band;

            connection {
                x -> a.in;
                a.out -> b.in;
                b.out -> y;

                vowel -> a.vowel;
                vowel -> b.vowel;
            }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    ASSERT_EQ (1, graph.getParameterCount());

    const auto& info = graph.getParameterInfo (0);

    EXPECT_EQ ("vowel", info.name);
    EXPECT_TRUE (info.isDiscrete());
    ASSERT_EQ (3, info.discreteValues.size());
    EXPECT_EQ ("A", info.discreteValues[0]);
    EXPECT_EQ ("E", info.discreteValues[1]);
    EXPECT_EQ ("I", info.discreteValues[2]);

    EXPECT_EQ ("A", info.labelForValue (0.0));
    EXPECT_EQ ("I", info.labelForValue (2.0));

    graph.setParameter ("vowel", 2.0f);
    EXPECT_FLOAT_EQ (2.0f, graph.getParameter ("a.vowel"));
    EXPECT_FLOAT_EQ (2.0f, graph.getParameter ("b.vowel"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, DroppedEventsAreCountedNotAllocated)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());

    graph.prepare (44100.0, 64, 1, 1);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f }, { 0, 67, 1.0f }, { 0, 71, 1.0f }, { 0, 74, 1.0f }, { 0, 76, 1.0f }, { 0, 79, 1.0f }, { 0, 81, 1.0f } });

    const auto droppedBefore = graph.getDroppedEventCount();

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    EXPECT_GT (graph.getDroppedEventCount(), droppedBefore);
}

TEST (YdspJitGraphTests, AutomatingANonFloat32ParameterIsCountedAsDropped)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input value int mode = 0;
            process { out = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    const auto slot = graph.getParameterSlot ("p.mode");
    ASSERT_GE (slot, 0);

    std::vector<float> input (32, 0.0f);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    const DspJitAutomationEvent automation[] = { { slot, 0, 1.0f } };

    const auto droppedBefore = graph.getDroppedEventCount();

    runProcess (graph, inPtrs, 1, outPtrs, 1, 32, nullptr, automation, 1);

    EXPECT_GT (graph.getDroppedEventCount(), droppedBefore);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, OutputEventQueueAcceptsUpToReservedCapacityThenDrops)
{
    yup::YdspOutputEventQueue queue;
    queue.entries.reserve (4);

    const auto capacity = queue.entries.capacity();

    for (size_t i = 0; i < capacity; ++i)
    {
        queue.staging.pitch = 60.0f + static_cast<float> (i);
        ydspCommitOutputEvent (&queue, 1, static_cast<int32_t> (i) * 8, 0);
    }

    ASSERT_EQ (capacity, queue.entries.size());
    EXPECT_EQ (capacity, queue.entries.capacity());
    EXPECT_EQ (0u, queue.droppedCount.load (std::memory_order_relaxed));

    EXPECT_EQ (0, queue.entries[0].sampleOffset);
    EXPECT_EQ (int64_t (1), queue.entries[0].shapeTag);
    EXPECT_FLOAT_EQ (60.0f, queue.entries[0].fields.pitch);

    const auto lastIndex = capacity - 1;
    EXPECT_EQ (static_cast<int32_t> (lastIndex) * 8, queue.entries[lastIndex].sampleOffset);
    EXPECT_FLOAT_EQ (60.0f + static_cast<float> (lastIndex), queue.entries[lastIndex].fields.pitch);

    queue.staging.pitch = 99.0f;
    ydspCommitOutputEvent (&queue, 1, static_cast<int32_t> (capacity) * 8, 0);

    EXPECT_EQ (capacity, queue.entries.size());
    EXPECT_EQ (capacity, queue.entries.capacity());
    EXPECT_EQ (1u, queue.droppedCount.load (std::memory_order_relaxed));
    EXPECT_FLOAT_EQ (60.0f + static_cast<float> (lastIndex), queue.entries[lastIndex].fields.pitch);
}

//==============================================================================
// MIDI expression, MPE, voice modes and channel-mode messages

namespace
{

constexpr const char* expressiveVoiceSource = R"YDSP(
    processor ExpressiveVoice {
        output stream out;
        input event midi;
        state float active;
        state float bend;
        state float press;
        state float timbre;
        event midi (e: noteOn) { active = 1.0; }
        event midi (e: noteOff) { active = 0.0; }
        event midi (e: pitchBend) { bend = e.bendSemitones; }
        event midi (e: pressure) { press = e.pressure; }
        event midi (e: slide) { timbre = e.slide; }
        process { out = active * (bend + press + timbre); }
    }
)YDSP";

constexpr const char* controlVoiceSource = R"YDSP(
    processor ControlVoice {
        output stream out;
        input event midi;
        state float active;
        state float cc;
        state float ccValue;
        state float program;
        event midi (e: noteOn) { active = 1.0; }
        event midi (e: noteOff) { active = 0.0; }
        event midi (e: controlChange) { cc = float (e.control); ccValue = e.value; }
        event midi (e: programChange) { program = float (e.program); }
        process { out = active * (cc + ccValue + program); }
    }
)YDSP";

constexpr const char* monoVoiceSource = R"YDSP(
    processor MonoVoice {
        output stream out;
        input event midi;
        state float sounding;
        event midi (e: noteOn) {
            sounding = e.pitch;
            if (e.isLegato) { sounding = sounding + 1000.0; }
        }
        event midi (e: noteOff) { sounding = 0.0; }
        process { out = sounding; }
    }
)YDSP";

} // namespace

TEST (YdspJitGraphTests, ControlChangeReachesEveryVoiceAtItsSampleOffset)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (controlVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ControlVoice;
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 1, 127), 100);

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "before CC at " << i;

    for (int i = 100; i < 256; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "after CC at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ProgramChangeReachesTheHandler)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (controlVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ControlVoice;
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::programChange (1, 7), 0);
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (7.0f, output[static_cast<size_t> (i)]) << "at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ChannelPitchBendAndPressureBroadcastInLegacyMode)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (expressiveVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ExpressiveVoice[2];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.setLegacyMidiMode (12); // a full-octave bend range makes the maths exact
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::noteOn (1, 64, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::pitchWheel (1, 16383), 100);
    midi.addEvent (yup::MidiMessage::channelPressureChange (1, 127), 150);

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "before bend at " << i;

    for (int i = 100; i < 150; ++i)
        EXPECT_NEAR (24.0f, output[static_cast<size_t> (i)], 1e-3f) << "bend at " << i;

    for (int i = 150; i < 256; ++i)
        EXPECT_NEAR (26.0f, output[static_cast<size_t> (i)], 1e-3f) << "pressure at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, PolyAftertouchFoldsIntoTheAffectedNotesPressure)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (expressiveVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ExpressiveVoice[2];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::noteOn (1, 64, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::aftertouchChange (1, 60, 127), 100);

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "before aftertouch at " << i;

    for (int i = 100; i < 256; ++i)
        EXPECT_NEAR (1.0f, output[static_cast<size_t> (i)], 1e-3f) << "after aftertouch at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, PerNoteMpeExpressionReachesOnlyTheOwningVoice)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (expressiveVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ExpressiveVoice[4];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    yup::MPEZoneLayout layout;
    layout.setLowerZone (15, 12, 2); // 12-semitone per-note bend range
    graph.setMpeZoneLayout (layout);

    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (2, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::noteOn (3, 64, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::pitchWheel (2, 16383), 100);        // member channel 2 only
    midi.addEvent (yup::MidiMessage::controllerEvent (3, 74, 127), 150); // slide on channel 3 only

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "before expression at " << i;

    for (int i = 100; i < 150; ++i)
        EXPECT_NEAR (12.0f, output[static_cast<size_t> (i)], 1e-3f) << "per-note bend at " << i;

    for (int i = 150; i < 256; ++i)
        EXPECT_NEAR (13.0f, output[static_cast<size_t> (i)], 1e-3f) << "per-note slide at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ExpressionForAnUnownedNoteIsDiscardedAndCounted)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (expressiveVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ExpressiveVoice[1] [[ stealing: none ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    yup::MPEZoneLayout layout;
    layout.setLowerZone (15, 12, 2);
    graph.setMpeZoneLayout (layout);

    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (2, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::noteOn (3, 64, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::pitchWheel (3, 16383), 100);

    const auto droppedBefore = graph.getDroppedEventCount();

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "at " << i;

    EXPECT_GT (graph.getDroppedEventCount(), droppedBefore);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, StealingPolicySelectsWhichVoiceIsReused)
{
    struct Policy
    {
        const char* annotation;
        float afterFirstNoteOff;
        float afterSecondNoteOff;
    };

    for (const auto& policy : { Policy { "stealing: oldest", 2.0f, 1.0f },
                                Policy { "stealing: newest", 1.0f, 1.0f },
                                Policy { "stealing: none", 1.0f, 0.0f } })
    {
        DspJitCompiler compiler;

        auto graph = compilePatch (std::string (activeVoiceSource) + "graph G { input event midi; output stream y; node v = ActiveVoice[2] [[ " + policy.annotation + " ]]; connection { midi -> v.midi; v.out -> y; } }", compiler);

        ASSERT_TRUE (graph.isValid()) << policy.annotation;
        graph.prepare (44100.0, 64);

        std::vector<float> output (64, 0.0f);
        float* outPtrs[] = { output.data() };

        const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f }, { 10, 67, 1.0f }, { 20, 60, 0.0f }, { 40, 64, 0.0f } });

        runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi);

        for (int i = 10; i < 20; ++i)
            EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << policy.annotation << " after the third note-on at " << i;

        for (int i = 20; i < 40; ++i)
            EXPECT_FLOAT_EQ (policy.afterFirstNoteOff, output[static_cast<size_t> (i)]) << policy.annotation << " after note-off 60 at " << i;

        for (int i = 40; i < 64; ++i)
            EXPECT_FLOAT_EQ (policy.afterSecondNoteOff, output[static_cast<size_t> (i)]) << policy.annotation << " after note-off 64 at " << i;
    }
}

TEST (YdspJitGraphTests, MonoNodeFollowsTheHeldNoteStackWithLegato)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (monoVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node bass = MonoVoice [[ mode: mono, priority: last ]];
            connection { midi -> bass.midi; bass.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 40, 1.0f }, { 100, 47, 1.0f }, { 200, 52, 1.0f }, { 300, 52, 0.0f }, { 400, 47, 0.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (40.0f, output[static_cast<size_t> (i)]) << "first note at " << i;

    for (int i = 100; i < 200; ++i)
        EXPECT_FLOAT_EQ (1047.0f, output[static_cast<size_t> (i)]) << "legato to 47 at " << i;

    for (int i = 200; i < 300; ++i)
        EXPECT_FLOAT_EQ (1052.0f, output[static_cast<size_t> (i)]) << "legato to 52 at " << i;

    for (int i = 300; i < 400; ++i)
        EXPECT_FLOAT_EQ (1047.0f, output[static_cast<size_t> (i)]) << "fall back to 47 at " << i;

    for (int i = 400; i < 512; ++i)
        EXPECT_FLOAT_EQ (1040.0f, output[static_cast<size_t> (i)]) << "fall back to 40 at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MonoNotePriorityChoosesTheSoundingNote)
{
    for (const auto& [priority, expected] : { std::pair<const char*, float> { "low", 40.0f },
                                              std::pair<const char*, float> { "high", 1052.0f } })
    {
        DspJitCompiler compiler;

        auto graph = compilePatch (std::string (monoVoiceSource) + "graph G { input event midi; output stream y; node bass = MonoVoice [[ mode: mono, priority: " + priority + " ]]; connection { midi -> bass.midi; bass.out -> y; } }", compiler);

        ASSERT_TRUE (graph.isValid()) << priority;
        graph.prepare (44100.0, 256);

        std::vector<float> output (256, 0.0f);
        float* outPtrs[] = { output.data() };

        const auto midi = makeNoteBuffer ({ { 0, 40, 1.0f }, { 100, 52, 1.0f } });

        runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

        for (int i = 0; i < 100; ++i)
            EXPECT_FLOAT_EQ (40.0f, output[static_cast<size_t> (i)]) << priority << " at " << i;

        for (int i = 100; i < 256; ++i)
            EXPECT_FLOAT_EQ (expected, output[static_cast<size_t> (i)]) << priority << " at " << i;
    }
}

TEST (YdspJitGraphTests, SustainPedalHoldsAReleasedNoteWithoutYdspSustainState)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 64, 127), 0); // sustain down
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 10);
    midi.addEvent (yup::MidiMessage::noteOff (1, 60), 100);            // withheld by the pedal
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 64, 0), 300); // sustain up

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 10; i < 300; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "sustained at " << i;

    for (int i = 300; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "released at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SostenutoPedalHoldsOnlyTheNotesHeldWhenItWentDown)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 66, 127), 50); // sostenuto captures note 60
    midi.addEvent (yup::MidiMessage::noteOn (1, 64, static_cast<uint8> (100)), 100);
    midi.addEvent (yup::MidiMessage::noteOff (1, 64), 200);            // not captured: released now
    midi.addEvent (yup::MidiMessage::noteOff (1, 60), 250);            // captured: held
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 66, 0), 400); // sostenuto up

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 100; i < 200; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "both held at " << i;

    for (int i = 250; i < 400; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "only the captured note at " << i;

    for (int i = 400; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "released at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AllNotesOffAndResetAllControllersReleaseWithoutZeroingState)
{
    for (const int controller : { 123, 121 })
    {
        DspJitCompiler compiler;

        auto graph = compilePatch (std::string (activeVoiceSource) + activeVoiceGraph, compiler);

        ASSERT_TRUE (graph.isValid());
        graph.prepare (44100.0, 512);

        std::vector<float> output (512, 0.0f);
        float* outPtrs[] = { output.data() };

        yup::MidiBuffer midi;
        midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
        midi.addEvent (yup::MidiMessage::noteOn (1, 64, static_cast<uint8> (100)), 0);
        midi.addEvent (yup::MidiMessage::controllerEvent (1, controller, 0), 100);

        runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

        for (int i = 0; i < 100; ++i)
            EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "CC" << controller << " before at " << i;

        for (int i = 100; i < 512; ++i)
            EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "CC" << controller << " after at " << i;
    }
}

TEST (YdspJitGraphTests, AllSoundOffSilencesAndReRunsInit)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor InitVoice {
            output stream out;
            input event midi;
            state float active;
            state float seed;
            init { seed = 0.25; }
            event midi (e: noteOn) { active = 1.0; }
            event midi (e: noteOff) { active = 2.0; }
            process { out = active + seed; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = InitVoice;
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 200);

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 0; i < 200; ++i)
        EXPECT_FLOAT_EQ (1.25f, output[static_cast<size_t> (i)]) << "before all-sound-off at " << i;

    for (int i = 200; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.25f, output[static_cast<size_t> (i)]) << "after all-sound-off at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, DenseMpeTrafficDoesNotAllocateDuringProcess)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (std::string (expressiveVoiceSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node v = ExpressiveVoice[8];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    yup::MPEZoneLayout layout;
    layout.setLowerZone (15, 12, 2);
    graph.setMpeZoneLayout (layout);

    graph.prepare (44100.0, 512, 128);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;

    for (int channel = 2; channel <= 9; ++channel)
        midi.addEvent (yup::MidiMessage::noteOn (channel, 48 + channel, static_cast<uint8> (100)), 0);

    for (int step = 0; step < 32; ++step)
    {
        const auto offset = 16 + step * 8;

        for (int channel = 2; channel <= 9; ++channel)
        {
            midi.addEvent (yup::MidiMessage::pitchWheel (channel, 8192 + step * 100), offset);
            midi.addEvent (yup::MidiMessage::channelPressureChange (channel, step * 4), offset);
            midi.addEvent (yup::MidiMessage::controllerEvent (channel, 74, step * 4), offset);
        }
    }

    for (int block = 0; block < 8; ++block)
        runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    EXPECT_NE (0.0f, output[511]);

    dumpAsmOnFailure (graph);
}

//==============================================================================
// Acceptance: a 4-voice polyphonic sine synth

TEST (YdspJitGraphTests, PolySineCompilesAndRunsSampleAccurately)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        declare name "PolySine";

        processor Voice {
            output stream out;

            input value float decay = 0.25 [[ name: "Decay", min: 0.02, max: 4.0 ]];

            input event midi;

            state float phase;
            state float freq;
            state float env;
            state float envCoeff;

            func noteToFreq (pitch: float) : float {
                return 440.0 * pow (2.0, (pitch - 69.0) / 12.0);
            }

            event midi (e: noteOn) {
                freq     = noteToFreq (e.pitch);
                env      = e.velocity;
                envCoeff = pow (0.001, 1.0 / (decay * sampleRate));
            }

            event midi (e: noteOff) {
                envCoeff = pow (0.001, 1.0 / (0.05 * sampleRate));
            }

            process {
                phase = phase + freq / sampleRate;
                if (phase >= 1.0) { phase = phase - 1.0; }
                env = env * envCoeff;
                out = sin (phase * 6.283185307) * env;
            }
        }

        graph PolySine {
            input event midi;
            output stream out;

            node voices = Voice[4];

            connection {
                midi -> voices.midi;
                voices.out -> out;
            }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_FALSE (compiler.getDiagnostics().hasErrors());

    EXPECT_TRUE (graph.getExecutionReport().isProvenRealtimeSafe());

    constexpr int blockSize = 512;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 69, static_cast<uint8> (127)), 0);

    runProcess (graph, nullptr, 0, outPtrs, 1, blockSize, &midi);

    std::vector<float> sustained;

    for (int b = 0; b < 20; ++b)
    {
        sustained.insert (sustained.end(), output.begin(), output.end());

        runProcess (graph, nullptr, 0, outPtrs, 1, blockSize);
    }

    int crossings = 0;

    for (size_t i = 1; i < sustained.size(); ++i)
        if (sustained[i - 1] <= 0.0f && sustained[i] > 0.0f)
            ++crossings;

    const auto seconds = static_cast<double> (sustained.size()) / 44100.0;

    const auto measuredHz = static_cast<double> (crossings) / seconds;

    EXPECT_NEAR (440.0, measuredHz, 15.0) << "measured " << measuredHz << " Hz";

    float earlyEnergy = 0.0f;
    float lateEnergy = 0.0f;

    for (int i = 0; i < 256; ++i)
    {
        earlyEnergy += output[static_cast<size_t> (i)] * output[static_cast<size_t> (i)];
        lateEnergy += output[static_cast<size_t> (i + 256)] * output[static_cast<size_t> (i + 256)];
    }

    EXPECT_LT (lateEnergy, earlyEnergy);

    graph.reset();

    std::vector<float> oneOutput (blockSize, 0.0f);
    float* onePtrs[] = { oneOutput.data() };

    yup::MidiBuffer oneMidi;
    oneMidi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (127)), 0);

    runProcess (graph, nullptr, 0, onePtrs, 1, blockSize, &oneMidi);

    graph.reset();

    std::vector<float> fourOutput (blockSize, 0.0f);
    float* fourPtrs[] = { fourOutput.data() };

    yup::MidiBuffer fourMidi;
    fourMidi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (127)), 0);
    fourMidi.addEvent (yup::MidiMessage::noteOn (1, 64, static_cast<uint8> (127)), 0);
    fourMidi.addEvent (yup::MidiMessage::noteOn (1, 67, static_cast<uint8> (127)), 0);
    fourMidi.addEvent (yup::MidiMessage::noteOn (1, 71, static_cast<uint8> (127)), 0);

    runProcess (graph, nullptr, 0, fourPtrs, 1, blockSize, &fourMidi);

    float fourEnergy = 0.0f;
    float oneEnergy = 0.0f;

    for (int i = 0; i < blockSize; ++i)
    {
        oneEnergy += oneOutput[static_cast<size_t> (i)] * oneOutput[static_cast<size_t> (i)];
        fourEnergy += fourOutput[static_cast<size_t> (i)] * fourOutput[static_cast<size_t> (i)];
    }

    EXPECT_GT (fourEnergy, oneEnergy);

    graph.reset();

    std::vector<float> timed (blockSize, 0.0f);
    float* timedPtrs[] = { timed.data() };

    yup::MidiBuffer timedMidi;
    timedMidi.addEvent (yup::MidiMessage::noteOn (1, 81, static_cast<uint8> (127)), 100);
    timedMidi.addEvent (yup::MidiMessage::noteOff (1, 81), 300);

    runProcess (graph, nullptr, 0, timedPtrs, 1, blockSize, &timedMidi);

    for (int i = 0; i < 100; ++i)
        EXPECT_NEAR (0.0f, timed[static_cast<size_t> (i)], 1e-6f) << "pre-attack at " << i;

    float attackPeak = 0.0f;
    for (int i = 100; i < 300; ++i)
        attackPeak = std::max (attackPeak, std::fabs (timed[static_cast<size_t> (i)]));

    EXPECT_GT (attackPeak, 1e-3f);

    float attackEnergy = 0.0f;
    float releaseEnergy = 0.0f;

    for (int i = 0; i < 200; ++i)
        attackEnergy += timed[100 + static_cast<size_t> (i)] * timed[100 + static_cast<size_t> (i)];

    for (int i = 0; i < 200; ++i)
        releaseEnergy += timed[300 + static_cast<size_t> (i)] * timed[300 + static_cast<size_t> (i)];

    EXPECT_LT (releaseEnergy, attackEnergy * 0.75f);

    const auto decaySlot = graph.getParameterSlot ("voices.decay");
    ASSERT_GE (decaySlot, 0);

    graph.reset();

    std::vector<float> fastOut (blockSize, 0.0f);
    float* fastPtrs[] = { fastOut.data() };

    yup::MidiBuffer lateMidi;
    lateMidi.addEvent (yup::MidiMessage::noteOn (1, 69, static_cast<uint8> (127)), 200);

    runProcess (graph, nullptr, 0, fastPtrs, 1, blockSize, &lateMidi);

    graph.reset();

    std::vector<float> slowOut (blockSize, 0.0f);
    float* slowPtrs[] = { slowOut.data() };

    DspJitAutomationEvent decayAutomation { decaySlot, 100, 4.0f };

    runProcess (graph, nullptr, 0, slowPtrs, 1, blockSize, &lateMidi, &decayAutomation, 1);

    float fastLate = 0.0f;
    float slowLate = 0.0f;

    for (int i = 400; i < blockSize; ++i)
    {
        fastLate += fastOut[static_cast<size_t> (i)] * fastOut[static_cast<size_t> (i)];
        slowLate += slowOut[static_cast<size_t> (i)] * slowOut[static_cast<size_t> (i)];
    }

    EXPECT_GT (slowLate, fastLate * 1.2f);

    dumpAsmOnFailure (graph);
}

//==============================================================================

namespace
{

constexpr const char* expressiveSynthSource = R"YDSP(
    declare name "ExpressiveSynth";

    processor Lead {
        output stream out;

        input event midi;

        state float phase;
        state float pitch;
        state float bend;
        state float press;
        state float timbre;
        state float modWheel;
        state float env;
        state float envCoeff;
        state float lp;

        func noteToFreq (p: float) : float {
            return 440.0 * pow (2.0, (p - 69.0) / 12.0);
        }

        init {
            envCoeff = 0.9999;
        }

        event midi (e: noteOn) {
            pitch    = e.pitch;
            env      = e.velocity;
            envCoeff = pow (0.001, 1.0 / (0.6 * sampleRate));
        }

        event midi (e: noteOff) {
            envCoeff = pow (0.001, 1.0 / (0.05 * sampleRate));
        }

        event midi (e: pitchBend)     { bend = e.bendSemitones; }
        event midi (e: pressure)      { press = e.pressure; }
        event midi (e: slide)         { timbre = e.slide; }
        event midi (e: controlChange) { if (e.control == 1) { modWheel = e.value; } }

        process {
            phase = phase + noteToFreq (pitch + bend) / sampleRate;
            if (phase >= 1.0) { phase = phase - 1.0; }

            env = env * envCoeff;

            lp = lp + (0.02 + 0.9 * (modWheel + timbre)) * (sin (phase * 6.283185307) - lp);

            out = lp * env * (0.25 + 0.75 * press);
        }
    }

    processor Bass {
        output stream out;

        input event midi;

        state float phase;
        state float pitch;
        state float env;
        state float envCoeff;

        func noteToFreq (p: float) : float {
            return 440.0 * pow (2.0, (p - 69.0) / 12.0);
        }

        init {
            envCoeff = 0.9999;
        }

        event midi (e: noteOn) {
            pitch = e.pitch;

            if (! e.isLegato) { env = e.velocity; }

            envCoeff = pow (0.001, 1.0 / (1.5 * sampleRate));
        }

        event midi (e: noteOff) {
            envCoeff = pow (0.001, 1.0 / (0.05 * sampleRate));
        }

        process {
            phase = phase + noteToFreq (pitch) / sampleRate;
            if (phase >= 1.0) { phase = phase - 1.0; }

            env = env * envCoeff;

            out = sin (phase * 6.283185307) * env * 0.5;
        }
    }

    graph ExpressiveSynth {
        input event midi;

        output stream leadOut;
        output stream bassOut;

        node lead = Lead[8] [[ mode: poly, stealing: oldest ]];
        node bass = Bass    [[ mode: mono, priority: last ]];

        connection {
            midi -> lead.midi;
            midi -> bass.midi;
            lead.out -> leadOut;
            bass.out -> bassOut;
        }
    }
)YDSP";

float blockEnergy (const std::vector<float>& block, int from, int to)
{
    float energy = 0.0f;

    for (int i = from; i < to; ++i)
        energy += block[static_cast<size_t> (i)] * block[static_cast<size_t> (i)];

    return energy;
}

} // namespace

TEST (YdspJitGraphTests, ExpressiveMpeSynthRespondsToEveryEventClass)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (expressiveSynthSource, compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    ASSERT_FALSE (compiler.getDiagnostics().hasErrors());

    yup::MPEZoneLayout layout;
    layout.setLowerZone (15, 12, 2);
    graph.setMpeZoneLayout (layout);

    constexpr int blockSize = 512;
    graph.prepare (44100.0, blockSize);

    std::vector<float> lead (blockSize, 0.0f);
    std::vector<float> bass (blockSize, 0.0f);
    float* outPtrs[] = { lead.data(), bass.data() };

    const auto runBlock = [&] (const yup::MidiBuffer& midi)
    {
        runProcess (graph, nullptr, 0, outPtrs, 2, blockSize, &midi);
    };

    yup::MidiBuffer noteMidi;
    noteMidi.addEvent (yup::MidiMessage::noteOn (2, 69, static_cast<uint8> (100)), 0);

    runBlock (noteMidi);

    const auto plainEnergy = blockEnergy (lead, 256, blockSize);
    EXPECT_GT (plainEnergy, 1e-6f);

    graph.reset();

    yup::MidiBuffer pressureMidi;
    pressureMidi.addEvent (yup::MidiMessage::noteOn (2, 69, static_cast<uint8> (100)), 0);
    pressureMidi.addEvent (yup::MidiMessage::channelPressureChange (2, 127), 1);

    runBlock (pressureMidi);

    EXPECT_GT (blockEnergy (lead, 256, blockSize), plainEnergy);

    graph.reset();

    yup::MidiBuffer slideMidi;
    slideMidi.addEvent (yup::MidiMessage::noteOn (2, 69, static_cast<uint8> (100)), 0);
    slideMidi.addEvent (yup::MidiMessage::controllerEvent (2, 74, 127), 1);

    runBlock (slideMidi);

    EXPECT_GT (blockEnergy (lead, 256, blockSize), plainEnergy);

    graph.reset();

    yup::MidiBuffer modMidi;
    modMidi.addEvent (yup::MidiMessage::noteOn (2, 69, static_cast<uint8> (100)), 0);
    modMidi.addEvent (yup::MidiMessage::controllerEvent (1, 1, 127), 1);

    runBlock (modMidi);

    EXPECT_GT (blockEnergy (lead, 256, blockSize), plainEnergy);

    const auto countRisingCrossings = [] (const std::vector<float>& block)
    {
        int crossings = 0;

        for (size_t i = 1; i < block.size(); ++i)
            if (block[i - 1] <= 0.0f && block[i] > 0.0f)
                ++crossings;

        return crossings;
    };

    graph.reset();
    runBlock (noteMidi);
    const auto plainCrossings = countRisingCrossings (lead);

    graph.reset();

    yup::MidiBuffer bendMidi;
    bendMidi.addEvent (yup::MidiMessage::noteOn (2, 69, static_cast<uint8> (100)), 0);
    bendMidi.addEvent (yup::MidiMessage::pitchWheel (2, 16383), 1); // +12 semitones

    runBlock (bendMidi);

    EXPECT_GT (countRisingCrossings (lead), plainCrossings);

    graph.reset();

    yup::MidiBuffer quietMidi;
    quietMidi.addEvent (yup::MidiMessage::noteOn (2, 43, static_cast<uint8> (20)), 200);

    runBlock (quietMidi);
    const auto freshEnergy = blockEnergy (bass, 400, blockSize);

    graph.reset();

    yup::MidiBuffer legatoMidi;
    legatoMidi.addEvent (yup::MidiMessage::noteOn (2, 36, static_cast<uint8> (127)), 0);
    legatoMidi.addEvent (yup::MidiMessage::noteOn (3, 43, static_cast<uint8> (20)), 200);

    runBlock (legatoMidi);

    EXPECT_GT (blockEnergy (bass, 400, blockSize), freshEnergy * 2.0f);

    yup::MidiBuffer panicMidi;
    panicMidi.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 0);

    runBlock (panicMidi);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (0.0f, lead[static_cast<size_t> (i)], 1e-6f) << "lead after all-sound-off at " << i;
        EXPECT_NEAR (0.0f, bass[static_cast<size_t> (i)], 1e-6f) << "bass after all-sound-off at " << i;
    }

    dumpAsmOnFailure (graph);
}

//==============================================================================

TEST (YdspJitGraphTests, NoteOnCarriesCurrentPitchBend)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor BendVoice {
            output stream out;
            input event midi;
            state float bend;
            event midi (e: noteOn) { bend = e.bendSemitones; }
            process { out = bend; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = BendVoice[4] [[ mode: poly, stealing: oldest ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);
    float* outPtrs[] = { output.data() };

    // A note pressed while the wheel is already bent must start at the current bend
    // (legacy mode: 16383 = +2 semitones with the default 2-semitone range).
    yup::MidiBuffer bentMidi;
    bentMidi.addEvent (yup::MidiMessage::pitchWheel (1, 16383), 0);
    bentMidi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 1);

    runProcess (graph, nullptr, 0, outPtrs, 1, blockSize, &bentMidi);

    for (int i = 1; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "bent noteOn at " << i;

    // A note pressed with the wheel at center starts un-bent.
    graph.reset();
    std::fill (output.begin(), output.end(), 0.0f);

    yup::MidiBuffer centerMidi;
    centerMidi.addEvent (yup::MidiMessage::pitchWheel (1, 8192), 0); // center
    centerMidi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 1);

    runProcess (graph, nullptr, 0, outPtrs, 1, blockSize, &centerMidi);

    for (int i = 1; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "center noteOn at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MonoNoteOnCarriesCurrentPitchBend)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor BendVoice {
            output stream out;
            input event midi;
            state float bend;
            event midi (e: noteOn) { bend = e.bendSemitones; }
            process { out = bend; }
        }
        graph G {
            input event midi;
            output stream y;
            node v = BendVoice [[ mode: mono, priority: last ]];
            connection { midi -> v.midi; v.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    constexpr int blockSize = 128;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer bentMidi;
    bentMidi.addEvent (yup::MidiMessage::pitchWheel (1, 16383), 0);
    bentMidi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 1);
    bentMidi.addEvent (yup::MidiMessage::noteOn (1, 62, static_cast<uint8> (100)), 50); // legato retrigger
    bentMidi.addEvent (yup::MidiMessage::noteOff (1, 62, static_cast<uint8> (64)), 80); // falls back to the held 60

    runProcess (graph, nullptr, 0, outPtrs, 1, blockSize, &bentMidi);

    for (int i = 1; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "mono bent noteOn at " << i;

    dumpAsmOnFailure (graph);
}

//==============================================================================

TEST (YdspJitGraphTests, LoopNestedInsideAnIfRunsExactlyOnce)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            state float acc[4];

            process {
                if (in > 0.5) {
                    for i in 0..4 { acc[i] = acc[i] + 1.0; }
                } else {
                    for i in 0..4 { acc[i] = acc[i] - 2.0; }
                }

                float sum = 0.0;

                for i in 0..4 { sum = sum + acc[i]; }

                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    constexpr int blockSize = 8;
    graph.prepare (44100.0, blockSize);

    std::vector<float> output (blockSize, 0.0f);
    float* outPtrs[] = { output.data() };

    const std::vector<float> ones (blockSize, 1.0f);
    const float* onePtrs[] = { ones.data() };

    runProcess (graph, onePtrs, 1, outPtrs, 1, blockSize);

    for (int i = 0; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (4.0f * static_cast<float> (i + 1), output[static_cast<size_t> (i)]) << "then-branch at " << i;

    graph.reset();

    const std::vector<float> zeros (blockSize, 0.0f);
    const float* zeroPtrs[] = { zeros.data() };

    runProcess (graph, zeroPtrs, 1, outPtrs, 1, blockSize);

    for (int i = 0; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (-8.0f * static_cast<float> (i + 1), output[static_cast<size_t> (i)]) << "else-branch at " << i;

    dumpAsmOnFailure (graph);
}

//==============================================================================
// Acceptance: the 16-voice additive electric piano

class YdspElectricPianoTests : public ::testing::Test
{
protected:
    static constexpr int blockSize = 256;
    static constexpr double sampleRate = 44100.0;

    YdspElectricPianoTests()
        : graph (patches::cachedPatch (patches::electricPiano))
    {
    }

    void SetUp() override
    {
        ASSERT_TRUE (graph.isValid()) << patches::cachedPatchDiagnostics (patches::electricPiano);

        patches::restoreFreshState (graph, sampleRate, blockSize);
    }

    void TearDown() override
    {
        dumpAsmOnFailure (graph);
    }

    float runBlock (const yup::MidiBuffer* midi = nullptr)
    {
        float* outPtrs[] = { left.data(), right.data() };
        runProcess (graph, nullptr, 0, outPtrs, 2, static_cast<int> (left.size()), midi);

        float energy = 0.0f;

        for (const auto sample : left)
            energy += sample * sample;

        return energy;
    }

    DspJitGraph& graph;

    std::vector<float> left = std::vector<float> (blockSize, 0.0f);
    std::vector<float> right = std::vector<float> (blockSize, 0.0f);
};

TEST_F (YdspElectricPianoTests, ExposesTheGraphEndpointsAndAliasedParameters)
{
    EXPECT_EQ (0, graph.getInputStreamCount());
    EXPECT_EQ (2, graph.getOutputStreamCount());

    EXPECT_EQ (8, graph.getParameterCount());

    ASSERT_TRUE (graph.hasParameter ("vibratoRate"));
    EXPECT_FLOAT_EQ (4.0f, graph.getParameter ("vibratoRate"));

    EXPECT_EQ (graph.getParameterSlot ("vibratoRate"), graph.getParameterSlot ("trem.vibratoRate"));
}

TEST_F (YdspElectricPianoTests, PlaysDecaysAndReleasesANote)
{
    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (110)), 0);

    const auto attackEnergy = runBlock (&midi);
    EXPECT_GT (attackEnergy, 0.0f);

    float heldEnergy = 0.0f;
    for (int block = 0; block < 8; ++block)
        heldEnergy = runBlock();

    EXPECT_GT (heldEnergy, 0.0f);
    EXPECT_LT (heldEnergy, attackEnergy);

    yup::MidiBuffer offMidi;
    offMidi.addEvent (yup::MidiMessage::noteOff (1, 60), 0);

    runBlock (&offMidi);

    float releasedEnergy = 0.0f;

    for (int block = 0; block < 16; ++block)
        releasedEnergy = runBlock();

    EXPECT_LT (releasedEnergy, heldEnergy);
}

TEST_F (YdspElectricPianoTests, TremoloPansTheTwoOutputChannels)
{
    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (110)), 0);

    EXPECT_GT (runBlock (&midi), 0.0f);

    bool channelsDiffer = false;

    for (size_t i = 0; i < left.size(); ++i)
        if (left[i] != right[i])
            channelsDiffer = true;

    EXPECT_TRUE (channelsDiffer);
}

TEST_F (YdspElectricPianoTests, DoesNotAllocateDuringProcess)
{
    graph.prepare (44100.0, blockSize, 128);

    yup::MidiBuffer midi;

    for (int note = 0; note < 16; ++note)
    {
        midi.addEvent (yup::MidiMessage::noteOn (1, 48 + note, static_cast<uint8> (100)), note * 4);
        midi.addEvent (yup::MidiMessage::noteOff (1, 48 + note), 128 + note * 4);
    }

    float energy = 0.0f;

    for (int block = 0; block < 8; ++block)
        energy += runBlock (&midi);

    EXPECT_NE (0.0f, energy);
}

//==============================================================================
// Idle-voice skipping: `state int x [[ role: voiceActivity ]]`

namespace
{

constexpr const char* sleepProbeSource = R"YDSP(
    processor SleepProbe {
        output stream out;
        input event midi;
        state int active [[ role: voiceActivity ]];
        state int ccCount;
        event midi (e: noteOn) { active = 1; }
        event midi (e: noteOff) { active = 0; }
        event midi (e: controlChange) { ccCount = ccCount + 1; }
        process { out = select (active > 0, 0.5, 0.25); }
    }
)YDSP";

float sumOf (const std::vector<float>& block, int from, int to)
{
    float total = 0.0f;

    for (int i = from; i < to; ++i)
        total += block[static_cast<size_t> (i)];

    return total;
}

} // namespace

TEST (YdspJitGraphTests, IdleVoiceBankProducesExactSilence)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, nullptr);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "at " << i;

    EXPECT_EQ (0, graph.getActiveVoiceCount ("voices"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, GetActiveVoiceCountIsSafeBeforePrepare)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());

    EXPECT_EQ (16, graph.getActiveVoiceCount ("voices"));

    graph.prepare (44100.0, 512);
}

TEST (YdspJitGraphTests, OnlySoundingVoicesContributeToTheMix)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ (1.0f, output[static_cast<size_t> (i)]) << "at " << i;

    EXPECT_EQ (2, graph.getActiveVoiceCount ("voices"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, VoiceRunsToTheBlockBoundaryThenSleeps)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi1 = makeNoteBuffer ({ { 0, 60, 1.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi1);

    for (int i = 0; i < 512; ++i)
        ASSERT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "held at " << i;

    const auto noteOff = makeNoteBuffer ({ { 60, 60, 0.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &noteOff);

    for (int i = 0; i < 60; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "before note-off at " << i;

    for (int i = 60; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.25f, output[static_cast<size_t> (i)]) << "after note-off at " << i;

    EXPECT_EQ (0, graph.getActiveVoiceCount ("voices"));

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, nullptr);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "asleep at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, NoteOnMidBlockRendersTheWholeBlock)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi = makeNoteBuffer ({ { 100, 60, 1.0f } });

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi);

    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ (0.25f, output[static_cast<size_t> (i)]) << "prefix at " << i;

    for (int i = 100; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "after note-on at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, WokenIdleVoiceLeavesNoStaleScratchBehind)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi2 = makeNoteBuffer ({ { 0, 60, 1.0f }, { 0, 64, 1.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi2);
    const auto midi3 = makeNoteBuffer ({ { 0, 64, 0.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi3);
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, nullptr);

    for (int i = 0; i < 256; ++i)
        ASSERT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "one voice sounding at " << i;

    ASSERT_EQ (1, graph.getActiveVoiceCount ("voices"));

    yup::MidiBuffer cc;
    cc.addEvent (yup::MidiMessage::controllerEvent (1, 1, 127), 128);

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &cc);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "after CC at " << i;

    EXPECT_FLOAT_EQ (128.0f, sumOf (output, 0, 256));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AutomationStillAppliesToAFullyIdleVoiceBank)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor LevelProbe {
            output stream out;
            input value float level = 0.5;
            input event midi;
            state int active [[ role: voiceActivity ]];
            event midi (e: noteOn) { active = 1; }
            event midi (e: noteOff) { active = 0; }
            process { out = select (active > 0, level, 0.0); }
        }
        graph G {
            input event midi;
            output stream y;
            node voices = LevelProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto levelSlot = graph.getParameterSlot ("voices.level");
    ASSERT_GE (levelSlot, 0);

    const DspJitAutomationEvent automation { levelSlot, 100, 2.0f };

    ASSERT_EQ (0, graph.getActiveVoiceCount ("voices"));

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, nullptr, &automation, 1);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "idle at " << i;

    EXPECT_FLOAT_EQ (2.0f, graph.getParameter ("voices.level"));

    const auto midi4 = makeNoteBuffer ({ { 0, 60, 1.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi4);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (2.0f, output[static_cast<size_t> (i)]) << "after wake at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, MonoVoiceSleepsWhenIdle)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe [[ mode: mono ]];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, nullptr);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "idle at " << i;

    EXPECT_EQ (0, graph.getActiveVoiceCount ("voices"));

    const auto midi5 = makeNoteBuffer ({ { 0, 60, 1.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi5);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "held at " << i;

    EXPECT_EQ (1, graph.getActiveVoiceCount ("voices"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, HeldVoiceRunsEvenWithItsFlagCleared)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor LyingProbe {
            output stream out;
            input event midi;
            state int active [[ role: voiceActivity ]];
            event midi (e: noteOn) { active = 0; }
            event midi (e: noteOff) { active = 0; }
            process { out = 0.25; }
        }
        graph G {
            input event midi;
            output stream y;
            node voices = LyingProbe[8];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi6 = makeNoteBuffer ({ { 0, 60, 1.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi6);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.25f, output[static_cast<size_t> (i)]) << "held at " << i;

    EXPECT_EQ (1, graph.getActiveVoiceCount ("voices"));

    const auto midi7 = makeNoteBuffer ({ { 0, 60, 0.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, &midi7);
    runProcess (graph, nullptr, 0, outPtrs, 1, 256, nullptr);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "released at " << i;

    EXPECT_EQ (0, graph.getActiveVoiceCount ("voices"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, WriteOnlyActivityFlagStillPutsTheVoiceToSleep)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor DecayProbe {
            output stream out;
            input event midi;
            state float env;
            state int active [[ role: voiceActivity ]];
            event midi (e: noteOn) { env = 1.0; active = 1; }
            event midi (e: noteOff) { env = 0.0; }
            process {
                env = env * 0.5;
                active = select (env < 0.01, 0, 1);
                out = env;
            }
        }
        graph G {
            input event midi;
            output stream y;
            node voices = DecayProbe[8];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> output (64, 0.0f);
    float* outPtrs[] = { output.data() };

    const auto midi8 = makeNoteBuffer ({ { 0, 60, 1.0f }, { 1, 60, 0.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi8);

    EXPECT_FLOAT_EQ (0.5f, output[0]);
    EXPECT_EQ (0, graph.getActiveVoiceCount ("voices"));

    runProcess (graph, nullptr, 0, outPtrs, 1, 64, nullptr);

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "asleep at " << i;

    const auto midi9 = makeNoteBuffer ({ { 0, 60, 1.0f } });
    runProcess (graph, nullptr, 0, outPtrs, 1, 64, &midi9);

    EXPECT_FLOAT_EQ (0.5f, output[0]);
    EXPECT_EQ (1, graph.getActiveVoiceCount ("voices"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, VoiceBankWithoutTheAnnotationRunsEveryVoice)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor PlainProbe {
            output stream out;
            input event midi;
            state int touched;
            event midi (e: noteOn) { touched = 1; }
            event midi (e: noteOff) { touched = 0; }
            process { out = 0.25; }
        }
        graph G {
            input event midi;
            output stream y;
            node voices = PlainProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 256);

    std::vector<float> output (256, 0.0f);
    float* outPtrs[] = { output.data() };

    runProcess (graph, nullptr, 0, outPtrs, 1, 256, nullptr);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (4.0f, output[static_cast<size_t> (i)]) << "at " << i;

    EXPECT_EQ (16, graph.getActiveVoiceCount ("voices"));

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AllSoundOffSilencesAndSleepsTheVoices)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[16];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 512);

    std::vector<float> output (512, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer midi;
    midi.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    midi.addEvent (yup::MidiMessage::controllerEvent (1, 120, 0), 200);

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, &midi);

    for (int i = 0; i < 200; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "before all-sound-off at " << i;

    for (int i = 200; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.25f, output[static_cast<size_t> (i)]) << "after all-sound-off at " << i;

    EXPECT_EQ (0, graph.getActiveVoiceCount ("voices"));

    runProcess (graph, nullptr, 0, outPtrs, 1, 512, nullptr);

    for (int i = 0; i < 512; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "asleep at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ActiveVoiceCountIsZeroForAnUnknownNode)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (String (sleepProbeSource) + R"YDSP(
        graph G {
            input event midi;
            output stream y;
            node voices = SleepProbe[4];
            connection { midi -> voices.midi; voices.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    EXPECT_EQ (0, graph.getActiveVoiceCount ("nope"));
}

//==============================================================================
// Fan-out and summing fan-in
//==============================================================================

namespace
{

constexpr const char* fanProcessors =
    "processor Gain { input stream in; output stream out; input value float g = 1.0; process { out = in * g; } }\n"
    "processor Sub { input stream a; input stream b; output stream out; process { out = a - b; } }\n";

DspJitGraph fanCompile (StringRef body, DspJitCompiler& compiler)
{
    return compilePatch (String (fanProcessors) + body, compiler);
}

const std::vector<float> fanInput { 1.0f, 2.0f, -0.5f, 0.25f, 4.0f, 0.0f, -3.0f, 1.5f };

std::vector<std::vector<float>> fanRun (DspJitGraph& graph,
                                        const std::vector<std::vector<float>>& inputs,
                                        int numOutputs)
{
    const auto blockSize = static_cast<int> (inputs.empty() ? fanInput.size() : inputs[0].size());

    std::vector<const float*> inPtrs;
    for (const auto& channel : inputs)
        inPtrs.push_back (channel.data());

    std::vector<std::vector<float>> outputs (static_cast<size_t> (numOutputs),
                                             std::vector<float> (static_cast<size_t> (blockSize), 0.0f));

    std::vector<float*> outPtrs;
    for (auto& channel : outputs)
        outPtrs.push_back (channel.data());

    runProcess (graph,
                inPtrs.empty() ? nullptr : inPtrs.data(),
                static_cast<int> (inPtrs.size()),
                outPtrs.data(),
                numOutputs,
                blockSize);

    return outputs;
}

} // namespace

TEST (YdspJitGraphTests, FansOutAGraphInputToTwoNodes)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream doubled;
            output stream tripled;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            connection { x -> a.in; x -> b.in; a.out -> doubled; b.out -> tripled; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 2);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        EXPECT_NEAR (fanInput[i] * 2.0f, out[0][i], 1e-6f) << "sample " << i;
        EXPECT_NEAR (fanInput[i] * 3.0f, out[1][i], 1e-6f) << "sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SumsTwoSourcesIntoANodeInput)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            node s = Gain (g = 1.0);
            connection { x -> a.in; x -> b.in; a.out -> s.in; b.out -> s.in; s.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
        EXPECT_NEAR (fanInput[i] * 5.0f, out[0][i], 1e-6f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SumsTwoSourcesIntoAGraphOutput)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            connection { x -> a.in; x -> b.in; a.out -> y; b.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
        EXPECT_NEAR (fanInput[i] * 5.0f, out[0][i], 1e-6f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SumsThreeSourcesIntoOneSlot)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 1.0);
            node b = Gain (g = 16777216.0);
            node c = Gain (g = -16777216.0);
            connection { x -> a.in; x -> b.in; x -> c.in; a.out -> y; b.out -> y; c.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> ones (8, 1.0f);
    const auto out = fanRun (graph, { ones }, 1);

    for (size_t i = 0; i < ones.size(); ++i)
        EXPECT_FLOAT_EQ (0.0f, out[0][i]) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, NodeOutputFeedsAGraphOutputAndANode)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream tap;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 5.0);
            connection { x -> a.in; a.out -> tap; a.out -> b.in; b.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 2);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        EXPECT_NEAR (fanInput[i] * 2.0f, out[0][i], 1e-6f) << "tap sample " << i;
        EXPECT_NEAR (fanInput[i] * 10.0f, out[1][i], 1e-5f) << "y sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, NodeOutputFeedsANodeAndAGraphOutputInTheOtherOrder)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream tap;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 5.0);
            connection { x -> a.in; a.out -> b.in; a.out -> tap; b.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 2);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        EXPECT_NEAR (fanInput[i] * 2.0f, out[0][i], 1e-6f) << "tap sample " << i;
        EXPECT_NEAR (fanInput[i] * 10.0f, out[1][i], 1e-5f) << "y sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, NodeOutputDrivesTwoGraphOutputs)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream left;
            output stream right;
            node a = Gain (g = 2.0);
            connection { x -> a.in; a.out -> left; a.out -> right; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 2);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        EXPECT_NEAR (fanInput[i] * 2.0f, out[0][i], 1e-6f) << "left sample " << i;
        EXPECT_NEAR (fanInput[i] * 2.0f, out[1][i], 1e-6f) << "right sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AppliesPerEdgeDelayBeforeSumming)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 1.0);
            connection { x -> a.in; x -> [4] -> a.in; a.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        const auto delayed = i >= 4 ? fanInput[i - 4] : 0.0f;
        EXPECT_NEAR (fanInput[i] + delayed, out[0][i], 1e-6f) << "sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SumsIntoOneSlotWithoutDisturbingTheOther)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            input stream w;
            output stream y;
            node s = Sub;
            connection { x -> s.a; w -> s.a; x -> s.b; s.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> w { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f };
    const auto out = fanRun (graph, { fanInput, w }, 1);

    for (size_t i = 0; i < w.size(); ++i)
        EXPECT_NEAR (w[i], out[0][i], 1e-5f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AppliesADelayOnAnEdgeIntoAGraphOutput)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 1.0);
            connection { x -> a.in; a.out -> [3] -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
        EXPECT_NEAR (i >= 3 ? fanInput[i - 3] : 0.0f, out[0][i], 1e-6f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, WiresAGraphInputStraightToAGraphOutput)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            output stream z;
            node a = Gain (g = 4.0);
            connection { x -> y; x -> a.in; a.out -> z; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 2);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        EXPECT_NEAR (fanInput[i], out[0][i], 1e-6f) << "passthrough sample " << i;
        EXPECT_NEAR (fanInput[i] * 4.0f, out[1][i], 1e-6f) << "gained sample " << i;
    }

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SumsAGraphInputAndANodeOutputIntoOneGraphOutput)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 2.0);
            connection { x -> a.in; x -> y; a.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
        EXPECT_NEAR (fanInput[i] * 3.0f, out[0][i], 1e-6f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, SumsFanInOnAFloat64Stream)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor WideGain { input stream float64 in; output stream float64 out; input value float64 g = 1.0; process { out = in * g; } }
        graph G {
            input stream float64 x;
            output stream float64 y;
            node a = WideGain (g = 2.0);
            node b = WideGain (g = 3.0);
            connection { x -> a.in; x -> b.in; a.out -> y; b.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    std::vector<double> input (fanInput.begin(), fanInput.end());
    std::vector<double> output (input.size(), 0.0);

    std::vector<DspJitInputBuffer> inputs { DspJitInputBuffer (Span<const double> (input.data(), input.size())) };
    std::vector<DspJitOutputBuffer> outputs { DspJitOutputBuffer (Span<double> (output.data(), output.size())) };

    ASSERT_EQ (DspJitProcessResult::ok, graph.process (inputs, outputs, static_cast<int> (input.size())));

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR (input[i] * 5.0, output[i], 1e-12) << "sample " << i;
}

TEST (YdspJitGraphTests, FanInSurvivesASecondPrepareAtADifferentBlockSize)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream x;
            output stream y;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            node s = Gain (g = 1.0);
            connection { x -> a.in; x -> b.in; a.out -> s.in; b.out -> s.in; s.out -> y; }
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());

    for (const int blockSize : { 8, 64, 16 })
    {
        graph.prepare (44100.0, blockSize);

        const std::vector<float> input (static_cast<size_t> (blockSize), 2.0f);
        const auto out = fanRun (graph, { input }, 1);

        for (int i = 0; i < blockSize; ++i)
            EXPECT_NEAR (10.0f, out[0][static_cast<size_t> (i)], 1e-6f) << "block " << blockSize << " sample " << i;
    }
}

TEST (YdspJitGraphTests, SplitMergeAlgebraMatchesTheHandWrittenConnectionBlock)
{
    DspJitCompiler algebraCompiler;
    DspJitCompiler wiredCompiler;

    auto algebra = fanCompile (R"YDSP(
        graph G {
            input stream dry;
            output stream wet;
            process = dry <: (Gain (g = 2.0) , Gain (g = 3.0)) :> wet;
        }
    )YDSP",
                               algebraCompiler);

    auto wired = fanCompile (R"YDSP(
        graph G {
            input stream dry;
            output stream wet;
            node a = Gain (g = 2.0);
            node b = Gain (g = 3.0);
            connection { dry -> a.in; dry -> b.in; a.out -> wet; b.out -> wet; }
        }
    )YDSP",
                             wiredCompiler);

    ASSERT_TRUE (algebra.isValid());
    ASSERT_TRUE (wired.isValid());

    algebra.prepare (44100.0, 8);
    wired.prepare (44100.0, 8);

    const auto fromAlgebra = fanRun (algebra, { fanInput }, 1);
    const auto fromWiring = fanRun (wired, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
    {
        EXPECT_FLOAT_EQ (fromWiring[0][i], fromAlgebra[0][i]) << "sample " << i;
        EXPECT_NEAR (fanInput[i] * 5.0f, fromAlgebra[0][i], 1e-6f) << "sample " << i;
    }
}

TEST (YdspJitGraphTests, SplitMergeCarriesTheIdentityFanThroughToRealEdges)
{
    DspJitCompiler compiler;

    auto graph = fanCompile (R"YDSP(
        graph G {
            input stream dry;
            output stream wet;
            process = dry : _ <: (Gain (g = 2.0) , Gain (g = 3.0)) :> _ : wet;
        }
    )YDSP",
                             compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto out = fanRun (graph, { fanInput }, 1);

    for (size_t i = 0; i < fanInput.size(); ++i)
        EXPECT_NEAR (fanInput[i] * 5.0f, out[0][i], 1e-6f) << "sample " << i;

    dumpAsmOnFailure (graph);
}

//==============================================================================
// Node-to-node event routing: mid-loop drain, carry queue, midiOut delivery

TEST (YdspJitGraphTests, RoutedNoteOnSoundsTheDestinationVoiceAtTheRoutedPitch)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Arp {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.8) -> noteOn; } }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch; }
            event midi (e: noteOff) { sounding = 0.0; }
            process { out = sounding; }
        }
        graph G {
            input stream trig;
            output stream y;
            node arp = Arp;
            node voice = Voice;
            connection { trig -> arp.trig; arp.noteOn -> voice.midi; voice.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> trig (64, 0.0f);
    trig[0] = 1.0f;

    std::vector<float> output (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (trig.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outputBuffers { yup::Span<float> (output.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outputBuffers, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (72.0f, output[static_cast<size_t> (i)]) << "at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, AnImportedProcessorsEmitStatementSurvivesTheImportClone)
{
    const auto tempDir = File::getSpecialLocation (File::tempDirectory)
                             .getChildFile ("yup_ydsp_import_emit_test");

    tempDir.deleteRecursively();
    tempDir.createDirectory();

    tempDir.getChildFile ("Arp.ydsp")
        .replaceWithText (R"YDSP(
            processor Arp {
                input stream trig;
                output event noteOn;
                process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.8) -> noteOn; } }
            }
        )YDSP");

    const auto patch = R"YDSP(
        import Arp as arpLib;
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch; }
            process { out = sounding; }
        }
        graph G {
            input stream trig;
            output stream y;
            node arp = arpLib.Arp;
            node voice = Voice;
            connection { trig -> arp.trig; arp.noteOn -> voice.midi; voice.out -> y; }
        }
    )YDSP";

    DspJitCompiler compiler;
    auto result = compiler.compile (patch, tempDir.getChildFile ("Patch.ydsp").getFullPathName());
    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    auto graph = std::move (result).getValue();
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> trig (64, 0.0f);
    trig[0] = 1.0f;

    std::vector<float> output (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (trig.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outputBuffers { yup::Span<float> (output.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outputBuffers, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (72.0f, output[static_cast<size_t> (i)]) << "at " << i;

    dumpAsmOnFailure (graph);

    tempDir.deleteRecursively();
}

TEST (YdspJitGraphTests, MidiOnlyNodeReceivesEveryHeldNoteWithoutSyntheticStealingReleases)
{
    // Before dispatchEventToNode()'s isMidiOnly bypass, a zero-stream
    // event-driven node still went through the ordinary voiceCount=1 slot
    // allocator: holding a second note while a first was still down stole
    // slot 0 and synthesized a noteOff for the still-held first note. A node
    // that manages its own polyphony (an arpeggiator's held-note table) must
    // see every noteOn/noteOff exactly once, verbatim, with no synthetic
    // releases injected by voice stealing.
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Collector {
            input event midi;
            output value float noteOffs;
            state float count;
            event midi (e: noteOn) { }
            event midi (e: noteOff) { count = count + 1.0; }
            process { noteOffs = count; }
        }
        graph G {
            input event midi;
            node c = Collector;
            connection { midi -> c.midi; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    yup::MidiBuffer midiA;
    midiA.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    runProcess (graph, nullptr, 0, nullptr, 0, 64, &midiA);

    EXPECT_NEAR (0.0f, graph.getOutputValue ("c.noteOffs"), 1e-6f);

    // Note 60 is still held: a second note-on must not evict it.
    yup::MidiBuffer midiB;
    midiB.addEvent (yup::MidiMessage::noteOn (1, 64, static_cast<uint8> (100)), 0);
    runProcess (graph, nullptr, 0, nullptr, 0, 64, &midiB);

    EXPECT_NEAR (0.0f, graph.getOutputValue ("c.noteOffs"), 1e-6f);

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, TwoSourceNodesRoutingTheSamePitchDoNotCollideOnOneVoiceBank)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor ArpA {
            input stream trig;
            output event noteOn;
            output event noteOff;
            process {
                if (trig > 0.5) { emit noteOn (pitch: 60, velocity: 0.1) -> noteOn; }
                if (trig < -0.5) { emit noteOff (pitch: 60, velocity: 0.1) -> noteOff; }
            }
        }
        processor ArpB {
            input stream trig;
            output event noteOn;
            output event noteOff;
            process {
                if (trig > 0.5) { emit noteOn (pitch: 60, velocity: 0.2) -> noteOn; }
                if (trig < -0.5) { emit noteOff (pitch: 60, velocity: 0.2) -> noteOff; }
            }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch + e.velocity; }
            event midi (e: noteOff) { sounding = 0.0; }
            process { out = sounding; }
        }
        graph G {
            input stream trigA;
            input stream trigB;
            output stream y;
            node arpA = ArpA;
            node arpB = ArpB;
            node voice = Voice[4] [[ mode: poly, stealing: oldest ]];
            connection {
                trigA -> arpA.trig;
                trigB -> arpB.trig;
                arpA.noteOn -> voice.midi;
                arpA.noteOff -> voice.midi;
                arpB.noteOn -> voice.midi;
                arpB.noteOff -> voice.midi;
                voice.out -> y;
            }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> trigAOn (64, 0.0f);
    trigAOn[0] = 1.0f;
    std::vector<float> trigBOn (64, 0.0f);
    trigBOn[0] = 1.0f;

    std::vector<float> outputBlock1 (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inBuf1 {
        yup::Span<const float> (trigAOn.data(), 64),
        yup::Span<const float> (trigBOn.data(), 64)
    };
    std::vector<yup::DspJitOutputBuffer> outBuf1 { yup::Span<float> (outputBlock1.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inBuf1, outBuf1, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (120.3f, outputBlock1[static_cast<size_t> (i)], 1e-3f) << "block 1 at " << i;

    std::vector<float> trigAOff (64, 0.0f);
    std::vector<float> trigBOff (64, 0.0f);
    trigBOff[0] = -1.0f;

    std::vector<float> outputBlock2 (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inBuf2 {
        yup::Span<const float> (trigAOff.data(), 64),
        yup::Span<const float> (trigBOff.data(), 64)
    };
    std::vector<yup::DspJitOutputBuffer> outBuf2 { yup::Span<float> (outputBlock2.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inBuf2, outBuf2, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (60.1f, outputBlock2[static_cast<size_t> (i)], 1e-3f) << "block 2 at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, LatencyAnnotatedEmitterDelaysTheRoutedEventByItsDeclaredLatency)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Source [[ latency: 8 ]] {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.8) -> noteOn; } }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch; }
            process { out = sounding; }
        }
        graph G {
            input stream trig;
            output stream y;
            node src = Source;
            node voice = Voice;
            connection { trig -> src.trig; src.noteOn -> voice.midi; voice.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> trig (64, 0.0f);
    trig[0] = 1.0f;

    std::vector<float> output (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (trig.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outputBuffers { yup::Span<float> (output.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outputBuffers, 64));

    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "before compensated onset at " << i;

    for (int i = 8; i < 64; ++i)
        EXPECT_FLOAT_EQ (72.0f, output[static_cast<size_t> (i)]) << "after compensated onset at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, ACompensatedEventStraddlingTheBlockBoundaryCarriesToTheNextBlock)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Source [[ latency: 100 ]] {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 72, velocity: 0.8) -> noteOn; } }
        }
        processor Voice {
            input event midi;
            output stream out;
            state float sounding;
            event midi (e: noteOn) { sounding = e.pitch; }
            process { out = sounding; }
        }
        graph G {
            input stream trig;
            output stream y;
            node src = Source;
            node voice = Voice;
            connection { trig -> src.trig; src.noteOn -> voice.midi; voice.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> trigOn (64, 0.0f);
    trigOn[0] = 1.0f;
    std::vector<float> trigOff (64, 0.0f);

    std::vector<float> outputBlock1 (64, 0.0f);
    std::vector<float> outputBlock2 (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inBuf1 { yup::Span<const float> (trigOn.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outBuf1 { yup::Span<float> (outputBlock1.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inBuf1, outBuf1, 64));

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (0.0f, outputBlock1[static_cast<size_t> (i)]) << "block 1 at " << i;

    std::vector<yup::DspJitInputBuffer> inBuf2 { yup::Span<const float> (trigOff.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outBuf2 { yup::Span<float> (outputBlock2.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inBuf2, outBuf2, 64));

    for (int i = 0; i < 36; ++i)
        EXPECT_FLOAT_EQ (0.0f, outputBlock2[static_cast<size_t> (i)]) << "block 2 before carried onset at " << i;

    for (int i = 36; i < 64; ++i)
        EXPECT_FLOAT_EQ (72.0f, outputBlock2[static_cast<size_t> (i)]) << "block 2 after carried onset at " << i;

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, CarryQueueOverflowIncrementsTheDroppedOutputEventCounter)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Source [[ latency: 2000 ]] {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 60, velocity: 0.8) -> noteOn; } }
        }
        processor Sink { input event midi; output stream out; process { out = 0.0; } }
        graph G {
            input stream trig;
            output stream y;
            node src = Source;
            node s1 = Sink;
            node s2 = Sink;
            connection { trig -> src.trig; src.noteOn -> s1.midi; src.noteOn -> s2.midi; s1.out -> y; s2.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64, 64, 32, 1);

    const auto before = graph.getDroppedOutputEventCount();

    std::vector<float> trig (64, 0.0f);
    trig[0] = 1.0f;

    std::vector<float> output (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (trig.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outputBuffers { yup::Span<float> (output.data(), 64) };

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outputBuffers, 64));

    EXPECT_EQ (before + 1, graph.getDroppedOutputEventCount());

    dumpAsmOnFailure (graph);
}

TEST (YdspJitGraphTests, RoutedNoteOnReachesTheHostMidiOutBuffer)
{
    DspJitCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor Arp {
            input stream trig;
            output event noteOn;
            process { if (trig > 0.5) { emit noteOn (pitch: 64, velocity: 0.5, channel: 3) -> noteOn; } }
        }
        processor Silence {
            output stream out;
            process { out = 0.0; }
        }
        graph G {
            input stream trig;
            output stream y;
            output event noteOn;
            node arp = Arp;
            node sil = Silence;
            connection { trig -> arp.trig; arp.noteOn -> noteOn; sil.out -> y; }
        }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    std::vector<float> trig (64, 0.0f);
    trig[0] = 1.0f;

    std::vector<float> output (64, 0.0f);

    std::vector<yup::DspJitInputBuffer> inputBuffers { yup::Span<const float> (trig.data(), 64) };
    std::vector<yup::DspJitOutputBuffer> outputBuffers { yup::Span<float> (output.data(), 64) };

    yup::MidiBuffer midiOut;

    EXPECT_EQ (yup::DspJitProcessResult::ok, graph.process (inputBuffers, outputBuffers, 64, nullptr, nullptr, 0, &midiOut));

    ASSERT_FALSE (midiOut.isEmpty());

    int numEvents = 0;

    for (const yup::MidiMessageMetadata metadata : midiOut)
    {
        const auto message = metadata.getMessage();

        EXPECT_EQ (0, metadata.samplePosition);
        EXPECT_TRUE (message.isNoteOn());
        EXPECT_EQ (64, message.getNoteNumber());
        EXPECT_EQ (4, message.getChannel());
        EXPECT_NEAR (0.5f, message.getFloatVelocity(), 1.0f / 127.0f);

        ++numEvents;
    }

    EXPECT_EQ (1, numEvents);

    dumpAsmOnFailure (graph);
}

} // namespace yup::test
