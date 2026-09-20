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

using namespace yup;

namespace
{

constexpr auto compilerOptionsSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;
        state float bank[8];

        process {
            float sum = 0.0;

            for i in 0..8 {
                bank[i] = bank[i] * 0.5 + in;
                sum = sum + bank[i];
            }

            out = sum;
        }
    }

    graph G {
        input stream x;
        output stream y;
        node p = P;
        connection { x -> p.in; p.out -> y; }
    }
)YDSP";

} // namespace

//==============================================================================

TEST (YdspCompilerOptionsTests, BaselineScalarTierLeavesTheKernelScalar)
{
    YdspCompileOptions options;
    options.optimizationTier = YdspOptimizationTier::baseline;
    options.targetPolicy = YdspTargetPolicy::baseline;
    options.baselineTarget = YdspNativeTarget::scalar;
    options.fastMath = false; // fastMath now defaults on natively; keep this tier strict
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    auto result = compiler.compile (compilerOptionsSource, options);

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    const auto& optimizationReport = compiler.getOptimizationReport();
    EXPECT_EQ (YdspOptimizationTier::baseline, optimizationReport.optimizationTier);
    EXPECT_EQ (YdspNativeTarget::scalar, optimizationReport.selectedIsa);
    EXPECT_EQ (1, optimizationReport.vectorWidth);
    EXPECT_FALSE (optimizationReport.vectorizationEnabled);
    EXPECT_FALSE (optimizationReport.unrollingEnabled);
    EXPECT_FALSE (optimizationReport.reductionSplittingEnabled);
    EXPECT_FALSE (optimizationReport.contractionEnabled);
    EXPECT_GT (optimizationReport.generatedCodeSize, 0u);
    EXPECT_GE (optimizationReport.compileTimeMilliseconds, 0.0);
    EXPECT_FALSE (optimizationReport.cacheHit);
    EXPECT_TRUE (optimizationReport.cacheDecision.contains ("No persistent"));

    const auto graph = std::move (result).getValue();
    ASSERT_FALSE (graph.getExecutionReport().getKernels().empty());
    EXPECT_FALSE (graph.getExecutionReport().getKernels().front().vectorized);
    EXPECT_FALSE (graph.getExecutionReport().getKernels().front().unrolled);
}

TEST (YdspCompilerOptionsTests, FastMathIsExplicitAndReported)
{
    YdspCompileOptions options;
    options.fastMath = true;
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    const auto result = compiler.compile (compilerOptionsSource, options);

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    const auto& optimizationReport = compiler.getOptimizationReport();
    EXPECT_TRUE (optimizationReport.fastMath);
    EXPECT_TRUE (optimizationReport.contractionEnabled);
    EXPECT_GT (optimizationReport.generatedCodeSize, 0u);
}

TEST (YdspCompilerOptionsTests, DisablingTheReportAvoidsReportCollection)
{
    YdspCompileOptions options;

    YdspCompiler compiler;
    const auto result = compiler.compile (compilerOptionsSource, options);

    ASSERT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    const auto& optimizationReport = compiler.getOptimizationReport();
    EXPECT_EQ (0u, optimizationReport.generatedCodeSize);
    EXPECT_EQ (0.0, optimizationReport.compileTimeMilliseconds);
    EXPECT_TRUE (optimizationReport.cacheDecision.isEmpty());
}

TEST (YdspCompilerOptionsTests, ScalarFloat32ExpInlinePathIsUsedUnderFastMathOnAArch64)
{
#if ASMJIT_ARCH_ARM
    const auto source = R"YDSP(
        processor P { input stream in; output stream out; process { out = exp (in); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    // A scalar baseline keeps the per-sample loop scalar, so the float32 exp
    // lowering is the thing under test. Under fastMath the inline degree-8
    // Estrin polynomial runs on the hot path (it keeps a rare libm fallback
    // for |x| > 1, so the kernel still contains a blr); strict mode calls
    // libm expf every sample and contains no polynomial at all.
    YdspCompileOptions options;
    options.optimizationTier = YdspOptimizationTier::baseline;
    options.targetPolicy = YdspTargetPolicy::baseline;
    options.baselineTarget = YdspNativeTarget::scalar;

    options.fastMath = false;
    YdspCompiler strictCompiler;
    auto strictResult = strictCompiler.compile (source, options);
    ASSERT_TRUE (strictResult.wasOk()) << strictCompiler.getDiagnostics().toString();

    const auto strictGraph = std::move (strictResult).getValue();
    ASSERT_TRUE (strictGraph.isValid());
    const auto strictListing = strictGraph.getDiagnostics().toString();
    EXPECT_TRUE (strictListing.contains ("blr"));
    EXPECT_FALSE (strictListing.contains ("fmadd"));

    options.fastMath = true;
    YdspCompiler fastCompiler;
    auto fastResult = fastCompiler.compile (source, options);
    ASSERT_TRUE (fastResult.wasOk()) << fastCompiler.getDiagnostics().toString();

    const auto fastGraph = std::move (fastResult).getValue();
    ASSERT_TRUE (fastGraph.isValid());
    const auto fastListing = fastGraph.getDiagnostics().toString();
    EXPECT_TRUE (fastListing.contains ("fmadd")) << "the inline Estrin exp must be present under fastMath";
#endif
}

TEST (YdspCompilerOptionsTests, ValidationAcceptsLibrariesWithoutAnExecutableGraph)
{
    for (const auto* source : {
             "func twice (x: float) : float { return x * 2.0; }",
             "processor Gain { input stream in; output stream out; process { out = in * 2.0; } }",
             "let scale = 2; func twice (x: float) : float { return x * float (scale); }" })
    {
        SCOPED_TRACE (source);
        YdspCompiler compiler;
        EXPECT_TRUE (compiler.validate (source).wasOk()) << compiler.getDiagnostics().toString();
        EXPECT_FALSE (compiler.getDiagnostics().hasErrors());
        EXPECT_FALSE (compiler.compile (source).wasOk());
        EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("at least one graph"));
    }
}

TEST (YdspCompilerOptionsTests, ValidationStillChecksProcessorAndLibraryBodies)
{
    for (const auto* source : {
             "func twice (x: float) : float { return missing; }",
             "func twice (x: float) : float { if (x > 0.0) { return missing; } return x; }",
             "func twice (x: float) : float { let value = missing; return value; }",
             "processor Gain { output stream out; func unused (x: float) : float { return missing; } process { out = 0.0; } }",
             "processor Gain { output stream out; process { out = missing; } }" })
    {
        SCOPED_TRACE (source);
        YdspCompiler compiler;
        ASSERT_FALSE (compiler.validate (source).wasOk());
        ASSERT_TRUE (compiler.getDiagnostics().hasErrors());
        EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("missing"));
        EXPECT_FALSE (compiler.getDiagnostics().toString().contains ("at least one graph"));
        EXPECT_GT (compiler.getDiagnostics().getItem (0).range.startColumn, 0);
    }
}

TEST (YdspCompilerOptionsTests, ValidationScopesFunctionParametersAndLocals)
{
    const auto* source = R"(
        func twice (x: float) : float { let value = x * 2.0; return value; }
        func half (x: float) : float { let value = x * 0.5; return twice (value); }
        processor Gain {
            output stream out;
            state float x = 1.0;
            func local (x: float) : float { let value = half (x); return value; }
            process { let value = x; out = local (value); }
        }
    )";
    YdspCompiler compiler;
    EXPECT_TRUE (compiler.validate (source).wasOk()) << compiler.getDiagnostics().toString();
}
