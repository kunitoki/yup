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

#include <iostream>
#include <memory>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================

std::unique_ptr<YdspIrProgram> vectorizerBuildIr (StringRef source, YdspDiagnostics& diagnostics)
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
    optimizer.setVectorizationEnabled (true);

    return optimizer.build (*analyzed);
}

String vectorizerPatch (StringRef processorBody)
{
    return String (processorBody)
         + "\ngraph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";
}

int vectorizerCountInst (const YdspIrFunction& fn, YdspIrOp op)
{
    int count = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == op)
                ++count;

    return count;
}

bool vectorizerWidens (StringRef processorBody)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (processorBody), diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, ir);

    if (ir == nullptr || ir->kernels.empty())
        return false;

    return ir->kernels[0]->vectorized;
}

YdspAudioGraph vectorizerCompileGraph (StringRef source, YdspCompiler& compiler)
{
    auto result = compiler.compile (source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

YdspAudioGraph vectorizerCompileGraph (StringRef source, YdspCompiler& compiler, const YdspCompileOptions& options)
{
    auto result = compiler.compile (source, options);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

void vectorizerRunBlock (YdspAudioGraph& graph, const float* input, float* output, int numSamples)
{
    std::vector<YdspInputBuffer> inputs;
    inputs.emplace_back (Span<const float> (input, static_cast<size_t> (numSamples)));

    std::vector<YdspOutputBuffer> outputs;
    outputs.emplace_back (Span<float> (output, static_cast<size_t> (numSamples)));

    graph.process (inputs, outputs, numSamples, nullptr, nullptr, 0);
}

constexpr auto vectorizerBankSource = R"YDSP(
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
)YDSP";

constexpr auto vectorizerElementWiseSource = R"YDSP(
    let taps = 4;

    processor P {
        input stream in;
        output stream out;

        state float a[taps];

        process {
            for i in 0..taps {
                a[i] = a[i] * 0.5 + in * 0.25;
            }

            out = a[0] + a[3];
        }
    }
)YDSP";

// Three loops that fail for three different reasons, plus one that widens:
// loop 0 widens (constant span, streams at the induction), loop 1 trips on a
// select consuming a widened comparison, loop 2 is shorter than one vector.
// (The state arrays avoid the built-in constant names `pi`, `e` and `inf`.)
constexpr auto vectorizerMixedReasonsSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;

        state float z[8];
        state float w[8];
        state float r[2];

        process block {
            for i in 0..8 { z[i] = z[i] * 0.5 + in[i]; }
            for j in 0..8 { w[j] = select (w[j] > in[0], w[j], 0.0); }
            for k in 0..2 { r[k] = r[k] * 0.5; }

            out[0] = z[0] + w[0] + r[0];
        }
    }
)YDSP";

} // namespace

//==============================================================================

TEST (YdspVectorizerTests, WidensAConstantBoundBankLoop)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (vectorizerBankSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (YdspVectorizer::vectorWidth, fn.vectorWidth);

    EXPECT_GT (vectorizerCountInst (fn, YdspIrOp::vsplat), 0);
    EXPECT_EQ (1, vectorizerCountInst (fn, YdspIrOp::vreduceAddF));

    bool foundWidenedLoad = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::loadStateArrayF && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundWidenedLoad = true;

    EXPECT_TRUE (foundWidenedLoad);
}

TEST (YdspVectorizerTests, WidensAnElementWiseLoopWithNoReduction)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (vectorizerElementWiseSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (0, vectorizerCountInst (fn, YdspIrOp::vreduceAddF));
}

TEST (YdspVectorizerTests, IsOffUnlessEnabled)
{
    YdspDiagnostics diagnostics;

    YdspLexer lexer (vectorizerPatch (vectorizerBankSource), diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();
    ASSERT_NE (nullptr, program);

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));
    ASSERT_NE (nullptr, analyzed);

    YdspOptimizer optimizer (diagnostics); // no setVectorizationEnabled
    auto ir = optimizer.build (*analyzed);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    EXPECT_FALSE (ir->kernels[0]->vectorized);
    EXPECT_EQ (1, ir->kernels[0]->vectorWidth);
    EXPECT_TRUE (ir->kernels[0]->valueLanes.empty());
}

//==============================================================================

