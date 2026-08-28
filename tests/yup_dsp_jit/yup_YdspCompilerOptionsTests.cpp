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
