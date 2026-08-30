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

#ifdef YUP_DSP_JIT_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_dsp_jit.h"

//==============================================================================

#include <algorithm>
#include <cmath>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <initializer_list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

//==============================================================================

#if YUP_EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

//==============================================================================

// Language front-end: lexer, AST and parser
#include "language/yup_YdspLexer.cpp"
#include "language/yup_YdspParser.cpp"
#include "compiler/yup_YdspBundle.cpp"

// Semantic analysis: type system and realtime-safety enforcement
#include "analysis/semantic/yup_YdspSemanticTypeSystem.cpp"
#include "analysis/semantic/yup_YdspSemanticProgram.cpp"
#include "analysis/semantic/yup_YdspSemanticProcessor.cpp"
#include "analysis/semantic/yup_YdspSemanticStatements.cpp"
#include "analysis/semantic/yup_YdspSemanticGraph.cpp"
#include "analysis/semantic/yup_YdspSemanticGraphForms.cpp"
#include "analysis/semantic/yup_YdspSemanticGraphInline.cpp"
#include "analysis/semantic/yup_YdspSemanticGraphFuse.cpp"
#include "analysis/semantic/yup_YdspSemanticGraphLatency.cpp"
#include "analysis/semantic/yup_YdspSemanticFunctions.cpp"

// Optimisation: typed IL
#include "optimiser/builder/yup_YdspBuilder.h"
#include "optimiser/builder/yup_YdspBuilderCore.cpp"
#include "optimiser/builder/yup_YdspBuilderExpressions.cpp"
#include "optimiser/builder/yup_YdspBuilderStatements.cpp"
#include "optimiser/builder/yup_YdspBuilderFunctions.cpp"

// Optimisation: compiler passes
#include "optimiser/passes/yup_YdspPassesShared.cpp"
#include "optimiser/passes/yup_YdspPassesConstantFolding.cpp"
#include "optimiser/passes/yup_YdspPassesAlgebraicSimplification.cpp"
#include "optimiser/passes/yup_YdspPassesCopyPropagation.cpp"
#include "optimiser/passes/yup_YdspPassesIfConversion.cpp"
#include "optimiser/passes/yup_YdspPassesFullyUnrollBoundedLoops.cpp"
#include "optimiser/passes/yup_YdspPassesSplitWidenedReductionChains.cpp"
#include "optimiser/passes/yup_YdspPassesStoreToLoadForwarding.cpp"
#include "optimiser/passes/yup_YdspPassesDeadCodeElimination.cpp"
#include "optimiser/passes/yup_YdspPassesLoopInvariantCodeMotion.cpp"
#include "optimiser/passes/yup_YdspPassesContractMultiplyAdd.cpp"
#include "optimiser/passes/yup_YdspPassesLowerFusedMultiplyAdd.cpp"

// Optimisation: vectorisation
#include "optimiser/yup_YdspVectorizer.cpp"
#include "optimiser/yup_YdspOptimizer.cpp"

// Backend: asmjit code generation (desktop only)
#if ! YUP_WASM
#include "backend/yup_YdspAsmJitCodegen.cpp"
#include "backend/yup_YdspAsmJitCodegenX64.cpp"
#include "backend/yup_YdspAsmJitCodegenARM64.cpp"
#endif

// WebAssembly backend: wasm binary emission and IR lowering (pure C++)
#include "backend/yup_YdspWasmEmitter.cpp"
#include "backend/yup_YdspWasmCodegen.cpp"
#include "backend/yup_YdspWasmRuntime.h"

// WebAssembly backend: JS glue for the browser's native WebAssembly API
#if YUP_EMSCRIPTEN
#include "native/yup_YdspWasmRuntime_emscripten.cpp"
#endif

// Runtime: the realtime graph
#include "runtime/yup_YdspGraphInternal.h"
#include "runtime/yup_YdspAudioGraph.cpp"
#include "runtime/yup_YdspGraphPimpl.cpp"
#include "runtime/yup_YdspGraphQuery.cpp"

// Compiler: the control-thread compiler
#include "compiler/yup_YdspDiagnostics.cpp"
#include "compiler/yup_YdspCompiler.cpp"