TEST (YdspVectorizerTests, WidensATripCountWithAScalarTail)
{
    // 6 modes with 4 lanes: two scalar iterations peeled, one vector trip.
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[6];
            process {
                for i in 0..6 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);

    // The peel is straight-line code in the preheader: one constI index per
    // leading scalar iteration, plus the re-started induction at start + tail.
    const auto& preheader = fn.blocks[static_cast<size_t> (ir->kernels[0]->loops[0].headerBlock - 1)];
    bool foundPeelIndexZero = false;
    bool foundPeelIndexOne = false;
    bool foundVectorStart = false;

    for (const auto& inst : preheader.insts)
    {
        if (inst.op != YdspIrOp::constI)
            continue;

        foundPeelIndexZero = foundPeelIndexZero || inst.ivalue == 0;
        foundPeelIndexOne = foundPeelIndexOne || inst.ivalue == 1;
        foundVectorStart = foundVectorStart || inst.ivalue == 2;
    }

    EXPECT_TRUE (foundPeelIndexZero) << "the first scalar iteration must be peeled at index 0";
    EXPECT_TRUE (foundPeelIndexOne) << "the second scalar iteration must be peeled at index 1";
    EXPECT_TRUE (foundVectorStart) << "the vector loop must start at start + tail = 2";
}

TEST (YdspVectorizerTests, LeavesATripCountShorterThanOneVectorScalar)
{
    // 2 modes never fill a single 4-lane vector: no widening to gain from.
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[2];
            process {
                for i in 0..2 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensAConstantBoundLoopWithANonZeroStart)
{
    // span = 12 - 2 = 10: two leading scalar iterations (i = 2, 3) and two
    // vector trips over [4, 12). The old divisibility-only rule would have
    // stepped from i = 2 by 4 and overrun the bound.
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[12];
            process {
                for i in 2..12 { z[i] = z[i] * 0.5 + in; }
                out = z[2] + z[11];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsARuntimeLoopBound)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[64];
            process block {
                for i in 0..blockSize { z[i] = z[i] * 0.5; }
                out[0] = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsAnIndirectIndex)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 {
                    let j = 7 - i;
                    z[j] = z[j] * 0.5 + in;
                }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsTheInductionVariableUsedAsAValue)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = z[i] * 0.5 + float (i); }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsATranscendentalOnAWidenedValue)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = sin (z[i]) * 0.5 + in; }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsASelectOnWidenedValues)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float w[8];
            process {
                for i in 0..8 { z[i] = select (z[i] > w[i], z[i], w[i]); }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsAScalarStateWriteInsideTheLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float env;
            process {
                for i in 0..8 {
                    z[i] = z[i] * 0.5 + in;
                    env = env * 0.5 + z[i];
                }
                out = env;
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensASampleLoopOverStreams)
{
    // The canonical per-sample gain shape: stream accesses at the induction
    // variable of the blockSize-bound sample loop.
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = in * 0.5 + 0.25; }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (YdspVectorizer::vectorWidth, fn.vectorWidth);

    // One widened stream load and one widened stream store in the loop body.
    bool foundWidenedLoad = false;
    bool foundWidenedStore = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::loadInput && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundWidenedLoad = true;

            if (inst.op == YdspIrOp::storeOutput && fn.laneCountOf (inst.b) == fn.vectorWidth)
                foundWidenedStore = true;
        }

    EXPECT_TRUE (foundWidenedLoad);
    EXPECT_TRUE (foundWidenedStore);

    // The runtime remainder is a second blockSize-bound loop appended after
    // the widened sample loop.
    ASSERT_EQ (2u, fn.loops.size());
    EXPECT_EQ (YdspLoopBoundKind::blockSize, fn.loops[1].bound.kind);
}

TEST (YdspVectorizerTests, WidensABlockSizeBoundStreamLoop)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensAConstantBoundStreamLoop)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..10 { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensAStreamLoopWithAStateArray)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process block {
                for i in 0..blockSize { out[i] = z[i] + in[i]; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensAStreamLoopWithAReduction)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                float s = 0.0;
                for i in 0..blockSize { s = s + in[i]; }
                out[0] = s;
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);

    // The vector accumulator folds into the scalar one in the tail loop's
    // header, and the scalar tail accumulates on top of it.
    EXPECT_EQ (1, vectorizerCountInst (fn, YdspIrOp::vreduceAddF));
}

