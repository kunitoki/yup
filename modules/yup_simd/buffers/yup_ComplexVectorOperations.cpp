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

namespace yup
{

namespace
{
void complexMultiply (const float* __restrict a, const float* __restrict b, float* __restrict y, int complexPairs, bool accumulate) noexcept
{
    int i = 0;

#if YUP_USE_AVX_INTRINSICS && YUP_USE_FMA_INTRINSICS
    constexpr int simdWidth = 4;
    const __m256 signs = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);

    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        const __m256 av = _mm256_loadu_ps (a + idx);
        const __m256 bv = _mm256_loadu_ps (b + idx);

        const __m256 aReal = _mm256_permute_ps (av, _MM_SHUFFLE (2, 2, 0, 0));
        const __m256 aImag = _mm256_permute_ps (av, _MM_SHUFFLE (3, 3, 1, 1));
        const __m256 bSwapped = _mm256_permute_ps (bv, _MM_SHUFFLE (2, 3, 0, 1));
        const __m256 realProducts = _mm256_mul_ps (aReal, bv);
        const __m256 imagProducts = _mm256_mul_ps (aImag, bSwapped);

        __m256 interleaved = _mm256_fmadd_ps (imagProducts, signs, realProducts);

        if (accumulate)
            interleaved = _mm256_add_ps (_mm256_loadu_ps (y + idx), interleaved);

        _mm256_storeu_ps (y + idx, interleaved);
    }

#elif YUP_USE_SSE_INTRINSICS
    constexpr int simdWidth = 2;
    const __m128 signs = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);

    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        const __m128 av = _mm_loadu_ps (a + idx);
        const __m128 bv = _mm_loadu_ps (b + idx);

        const __m128 aReal = _mm_shuffle_ps (av, av, _MM_SHUFFLE (2, 2, 0, 0));
        const __m128 aImag = _mm_shuffle_ps (av, av, _MM_SHUFFLE (3, 3, 1, 1));
        const __m128 bSwapped = _mm_shuffle_ps (bv, bv, _MM_SHUFFLE (2, 3, 0, 1));
        const __m128 realProducts = _mm_mul_ps (aReal, bv);
        const __m128 imagProducts = _mm_mul_ps (aImag, bSwapped);

        __m128 interleaved = _mm_add_ps (realProducts, _mm_mul_ps (imagProducts, signs));

        if (accumulate)
            interleaved = _mm_add_ps (_mm_loadu_ps (y + idx), interleaved);

        _mm_storeu_ps (y + idx, interleaved);
    }

#elif YUP_USE_ARM_NEON
    constexpr int simdWidth = 4;
    for (; i <= complexPairs - simdWidth; i += simdWidth)
    {
        const int idx = i * 2;

        float32x4x2_t av = vld2q_f32 (a + idx);
        float32x4x2_t bv = vld2q_f32 (b + idx);

        float32x4_t real = vmulq_f32 (av.val[0], bv.val[0]);
        real = vfmsq_f32 (real, av.val[1], bv.val[1]);
        float32x4_t imag = vmulq_f32 (av.val[0], bv.val[1]);
        imag = vfmaq_f32 (imag, av.val[1], bv.val[0]);

        if (accumulate)
        {
            float32x4x2_t yv = vld2q_f32 (y + idx);
            real = vaddq_f32 (yv.val[0], real);
            imag = vaddq_f32 (yv.val[1], imag);
        }

        float32x4x2_t out = { real, imag };
        vst2q_f32 (y + idx, out);
    }

#endif

    for (; i < complexPairs; ++i)
    {
        const int realIndex = i * 2;
        const int imagIndex = realIndex + 1;

        const float ar = a[realIndex];
        const float ai = a[imagIndex];
        const float br = b[realIndex];
        const float bi = b[imagIndex];

        const float real = ar * br - ai * bi;
        const float imag = ar * bi + ai * br;

        if (accumulate)
        {
            y[realIndex] += real;
            y[imagIndex] += imag;
        }
        else
        {
            y[realIndex] = real;
            y[imagIndex] = imag;
        }
    }
}
} // namespace

//==============================================================================
void YUP_CALLTYPE ComplexVectorOperations::multiplyAccumulate (const float* a, const float* b, float* y, int complexPairs) noexcept
{
    complexMultiply (a, b, y, complexPairs, true);
}

void YUP_CALLTYPE ComplexVectorOperations::multiply (float* dest, const float* a, const float* b, int complexPairs) noexcept
{
    complexMultiply (a, b, dest, complexPairs, false);
}

void YUP_CALLTYPE ComplexVectorOperations::powerSpectrum (float* dest, const float* src, int complexPairs) noexcept
{
    int i = 0;

#if YUP_USE_SSE_INTRINSICS
    for (; i + 2 <= complexPairs; i += 2)
    {
        const __m128 values = _mm_loadu_ps (src + i * 2);
        const __m128 squared = _mm_mul_ps (values, values);
        alignas (16) float lanes[4];
        _mm_store_ps (lanes, squared);
        dest[i] = lanes[0] + lanes[1];
        dest[i + 1] = lanes[2] + lanes[3];
    }
#elif YUP_USE_ARM_NEON
    for (; i + 4 <= complexPairs; i += 4)
    {
        const float32x4x2_t values = vld2q_f32 (src + i * 2);
        vst1q_f32 (dest + i, vmlaq_f32 (vmulq_f32 (values.val[0], values.val[0]), values.val[1], values.val[1]));
    }
#endif

    for (; i < complexPairs; ++i)
    {
        const float real = src[i * 2];
        const float imag = src[i * 2 + 1];
        dest[i] = real * real + imag * imag;
    }
}

} // namespace yup
