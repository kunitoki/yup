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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                   yup_dsp_jit
    vendor:               yup
    version:              2.0.0
    name:                 YUP DSP JIT
    description:          YDSP, a realtime JIT-compiled audio DSP language.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_core yup_dsp yup_audio_basics
    macDeps:              asmjit_library
    linuxDeps:            asmjit_library
    windowsDeps:          asmjit_library

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_DSP_JIT_H_INCLUDED

#include <yup_core/yup_core.h>
#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_dsp/yup_dsp.h>

//==============================================================================

#if YUP_IOS
#error "yup_dsp_jit is not supported on iOS targets"
#endif

#if ! YUP_WASM
#include <asmjit_library/asmjit_library.h>
#endif

//==============================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

//==============================================================================

#include "backend/yup_YdspAbi.h"

// Runtime: the realtime graph API
#include "runtime/yup_YdspTypes.h"
#include "runtime/yup_YdspExecutionReport.h"
#include "runtime/yup_YdspAudioGraph.h"

// Compiler: the control-thread compile API
#include "compiler/yup_YdspRecursionGuard.h"
#include "compiler/yup_YdspDiagnostics.h"
#include "compiler/yup_YdspCompileOptions.h"
#include "compiler/yup_YdspBundle.h"
#include "compiler/yup_YdspCompiler.h"

#include "language/yup_YdspAst.h"
#include "language/yup_YdspLexer.h"
#include "language/yup_YdspParser.h"
#include "analysis/yup_YdspTypes.h"
#include "analysis/yup_YdspSemanticAnalyzer.h"
#include "optimiser/yup_YdspIr.h"
#include "optimiser/yup_YdspVectorizer.h"
#include "optimiser/yup_YdspOptimizer.h"
#include "backend/yup_YdspWasmEmitter.h"

#if ! YUP_WASM
#include "backend/yup_YdspAsmJitCodegen.h"
#include "backend/yup_YdspAsmJitCodegenX64.h"
#include "backend/yup_YdspAsmJitCodegenARM64.h"
#endif

#include "backend/yup_YdspWasmCodegen.h"
