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

namespace ComplexVectorHelpers
{
// Shared with yup_FloatVectorOperations.cpp, which comes first in the module's unity build
using FloatVectorHelpers::multiplyAdd;
using FloatVectorHelpers::multiplySubtract;
using FloatVectorHelpers::negativeMultiplyAdd;
using FloatVectorHelpers::squareRoot;

#if ! defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
using FloatVectorHelpers::hasNativeBatch;

template <typename Type>
std::complex<Type>* asComplex (Type* values) noexcept
{
    return reinterpret_cast<std::complex<Type>*> (values);
}

template <typename Type>
const std::complex<Type>* asComplex (const Type* values) noexcept
{
    return reinterpret_cast<const std::complex<Type>*> (values);
}
#endif

int firstComplexPair (ComplexLayout layout) noexcept
{
    return layout == ComplexLayout::packedReal ? 1 : 0;
}

// dest (+)= a * b or a * conj (b), with fused multiply-adds on batches. The tail goes through a zero padded
// batch too, so a pair gets the same result wherever it sits. Every pair is loaded before it is stored, so
// dest may alias a or b.
template <bool conjugate, bool accumulate, typename Type>
void multiply (Type* dest, const Type* a, const Type* b, int complexPairs, ComplexLayout layout) noexcept
{
    if (complexPairs < 1)
        return;

    // (ar + i ai) * (br + i bi), or times (br - i bi) when conjugating, plus (pr + i pi) when accumulating
    const auto product = [] (const auto& ar, const auto& ai, const auto& br, const auto& bi, const auto& pr, const auto& pi)
    {
        if constexpr (accumulate)
            return std::make_pair (multiplyAdd (ar, br, negativeMultiplyAdd (ai, bi, pr)), multiplyAdd (ai, br, multiplyAdd (ar, bi, pi)));
        else if constexpr (conjugate)
            return std::make_pair (multiplyAdd (ar, br, ai * bi), multiplySubtract (ai, br, ar * bi));
        else
            return std::make_pair (multiplySubtract (ar, br, ai * bi), multiplyAdd (ai, br, ar * bi));
    };

    static_assert (! (conjugate && accumulate));

    int i = firstComplexPair (layout);

    if (i == 1)
    {
        if constexpr (accumulate)
        {
            dest[0] += a[0] * b[0];
            dest[1] += a[1] * b[1];
        }
        else
        {
            dest[0] = a[0] * b[0];
            dest[1] = a[1] * b[1];
        }
    }

#if ! defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
    if constexpr (hasNativeBatch<Type>)
    {
        using Complex = std::complex<Type>;
        using Batch = xsimd::batch<Complex>;
        constexpr int batchSize = static_cast<int> (Batch::size);

        const auto multiplyBatch = [&product] (Complex* d, const Complex* x, const Complex* y)
        {
            const auto xs = Batch::load_unaligned (x);
            const auto ys = Batch::load_unaligned (y);

            typename Batch::real_batch previousReal (Type (0)), previousImag (Type (0));

            if constexpr (accumulate)
            {
                const auto previous = Batch::load_unaligned (d);
                previousReal = previous.real();
                previousImag = previous.imag();
            }

            const auto [real, imag] = product (xs.real(), xs.imag(), ys.real(), ys.imag(), previousReal, previousImag);
            Batch (real, imag).store_unaligned (d);
        };

        for (; i + batchSize <= complexPairs; i += batchSize)
            multiplyBatch (asComplex (dest) + i, asComplex (a) + i, asComplex (b) + i);

        if (i < complexPairs)
        {
            const int remaining = complexPairs - i;
            Complex x[Batch::size] = {}, y[Batch::size] = {}, d[Batch::size] = {};

            std::copy_n (asComplex (a) + i, remaining, x);
            std::copy_n (asComplex (b) + i, remaining, y);

            if constexpr (accumulate)
                std::copy_n (asComplex (dest) + i, remaining, d);

            multiplyBatch (d, x, y);
            std::copy_n (d, remaining, asComplex (dest) + i);
        }

        return;
    }
#endif

    for (; i < complexPairs; ++i)
    {
        const Type pr = accumulate ? dest[i * 2] : Type (0);
        const Type pi = accumulate ? dest[i * 2 + 1] : Type (0);
        const auto [real, imag] = product (a[i * 2], a[i * 2 + 1], b[i * 2], b[i * 2 + 1], pr, pi);

        dest[i * 2] = real;
        dest[i * 2 + 1] = imag;
    }
}

// dest[k] = operation (real, imag) of pair k, with an operation written for both xsimd batches and scalars.
// The tail goes through a zero padded batch too, so a pair gets the same result wherever it sits in the
// buffer. A batch of pairs is read before its values are written: dest may alias src.
template <typename Type, typename Operation>
void toReal (Type* dest, const Type* src, int complexPairs, ComplexLayout layout, Operation&& operation) noexcept
{
    if (complexPairs < 1)
        return;

    int i = firstComplexPair (layout);

    if (i == 1)
        dest[0] = operation (src[0], Type (0));

#if ! defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
    if constexpr (hasNativeBatch<Type>)
    {
        using Batch = xsimd::batch<std::complex<Type>>;
        constexpr int batchSize = static_cast<int> (Batch::size);

        // four independent batches at a time hide the latency of sqrt and atan2;
        // all loads come first, so dest may be src
        for (; i + 4 * batchSize <= complexPairs; i += 4 * batchSize)
        {
            const auto x0 = Batch::load_unaligned (asComplex (src) + i);
            const auto x1 = Batch::load_unaligned (asComplex (src) + i + batchSize);
            const auto x2 = Batch::load_unaligned (asComplex (src) + i + 2 * batchSize);
            const auto x3 = Batch::load_unaligned (asComplex (src) + i + 3 * batchSize);

            const auto y0 = operation (x0.real(), x0.imag());
            const auto y1 = operation (x1.real(), x1.imag());
            const auto y2 = operation (x2.real(), x2.imag());
            const auto y3 = operation (x3.real(), x3.imag());

            y0.store_unaligned (dest + i);
            y1.store_unaligned (dest + i + batchSize);
            y2.store_unaligned (dest + i + 2 * batchSize);
            y3.store_unaligned (dest + i + 3 * batchSize);
        }

        for (; i + batchSize <= complexPairs; i += batchSize)
        {
            const auto x = Batch::load_unaligned (asComplex (src) + i);
            operation (x.real(), x.imag()).store_unaligned (dest + i);
        }

        if (i < complexPairs)
        {
            std::complex<Type> pairs[Batch::size] = {};
            Type values[Batch::size];

            std::copy (asComplex (src) + i, asComplex (src) + complexPairs, pairs);

            const auto x = Batch::load_unaligned (pairs);
            operation (x.real(), x.imag()).store_unaligned (values);

            std::copy (values, values + (complexPairs - i), dest + i);
        }

        return;
    }
#endif

    for (; i < complexPairs; ++i)
        dest[i] = operation (src[i * 2], src[i * 2 + 1]);
}

template <typename Type>
void deinterleave (Type* real, Type* imag, const Type* src, int complexPairs, ComplexLayout layout) noexcept
{
    if (complexPairs < 1)
        return;

    const int first = firstComplexPair (layout);
    int i = first;

    if (first == 1)
    {
        real[0] = src[0];
        real[complexPairs] = src[1];
    }

#if ! defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
    if constexpr (hasNativeBatch<Type>)
    {
        using Batch = xsimd::batch<std::complex<Type>>;
        constexpr int batchSize = static_cast<int> (Batch::size);

        for (; i + batchSize <= complexPairs; i += batchSize)
        {
            const auto x = Batch::load_unaligned (asComplex (src) + i);
            x.real().store_unaligned (real + i);
            x.imag().store_unaligned (imag + i - first);
        }
    }
#endif

    for (; i < complexPairs; ++i)
    {
        real[i] = src[i * 2];
        imag[i - first] = src[i * 2 + 1];
    }
}

template <typename Type>
void interleave (Type* dest, const Type* real, const Type* imag, int complexPairs, ComplexLayout layout) noexcept
{
    if (complexPairs < 1)
        return;

    const int first = firstComplexPair (layout);
    int i = first;

    if (first == 1)
    {
        dest[0] = real[0];
        dest[1] = real[complexPairs];
    }

#if ! defined(XSIMD_NO_SUPPORTED_ARCHITECTURE)
    if constexpr (hasNativeBatch<Type>)
    {
        using Batch = xsimd::batch<std::complex<Type>>;
        using RealBatch = xsimd::batch<Type>;
        constexpr int batchSize = static_cast<int> (Batch::size);

        for (; i + batchSize <= complexPairs; i += batchSize)
            Batch (RealBatch::load_unaligned (real + i), RealBatch::load_unaligned (imag + i - first)).store_unaligned (asComplex (dest) + i);
    }
#endif

    for (; i < complexPairs; ++i)
    {
        dest[i * 2] = real[i];
        dest[i * 2 + 1] = imag[i - first];
    }
}

template <typename Type>
void powerSpectrum (Type* dest, const Type* src, int complexPairs, ComplexLayout layout) noexcept
{
    toReal (dest, src, complexPairs, layout, [] (const auto& real, const auto& imag)
    {
        return multiplyAdd (real, real, imag * imag);
    });
}

template <typename Type>
void magnitude (Type* dest, const Type* src, int complexPairs, ComplexLayout layout) noexcept
{
    toReal (dest, src, complexPairs, layout, [] (const auto& real, const auto& imag)
    {
        return squareRoot (multiplyAdd (real, real, imag * imag));
    });
}

template <typename Type>
void phase (Type* dest, const Type* src, int complexPairs, ComplexLayout layout) noexcept
{
#if YUP_USE_VDSP_FRAMEWORK
    // vForce is faster than xsimd here: split a block of pairs on the stack (so dest may be src) and convert it
    if (complexPairs < 1)
        return;

    int i = firstComplexPair (layout);

    if (i == 1)
        dest[0] = std::atan2 (Type (0), src[0]);

    constexpr int blockSize = 256;
    Type real[blockSize], imag[blockSize];

    for (; i < complexPairs; i += blockSize)
    {
        const int count = jmin (blockSize, complexPairs - i);

        for (int k = 0; k < count; ++k)
        {
            real[k] = src[(i + k) * 2];
            imag[k] = src[(i + k) * 2 + 1];
        }

        if constexpr (std::is_same_v<Type, float>)
            vvatan2f (dest + i, imag, real, &count);
        else
            vvatan2 (dest + i, imag, real, &count);
    }
#else
    toReal (dest, src, complexPairs, layout, [] (const auto& real, const auto& imag)
    {
        using Value = std::decay_t<decltype (real)>;

        if constexpr (std::is_floating_point_v<Value>)
        {
            return std::atan2 (imag, real);
        }
        else
        {
            // xsimd::atan2 is NaN at the origin, where std::atan2 gives 0 or pi (by the sign of real) with the sign of imag
            const Value zero (Type (0));
            const auto realIsNegative = xsimd::copysign (Value (Type (1)), real) < zero;
            const auto origin = xsimd::copysign (xsimd::select (realIsNegative, Value (MathConstants<Type>::pi), zero), imag);

            return xsimd::select ((real == zero) & (imag == zero), origin, xsimd::atan2 (imag, real));
        }
    });
#endif
}
} // namespace ComplexVectorHelpers

