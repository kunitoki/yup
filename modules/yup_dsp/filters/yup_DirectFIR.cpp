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

namespace
{

//==============================================================================

float dotProduct (const float* __restrict a, const float* __restrict b, std::size_t len) noexcept
{
    float acc = 0.0f;
    std::size_t i = 0;

#if YUP_USE_AVX_INTRINSICS && YUP_USE_FMA_INTRINSICS
    __m256 vacc = _mm256_setzero_ps();
    for (; i + 8 <= len; i += 8)
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
    acc += _mm_cvtss_f32 (vsum);

#elif YUP_USE_SSE_INTRINSICS
    __m128 vacc = _mm_setzero_ps();
#if YUP_USE_FMA_INTRINSICS
    for (; i + 4 <= len; i += 4)
    {
        __m128 va = _mm_loadu_ps (a + i);
        __m128 vb = _mm_loadu_ps (b + i);
        vacc = _mm_fmadd_ps (va, vb, vacc);
    }
#else
    for (; i + 4 <= len; i += 4)
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
    acc += _mm_cvtss_f32 (sums);

#elif YUP_USE_ARM_NEON
    float32x4_t vacc = vdupq_n_f32 (0.0f);
    for (; i + 4 <= len; i += 4)
    {
        float32x4_t va = vld1q_f32 (a + i);
        float32x4_t vb = vld1q_f32 (b + i);
        vacc = vmlaq_f32 (vacc, va, vb);
    }
#if YUP_64BIT
    acc += vaddvq_f32 (vacc);
#else
    float32x2_t vlow = vget_low_f32 (vacc);
    float32x2_t vhigh = vget_high_f32 (vacc);
    float32x2_t vsum2 = vpadd_f32 (vlow, vhigh);
    vsum2 = vpadd_f32 (vsum2, vsum2);
    acc += vget_lane_f32 (vsum2, 0);
#endif

#endif

    // Handle remaining samples
    for (; i < len; ++i)
        acc += a[i] * b[i];

    return acc;
}

} // namespace

//==============================================================================

DirectFIR::DirectFIR() = default;

DirectFIR::~DirectFIR() = default;

DirectFIR::DirectFIR (DirectFIR&& other) noexcept
    : coefficientsReversed (std::move (other.coefficientsReversed))
    , history (std::move (other.history))
    , numCoefficients (std::exchange (other.numCoefficients, 0))
    , paddedLen (std::exchange (other.paddedLen, 0))
    , writeIndex (std::exchange (other.writeIndex, 0))
    , currentScaling (std::exchange (other.currentScaling, 1.0f))
{
}

DirectFIR& DirectFIR::operator= (DirectFIR&& other) noexcept
{
    if (this != &other)
    {
        coefficientsReversed = std::move (other.coefficientsReversed);
        history = std::move (other.history);
        numCoefficients = std::exchange (other.numCoefficients, 0);
        paddedLen = std::exchange (other.paddedLen, 0);
        writeIndex = std::exchange (other.writeIndex, 0);
        currentScaling = std::exchange (other.currentScaling, 1.0f);
    }
    return *this;
}

void DirectFIR::setCoefficients (std::vector<float> coefficients, float scaling)
{
    currentScaling = scaling;
    if (! approximatelyEqual (currentScaling, 1.0f))
        FloatVectorOperations::multiply (coefficients.data(), scaling, coefficients.size());

    coefficientsReversed = std::move (coefficients);
    std::reverse (coefficientsReversed.begin(), coefficientsReversed.end());

    numCoefficients = coefficientsReversed.size();
    paddedLen = (numCoefficients + 3u) & ~3u; // Round up to multiple of 4 for SIMD
    coefficientsReversed.resize (paddedLen, 0.0f);

    history.assign (2 * numCoefficients, 0.0f);

    reset();
}

void DirectFIR::setCoefficients (const float* coefficients, std::size_t numCoefficientsIn, float scaling)
{
    if (coefficients == nullptr || numCoefficientsIn == 0)
    {
        reset();
        numCoefficients = 0;
        return;
    }

    std::vector<float> coefficientsVector (coefficients, coefficients + numCoefficientsIn);
    setCoefficients (std::move (coefficientsVector), scaling);
}

std::size_t DirectFIR::getNumCoefficients() const noexcept
{
    return numCoefficients;
}

bool DirectFIR::hasCoefficients() const noexcept
{
    return numCoefficients > 0;
}

const std::vector<float>& DirectFIR::getCoefficients() const noexcept
{
    return coefficientsReversed;
}

float DirectFIR::getScaling() const noexcept
{
    return currentScaling;
}

void DirectFIR::reset()
{
    std::fill (history.begin(), history.end(), 0.0f);
    writeIndex = 0;
}

void DirectFIR::process (const float* input, float* output, std::size_t numSamples) noexcept
{
    const std::size_t M = numCoefficients;
    if (M == 0 || input == nullptr || output == nullptr)
        return;

    const float* h = coefficientsReversed.data();

    for (std::size_t i = 0; i < numSamples; ++i)
    {
        const float x = input[i];

        // Update circular buffer with current input sample
        history[writeIndex] = x;
        history[writeIndex + M] = x; // Duplicate for efficient circular access

        // Point to the start of the delay line for this sample
        const float* w = history.data() + writeIndex + 1;

        float sum = 0.0f;

#if YUP_ENABLE_VDSP
        // Use Apple's optimized vDSP if available
        vDSP_dotpr (w, 1, h, 1, &sum, M);
#else
        // Use our own SIMD-optimized dot product
        sum = dotProduct (w, h, M);
#endif

        // Accumulate result into output
        output[i] += sum;

        // Advance circular buffer write pointer
        if (++writeIndex == M)
            writeIndex = 0;
    }
}

} // namespace yup
