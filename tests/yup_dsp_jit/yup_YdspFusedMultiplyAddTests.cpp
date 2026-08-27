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
#include <memory>
#include <vector>

namespace yup::test
{

namespace
{

//==============================================================================
// The fused multiply-add: `fma (a, b, c)` in a patch, and the contraction pass
// that produces the same operation from `a * b + c`.
//
// The property under test throughout is that `fmaF` means *one* value, not "one
// value per target". A backend with the instruction emits it; a backend without
// one computes the same value in float64 and rounds once. So the tests below
// check the arithmetic against std::fma rather than against whichever lowering
// this build happens to use, and the expansion is checked to produce the same
// answer as the instruction rather than merely to be well-formed.

/** Builds IR the way the native compile path does, with the passes a test asks
    for. Deliberately drives YdspOptimizer rather than DspJitCompiler: the
    contraction switch is not part of the compiler's surface (it belongs to the
    compilation options being added to compile()), and every other pass here is
    already tested this way. */
std::unique_ptr<YdspIrProgram> fmaBuildIr (StringRef source,
                                           DspJitDiagnostics& diagnostics,
                                           bool contract,
                                           bool targetHasFma = true)
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
    optimizer.setContractionEnabled (contract);
    optimizer.setTargetHasFusedMultiplyAdd (targetHasFma);

    return optimizer.build (*analyzed);
}

/** Wraps a processor in the smallest graph that drives it. */
String fmaPatch (StringRef processorBody)
{
    return String (processorBody)
         + "\ngraph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";
}

int fmaCountInst (const YdspIrFunction& fn, YdspIrOp op)
{
    int count = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == op)
                ++count;

    return count;
}

/** The kernel of the single processor in a built program. */
const YdspIrFunction* fmaKernel (const YdspIrProgram& ir)
{
    return ir.kernels.empty() ? nullptr : ir.kernels.front().get();
}

/** Compiles and runs a patch over one block, returning the output. */
std::vector<float> fmaRun (StringRef source, const std::vector<float>& input)
{
    DspJitCompiler compiler;

    auto result = compiler.compile (source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    auto graph = std::move (result).getValue();
    graph.prepare (48000.0, static_cast<int> (input.size()));

    std::vector<float> output (input.size(), 0.0f);

    std::vector<DspJitInputBuffer> inputs { DspJitInputBuffer (Span<const float> (input.data(), input.size())) };
    std::vector<DspJitOutputBuffer> outputs { DspJitOutputBuffer (Span<float> (output.data(), output.size())) };

    graph.process (inputs, outputs, static_cast<int> (input.size()));

    return output;
}

/** Operands for which fusing is observable, which is a stronger requirement
    than it sounds and is asserted below rather than assumed.

    Fusing only removes a rounding, so it can only change the answer where that
    rounding was doing something. Multiplying by an exact power of two rounds to
    itself, so `x * 0.5 + y * 0.5` is bit-identical either way; and awkward-
    looking operands are no guarantee either - a first attempt at this test used
    1.0000001 / 0.30000001 / 0.70000005 and those agree exactly too.

    `0.1 * 0.6 + 1.0` does not: two roundings give 1.05999994, one gives
    1.06000006. Spelled the same way in the YDSP source below, as literals in
    both places, so that what the lexer parses is what the reference computes
    with. */
constexpr float fmaAwkwardA = 0.1f;
constexpr float fmaAwkwardB = 0.6f;
constexpr float fmaAwkwardC = 1.0f;

constexpr auto fmaIntrinsicSource = R"(
    processor P {
        input stream in;
        output stream out;
        process { out = fma (in, 0.6, 1.0); }
    }
)";

} // namespace

//==============================================================================

TEST (YdspFusedMultiplyAddTests, LowersTheIntrinsicToASingleOp)
{
    DspJitDiagnostics diagnostics;

    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            process { out = fma (in, 0.3, 0.7); }
        }
    )"),
                                diagnostics,
                                false);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);

    EXPECT_EQ (1, fmaCountInst (*kernel, YdspIrOp::fmaF));
}

TEST (YdspFusedMultiplyAddTests, RejectsFloat64Operands)
{
    DspJitDiagnostics diagnostics;

    fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            process {
                float64 wide = float64 (in);
                out = float32 (fma (wide, wide, wide));
            }
        }
    )"),
                diagnostics,
                false);

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_TRUE (diagnostics.toString().contains ("float32 only"));
}

