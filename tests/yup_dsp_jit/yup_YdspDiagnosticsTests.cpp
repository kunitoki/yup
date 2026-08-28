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

#include <string>

namespace yup::test
{

//==============================================================================
// YdspDiagnosticsTests
//==============================================================================

TEST (YdspJitDiagnosticsTests, StartsEmptyWithNoErrors)
{
    YdspDiagnostics diagnostics;

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ (0, diagnostics.getCount());
}

TEST (YdspJitDiagnosticsTests, AddErrorAndQuery)
{
    YdspDiagnostics diagnostics;

    diagnostics.addError (1, 5, "syntax error: unexpected token");

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (1, diagnostics.getCount());

    const auto& item = diagnostics.getItem (0);
    EXPECT_EQ (YdspSeverity::error, item.severity);
    EXPECT_EQ (1, item.line);
    EXPECT_EQ (5, item.column);
    EXPECT_EQ ("syntax error: unexpected token", item.message);
}

TEST (YdspJitDiagnosticsTests, AddWarningDoesNotSetHasErrors)
{
    YdspDiagnostics diagnostics;

    diagnostics.addWarning (3, 10, "unused variable 'x'");

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ (1, diagnostics.getCount());

    const auto& item = diagnostics.getItem (0);
    EXPECT_EQ (YdspSeverity::warning, item.severity);
    EXPECT_EQ (3, item.line);
    EXPECT_EQ (10, item.column);
    EXPECT_EQ ("unused variable 'x'", item.message);
}

TEST (YdspJitDiagnosticsTests, AddInfoDoesNotSetHasErrors)
{
    YdspDiagnostics diagnostics;

    diagnostics.addInfo (2, 1, "loop bound inferred as blockSize");

    EXPECT_FALSE (diagnostics.hasErrors());
    EXPECT_EQ (1, diagnostics.getCount());

    const auto& item = diagnostics.getItem (0);
    EXPECT_EQ (YdspSeverity::info, item.severity);
}

TEST (YdspJitDiagnosticsTests, ErrorsDetectedAmongMixedSeverities)
{
    YdspDiagnostics diagnostics;

    diagnostics.addInfo (1, 1, "parsing program");
    diagnostics.addWarning (2, 1, "implicit float conversion");
    diagnostics.addError (3, 1, "unknown symbol 'foo'");
    diagnostics.addInfo (4, 1, "compilation finished");

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (4, diagnostics.getCount());

    EXPECT_EQ (YdspSeverity::info, diagnostics.getItem (0).severity);
    EXPECT_EQ (YdspSeverity::warning, diagnostics.getItem (1).severity);
    EXPECT_EQ (YdspSeverity::error, diagnostics.getItem (2).severity);
    EXPECT_EQ (YdspSeverity::info, diagnostics.getItem (3).severity);
}

TEST (YdspJitDiagnosticsTests, ToStringWithoutSourceRendersMessages)
{
    YdspDiagnostics diagnostics;

    diagnostics.addError (1, 5, "syntax error");
    diagnostics.addWarning (2, 8, "unused variable");

    const auto str = diagnostics.toString();

    EXPECT_TRUE (str.contains ("error"));
    EXPECT_TRUE (str.contains ("syntax error"));
    EXPECT_TRUE (str.contains ("warning"));
    EXPECT_TRUE (str.contains ("unused variable"));
    // Line numbers should appear
    EXPECT_TRUE (str.contains ("1"));
    EXPECT_TRUE (str.contains ("2"));
}

TEST (YdspJitDiagnosticsTests, ToStringWithSourceRendersCaret)
{
    YdspDiagnostics diagnostics;

    diagnostics.setSource ("processor P {\n  out = in;\n}");
    diagnostics.addError (2, 3, "expected ';'");

    const auto str = diagnostics.toString();

    // Should contain a caret (^) marker
    EXPECT_TRUE (str.contains ("^"));
    // Should contain the source line
    EXPECT_TRUE (str.contains ("out = in;"));
    // Should contain the error message
    EXPECT_TRUE (str.contains ("expected ';'"));

    // The caret lands exactly under column 3 of line 2 (the 'o' of "out"):
    // the source line renders as "  2 |   out = in;" and the caret line has
    // eight leading spaces before the '^'.
    EXPECT_TRUE (str.contains ("  2 |   out = in;\n        ^"));
}

TEST (YdspJitDiagnosticsTests, MultipleItemsAtSameLocation)
{
    YdspDiagnostics diagnostics;

    diagnostics.addError (1, 10, "type mismatch");
    diagnostics.addError (1, 10, "cannot assign to let");

    EXPECT_TRUE (diagnostics.hasErrors());
    EXPECT_EQ (2, diagnostics.getCount());

    EXPECT_EQ (1, diagnostics.getItem (0).line);
    EXPECT_EQ (10, diagnostics.getItem (0).column);
    EXPECT_EQ (1, diagnostics.getItem (1).line);
    EXPECT_EQ (10, diagnostics.getItem (1).column);
}

TEST (YdspJitDiagnosticsTests, ClearingSourceThenToString)
{
    YdspDiagnostics diagnostics;

    diagnostics.setSource ("processor P { }");
    diagnostics.addError (1, 1, "error");

    auto s1 = diagnostics.toString();
    EXPECT_TRUE (s1.contains ("^"));

    // Setting empty source should still render (just without carets)
    diagnostics.setSource ("");
    auto s2 = diagnostics.toString();
    EXPECT_FALSE (s2.contains ("^"));
    EXPECT_TRUE (s2.contains ("error"));
}

//==============================================================================
// YdspExecutionReportTests
//==============================================================================

TEST (YdspJitExecutionReportTests, EmptyReportIsSafe)
{
    YdspExecutionReport report;

    EXPECT_TRUE (report.getKernels().empty());
    EXPECT_EQ (0, report.getTotalBoundedIterations());
    EXPECT_TRUE (report.isProvenRealtimeSafe());
}

TEST (YdspJitExecutionReportTests, SingleKernelReportFields)
{
    YdspExecutionReport report;

    auto& kernels = report.getKernels();
    kernels.push_back ({ "MyKernel", 42, 128, true, { "blockSize" } });

    ASSERT_EQ (1u, report.getKernels().size());
    EXPECT_EQ ("MyKernel", report.getKernels()[0].name);
    EXPECT_EQ (42, report.getKernels()[0].instructionCount);
    EXPECT_EQ (128, report.getKernels()[0].boundedIterationCount);
    EXPECT_TRUE (report.getKernels()[0].provenRealtimeSafe);
    ASSERT_EQ (1u, report.getKernels()[0].loopBounds.size());
    EXPECT_EQ ("blockSize", report.getKernels()[0].loopBounds[0]);
    EXPECT_EQ (128, report.getTotalBoundedIterations());
    EXPECT_TRUE (report.isProvenRealtimeSafe());
}

TEST (YdspJitExecutionReportTests, MultipleKernelsAggregateCorrectly)
{
    YdspExecutionReport report;

    auto& kernels = report.getKernels();
    kernels.push_back ({ "A", 10, 32, true, {} });
    kernels.push_back ({ "B", 20, 64, true, {} });

    EXPECT_EQ (32 + 64, report.getTotalBoundedIterations());
    EXPECT_TRUE (report.isProvenRealtimeSafe());
}

TEST (YdspJitExecutionReportTests, OneUnsafeKernelMakesReportUnsafe)
{
    YdspExecutionReport report;

    auto& kernels = report.getKernels();
    kernels.push_back ({ "Safe", 10, 64, true, {} });
    kernels.push_back ({ "Unsafe", 0, 0, false, {} });

    EXPECT_FALSE (report.isProvenRealtimeSafe());
    EXPECT_EQ (64, report.getTotalBoundedIterations());
}

} // namespace yup::test
