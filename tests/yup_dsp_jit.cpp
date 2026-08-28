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

#include "yup_dsp_jit/yup_YdspDiagnosticsTests.cpp"
#include "yup_dsp_jit/yup_YdspCompilerOptionsTests.cpp"
#include "yup_dsp_jit/yup_YdspExamplePatchTests.cpp"
#include "yup_dsp_jit/yup_YdspFusionTests.cpp"
#include "yup_dsp_jit/yup_YdspFusedMultiplyAddTests.cpp"
#include "yup_dsp_jit/yup_YdspGraphTests.cpp"
#include "yup_dsp_jit/yup_YdspLatencyTests.cpp"
#include "yup_dsp_jit/yup_YdspLexerParserTests.cpp"
#include "yup_dsp_jit/yup_YdspOptimizerTests.cpp"
#include "yup_dsp_jit/yup_YdspSemanticAnalyzerTests.cpp"
#include "yup_dsp_jit/yup_YdspSmokeTests.cpp"
#include "yup_dsp_jit/yup_YdspSubgraphTests.cpp"
#include "yup_dsp_jit/yup_YdspVectorizerTests.cpp"
#include "yup_dsp_jit/yup_YdspWasmBackendTests.cpp"
#include "yup_dsp_jit/yup_YdspWasmTests.cpp"
#if ! YUP_WASM
#include "yup_dsp_jit/yup_YdspAsmJitCodegenTests.cpp"
#endif

#include "yup_dsp_jit/yup_YdspBenchmarkTests.cpp"
