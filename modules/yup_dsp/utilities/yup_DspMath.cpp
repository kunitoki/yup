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
*/

namespace yup
{

//==============================================================================

template <>
float dotProduct (const float* __restrict a, const float* __restrict b, std::size_t length) noexcept
{
    float accumulation = 0.0f;

#if YUP_ENABLE_VDSP
    vDSP_dotpr (a, 1, b, 1, &accumulation, length);

#else
    std::size_t i = 0;

#if YUP_USE_AVX_INTRINSICS && YUP_USE_FMA_INTRINSICS
    __m256 vacc = _mm256_setzero_ps();
    for (; i + 8 <= length; i += 8)
    {
        __m256 va = _mm256_loadu_ps (a + i);
        __m256 vb = _mm256_loadu_ps (b + i);
        vacc = _mm256_fmadd_ps (va, vb, vacc);
    }
    __m128 low = _mm256_castps256_ps128 (vacc);
    __m128 high = _mm256_extractf128_ps (vacc, 1);
    __m128 vsum = _mm_add_ps (low, high);
    vsum = _mm_hadd_ps (vsum, vsum);
    vsum = _mm_hadd_ps (vsum, vsum);
    accumulation += _mm_cvtss_f32 (vsum);

#elif YUP_USE_SSE_INTRINSICS
    __m128 vacc = _mm_setzero_ps();
#if YUP_USE_FMA_INTRINSICS
    for (; i + 4 <= length; i += 4)
    {
        __m128 va = _mm_loadu_ps (a + i);
        __m128 vb = _mm_loadu_ps (b + i);
        vacc = _mm_fmadd_ps (va, vb, vacc);
    }
#else
    for (; i + 4 <= length; i += 4)
    {
        __m128 va = _mm_loadu_ps (a + i);
        __m128 vb = _mm_loadu_ps (b + i);
        vacc = _mm_add_ps (vacc, _mm_mul_ps (va, vb));
    }
#endif
    __m128 shuf = _mm_shuffle_ps (vacc, vacc, _MM_SHUFFLE (2, 3, 0, 1));
    __m128 sums = _mm_add_ps (vacc, shuf);
    shuf = _mm_movehl_ps (shuf, sums);
    sums = _mm_add_ss (sums, shuf);
    accumulation += _mm_cvtss_f32 (sums);

#elif YUP_USE_ARM_NEON
    float32x4_t vacc = vdupq_n_f32 (0.0f);
    for (; i + 4 <= length; i += 4)
    {
        float32x4_t va = vld1q_f32 (a + i);
        float32x4_t vb = vld1q_f32 (b + i);
        vacc = vmlaq_f32 (vacc, va, vb);
    }
#if YUP_64BIT
    accumulation += vaddvq_f32 (vacc);
#else
    float32x2_t vlow = vget_low_f32 (vacc);
    float32x2_t vhigh = vget_high_f32 (vacc);
    float32x2_t vsum2 = vpadd_f32 (vlow, vhigh);
    vsum2 = vpadd_f32 (vsum2, vsum2);
    accumulation += vget_lane_f32 (vsum2, 0);
#endif

#endif

    // Handle remaining samples
    for (; i < length; ++i)
        accumulation += a[i] * b[i];
#endif

    return accumulation;
}

} // namespace yup