TEST (YdspFusedMultiplyAddTests, ExpandsToFloat64WhenTheTargetHasNoInstruction)
{
    DspJitDiagnostics diagnostics;

    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            process { out = fma (in, 0.3, 0.7); }
        }
    )"),
                                diagnostics,
                                false,
                                false);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);

    // Nothing is left for a backend that cannot lower it, and what replaced it
    // is the widen / multiply / add / narrow sequence.
    EXPECT_EQ (0, fmaCountInst (*kernel, YdspIrOp::fmaF));
    EXPECT_EQ (3, fmaCountInst (*kernel, YdspIrOp::extF));
    EXPECT_EQ (1, fmaCountInst (*kernel, YdspIrOp::truncF));
}

TEST (YdspFusedMultiplyAddTests, DoesNotExpandWhenTheTargetHasTheInstruction)
{
    DspJitDiagnostics diagnostics;

    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            process { out = fma (in, 0.3, 0.7); }
        }
    )"),
                                diagnostics,
                                false,
                                true);

    ASSERT_NE (nullptr, ir);

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);

    EXPECT_EQ (1, fmaCountInst (*kernel, YdspIrOp::fmaF));
    EXPECT_EQ (0, fmaCountInst (*kernel, YdspIrOp::extF));
}

//==============================================================================

TEST (YdspFusedMultiplyAddTests, ContractionDoesNothingUnlessEnabled)
{
    DspJitDiagnostics diagnostics;

    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            process { out = in * 0.3 + 0.7; }
        }
    )"),
                                diagnostics,
                                false);

    ASSERT_NE (nullptr, ir);

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);

    EXPECT_EQ (0, fmaCountInst (*kernel, YdspIrOp::fmaF));
}

TEST (YdspFusedMultiplyAddTests, ContractionFusesAMultiplyFeedingAnAdd)
{
    DspJitDiagnostics diagnostics;

    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            state float last;
            process {
                last = last * 0.3 + in;
                out = last;
            }
        }
    )"),
                                diagnostics,
                                true);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);

    EXPECT_EQ (1, fmaCountInst (*kernel, YdspIrOp::fmaF));

    // The multiply it consumed is gone rather than merely unread - the pass
    // only pays for itself if the dead-code pass behind it collects the
    // orphan.
    EXPECT_EQ (0, fmaCountInst (*kernel, YdspIrOp::mulF));
}

TEST (YdspFusedMultiplyAddTests, ContractionFusesTheMultiplyOnTheRecurrence)
{
    DspJitDiagnostics diagnostics;

    // The wave folder's one-pole, and the case the choice exists for: *both*
    // operands of the add are fusable multiplies, only one can be fused, and
    // they are not equivalent. Fusing the `last` multiply leaves
    // `last -> fma -> last`; fusing the `y` one leaves
    // `last -> mul -> fma -> last`, a link longer round the chain that is the
    // whole critical path of this shape.
    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            state float last;
            process {
                let y = in * 3.0;
                last = last * 0.5 + y * 0.5;
                out = last;
            }
        }
    )"),
                                diagnostics,
                                true);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);
    ASSERT_EQ (1, fmaCountInst (*kernel, YdspIrOp::fmaF));

    // One multiply survives - the one that was not fused - so counting alone
    // cannot say which was chosen. What identifies it is that the fused
    // operands are the ones feeding the surviving multiply's *consumer*: the
    // fused op must read the state register, not the multiply's result.
    const YdspIrInst* fused = nullptr;
    const YdspIrInst* survivor = nullptr;

    for (const auto& block : kernel->blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::fmaF)
                fused = &inst;
            else if (inst.op == YdspIrOp::mulF)
                survivor = &inst;
        }
    }

    ASSERT_NE (nullptr, fused);
    ASSERT_NE (nullptr, survivor);

    // The surviving multiply is `y * 0.5`, so it feeds the fused op's *addend*.
    // Had the other one been chosen, it would feed a factor instead.
    EXPECT_EQ (survivor->result, fused->c);
    EXPECT_NE (survivor->result, fused->a);
    EXPECT_NE (survivor->result, fused->b);
}

