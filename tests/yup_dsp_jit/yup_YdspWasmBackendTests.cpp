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

#if YUP_WASM

#include <gtest/gtest.h>

#include <yup_dsp_jit/yup_dsp_jit.h>

#include "yup_YdspTestPatches.h"

#include <cmath>

using namespace yup;

namespace
{

//==============================================================================

using yup::test::patches::compilePatch;
using yup::test::patches::makeRamp;

void runWasmProcess (YdspAudioGraph& graph,
                     const float* const* inputs,
                     int numInputs,
                     float* const* outputs,
                     int numOutputs,
                     int numSamples,
                     const yup::MidiBuffer* midi = nullptr)
{
    std::vector<YdspInputBuffer> inputBuffers;
    inputBuffers.reserve (static_cast<size_t> (numInputs));

    for (int i = 0; i < numInputs; ++i)
        inputBuffers.emplace_back (yup::Span<const float> (inputs[i], static_cast<size_t> (numSamples)));

    std::vector<YdspOutputBuffer> outputBuffers;
    outputBuffers.reserve (static_cast<size_t> (numOutputs));

    for (int i = 0; i < numOutputs; ++i)
        outputBuffers.emplace_back (yup::Span<float> (outputs[i], static_cast<size_t> (numSamples)));

    const auto result = graph.process (inputBuffers, outputBuffers, numSamples, midi, nullptr, 0);
    EXPECT_EQ (YdspProcessResult::ok, result);
}

constexpr const char* wasmPassThroughSource = R"YDSP(
    processor P { input stream in; output stream out; process { out = in; } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* wasmAccumulateSource = R"YDSP(
    processor P { input stream in; output stream out; state float s; process { s = s + in; out = s; } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* wasmParamSource = R"YDSP(
    processor P { input stream in; output stream out; input value float gain = 0.5; process { out = in * gain; } }
    graph G { input stream x; output stream y; input value float gain = 1.0; node p = P; connection { x -> p.in; p.out -> y; gain -> p.gain; } }
)YDSP";

constexpr const char* wasmSmoothSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;
        input value float gain = 0.5 [[ smoothing: 0.0002 ]];
        process { out = in * gain; }
    }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* wasmSinSource = R"YDSP(
    processor P { input stream in; output stream out; process { out = sin (in); } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* wasmDelaySource = R"YDSP(
    processor P { input stream in; output stream out; process { out = in @ 4; } }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

constexpr const char* wasmSubgraphSource = R"YDSP(
    processor Gain { input stream in; output stream out; input value float g = 2.0; process { out = in * g; } }
    graph Sub {
        input stream in;
        output stream out;
        input value float amount = 3.0;
        node a = Gain;
        connection { in -> a.in; a.out -> out; amount -> a.g; }
    }
    graph Main [[ main ]] {
        input stream x;
        output stream y;
        node s = Sub (amount = 4.0);
        connection { x -> s.in; s.out -> y; }
    }
)YDSP";

} // namespace

//==============================================================================

TEST (YdspWasmBackendTests, CompilesAndRunsPassThrough)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmPassThroughSource, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 128);

    graph.prewarmKernels();

    const auto input = makeRamp (128);
    std::vector<float> output (128, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 128);

    for (int i = 0; i < 128; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i)], output[static_cast<size_t> (i)]);
}

TEST (YdspWasmBackendTests, StateAccumulatesAcrossBlocks)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmAccumulateSource, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> ones (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    const float* inPtrs[] = { ones.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 8);

    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (static_cast<float> (i + 1), output[static_cast<size_t> (i)]);

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 8);

    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (static_cast<float> (9 + i), output[static_cast<size_t> (i)]);
}

TEST (YdspWasmBackendTests, ParamsDriveTheKernel)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmParamSource, compiler);

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.hasParameter ("gain"));
    graph.prepare (44100.0, 64);

    const auto input = makeRamp (64);
    std::vector<float> output (64, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    graph.setParameter ("gain", 0.25f);
    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i)] * 0.25f, output[static_cast<size_t> (i)]);
}

TEST (YdspWasmBackendTests, SinIntrinsicMatchesHostMath)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmSinSource, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 128);

    const auto input = makeRamp (128, -0.3f);
    std::vector<float> output (128, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 128);

    for (int i = 0; i < 128; ++i)
        EXPECT_NEAR (std::sin (input[static_cast<size_t> (i)]), output[static_cast<size_t> (i)], 1e-6f);
}

