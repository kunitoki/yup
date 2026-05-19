/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_simd
    vendor:             yup
    version:            1.0.0
    name:               YUP single instruction multiple data (SIMD) support
    description:        Classes and functions for SIMD operations using SSE, AVX, FMA, NEON, and Accelerate framework.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core
    appleFrameworks:    Accelerate

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_SIMD_H_INCLUDED

#include <yup_core/yup_core.h>

//==============================================================================
#ifndef YUP_USE_SSE_INTRINSICS
#if defined(__SSE__) || defined(_M_X64) || defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define YUP_USE_SSE_INTRINSICS 1
#endif
#endif

#ifndef YUP_USE_AVX_INTRINSICS
#if defined(__AVX2__)
#define YUP_USE_AVX_INTRINSICS 1
#endif
#endif

#ifndef YUP_USE_FMA_INTRINSICS
#if defined(__FMA__)
#define YUP_USE_FMA_INTRINSICS 1
#endif
#endif

#if ! YUP_INTEL
#undef YUP_USE_SSE_INTRINSICS
#undef YUP_USE_AVX_INTRINSICS
#undef YUP_USE_FMA_INTRINSICS
#endif

#if __ARM_NEON__ && ! (YUP_USE_VDSP_FRAMEWORK || defined(YUP_USE_ARM_NEON))
#define YUP_USE_ARM_NEON 1
#endif

#if TARGET_IPHONE_SIMULATOR
#ifdef YUP_USE_ARM_NEON
#undef YUP_USE_ARM_NEON
#endif
#define YUP_USE_ARM_NEON 0
#endif

//==============================================================================
#if (YUP_MAC || YUP_IOS) && __has_include(<Accelerate/Accelerate.h>)
#ifndef YUP_USE_VDSP_FRAMEWORK
#define YUP_USE_VDSP_FRAMEWORK 1
#endif

#elif YUP_USE_VDSP_FRAMEWORK
#undef YUP_USE_VDSP_FRAMEWORK
#endif

//==============================================================================
#if YUP_USE_AVX_INTRINSICS || YUP_USE_FMA_INTRINSICS
#include <immintrin.h>
#endif

#if YUP_USE_SSE_INTRINSICS
#include <emmintrin.h>
#endif

#if YUP_USE_ARM_NEON
#if YUP_64BIT && YUP_WINDOWS
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif
#endif

//==============================================================================
#include <chrono>
#include <type_traits>

//==============================================================================
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4661)
#include "buffers/yup_FloatVectorOperations.h"
YUP_END_IGNORE_WARNINGS_MSVC