TEST (YdspVectorizerTests, RejectsAStreamStoreAtANonInductionIndex)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process block {
                for i in 0..8 { out[0] = z[i] + in[i]; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensEveryStreamLoopInAKernel)
{
    // Two blockSize-bound stream loops: widening the first appends its scalar
    // tail loop, which must not shift the second loop's index or be widened
    // itself.
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5; }
                for j in 0..blockSize { out[j] = out[j] + 1.0; }
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);

    // Two widened stream loads (one per loop) and two widened stream stores.
    int widenedLoads = 0;
    int widenedStores = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if ((inst.op == YdspIrOp::loadInput || inst.op == YdspIrOp::loadOutput)
                && fn.laneCountOf (inst.result) == fn.vectorWidth)
                ++widenedLoads;

            if (inst.op == YdspIrOp::storeOutput && fn.laneCountOf (inst.b) == fn.vectorWidth)
                ++widenedStores;
        }

    EXPECT_EQ (2, widenedLoads);
    EXPECT_EQ (2, widenedStores);

    // Each widened loop appended exactly one scalar tail.
    EXPECT_EQ (4u, fn.loops.size());
}

TEST (YdspVectorizerTests, RejectsControlFlowInsideTheLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 {
                    if (in > 0.5) { z[i] = z[i] * 0.5; } else { z[i] = z[i] * 0.25; }
                }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsANestedLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 {
                    for j in 0..4 { z[i] = z[i] * 0.5 + in; }
                }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsALoopWithNoArrayAccessToWiden)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float sum = 0.0;
                for i in 0..8 { sum = sum + in; }
                out = sum;
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsALoopUsingIntegerMin)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int buf[16];
            process {
                for i in 0..16 { buf[i] = min (buf[i], 10); }
                out = in + float32 (buf[0]);
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsAnEmitInsideTheLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = vectorizerBuildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            output event noteOn;
            state float z[8];
            process {
                for i in 0..8 {
                    z[i] = z[i] * 0.5 + in;
                    emit noteOn (pitch: 60, velocity: 0.8) -> noteOn;
                }
                out = z[0];
            }
        }
        graph G { input stream x; output stream y; output event noteOn; node p = P; connection { x -> p.in; p.out -> y; p.noteOn -> noteOn; } }
    )YDSP",
                                  diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());
    EXPECT_FALSE (ir->kernels[0]->vectorized);
}

//==============================================================================

TEST (YdspVectorizerTests, WidenedBankProducesTheExpectedOutput)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerBankSource), compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> input (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 8);

    float expected = 0.0f;

    for (int sample = 0; sample < 8; ++sample)
    {
        expected = expected * 0.5f + 1.0f;
        EXPECT_FLOAT_EQ (8.0f * expected, output[static_cast<size_t> (sample)]) << "sample " << sample;
    }

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, SplitAccumulatorBankProducesTheExpectedOutput)
{
    for (const int modes : { 16, 32 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (String ("let modes = ") + String (modes) + R"YDSP(;

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
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, 8);

        const std::vector<float> input (8, 1.0f);
        std::vector<float> output (8, 0.0f);

        vectorizerRunBlock (graph, input.data(), output.data(), 8);

        float expected = 0.0f;

        for (int sample = 0; sample < 8; ++sample)
        {
            expected = expected * 0.5f + 1.0f;

            EXPECT_FLOAT_EQ (static_cast<float> (modes) * expected, output[static_cast<size_t> (sample)])
                << modes << " modes, sample " << sample;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, WidenedElementWiseLoopIsBitExact)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerElementWiseSource), compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> input (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 8);

    float tap = 0.0f;

    for (int sample = 0; sample < 8; ++sample)
    {
        tap = tap * 0.5f + 0.25f;
        EXPECT_FLOAT_EQ (2.0f * tap, output[static_cast<size_t> (sample)]) << "sample " << sample;
    }

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, ReportsTheLaneCountPerKernel)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerBankSource), compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto& kernels = graph.getExecutionReport().getKernels();
    ASSERT_FALSE (kernels.empty());

    bool foundVectorized = false;

    for (const auto& kernel : kernels)
    {
        if (! kernel.vectorized)
            continue;

        foundVectorized = true;
        EXPECT_TRUE (kernel.vectorWidth == 4 || kernel.vectorWidth == 8);

        EXPECT_EQ (8, kernel.boundedIterationCount);
    }

#if ! YUP_WASM || defined (__wasm_simd128__)
    EXPECT_TRUE (foundVectorized);
#else
    EXPECT_FALSE (foundVectorized); // a scalar wasm build (no -msimd128) stays scalar
#endif
}

#if ! YUP_WASM

