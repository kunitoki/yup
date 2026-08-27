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

std::unique_ptr<YdspIrProgram> vectorizerBuildIr (StringRef source, DspJitDiagnostics& diagnostics)
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
    DspJitDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (processorBody), diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, ir);

    if (ir == nullptr || ir->kernels.empty())
        return false;

    return ir->kernels[0]->vectorized;
}

DspJitGraph vectorizerCompileGraph (StringRef source, DspJitCompiler& compiler)
{
    auto result = compiler.compile (source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

void vectorizerRunBlock (DspJitGraph& graph, const float* input, float* output, int numSamples)
{
    std::vector<DspJitInputBuffer> inputs;
    inputs.emplace_back (Span<const float> (input, static_cast<size_t> (numSamples)));

    std::vector<DspJitOutputBuffer> outputs;
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

} // namespace

//==============================================================================

TEST (YdspVectorizerTests, WidensAConstantBoundBankLoop)
{
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;
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
    DspJitDiagnostics diagnostics;

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

TEST (YdspVectorizerTests, RejectsATripCountThatIsNotAWholeNumberOfVectors)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[6];
            process {
                for i in 0..6 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
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

TEST (YdspVectorizerTests, RejectsStreamAccessesInsideTheLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process block {
                for i in 0..8 { out[i] = z[i] + in[i]; }
            }
        }
    )YDSP"));
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
    DspJitDiagnostics diagnostics;

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
    DspJitCompiler compiler;
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
        DspJitCompiler compiler;

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
    DspJitCompiler compiler;
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
    DspJitCompiler compiler;
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
        EXPECT_EQ (YdspVectorizer::vectorWidth, kernel.vectorWidth);

        EXPECT_EQ (8, kernel.boundedIterationCount);
    }

#if ! YUP_WASM
    EXPECT_TRUE (foundVectorized);
#else
    EXPECT_FALSE (foundVectorized); // the wasm backend stays scalar
#endif
}

#if ! YUP_WASM

TEST (YdspVectorizerTests, EmitsPackedInstructions)
{
    DspJitCompiler compiler;
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