TEST (YdspFusedMultiplyAddTests, ContractionLeavesAMultiplyWithASecondReader)
{
    DspJitDiagnostics diagnostics;

    // `scaled` is read by the add *and* by the output, so folding the multiply
    // into the add would delete a value something else still needs.
    const auto ir = fmaBuildIr (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            state float last;
            process {
                let scaled = in * 0.3;
                last = scaled + last;
                out = last + scaled;
            }
        }
    )"),
                                diagnostics,
                                true);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);

    EXPECT_EQ (0, fmaCountInst (*kernel, YdspIrOp::fmaF));
    EXPECT_EQ (1, fmaCountInst (*kernel, YdspIrOp::mulF));
}

TEST (YdspFusedMultiplyAddTests, ContractionDoesNotWidenABankLoop)
{
    DspJitDiagnostics diagnostics;

    // Contraction runs after the vectoriser and skips anything widened, so a
    // bank keeps its packed multiply and add rather than acquiring a scalar
    // fused op that no packed form backs.
    YdspLexer lexer (fmaPatch (R"(
        processor P {
            input stream in;
            output stream out;
            state float z[16];
            process {
                float sum = 0.0;
                for i in 0..16 {
                    z[i] = z[i] * 0.9 + in;
                    sum = sum + z[i];
                }
                out = sum;
            }
        }
    )"),
                     diagnostics);

    auto tokens = lexer.tokenize();
    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();
    ASSERT_NE (nullptr, program);

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));
    ASSERT_NE (nullptr, analyzed);

    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorizationEnabled (true);
    optimizer.setContractionEnabled (true);

    const auto ir = optimizer.build (*analyzed);
    ASSERT_NE (nullptr, ir);

    const auto* kernel = fmaKernel (*ir);
    ASSERT_NE (nullptr, kernel);
    ASSERT_TRUE (kernel->vectorized);

    EXPECT_EQ (0, fmaCountInst (*kernel, YdspIrOp::fmaF));
}

//==============================================================================
// The arithmetic. These run the compiled kernel, so they check whichever
// lowering this build selected - the instruction on a target that has one, the
// float64 expansion otherwise - against the same reference either way.

TEST (YdspFusedMultiplyAddTests, TheOperandsMakeFusingObservable)
{
    // The guard on every test below. Fusing only removes a rounding, so with
    // operands where that rounding did nothing the arithmetic tests would pass
    // against either lowering and prove nothing at all.
    EXPECT_NE (std::fma (fmaAwkwardA, fmaAwkwardB, fmaAwkwardC),
               fmaAwkwardA * fmaAwkwardB + fmaAwkwardC);
}

TEST (YdspFusedMultiplyAddTests, TheIntrinsicRoundsOnce)
{
    const std::vector<float> input (8, fmaAwkwardA);
    const auto output = fmaRun (fmaPatch (fmaIntrinsicSource), input);

    ASSERT_EQ (input.size(), output.size());

    // Exact equality, not EXPECT_FLOAT_EQ: that admits 4 ULP, and the whole
    // difference this test exists to see is one.
    const auto expected = std::fma (fmaAwkwardA, fmaAwkwardB, fmaAwkwardC);

    for (const auto sample : output)
        EXPECT_EQ (expected, sample);
}

TEST (YdspFusedMultiplyAddTests, TheExpansionMatchesTheInstruction)
{
    // Both lowerings of one source. The portability property the whole design
    // rests on is that these are the same value, so the comparison is exact.
    DspJitDiagnostics withInstruction;
    DspJitDiagnostics withExpansion;

    const auto source = fmaPatch (fmaIntrinsicSource);

    const auto native = fmaBuildIr (source, withInstruction, false, true);
    const auto expanded = fmaBuildIr (source, withExpansion, false, false);

    ASSERT_NE (nullptr, native);
    ASSERT_NE (nullptr, expanded);

    // The two IR shapes differ, which is the point of the pass.
    EXPECT_EQ (1, fmaCountInst (*fmaKernel (*native), YdspIrOp::fmaF));
    EXPECT_EQ (0, fmaCountInst (*fmaKernel (*expanded), YdspIrOp::fmaF));

    // What must not differ is the value. This build compiles through whichever
    // path its host supports; the reference is the same either way.
    const std::vector<float> input (8, fmaAwkwardA);
    const auto output = fmaRun (source, input);

    ASSERT_EQ (input.size(), output.size());

    for (const auto sample : output)
        EXPECT_EQ (std::fma (fmaAwkwardA, fmaAwkwardB, fmaAwkwardC), sample);
}

} // namespace yup::test