TEST (YdspVectorizerTests, EmitsPackedInstructions)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerBankSource), compiler);

    ASSERT_TRUE (graph.isValid());

    const auto listing = graph.getDiagnostics().toString();
    ASSERT_FALSE (listing.isEmpty());

#if ASMJIT_ARCH_ARM
    EXPECT_TRUE (listing.contains ("v0.4s") || listing.contains (".4s"))
        << "expected an ASIMD vector form in the listing";
    EXPECT_TRUE (listing.contains ("faddp"));
#elif ASMJIT_ARCH_X86
    EXPECT_TRUE (listing.contains ("mulps") || listing.contains ("addps"));
    EXPECT_TRUE (listing.contains ("shufps"));
#endif

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << listing << std::endl;
}

#endif

//==============================================================================

TEST (YdspVectorizerTests, PeeledBankProducesTheExpectedOutput)
{
    for (const int modes : { 6, 10, 14 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (String ("let modes = ") + String (modes) + R"YDSP(;

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
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, 8);

        const std::vector<float> input (8, 1.0f);
        std::vector<float> output (8, 0.0f);

        vectorizerRunBlock (graph, input.data(), output.data(), 8);

        float expected = 0.0f;

        for (int sample = 0; sample < 8; ++sample)
        {
            expected = expected * 0.5f + 1.0f;

            EXPECT_FLOAT_EQ (static_cast<float> (modes) * expected, output[static_cast<size_t> (sample)])
                << modes << " modes, sample " << sample;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, PeeledNonZeroStartLoopProducesTheExpectedOutput)
{
    YdspCompiler compiler;

    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            state float z[12];

            process {
                for i in 2..12 { z[i] = z[i] * 0.5 + in; }
                out = z[2] + z[11];
            }
        }
    )YDSP"),
                                         compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    graph.prepare (44100.0, 8);

    const std::vector<float> input (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 8);

    // The state starts zeroed: z[2] and z[11] decay identically per sample.
    float z = 0.0f;

    for (int sample = 0; sample < 8; ++sample)
    {
        z = z * 0.5f + 1.0f;

        EXPECT_FLOAT_EQ (2.0f * z, output[static_cast<size_t> (sample)]) << "sample " << sample;
    }

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, StreamLoopProducesTheExpectedOutput)
{
    for (const int blockSize : { 10, 63 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    for i in 0..blockSize { out[i] = in[i] * 0.5 + in[i] * in[i]; }
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, blockSize);

        std::vector<float> input (static_cast<size_t> (blockSize));
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);

        for (int s = 0; s < blockSize; ++s)
            input[static_cast<size_t> (s)] = 0.25f * static_cast<float> (s);

        vectorizerRunBlock (graph, input.data(), output.data(), blockSize);

        for (int s = 0; s < blockSize; ++s)
        {
            const auto in = input[static_cast<size_t> (s)];

            EXPECT_FLOAT_EQ (in * 0.5f + in * in, output[static_cast<size_t> (s)])
                << blockSize << " samples, sample " << s;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, SampleModeGainProducesTheExpectedOutput)
{
    YdspCompiler compiler;

    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = in * 0.5 + 0.25; }
        }
    )YDSP"),
                                         compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    graph.prepare (44100.0, 10);

    const std::vector<float> input (10, 1.0f);
    std::vector<float> output (10, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 10);

    for (int s = 0; s < 10; ++s)
        EXPECT_FLOAT_EQ (0.75f, output[static_cast<size_t> (s)]) << "sample " << s;

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, StreamLoopReductionProducesTheExpectedOutput)
{
    for (const int blockSize : { 10, 63 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    float s = 0.0;
                    for i in 0..blockSize { s = s + in[i]; }
                    out[0] = s;
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, blockSize);

        // 0.25 is a power of two, so every reassociated partial sum is exact:
        // the folded vector result must equal the scalar-ordered sum bit for bit.
        std::vector<float> input (static_cast<size_t> (blockSize), 0.25f);
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);

        vectorizerRunBlock (graph, input.data(), output.data(), blockSize);

        EXPECT_FLOAT_EQ (0.25f * static_cast<float> (blockSize), output[0])
            << blockSize << " samples";

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

#if ! YUP_WASM

TEST (YdspVectorizerTests, EmitsPackedStreamInstructions)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"),
                                         compiler);

    ASSERT_TRUE (graph.isValid());

    const auto listing = graph.getDiagnostics().toString();
    ASSERT_FALSE (listing.isEmpty());

