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

    ID:               sleef_library
    vendor:           sleef
    version:          3.9.0
    name:             SLEEF vectorized math library
    description:      Vectorized and scalar elementary math functions.
    website:          https://sleef.org
    license:          Boost

    windowsDefines:   SLEEF_STATIC_LIBS

    searchpaths:      upstream/src/arch upstream/src/common upstream/src/libm

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if defined(__aarch64__) || defined(_M_ARM64)
  #include <arm_neon.h>
#elif defined(__x86_64__) || defined(_M_X64)
  #include <emmintrin.h>
#endif

//==============================================================================

#if defined(__cplusplus)
extern "C"
{
#endif

/** 4-lane float type, as compiled by the single-ISA SLEEF unit (x86 SSE2 or
    AArch64 ASIMD). SLEEF passes vectors by value in SIMD registers on both
    targets.
*/
#if defined(__x86_64__) || defined(_M_X64)
    typedef __m128 Sleef_float32x4;
#elif defined(__aarch64__) || defined(_M_ARM64)
    typedef float32x4_t Sleef_float32x4;
#else
#error "sleef_library supports x86-64 and AArch64 only"
#endif

//==============================================================================
// Scalar float32 transcendentals. Accuracy tiers follow SLEEF: `u10` is ~1 ULP
// (strict), `u35` is ~3.5 ULP (fast). Functions with no `u35` tier only exist
// at `u10`.

float Sleef_sinf_u10 (float a);
float Sleef_sinf_u35 (float a);
float Sleef_cosf_u10 (float a);
float Sleef_cosf_u35 (float a);
float Sleef_tanf_u10 (float a);
float Sleef_tanf_u35 (float a);

float Sleef_asinf_u10 (float a);
float Sleef_asinf_u35 (float a);
float Sleef_acosf_u10 (float a);
float Sleef_acosf_u35 (float a);
float Sleef_atanf_u10 (float a);
float Sleef_atanf_u35 (float a);

float Sleef_sinhf_u10 (float a);
float Sleef_sinhf_u35 (float a);
float Sleef_coshf_u10 (float a);
float Sleef_coshf_u35 (float a);
float Sleef_tanhf_u10 (float a);
float Sleef_tanhf_u35 (float a);

float Sleef_asinhf_u10 (float a);
float Sleef_acoshf_u10 (float a);
float Sleef_atanhf_u10 (float a);

float Sleef_expf_u10 (float a);
float Sleef_logf_u10 (float a);
float Sleef_logf_u35 (float a);
float Sleef_log10f_u10 (float a);

float Sleef_powf_u10 (float a, float b);
float Sleef_atan2f_u10 (float a, float b);
float Sleef_atan2f_u35 (float a, float b);

float Sleef_fmodf (float a, float b);

//==============================================================================
// 4-lane float32 transcendentals. Names carry the lane width (`f4`); there is
// no ISA suffix because the module compiles exactly one ISA per target.

Sleef_float32x4 Sleef_sinf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_sinf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_cosf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_cosf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_tanf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_tanf4_u35 (Sleef_float32x4 a);

Sleef_float32x4 Sleef_asinf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_asinf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_acosf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_acosf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_atanf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_atanf4_u35 (Sleef_float32x4 a);

Sleef_float32x4 Sleef_sinhf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_sinhf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_coshf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_coshf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_tanhf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_tanhf4_u35 (Sleef_float32x4 a);

Sleef_float32x4 Sleef_asinhf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_acoshf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_atanhf4_u10 (Sleef_float32x4 a);

Sleef_float32x4 Sleef_expf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_logf4_u10 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_logf4_u35 (Sleef_float32x4 a);
Sleef_float32x4 Sleef_log10f4_u10 (Sleef_float32x4 a);

Sleef_float32x4 Sleef_powf4_u10 (Sleef_float32x4 a, Sleef_float32x4 b);
Sleef_float32x4 Sleef_atan2f4_u10 (Sleef_float32x4 a, Sleef_float32x4 b);
Sleef_float32x4 Sleef_atan2f4_u35 (Sleef_float32x4 a, Sleef_float32x4 b);

Sleef_float32x4 Sleef_fmodf4 (Sleef_float32x4 a, Sleef_float32x4 b);

#if defined(__cplusplus)
}
#endif