TEST (YdspWasmBackendTests, PolyphonicVoiceBankWithMidiEvents)
{
    YdspCompiler compiler;
    auto graph = compilePatch (R"YDSP(
        processor P {
            output stream out;
            input value float gain = 0.5;
            input event midi;
            state float phase;
            state float freq;
            state float env;

            func noteToFreq (pitch: float) : float { return 440.0 * pow (2.0, (pitch - 69.0) / 12.0); }

            event midi (e: noteOn) { freq = noteToFreq (e.pitch); env = e.velocity; }
            event midi (e: noteOff) { env = 0.0; }

            process {
                phase = phase + freq / sampleRate;
                if (phase >= 1.0) { phase = phase - 1.0; }
                out = env * phase;
            }
        }
        graph G { input event midi; output stream y; node p = P[4]; connection { midi -> p.midi; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 128);

    std::vector<float> output (128, 0.0f);
    float* outPtrs[] = { output.data() };

    yup::MidiBuffer noteOn;
    noteOn.addEvent (yup::MidiMessage::noteOn (1, 69, static_cast<uint8> (127)), 0);
    runWasmProcess (graph, nullptr, 0, outPtrs, 1, 128, &noteOn);

    float peak = 0.0f;

    for (const auto sample : output)
        peak = std::max (peak, std::abs (sample));

    EXPECT_GT (peak, 0.0f); // the voice produced audio

    yup::MidiBuffer noteOff;
    noteOff.addEvent (yup::MidiMessage::noteOff (1, 69, static_cast<uint8> (0)), 0);
    runWasmProcess (graph, nullptr, 0, outPtrs, 1, 128, &noteOff);

    std::fill (output.begin(), output.end(), 0.0f);
    runWasmProcess (graph, nullptr, 0, outPtrs, 1, 128);

    for (const auto sample : output)
        EXPECT_FLOAT_EQ (0.0f, sample);
}

//==============================================================================

TEST (YdspWasmBackendTests, ElectricPianoRunsInWasm)
{
    YdspCompiler compiler;

    auto graph = compilePatch (yup::test::patches::electricPiano, compiler);

    ASSERT_TRUE (graph.isValid());
    EXPECT_EQ (2, graph.getOutputStreamCount());
    EXPECT_FLOAT_EQ (4.0f, graph.getParameter ("vibratoRate"));

    graph.prepare (44100.0, 128);

    std::vector<float> left (128, 0.0f);
    std::vector<float> right (128, 0.0f);
    float* outPtrs[] = { left.data(), right.data() };

    yup::MidiBuffer noteOn;
    noteOn.addEvent (yup::MidiMessage::noteOn (1, 60, static_cast<uint8> (100)), 0);
    runWasmProcess (graph, nullptr, 0, outPtrs, 2, 128, &noteOn);

    float peak = 0.0f;
    bool channelsDiffer = false;

    for (size_t i = 0; i < left.size(); ++i)
    {
        peak = std::max (peak, std::abs (left[i]));

        if (left[i] != right[i])
            channelsDiffer = true;
    }

    EXPECT_GT (peak, 0.0f);
    EXPECT_TRUE (channelsDiffer);
}

TEST (YdspWasmBackendTests, SmoothedParameterRampsAcrossBlocks)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmSmoothSource, compiler);

    ASSERT_TRUE (graph.isValid());
    ASSERT_TRUE (graph.hasParameter ("p.gain"));
    graph.prepare (44100.0, 256);

    const std::vector<float> ones (256, 1.0f);
    std::vector<float> output (256, 0.0f);

    const float* inPtrs[] = { ones.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 256);

    for (int i = 0; i < 256; ++i)
        EXPECT_FLOAT_EQ (0.5f, output[static_cast<size_t> (i)]) << "primed block at " << i;

    graph.setParameter ("p.gain", 1.0f);
    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 256);

    EXPECT_GT (output[0], 0.5f);
    EXPECT_LT (output[0], 0.6f);

    for (int i = 1; i < 256; ++i)
        EXPECT_GE (output[static_cast<size_t> (i)], output[static_cast<size_t> (i - 1)]) << "not monotone at " << i;

    EXPECT_FLOAT_EQ (1.0f, output[255]);
}

TEST (YdspWasmBackendTests, DelayRingWrapsWithoutIntegerDivision)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmDelaySource, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    const auto input = makeRamp (32, 1.0f);
    std::vector<float> output (32, -1.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 32);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (0.0f, output[static_cast<size_t> (i)]) << "pre-roll at " << i;

    for (int i = 4; i < 32; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (i - 4)], output[static_cast<size_t> (i)]) << "delayed at " << i;

    const auto second = makeRamp (32, 100.0f);
    const float* secondPtrs[] = { second.data() };

    runWasmProcess (graph, secondPtrs, 1, outPtrs, 1, 32);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (input[static_cast<size_t> (28 + i)], output[static_cast<size_t> (i)]) << "carry-over at " << i;

    for (int i = 4; i < 32; ++i)
        EXPECT_FLOAT_EQ (second[static_cast<size_t> (i - 4)], output[static_cast<size_t> (i)]) << "delayed at " << i;
}

TEST (YdspWasmBackendTests, IntegerMinMaxClampAbsSignMatchTheComposedSelectLowering)
{
    YdspCompiler compiler;

    auto graph = compilePatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                let n = int32 (in * 100.0) - 50;
                let mn = min (n, 3);
                let mx = max (n, -3);
                let cl = clamp (n, -10, 10);
                let ab = abs (n);
                let sg = sign (n);
                out = float32 (mn + mx + cl + ab + sg);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 100);

    const auto input = makeRamp (100);
    std::vector<float> output (100, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 100);

    for (int i = 0; i < 100; ++i)
    {
        const auto n = static_cast<int> (input[static_cast<size_t> (i)] * 100.0f) - 50;
        const auto mn = n < 3 ? n : 3;
        const auto mx = n > -3 ? n : -3;
        const auto clamped = n < -10 ? -10 : (n > 10 ? 10 : n);
        const auto ab = n < 0 ? -n : n;
        const auto sg = n > 0 ? 1 : (n < 0 ? -1 : 0);
        const auto expected = static_cast<float> (mn + mx + clamped + ab + sg);

        EXPECT_NEAR (expected, output[static_cast<size_t> (i)], 1e-3f) << "at " << i;
    }
}

TEST (YdspWasmBackendTests, SubgraphIsInlinedBeforeCodegen)
{
    YdspCompiler compiler;
    auto graph = compilePatch (wasmSubgraphSource, compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    EXPECT_TRUE (graph.hasParameter ("s.a.g"));
    EXPECT_NEAR (4.0f, graph.getParameter ("s.a.g"), 1e-6f);

    const auto input = makeRamp (64, 0.25f);
    std::vector<float> output (64, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 64);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (input[static_cast<size_t> (i)] * 4.0f, output[static_cast<size_t> (i)], 1e-4f);
}

#endif // YUP_WASM
