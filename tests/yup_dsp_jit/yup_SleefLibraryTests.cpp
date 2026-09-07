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

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace yup;

namespace yup::test
{

namespace
{

//==============================================================================
// SLEEF ships no public headers in the source tree; the module exports the
// subset the JIT needs (see sleef_library.h). These tests prove that subset
// compiles and links, and that scalar and 4-lane results agree with libm to a
// few ULP - the plain-C ABI check that Phase 2's InvokeNode lowering builds on.
//
// Tolerances are given in float steps (ULP distance), which stays meaningful
// where the function crosses zero, unlike a relative compare. u10 tiers sit
// within ~1 ULP of the correctly-rounded libm result (so a few steps apart),
// u35 within ~3.5 ULP (so ~8 steps is a safe cap), and an ABI break shows up
// as garbage orders of magnitude larger.

std::vector<float> benchmarkSleefRange (float start, float end, int samples)
{
    std::vector<float> values (static_cast<size_t> (samples));

    for (int i = 0; i < samples; ++i)
        values[static_cast<size_t> (i)] = start + (end - start) * static_cast<float> (i) / static_cast<float> (samples - 1);

    return values;
}

} // namespace

class SleefLibraryTests : public ::testing::Test
{
protected:
    // ULP distance between two finite floats, as a count of representable
    // values between them (0 when equal).
    static int floatUlpDistance (float a, float b)
    {
        if (a == b)
            return 0;

        const auto order = [] (float value)
        {
            auto bits = std::bit_cast<int32_t> (value);
            return bits < 0 ? static_cast<int64_t> (0x80000000ll) - bits : static_cast<int64_t> (bits);
        };

        const auto da = order (a);
        const auto db = order (b);

        return static_cast<int> (da > db ? da - db : db - da);
    }

    static void expectScalarClose (float sleefValue, float libmValue, int maxUlps)
    {
        const auto sleefNan = std::isnan (sleefValue);
        const auto libmNan = std::isnan (libmValue);
        const auto sleefInf = std::isinf (sleefValue);
        const auto libmInf = std::isinf (libmValue);

        EXPECT_EQ (libmNan, sleefNan) << "sleef " << sleefValue << " vs libm " << libmValue;
        EXPECT_EQ (libmInf, sleefInf) << "sleef " << sleefValue << " vs libm " << libmValue;

        if (libmNan || libmInf || sleefNan || sleefInf)
            return;

        EXPECT_LE (floatUlpDistance (sleefValue, libmValue), maxUlps)
            << "sleef " << sleefValue << " vs libm " << libmValue;
    }

    using SleefScalarFloatFn = float (*) (float);
    using SleefVectorFloatFn = Sleef_float32x4 (*) (Sleef_float32x4);

    // The libm reference is a concrete `float (*) (float)` rather than a
    // template parameter so an overloaded name such as `std::sin` resolves to
    // its float overload at the call site.
    void expectUnaryScalar (SleefScalarFloatFn sleefFn, SleefScalarFloatFn libmFn, const std::vector<float>& inputs, int maxUlps)
    {
        for (const auto x : inputs)
            expectScalarClose (sleefFn (x), libmFn (x), maxUlps);
    }

    static std::array<float, 4> sleefLanes (Sleef_float32x4 value)
    {
        std::array<float, 4> lanes {};
#if defined(__aarch64__) || defined(_M_ARM64)
        vst1q_f32 (lanes.data(), value);
#else
        _mm_storeu_ps (lanes.data(), value);
#endif
        return lanes;
    }

    static Sleef_float32x4 sleefLoad (const std::array<float, 4>& lanes)
    {
#if defined(__aarch64__) || defined(_M_ARM64)
        return vld1q_f32 (lanes.data());
#else
        return _mm_loadu_ps (lanes.data());
#endif
    }