#if ASMJIT_ARCH_ARM
    EXPECT_TRUE (listing.contains (".4s"))
        << "expected an ASIMD vector form for the stream loop in the listing";
#elif ASMJIT_ARCH_X86
    EXPECT_TRUE (listing.contains ("movups"))
        << "expected packed stream loads/stores in the listing";
    EXPECT_TRUE (listing.contains ("mulps") || listing.contains ("addps"))
        << "expected packed arithmetic in the listing";
#endif

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << listing << std::endl;
}

#endif

//==============================================================================

TEST (YdspVectorizerTests, ReportsWhyEachLoopWasNotVectorized)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (vectorizerMixedReasonsSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    const auto& results = fn.vectorizationResults;

    ASSERT_EQ (3u, results.size());

    EXPECT_TRUE (results[0].widened());
    EXPECT_EQ (0, results[0].loopId);
    EXPECT_EQ (YdspVectorizer::vectorWidth, results[0].laneCount);

    EXPECT_FALSE (results[1].widened());
    EXPECT_EQ (YdspVectorizationReason::unsupportedWidenedOp, results[1].reason);
    EXPECT_TRUE (results[1].describe().contains ("was not vectorized"));
    EXPECT_TRUE (results[1].describe().contains ("select"));

    EXPECT_FALSE (results[2].widened());
    EXPECT_EQ (YdspVectorizationReason::shortTripCount, results[2].reason);

    // The report aggregates the rejection reasons, deduplicated.
    YdspVectorizationReport report;
    report.loops = results;

    EXPECT_EQ (1, report.countWidened());
    EXPECT_EQ (2, report.rejectionReasons().size());
}

TEST (YdspVectorizerTests, ReportsARuntimeStartAsTheReason)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float offset;
            process block {
                let start = int32 (offset);
                for i in start..8 { z[i] = z[i] * 0.5 + in[0]; }
                out[0] = z[0];
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& results = ir->kernels[0]->vectorizationResults;

    ASSERT_EQ (1u, results.size());
    EXPECT_EQ (YdspVectorizationReason::nonConstantStart, results[0].reason);
}

TEST (YdspVectorizerTests, ExecutionReportCarriesPerLoopReasons)
{
#if ! YUP_WASM || defined (__wasm_simd128__)
    YdspCompileOptions options;
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerMixedReasonsSource), compiler, options);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    const auto& kernels = graph.getExecutionReport().getKernels();
    ASSERT_FALSE (kernels.empty());

    const auto& report = kernels[0].loopVectorization;

    ASSERT_EQ (3u, report.loops.size());
    EXPECT_EQ (1, report.countWidened());
    EXPECT_TRUE (report.loops[0].widened());
    EXPECT_EQ (YdspVectorizationReason::unsupportedWidenedOp, report.loops[1].reason);
    EXPECT_EQ (YdspVectorizationReason::shortTripCount, report.loops[2].reason);

    EXPECT_TRUE (report.rejectionReasons().contains ("a select, comparison, transcendental or rounding consumes a widened value"));
    EXPECT_TRUE (report.rejectionReasons().contains ("the loop span is shorter than one vector"));
#endif
}

TEST (YdspVectorizerTests, MissedVectorizationAppearsAsInfoDiagnostics)
{
#if ! YUP_WASM || defined (__wasm_simd128__)
    YdspCompileOptions options;
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerMixedReasonsSource), compiler, options);

    ASSERT_TRUE (graph.isValid());

    // A successful compile moves the diagnostics into the graph.
    const auto text = graph.getDiagnostics().toString();

    EXPECT_TRUE (text.contains ("kernel 'P': loop 1 was not vectorized"))
        << "expected the select rejection as an info diagnostic";
    EXPECT_TRUE (text.contains ("kernel 'P': loop 2 was not vectorized"))
        << "expected the short-trip-count rejection as an info diagnostic";
    EXPECT_FALSE (text.contains ("loop 0 was not vectorized"))
        << "the widened loop must not be reported as a miss";
#endif
}

TEST (YdspVectorizerTests, MissedVectorizationIsSilentUnlessRequested)
{
    // Without emitOptimizationReport no info diagnostics are produced.
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerMixedReasonsSource), compiler);

    ASSERT_TRUE (graph.isValid());

    EXPECT_FALSE (graph.getDiagnostics().toString().contains ("was not vectorized"));
}
