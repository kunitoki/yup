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
#include <bit>
#include <cmath>
#include <limits>
#include <vector>

#if YUP_WASM

#include "yup_YdspTestPatches.h"

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

    const yup::MidiBuffer* processEventInputs[] { midi };

    const auto result = graph.process (yup::YdspProcessRequest {
        inputBuffers,
        outputBuffers,
        numSamples,
        yup::Span<const yup::MidiBuffer*> (processEventInputs, graph.getEventInputCount() > 0 ? 1 : 0),
        {}
    });
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
    processor P { input stream in; output stream out; input parameter float gain = 0.5; process { out = in * gain; } }
    graph G { input stream x; output stream y; input parameter float gain = 1.0; node p = P; connection { x -> p.in; p.out -> y; gain -> p.gain; } }
)YDSP";

constexpr const char* wasmSmoothSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;
        input parameter float gain = 0.5 [[ smoothing: 0.0002 ]];
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
    processor Gain { input stream in; output stream out; input parameter float g = 2.0; process { out = in * g; } }
    graph Sub {
        input stream in;
        output stream out;
        input parameter float amount = 3.0;
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

TEST (YdspWasmBackendTests, VectorizedStateBankMatchesScalarReference)
{
    // The automatic tier vectorises this constant-bound bank loop to f32x4
    // (the emscripten build compiles with -msimd128), so the module runs the
    // packed lowering: v128 load/store, f32x4 arithmetic, splats and the
    // shuffle-based horizontal reduction. Compare its output against the
    // scalar recurrence (EXPECT_FLOAT_EQ tolerates the reassociated sum).
    YdspCompiler compiler;
    auto graph = compilePatch (R"YDSP(
        let modes = 8;

        processor P {
            input stream in;
            output stream out;

            state float z[modes];

            process {
                float sum = 0.0;

                for i in 0..modes {
                    z[i] = z[i] * 0.5 + in;
                    sum = sum + z[i];
                }

                out = sum;
            }
        }

        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    const std::vector<float> ones (32, 1.0f);
    std::vector<float> output (32, 0.0f);

    const float* inPtrs[] = { ones.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 32);

    float z[8] = {};
    std::vector<float> reference (32, 0.0f);

    for (int s = 0; s < 32; ++s)
    {
        float sum = 0.0f;

        for (int i = 0; i < 8; ++i)
        {
            z[i] = z[i] * 0.5f + 1.0f;
            sum += z[i];
        }

        reference[static_cast<size_t> (s)] = sum;
    }

    for (int s = 0; s < 32; ++s)
        EXPECT_FLOAT_EQ (reference[static_cast<size_t> (s)], output[static_cast<size_t> (s)]);
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
            input parameter float gain = 0.5;
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

TEST (YdspWasmBackendTests, VectorizedInt32BankMatchesScalarReference)
{
    YdspCompiler compiler;
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int acc[11];
            process {
                let drive = int (in);
                for i in 0..11 {
                    let x = acc[i] + drive - 3;
                    let scaled = x * 2;
                    let clamped = clamp (abs (scaled), 1, 200);
                    let masked = ((clamped & 127) | 4) ^ 1;
                    let shifted = (masked << 2) >> 1;
                    acc[i] = select (shifted >= 20, min (shifted, 150), max (-shifted, -50));
                }
                out = float (acc[0] + acc[3] + acc[10]);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
    auto scalarResult = compiler.compile (source, baselineOptions);
    ASSERT_TRUE (scalarResult.wasOk());
    auto referenceGraph = std::move (scalarResult).getValue();
    referenceGraph.prepare (44100.0, 16);

    const std::vector<float> input { 10.0f, -5.0f, 0.0f, 33.0f, -12.0f, 7.0f, 100.0f, -40.0f,
                                     1.0f, -1.0f, 15.0f, -20.0f, 4.0f, 8.0f, -9.0f, 25.0f };
    std::vector<float> output (16, 0.0f), reference (16, 0.0f);

    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };
    float* refPtrs[] = { reference.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 16);
    runWasmProcess (referenceGraph, inPtrs, 1, refPtrs, 1, 16);

    for (int i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ (reference[static_cast<size_t> (i)], output[static_cast<size_t> (i)]) << "at " << i;
}

TEST (YdspWasmBackendTests, VectorizedFloatIntConversionsAndSelectMatchScalarReference)
{
    YdspCompiler compiler;
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    let n = int (in[i]);
                    let adjusted = select (n < 0, n - 5, n + 7);
                    let f = float (adjusted);
                    out[i] = select (n == 0, 99.0, f);
                }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
    auto scalarResult = compiler.compile (source, baselineOptions);
    ASSERT_TRUE (scalarResult.wasOk());
    auto referenceGraph = std::move (scalarResult).getValue();

    const std::vector<float> input {
        0.0f, -0.0f, 3.75f, -4.25f,
        2147483520.0f, 2147483648.0f, -2147483648.0f, -1e12f,
        1e12f, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(),
        42.5f, -19.9f, 0.25f, -0.25f,
        100.0f, -100.0f, 7.0f
    };

    const int numSamples = static_cast<int> (input.size());
    graph.prepare (44100.0, numSamples);
    referenceGraph.prepare (44100.0, numSamples);

    std::vector<float> output (input.size(), 0.0f), reference (input.size(), 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };
    float* refPtrs[] = { reference.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, numSamples);
    runWasmProcess (referenceGraph, inPtrs, 1, refPtrs, 1, numSamples);

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_FLOAT_EQ (reference[i], output[i]) << "at " << i;
}

namespace
{

std::vector<uint8_t> compileBackendWasmBytes (StringRef source, const char* kernelName, YdspDiagnostics& diagnostics, bool enableVectorization = false)
{
    YdspLexer lexer (source, diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();

    if (program == nullptr || diagnostics.hasErrors())
        return {};

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));

    if (analyzed == nullptr || diagnostics.hasErrors())
        return {};

    YdspOptimizer optimizer (diagnostics);

    if (enableVectorization)
    {
        optimizer.setVectorizationEnabled (true);
        optimizer.setVectorWidth (4);
    }

    auto ir = optimizer.build (*analyzed);

    if (ir == nullptr || diagnostics.hasErrors())
        return {};

    for (const auto& fn : ir->kernels)
    {
        if (fn->name == kernelName)
            return YdspWasmCodegen::compile (*fn, diagnostics);
    }

    return {};
}

int countWatOccurrences (const String& wat, const String& needle)
{
    int count = 0;
    int pos = 0;

    while ((pos = wat.indexOf (pos, needle)) >= 0)
    {
        ++count;
        pos += needle.length();
    }

    return count;
}

} // namespace

TEST (YdspWasmBackendTests, StackForwardingAndLocalTeeReduceLocalsInArithmeticChain)
{
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                let t = in * 0.5 + 0.25;
                let y = (t * t + t) * 1.5 - 0.125;
                out = y;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileBackendWasmBytes (source, "P", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto wat = YdspWasmCodegen::toText (bytes);
    const auto loopWat = wat.fromFirstOccurrenceOf ("loop", false, false);
    EXPECT_GT (countWatOccurrences (wat, "local.tee"), 0);

    // Every single-use intermediate in the chain stays on the operand stack,
    // leaving only the stored output value and loop induction update as local.set.
    EXPECT_LE (countWatOccurrences (loopWat, "local.set"), 2);

    YdspCompiler compiler;
    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 64);

    const auto input = makeRamp (64, -0.5f);
    std::vector<float> output (64, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 64);

    for (size_t i = 0; i < input.size(); ++i)
    {
        const float t = input[i] * 0.5f + 0.25f;
        const float expected = (t * t + t) * 1.5f - 0.125f;
        EXPECT_FLOAT_EQ (expected, output[i]) << "at " << i;
    }
}

TEST (YdspWasmBackendTests, FmaSelectClampAndAdvanceWrapPreserveExactResults)
{
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                let d = in @ 4;
                let f = fma (in, 0.5, d);
                let c = clamp (f, -0.5, 0.75);
                let n = clamp (int32 (in * 10.0), -2, 3);
                out = select (in > 0.2, c, float32 (n) * 0.1);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileBackendWasmBytes (source, "P", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto wat = YdspWasmCodegen::toText (bytes);

    // advanceWrapI lowers through branchless select + local.tee instead of an
    // if (result i32) block.
    EXPECT_FALSE (wat.contains ("if (result i32)"));
    EXPECT_GT (countWatOccurrences (wat, "select"), 0);

    YdspCompiler compiler;
    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    const std::vector<float> input { 0.0f, 0.1f, 0.25f, 0.5f, -0.4f, 0.8f, -0.1f, 0.3f,
                                     1.0f, -0.6f, 0.15f, 0.4f, -0.25f, 0.7f, 0.05f, -0.9f };
    std::vector<float> output (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 16);

    for (size_t i = 0; i < input.size(); ++i)
    {
        const float d = i >= 4 ? input[i - 4] : 0.0f;
        const float f = std::fma (input[i], 0.5f, d);
        const float c = std::clamp (f, -0.5f, 0.75f);
        const int n = std::clamp (static_cast<int> (input[i] * 10.0f), -2, 3);
        const float expected = input[i] > 0.2f ? c : static_cast<float> (n) * 0.1f;
        EXPECT_NEAR (expected, output[i], 1e-6f) << "at " << i;
    }
}

TEST (YdspWasmBackendTests, HoistedStreamPointersAvoidRedundantCtxLoadsInLoop)
{
    const auto source = R"YDSP(
        processor P {
            input stream in0, in1;
            output stream out0, out1;
            process block {
                for i in 0..blockSize {
                    out0[i] = in0[i] + in1[i];
                    out1[i] = in0[i] - in1[i];
                }
            }
        }
        graph G {
            input stream x0, x1;
            output stream y0, y1;
            node p = P;
            connection { x0 -> p.in0; x1 -> p.in1; p.out0 -> y0; p.out1 -> y1; }
        }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileBackendWasmBytes (source, "P", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto wat = YdspWasmCodegen::toText (bytes);
    const auto loopWat = wat.fromFirstOccurrenceOf ("loop", false, false);

    // ctx->inputs / ctx->outputs and each channel pointer are hoisted into the
    // prologue, leaving zero i32.load instructions inside the sample loop.
    EXPECT_EQ (3, countWatOccurrences (wat, "i32.load offset=0"));
    EXPECT_EQ (3, countWatOccurrences (wat, "i32.load offset=4"));
    EXPECT_FALSE (loopWat.contains ("i32.load"));
    EXPECT_GT (countWatOccurrences (wat, "local.tee"), 0);

    YdspCompiler compiler;
    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 32);

    const auto in0 = makeRamp (32, 0.0f);
    const auto in1 = makeRamp (32, 1.0f);
    std::vector<float> out0 (32, 0.0f), out1 (32, 0.0f);
    const float* inPtrs[] = { in0.data(), in1.data() };
    float* outPtrs[] = { out0.data(), out1.data() };

    runWasmProcess (graph, inPtrs, 2, outPtrs, 2, 32);

    for (size_t i = 0; i < 32; ++i)
    {
        EXPECT_FLOAT_EQ (in0[i] + in1[i], out0[i]) << "at " << i;
        EXPECT_FLOAT_EQ (in0[i] - in1[i], out1[i]) << "at " << i;
    }
}

#if defined (__wasm_simd128__)
TEST (YdspWasmBackendTests, VectorizedKernelUsesLocalTeeAndMatchesScalarReference)
{
    const auto source = R"YDSP(
        let modes = 8;

        processor P {
            input stream in;
            output stream out;
            state float z[modes];
            process {
                float sum = 0.0;
                for i in 0..modes {
                    let v = z[i] * 0.5 + in;
                    z[i] = v;
                    sum = sum + v * v;
                }
                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileBackendWasmBytes (source, "P", diagnostics, true);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto wat = YdspWasmCodegen::toText (bytes);
    EXPECT_GT (countWatOccurrences (wat, "local.tee"), 0);
    EXPECT_TRUE (wat.contains ("f32x4.mul"));
    EXPECT_TRUE (wat.contains ("f32x4.add"));
    EXPECT_TRUE (wat.contains ("f32x4.extract_lane"));

    YdspCompiler compiler;
    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
    auto scalarResult = compiler.compile (source, baselineOptions);
    ASSERT_TRUE (scalarResult.wasOk());
    auto referenceGraph = std::move (scalarResult).getValue();
    referenceGraph.prepare (44100.0, 16);

    const auto input = makeRamp (16, 0.1f);
    std::vector<float> output (16, 0.0f), reference (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };
    float* refPtrs[] = { reference.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 16);
    runWasmProcess (referenceGraph, inPtrs, 1, refPtrs, 1, 16);

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR (reference[i], output[i], 1e-5f) << "at " << i;
}

TEST (YdspWasmBackendTests, FastMathBitcastExecutesAccuratelyInWasm)
{
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = tanh (in) + exp (in * 0.25); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileBackendWasmBytes (source, "P", diagnostics, false);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    const auto wat = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (wat.contains ("i32.reinterpret_f32"));
    EXPECT_TRUE (wat.contains ("f32.reinterpret_i32"));

    YdspCompiler compiler;
    YdspCompileOptions options;
    options.fastMath = true;
    auto compiled = compiler.compile (source, options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
    auto graph = std::move (compiled).getValue();
    graph.prepare (44100.0, 32);

    const auto input = makeRamp (32, -1.0f);
    std::vector<float> output (32, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 32);

    for (size_t i = 0; i < input.size(); ++i)
    {
        const float expected = std::tanh (input[i]) + std::exp (input[i] * 0.25f);
        EXPECT_NEAR (expected, output[i], 1e-4f) << "at " << i;
    }
}

TEST (YdspWasmBackendTests, VectorizedFastMathAndSlpLaneOpsExecuteInWasm)
{
    const auto source = R"YDSP(
        let modes = 8;

        processor P {
            input stream in;
            output stream out;
            state float z[modes];
            process {
                float sum = 0.0;
                for i in 0..modes {
                    let v = tanh (z[i] * 0.25 + in);
                    z[i] = v;
                    sum = sum + v;
                }
                let a = z[0] * 0.5 + 0.1;
                let b = z[2] * 0.5 + 0.2;
                let c = z[5] * 0.5 + 0.3;
                let d = z[7] * 0.5 + 0.4;
                out = sum + (a + b + c + d);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspCompiler compiler;
    auto graph = compilePatch (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 16);

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;
    baselineOptions.fastMath = false;
    auto scalarResult = compiler.compile (source, baselineOptions);
    ASSERT_TRUE (scalarResult.wasOk());
    auto referenceGraph = std::move (scalarResult).getValue();
    referenceGraph.prepare (44100.0, 16);

    const auto input = makeRamp (16, -0.5f);
    std::vector<float> output (16, 0.0f), reference (16, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };
    float* refPtrs[] = { reference.data() };

    runWasmProcess (graph, inPtrs, 1, outPtrs, 1, 16);
    runWasmProcess (referenceGraph, inPtrs, 1, refPtrs, 1, 16);

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR (reference[i], output[i], 1e-4f) << "at " << i;
}
#endif

#endif // YUP_WASM

//==============================================================================

class YdspWasmBitcastAndLaneTests : public ::testing::Test
{
protected:
    struct WasmSection
    {
        uint8_t id = 0;
        std::vector<uint8_t> payload;
    };

    struct WasmFuncType
    {
        std::vector<uint8_t> params;
        std::vector<uint8_t> results;
    };

    static uint32_t readLebU (const std::vector<uint8_t>& data, size_t& pos)
    {
        uint32_t value = 0;
        uint32_t shift = 0;

        while (pos < data.size())
        {
            const auto byte = data[pos++];
            value |= static_cast<uint32_t> (byte & 0x7F) << shift;
            shift += 7;

            if ((byte & 0x80) == 0)
                break;
        }

        return value;
    }

    static std::string readName (const std::vector<uint8_t>& data, size_t& pos)
    {
        const auto length = readLebU (data, pos);
        std::string name;
        name.reserve (length);

        for (uint32_t i = 0; i < length && pos < data.size(); ++i)
            name.push_back (static_cast<char> (data[pos++]));

        return name;
    }

    static std::vector<WasmSection> parseSections (const std::vector<uint8_t>& bytes)
    {
        std::vector<WasmSection> sections;
        size_t pos = 8;

        while (pos < bytes.size())
        {
            WasmSection section;
            section.id = bytes[pos++];
            const auto size = readLebU (bytes, pos);

            if (size > bytes.size() - pos)
                break;

            section.payload.assign (bytes.begin() + static_cast<ptrdiff_t> (pos),
                                    bytes.begin() + static_cast<ptrdiff_t> (pos + size));
            pos += size;
            sections.push_back (std::move (section));
        }

        return sections;
    }

    static std::vector<WasmFuncType> parseFuncTypes (const WasmSection& section)
    {
        std::vector<WasmFuncType> types;
        size_t pos = 0;
        const auto count = readLebU (section.payload, pos);

        for (uint32_t i = 0; i < count; ++i)
        {
            WasmFuncType type;
            (void) section.payload[pos++];

            const auto numParams = readLebU (section.payload, pos);
            type.params.assign (section.payload.begin() + static_cast<ptrdiff_t> (pos),
                                section.payload.begin() + static_cast<ptrdiff_t> (pos + numParams));
            pos += numParams;

            const auto numResults = readLebU (section.payload, pos);
            type.results.assign (section.payload.begin() + static_cast<ptrdiff_t> (pos),
                                 section.payload.begin() + static_cast<ptrdiff_t> (pos + numResults));
            pos += numResults;

            types.push_back (std::move (type));
        }

        return types;
    }

    static std::vector<uint8_t> collectCodeInstructions (const WasmSection& section)
    {
        std::vector<uint8_t> instructions;
        size_t pos = 0;
        const auto count = readLebU (section.payload, pos);

        for (uint32_t i = 0; i < count; ++i)
        {
            const auto bodySize = readLebU (section.payload, pos);
            const auto bodyStart = pos;
            const auto numGroups = readLebU (section.payload, pos);

            for (uint32_t g = 0; g < numGroups; ++g)
            {
                (void) readLebU (section.payload, pos);
                ++pos;
            }

            instructions.insert (instructions.end(),
                                 section.payload.begin() + static_cast<ptrdiff_t> (pos),
                                 section.payload.begin() + static_cast<ptrdiff_t> (bodyStart + bodySize - 1));

            pos = bodyStart + bodySize;
        }

        return instructions;
    }

    static bool containsSubsequence (const std::vector<uint8_t>& haystack, const std::vector<uint8_t>& needle)
    {
        if (needle.empty() || needle.size() > haystack.size())
            return false;

        for (size_t i = 0; i + needle.size() <= haystack.size(); ++i)
        {
            bool match = true;

            for (size_t j = 0; j < needle.size(); ++j)
            {
                if (haystack[i + j] != needle[j])
                {
                    match = false;
                    break;
                }
            }

            if (match)
                return true;
        }

        return false;
    }

    static void expectBalancedStack (const std::vector<uint8_t>& bytes)
    {
        const auto sections = parseSections (bytes);
        ASSERT_EQ (5u, sections.size());
        const auto code = collectCodeInstructions (sections[4]);

        std::vector<WasmFuncType> types;

        for (const auto& section : sections)
            if (section.id == YdspWasmEmitter::sectionType)
                types = parseFuncTypes (section);

        std::vector<int> paramCount;
        std::vector<int> resultCount;

        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionImport)
                continue;

            size_t pos = 0;
            const auto count = readLebU (section.payload, pos);

            for (uint32_t i = 0; i < count; ++i)
            {
                (void) readName (section.payload, pos);
                (void) readName (section.payload, pos);
                const auto kind = section.payload[pos++];

                if (kind == YdspWasmEmitter::importKindFunc)
                {
                    const auto typeIndex = readLebU (section.payload, pos);
                    paramCount.push_back (typeIndex < types.size() ? static_cast<int> (types[typeIndex].params.size()) : 0);
                    resultCount.push_back (typeIndex < types.size() ? static_cast<int> (types[typeIndex].results.size()) : 0);
                }
                else if (kind == YdspWasmEmitter::importKindMemory)
                {
                    const auto limitsFlag = section.payload[pos++];
                    (void) readLebU (section.payload, pos);

                    if ((limitsFlag & 0x01) != 0)
                        (void) readLebU (section.payload, pos);
                }
            }
        }

        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionFunction)
                continue;

            size_t pos = 0;
            const auto count = readLebU (section.payload, pos);

            for (uint32_t i = 0; i < count && pos < section.payload.size(); ++i)
            {
                const auto typeIndex = readLebU (section.payload, pos);
                paramCount.push_back (typeIndex < types.size() ? static_cast<int> (types[typeIndex].params.size()) : 0);
                resultCount.push_back (typeIndex < types.size() ? static_cast<int> (types[typeIndex].results.size()) : 0);
            }
        }

        int depth = 0;
        size_t pos = 0;
        struct ControlFrame
        {
            int entryDepth = 0;
            int results = 0;
        };
        std::vector<ControlFrame> control;

        const auto byte = [&] { return code[pos++]; };
        const auto lebU = [&] { return readLebU (code, pos); };
        const auto delta = [&] (int d)
        {
            depth += d;
            EXPECT_GE (depth, 0) << "value stack underflow at byte offset " << pos;
        };

        while (pos < code.size())
        {
            const auto op = byte();

            switch (op)
            {
                case 0x00:
                case 0x01:
                case 0x0F:
                    break;

                case 0x02:
                case 0x03:
                {
                    const auto type = byte();
                    control.push_back ({ depth, type == 0x40 ? 0 : 1 });
                    break;
                }

                case 0x04:
                {
                    const auto type = byte();
                    delta (-1);
                    control.push_back ({ depth, type == 0x40 ? 0 : 1 });
                    break;
                }

                case 0x05:
                    ASSERT_FALSE (control.empty());
                    EXPECT_EQ (control.back().entryDepth + control.back().results, depth);
                    depth = control.back().entryDepth;
                    break;

                case 0x0B:
                    if (! control.empty())
                    {
                        EXPECT_EQ (control.back().entryDepth + control.back().results, depth);
                        control.pop_back();
                    }
                    break;

                case 0x0C:
                    (void) lebU();
                    break;

                case 0x0D:
                    (void) lebU();
                    delta (-1);
                    break;

                case 0x10:
                {
                    const auto index = lebU();
                    const int params = index < paramCount.size() ? paramCount[index] : 0;
                    const int results = index < resultCount.size() ? resultCount[index] : 0;
                    delta (results - params);
                    break;
                }

                case 0x1A:
                    delta (-1);
                    break;

                case 0x1B:
                    delta (-2);
                    break;

                case 0x20:
                case 0x21:
                case 0x22:
                    (void) lebU();
                    delta (op == 0x20 ? 1 : (op == 0x21 ? -1 : 0));
                    break;

                case 0x41:
                case 0x42:
                    (void) lebU();
                    delta (1);
                    break;

                case 0x43:
                    pos += 4;
                    delta (1);
                    break;

                case 0x44:
                    pos += 8;
                    delta (1);
                    break;

                case 0x28: case 0x29: case 0x2A: case 0x2B:
                case 0x2C: case 0x2D: case 0x2E: case 0x2F:
                case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
                    (void) lebU();
                    (void) lebU();
                    break;

                case 0x36: case 0x37: case 0x38: case 0x39:
                case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E:
                    (void) lebU();
                    (void) lebU();
                    delta (-2);
                    break;

                case 0xFC:
                    EXPECT_LT (lebU(), 8u);
                    break;

                case 0x45: case 0x50:
                case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: case 0x90: case 0x91:
                case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
                case 0xA7: case 0xAC: case 0xAD: case 0xB6: case 0xBB:
                case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                case 0xA8: case 0xAA: case 0xAE: case 0xB0:
                case 0xB2: case 0xB4: case 0xB7: case 0xB9:
                    break;

                case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4E:
                case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x59:
                case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: case 0x60:
                case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66:
                case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
                case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
                case 0x7C: case 0x7D: case 0x7E: case 0x7F: case 0x80: case 0x81:
                case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
                case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98:
                case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6:
                    delta (-1);
                    break;

#if defined (__wasm_simd128__)
                case YdspWasmEmitter::simdPrefix:
                {
                    const auto sub = lebU();

                    switch (sub)
                    {
                        case YdspWasmEmitter::opV128Load:
                            (void) lebU();
                            (void) lebU();
                            break;

                        case YdspWasmEmitter::opV128Store:
                            (void) lebU();
                            (void) lebU();
                            delta (-2);
                            break;

                        case YdspWasmEmitter::opV128Const:
                            pos += 16;
                            delta (1);
                            break;

                        case YdspWasmEmitter::opI8x16Shuffle:
                            pos += 16;
                            delta (-1);
                            break;

                        case YdspWasmEmitter::opI32x4ExtractLane:
                        case YdspWasmEmitter::opF32x4ExtractLane:
                            (void) byte();
                            break;

                        case YdspWasmEmitter::opI32x4ReplaceLane:
                        case YdspWasmEmitter::opF32x4ReplaceLane:
                            (void) byte();
                            delta (-1);
                            break;

                        case YdspWasmEmitter::opI32x4Splat:
                        case YdspWasmEmitter::opF32x4Splat:
                        case YdspWasmEmitter::opF32x4Ceil:
                        case YdspWasmEmitter::opF32x4Floor:
                        case YdspWasmEmitter::opF32x4Nearest:
                        case YdspWasmEmitter::opV128Not:
                        case YdspWasmEmitter::opI32x4Abs:
                        case YdspWasmEmitter::opI32x4Neg:
                        case YdspWasmEmitter::opF32x4Abs:
                        case YdspWasmEmitter::opF32x4Neg:
                        case YdspWasmEmitter::opF32x4Sqrt:
                        case YdspWasmEmitter::opI32x4TruncSatF32x4S:
                        case YdspWasmEmitter::opF32x4ConvertI32x4S:
                            break;

                        case YdspWasmEmitter::opV128Bitselect:
                            delta (-2);
                            break;

                        default:
                            delta (-1);
                            break;
                    }

                    break;
                }
#endif

                default:
                    FAIL() << "unhandled opcode 0x" << std::hex << static_cast<int> (op) << " at byte offset " << std::dec << pos;
            }
        }

        EXPECT_EQ (0, depth) << "value stack not balanced at end of code section";
        EXPECT_TRUE (control.empty());
    }

    static int newValue (YdspIrFunction& fn, YdspValueType type, int lanes = 1)
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
        {
            fn.valueLanes.push_back (lanes);
        }
        else if (lanes > 1)
        {
            fn.valueLanes.resize (static_cast<size_t> (id), 1);
            fn.valueLanes.push_back (lanes);
        }

        return id;
    }

    static int emitInst (YdspIrFunction& fn,
                         YdspIrOp op,
                         int result = -1,
                         int a = -1,
                         int b = -1,
                         int memIndex = -1,
                         double fvalue = 0.0,
                         int64_t ivalue = 0)
    {
        YdspIrInst inst;
        inst.op = op;
        inst.result = result;
        inst.a = a;
        inst.b = b;
        inst.memIndex = memIndex;
        inst.fvalue = fvalue;
        inst.ivalue = ivalue;
        fn.blocks[0].insts.push_back (inst);
        return result;
    }

    static std::vector<uint8_t> compileSourceForKernel (StringRef source, const char* kernelName, YdspDiagnostics& diagnostics, bool enableVectorization, bool enableFastMath)
    {
        YdspLexer lexer (source, diagnostics);
        auto tokens = lexer.tokenize();

        YdspParser parser (std::move (tokens), diagnostics);
        auto program = parser.parseProgram();

        if (program == nullptr || diagnostics.hasErrors())
            return {};

        YdspSemanticAnalyzer analyzer (diagnostics);
        auto analyzed = analyzer.analyze (std::move (program));

        if (analyzed == nullptr || diagnostics.hasErrors())
            return {};

        YdspOptimizer optimizer (diagnostics);
        optimizer.setFastMath (enableFastMath);

        if (enableVectorization)
        {
            optimizer.setVectorizationEnabled (true);
            optimizer.setVectorWidth (4);
        }

        auto ir = optimizer.build (*analyzed);

        if (ir == nullptr || diagnostics.hasErrors())
            return {};

        for (const auto& fn : ir->kernels)
            if (fn->name == kernelName)
                return YdspWasmCodegen::compile (*fn, diagnostics);

        return {};
    }
};

TEST_F (YdspWasmBitcastAndLaneTests, ScalarBitcastEmitsReinterpretOpcodesAndBalancedStack)
{
    YdspIrFunction fn;
    fn.name = "bitcast_scalar";
    fn.numParams = 1;
    fn.numParamsOut = 1;
    fn.paramTypes = { YdspValueType::float32Type };
    fn.paramOutTypes = { YdspValueType::float32Type };
    fn.blocks.resize (1);

    const int x = emitInst (fn, YdspIrOp::loadParam, newValue (fn, YdspValueType::float32Type), 0);
    const int bits = emitInst (fn, YdspIrOp::bitcastFtoI, newValue (fn, YdspValueType::int32Type), x);
    const int mask = emitInst (fn, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, 0.0, 0x7FFFFFFF);
    const int masked = emitInst (fn, YdspIrOp::andI, newValue (fn, YdspValueType::int32Type), bits, mask);
    const int absVal = emitInst (fn, YdspIrOp::bitcastItoF, newValue (fn, YdspValueType::float32Type), masked);
    emitInst (fn, YdspIrOp::storeParamOut, -1, absVal, -1, 0);

    YdspDiagnostics diagnostics;
    const auto bytes = YdspWasmCodegen::compile (fn, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    expectBalancedStack (bytes);

    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opI32ReinterpretF32 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::opF32ReinterpretI32 }));

    const auto wat = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (wat.contains ("i32.reinterpret_f32"));
    EXPECT_TRUE (wat.contains ("f32.reinterpret_i32"));
}

TEST_F (YdspWasmBitcastAndLaneTests, FastMathSourceEmitsReinterpretOpcodesWithBalancedStack)
{
    const auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = sin (in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileSourceForKernel (source, "P", diagnostics, false, true);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    expectBalancedStack (bytes);

    const auto wat = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (wat.contains ("i32.reinterpret_f32"));
    EXPECT_TRUE (wat.contains ("f32.reinterpret_i32"));
}

#if defined (__wasm_simd128__)
TEST_F (YdspWasmBitcastAndLaneTests, VectorBitcastIsZeroCostPassThroughOnV128)
{
    YdspIrFunction fn;
    fn.name = "bitcast_vec";
    fn.vectorized = true;
    fn.vectorWidth = 4;
    fn.numParams = 1;
    fn.numParamsOut = 1;
    fn.paramTypes = { YdspValueType::float32Type };
    fn.paramOutTypes = { YdspValueType::float32Type };
    fn.blocks.resize (1);

    const int x = emitInst (fn, YdspIrOp::loadParam, newValue (fn, YdspValueType::float32Type, 1), 0);
    const int vx = emitInst (fn, YdspIrOp::vsplat, newValue (fn, YdspValueType::float32Type, 4), x);
    const int bits = emitInst (fn, YdspIrOp::bitcastFtoI, newValue (fn, YdspValueType::int32Type, 4), vx);
    const int maskScalar = emitInst (fn, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type, 1), -1, -1, -1, 0.0, 0x7FFFFFFF);
    const int vmask = emitInst (fn, YdspIrOp::vsplat, newValue (fn, YdspValueType::int32Type, 4), maskScalar);
    const int masked = emitInst (fn, YdspIrOp::andI, newValue (fn, YdspValueType::int32Type, 4), bits, vmask);
    const int back = emitInst (fn, YdspIrOp::bitcastItoF, newValue (fn, YdspValueType::float32Type, 4), masked);
    const int sum = emitInst (fn, YdspIrOp::vreduceAddF, newValue (fn, YdspValueType::float32Type, 1), back);
    emitInst (fn, YdspIrOp::storeParamOut, -1, sum, -1, 0);

    YdspDiagnostics diagnostics;
    const auto bytes = YdspWasmCodegen::compile (fn, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    expectBalancedStack (bytes);

    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);
    EXPECT_FALSE (containsSubsequence (code, { YdspWasmEmitter::opI32ReinterpretF32 }));
    EXPECT_FALSE (containsSubsequence (code, { YdspWasmEmitter::opF32ReinterpretI32 }));
}

TEST_F (YdspWasmBitcastAndLaneTests, VectorInsertAndExtractEmitExpectedSimdOpcodesAndBalancedStack)
{
    YdspIrFunction fn;
    fn.name = "lane_insert_extract";
    fn.vectorized = true;
    fn.vectorWidth = 4;
    fn.numParams = 2;
    fn.numParamsOut = 1;
    fn.paramTypes = { YdspValueType::float32Type, YdspValueType::float32Type };
    fn.paramOutTypes = { YdspValueType::float32Type };
    fn.blocks.resize (1);

    const int p0 = emitInst (fn, YdspIrOp::loadParam, newValue (fn, YdspValueType::float32Type, 1), 0);
    const int p1 = emitInst (fn, YdspIrOp::loadParam, newValue (fn, YdspValueType::float32Type, 1), 1);
    const int v0 = emitInst (fn, YdspIrOp::vinsertF, newValue (fn, YdspValueType::float32Type, 4), -1, p0, 0);
    const int sum01 = emitInst (fn, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type, 1), p0, p1);
    const int v1 = emitInst (fn, YdspIrOp::vinsertF, newValue (fn, YdspValueType::float32Type, 4), v0, sum01, 1);
    const int c2 = emitInst (fn, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type, 1), -1, -1, -1, 2.5);
    const int v2 = emitInst (fn, YdspIrOp::vinsertF, newValue (fn, YdspValueType::float32Type, 4), v1, c2, 2);
    const int c3 = emitInst (fn, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type, 1), -1, -1, -1, -1.25);
    const int v3 = emitInst (fn, YdspIrOp::vinsertF, newValue (fn, YdspValueType::float32Type, 4), v2, c3, 3);
    const int e1 = emitInst (fn, YdspIrOp::vextractF, newValue (fn, YdspValueType::float32Type, 1), v3, -1, 1);
    const int e2 = emitInst (fn, YdspIrOp::vextractF, newValue (fn, YdspValueType::float32Type, 1), v3, -1, 2);
    const int total = emitInst (fn, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type, 1), e1, e2);
    emitInst (fn, YdspIrOp::storeParamOut, -1, total, -1, 0);

    YdspDiagnostics diagnostics;
    const auto bytes = YdspWasmCodegen::compile (fn, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    expectBalancedStack (bytes);

    const auto sections = parseSections (bytes);
    const auto code = collectCodeInstructions (sections[4]);
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opV128Const, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ReplaceLane, 0 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ReplaceLane, 1 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ReplaceLane, 2 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ReplaceLane, 3 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ExtractLane, 1 }));
    EXPECT_TRUE (containsSubsequence (code, { YdspWasmEmitter::simdPrefix, YdspWasmEmitter::opF32x4ExtractLane, 2 }));

    const auto wat = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (wat.contains ("v128.const"));
    EXPECT_TRUE (wat.contains ("f32x4.replace_lane 0"));
    EXPECT_TRUE (wat.contains ("f32x4.replace_lane 3"));
    EXPECT_TRUE (wat.contains ("f32x4.extract_lane 1"));
    EXPECT_TRUE (wat.contains ("f32x4.extract_lane 2"));
}

TEST_F (YdspWasmBitcastAndLaneTests, SlpVectorizedSourceEmitsBalancedGatherAndExtract)
{
    const auto source = R"YDSP(
        processor P {
            input stream in0, in1, in2, in3;
            output stream out;
            state float z0, z1, z2, z3;
            process {
                z0 = z0 * 0.5 + in0;
                z1 = z1 * 0.5 + in1;
                z2 = z2 * 0.5 + in2;
                z3 = z3 * 0.5 + in3;
                out = z0 + z1 + z2 + z3;
            }
        }
        graph G {
            input stream x0, x1, x2, x3;
            output stream y;
            node p = P;
            connection { x0 -> p.in0; x1 -> p.in1; x2 -> p.in2; x3 -> p.in3; p.out -> y; }
        }
    )YDSP";

    YdspDiagnostics diagnostics;
    const auto bytes = compileSourceForKernel (source, "P", diagnostics, true, true);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_FALSE (bytes.empty());

    expectBalancedStack (bytes);

    const auto wat = YdspWasmCodegen::toText (bytes);
    EXPECT_TRUE (wat.contains ("f32x4.replace_lane"));
    EXPECT_TRUE (wat.contains ("f32x4.extract_lane"));
}
#endif