//==============================================================================
void YUP_CALLTYPE ComplexVectorOperations::multiply (float* dest, const float* a, const float* b, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::multiply<false, false> (dest, a, b, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::multiply (double* dest, const double* a, const double* b, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::multiply<false, false> (dest, a, b, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::multiplyConjugate (float* dest, const float* a, const float* b, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::multiply<true, false> (dest, a, b, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::multiplyConjugate (double* dest, const double* a, const double* b, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::multiply<true, false> (dest, a, b, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::multiplyAccumulate (const float* a, const float* b, float* y, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::multiply<false, true> (y, a, b, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::multiplyAccumulate (const double* a, const double* b, double* y, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::multiply<false, true> (y, a, b, complexPairs, layout);
}

//==============================================================================
void YUP_CALLTYPE ComplexVectorOperations::powerSpectrum (float* dest, const float* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::powerSpectrum (dest, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::powerSpectrum (double* dest, const double* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::powerSpectrum (dest, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::magnitude (float* dest, const float* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::magnitude (dest, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::magnitude (double* dest, const double* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::magnitude (dest, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::phase (float* dest, const float* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::phase (dest, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::phase (double* dest, const double* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::phase (dest, src, complexPairs, layout);
}

//==============================================================================
void YUP_CALLTYPE ComplexVectorOperations::deinterleave (float* real, float* imag, const float* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::deinterleave (real, imag, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::deinterleave (double* real, double* imag, const double* src, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::deinterleave (real, imag, src, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::interleave (float* dest, const float* real, const float* imag, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::interleave (dest, real, imag, complexPairs, layout);
}

void YUP_CALLTYPE ComplexVectorOperations::interleave (double* dest, const double* real, const double* imag, int complexPairs, ComplexLayout layout) noexcept
{
    ComplexVectorHelpers::interleave (dest, real, imag, complexPairs, layout);
}

} // namespace yup
