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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wunused-member-function"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wrestrict"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4100)
#pragma warning(disable : 4146)
#pragma warning(disable : 4189)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4305)
#pragma warning(disable : 4389)
#pragma warning(disable : 4456)
#pragma warning(disable : 4457)
#pragma warning(disable : 4702)
#pragma warning(disable : 4800)
#pragma warning(disable : 4996)
#endif

#include "glslang.h"

// --- GenericCodeGen -------------------------------------------------------

#include "upstream/glslang/GenericCodeGen/CodeGen.cpp"
#include "upstream/glslang/GenericCodeGen/Link.cpp"

// --- MachineIndependent ---------------------------------------------------

#include "upstream/glslang/MachineIndependent/PoolAlloc.cpp"
#include "upstream/glslang/MachineIndependent/InfoSink.cpp"
#include "upstream/glslang/MachineIndependent/Constant.cpp"
#include "upstream/glslang/MachineIndependent/Intermediate.cpp"
#include "upstream/glslang/MachineIndependent/IntermTraverse.cpp"
#include "upstream/glslang/MachineIndependent/intermOut.cpp"
#include "upstream/glslang/MachineIndependent/SymbolTable.cpp"
#include "upstream/glslang/MachineIndependent/Versions.cpp"
#include "upstream/glslang/MachineIndependent/Initialize.cpp"
#include "upstream/glslang/MachineIndependent/Scan.cpp"
#include "upstream/glslang/MachineIndependent/ParseContextBase.cpp"
#include "upstream/glslang/MachineIndependent/ParseHelper.cpp"
#include "upstream/glslang/MachineIndependent/parseConst.cpp"
#include "upstream/glslang/MachineIndependent/ShaderLang.cpp"
#include "upstream/glslang/MachineIndependent/linkValidate.cpp"
#include "upstream/glslang/MachineIndependent/reflection.cpp"
#include "upstream/glslang/MachineIndependent/iomapper.cpp"
#include "upstream/glslang/MachineIndependent/limits.cpp"
#include "upstream/glslang/MachineIndependent/RemoveTree.cpp"
#include "upstream/glslang/MachineIndependent/SpirvIntrinsics.cpp"
#include "upstream/glslang/MachineIndependent/attribute.cpp"
#include "upstream/glslang/MachineIndependent/propagateNoContraction.cpp"

// --- Preprocessor ---------------------------------------------------------

#include "upstream/glslang/MachineIndependent/preprocessor/Pp.cpp"
#include "upstream/glslang/MachineIndependent/preprocessor/PpAtom.cpp"
#include "upstream/glslang/MachineIndependent/preprocessor/PpContext.cpp"
#include "upstream/glslang/MachineIndependent/preprocessor/PpScanner.cpp"
#include "upstream/glslang/MachineIndependent/preprocessor/PpTokens.cpp"

// --- C Interface ----------------------------------------------------------

#include "upstream/glslang/CInterface/glslang_c_interface.cpp"

// --- ResourceLimits -------------------------------------------------------

#include "upstream/glslang/ResourceLimits/ResourceLimits.cpp"
#include "upstream/glslang/ResourceLimits/resource_limits_c.cpp"

// --- Platform Specific ----------------------------------------------------

#if defined(__EMSCRIPTEN__)
namespace glslang
{
void OS_DumpMemoryCounters() {}
} // namespace glslang
#elif defined(_WIN32)
#include "upstream/glslang/OSDependent/Windows/ossource.cpp"
#else
#include "upstream/glslang/OSDependent/Unix/ossource.cpp"
#endif

// --- End diagnostic suppression -------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