    void expectUnaryVector (SleefVectorFloatFn sleefFn, SleefScalarFloatFn libmFn, const std::vector<float>& inputs, int maxUlps)
    {
        for (size_t i = 0; i + 3 < inputs.size(); i += 4)
        {
            const std::array<float, 4> group { inputs[i], inputs[i + 1], inputs[i + 2], inputs[i + 3] };
            const auto result = sleefLanes (sleefFn (sleefLoad (group)));

            for (int lane = 0; lane < 4; ++lane)
                expectScalarClose (result[static_cast<size_t> (lane)],
                                   libmFn (group[static_cast<size_t> (lane)]),
                                   maxUlps);
        }
    }
};

TEST_F (SleefLibraryTests, ScalarTranscendentalsMatchLibm)
{
    const auto aroundPi = benchmarkSleefRange (-3.13f, 3.13f, 2001);
    const auto aroundUnit = benchmarkSleefRange (-0.99f, 0.99f, 2001);
    const auto positive = benchmarkSleefRange (0.01f, 7.9f, 2001);
    const auto moderate = benchmarkSleefRange (-7.9f, 7.9f, 2001);
    const auto acoshDomain = benchmarkSleefRange (1.0f, 8.0f, 2001);

    expectUnaryScalar (Sleef_sinf_u10, std::sin, aroundPi, 4);
    expectUnaryScalar (Sleef_sinf_u35, std::sin, aroundPi, 8);
    expectUnaryScalar (Sleef_cosf_u10, std::cos, aroundPi, 4);
    expectUnaryScalar (Sleef_cosf_u35, std::cos, aroundPi, 8);
    expectUnaryScalar (Sleef_tanf_u10, std::tan, moderate, 4);
    expectUnaryScalar (Sleef_tanf_u35, std::tan, moderate, 8);

    expectUnaryScalar (Sleef_asinf_u10, std::asin, aroundUnit, 4);
    expectUnaryScalar (Sleef_acosf_u10, std::acos, aroundUnit, 4);
    expectUnaryScalar (Sleef_atanf_u10, std::atan, moderate, 4);
    expectUnaryScalar (Sleef_atanf_u35, std::atan, moderate, 8);

    expectUnaryScalar (Sleef_sinhf_u10, std::sinh, moderate, 4);
    expectUnaryScalar (Sleef_coshf_u10, std::cosh, moderate, 4);
    expectUnaryScalar (Sleef_tanhf_u10, std::tanh, moderate, 4);
    expectUnaryScalar (Sleef_asinhf_u10, std::asinh, moderate, 4);
    expectUnaryScalar (Sleef_acoshf_u10, std::acosh, acoshDomain, 4);
    expectUnaryScalar (Sleef_atanhf_u10, std::atanh, aroundUnit, 4);

    expectUnaryScalar (Sleef_expf_u10, std::exp, moderate, 4);
    expectUnaryScalar (Sleef_logf_u10, std::log, positive, 4);
    expectUnaryScalar (Sleef_logf_u35, std::log, positive, 8);
    expectUnaryScalar (Sleef_log10f_u10, std::log10, positive, 4);
}

TEST_F (SleefLibraryTests, ScalarBinaryTranscendentalsMatchLibm)
{
    const auto a = benchmarkSleefRange (-3.0f, 3.0f, 101);
    const auto b = benchmarkSleefRange (0.5f, 4.0f, 101);

    for (const auto x : a)
        for (const auto y : b)
        {
            expectScalarClose (Sleef_powf_u10 (x < 0.0f ? -x : x, y), std::pow (x < 0.0f ? -x : x, y), 8);
            expectScalarClose (Sleef_atan2f_u10 (x, y), std::atan2 (x, y), 4);
            expectScalarClose (Sleef_fmodf (x, y + 0.25f), std::fmod (x, y + 0.25f), 4);
        }
}

TEST_F (SleefLibraryTests, Vector4TranscendentalsMatchLibm)
{
    const auto aroundPi = benchmarkSleefRange (-3.13f, 3.13f, 2000);
    const auto aroundUnit = benchmarkSleefRange (-0.99f, 0.99f, 2000);
    const auto positive = benchmarkSleefRange (0.01f, 7.9f, 2000);
    const auto moderate = benchmarkSleefRange (-7.9f, 7.9f, 2000);

    expectUnaryVector (Sleef_sinf4_u10, std::sin, aroundPi, 4);
    expectUnaryVector (Sleef_sinf4_u35, std::sin, aroundPi, 8);
    expectUnaryVector (Sleef_cosf4_u10, std::cos, aroundPi, 4);
    expectUnaryVector (Sleef_tanhf4_u10, std::tanh, moderate, 4);
    expectUnaryVector (Sleef_expf4_u10, std::exp, moderate, 4);
    expectUnaryVector (Sleef_logf4_u10, std::log, positive, 4);
    expectUnaryVector (Sleef_log10f4_u10, std::log10, positive, 4);
    expectUnaryVector (Sleef_asinf4_u10, std::asin, aroundUnit, 4);
    expectUnaryVector (Sleef_acosf4_u10, std::acos, aroundUnit, 4);
    expectUnaryVector (Sleef_sinhf4_u10, std::sinh, moderate, 4);
    expectUnaryVector (Sleef_coshf4_u10, std::cosh, moderate, 4);
    expectUnaryVector (Sleef_asinhf4_u10, std::asinh, moderate, 4);
    expectUnaryVector (Sleef_atanhf4_u10, std::atanh, aroundUnit, 4);
    expectUnaryVector (Sleef_atanf4_u10, std::atan, moderate, 4);
    expectUnaryVector (Sleef_atanf4_u35, std::atan, moderate, 8);
}

TEST_F (SleefLibraryTests, EveryDeclaredSymbolLinks)
{
    // Touch every prototype declared by sleef_library.h so a missing or
    // misspelled export fails at link time, not in Phase 2.

    const auto v1 = sleefLoad ({ 0.5f, 1.0f, 1.5f, 2.0f });
    const auto v2 = sleefLoad ({ 2.0f, 1.5f, 1.0f, 0.5f });
    const auto vUnit = sleefLoad ({ -0.5f, 0.25f, 0.5f, 0.75f }); // in-domain for asin/acos/atanh

    EXPECT_TRUE (std::isfinite (Sleef_sinf_u10 (1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_sinf_u35 (1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_cosf_u10 (1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_cosf_u35 (1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_tanf_u10 (1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_tanf_u35 (1.0f)));

    EXPECT_TRUE (std::isfinite (Sleef_asinf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_asinf_u35 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_acosf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_acosf_u35 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_atanf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_atanf_u35 (0.5f)));

    EXPECT_TRUE (std::isfinite (Sleef_sinhf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_sinhf_u35 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_coshf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_coshf_u35 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_tanhf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_tanhf_u35 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_asinhf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_acoshf_u10 (1.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_atanhf_u10 (0.5f)));

    EXPECT_TRUE (std::isfinite (Sleef_expf_u10 (0.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_logf_u10 (1.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_logf_u35 (1.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_log10f_u10 (1.5f)));
    EXPECT_TRUE (std::isfinite (Sleef_powf_u10 (1.5f, 2.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_atan2f_u10 (0.5f, 1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_atan2f_u35 (0.5f, 1.0f)));
    EXPECT_TRUE (std::isfinite (Sleef_fmodf (5.5f, 2.0f)));

    for (const auto& value : sleefLanes (Sleef_sinf4_u10 (v1)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_sinf4_u35 (v1)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_cosf4_u10 (v1)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_cosf4_u35 (v1)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_tanf4_u10 (v1)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_tanf4_u35 (v1)))
        EXPECT_TRUE (std::isfinite (value));

    for (const auto& value : sleefLanes (Sleef_asinf4_u10 (vUnit)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_asinf4_u35 (vUnit)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_acosf4_u10 (vUnit)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_acosf4_u35 (vUnit)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_atanf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_atanf4_u35 (v2)))
        EXPECT_TRUE (std::isfinite (value));

    for (const auto& value : sleefLanes (Sleef_sinhf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_sinhf4_u35 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_coshf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_coshf4_u35 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_tanhf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_tanhf4_u35 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_asinhf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_acoshf4_u10 (sleefLoad ({ 1.5f, 2.0f, 2.5f, 3.0f }))))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_atanhf4_u10 (vUnit)))
        EXPECT_TRUE (std::isfinite (value));

    for (const auto& value : sleefLanes (Sleef_expf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_logf4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_logf4_u35 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_log10f4_u10 (v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_powf4_u10 (v1, v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_atan2f4_u10 (v1, v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_atan2f4_u35 (v1, v2)))
        EXPECT_TRUE (std::isfinite (value));
    for (const auto& value : sleefLanes (Sleef_fmodf4 (sleefLoad ({ 5.5f, 4.5f, 3.5f, 2.5f }), v2)))
        EXPECT_TRUE (std::isfinite (value));
}

} // namespace yup::test
