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

#pragma once

namespace yup
{

//==============================================================================
/** Fixed-width SIMD-style register backed by xsimd batches.

    The lane count is part of the type, which keeps code portable across platforms
    where the native SIMD width differs.

    @tparam T The element type. Must be supported by xsimd batches.
    @tparam N The number of elements. Must be greater than 0.
*/
template <typename T, int N>
class SIMDRegister
{
    static_assert (N > 0, "SIMDRegister must contain at least one element");

public:
    //==============================================================================
    /** The element type of the SIMD register. */
    using ElementType = T;

    /** The number of elements in the SIMD register. */
    static constexpr int size = N;

    //==============================================================================
    /** Default constructor. Initializes all elements to zero. */
    SIMDRegister() noexcept
    {
        for (auto& batch : data)
            batch = Batch (T {});
    }

    /** Constructs a SIMD register with all elements set to the given scalar value.
    
        @param scalar The value to broadcast to all elements of the register.
    */
    explicit SIMDRegister (T scalar) noexcept
    {
        for (auto& batch : data)
            batch = Batch (scalar);
    }

    /** Constructs a SIMD register by loading elements from a pointer.
    
        @param ptr The pointer to the elements to load.
    */
    explicit SIMDRegister (const T* ptr) noexcept
    {
        load (ptr, false);
    }

    //==============================================================================
    /** Loads a SIMD register from a pointer, assuming the pointer is aligned.
    
        @param ptr The pointer to the elements to load. Must be aligned to the SIMD width.

        @returns A SIMD register containing the loaded elements.
    */
    static forcedinline SIMDRegister loadAligned (const T* ptr) noexcept
    {
        SIMDRegister result;
        result.load (ptr, true);
        return result;
    }

    /** Loads a SIMD register from a pointer, assuming the pointer is not aligned.
    
        @param ptr The pointer to the elements to load. May be unaligned.

        @returns A SIMD register containing the loaded elements.
    */
    static forcedinline SIMDRegister loadUnaligned (const T* ptr) noexcept
    {
        SIMDRegister result;
        result.load (ptr, false);
        return result;
    }

    //==============================================================================
    /** Constructs a SIMD register with all elements set to the given scalar value.
    
        @param scalar The value to broadcast to all elements of the register.

        @returns A SIMD register with all elements set to the given scalar value.
    */
    static forcedinline SIMDRegister broadcast (T scalar) noexcept
    {
        return SIMDRegister (scalar);
    }

    //==============================================================================
    /** Constructs a SIMD register with all elements set to zero.
    
        @returns A SIMD register with all elements set to zero.
    */
    static forcedinline SIMDRegister zero() noexcept
    {
        return SIMDRegister();
    }

    //==============================================================================
    /** Stores the SIMD register to a pointer, assuming the pointer is aligned.
    
        @param ptr The pointer to store the elements. Must be aligned to the SIMD width.
    */
    forcedinline void storeAligned (T* ptr) const noexcept
    {
        store (ptr, true);
    }

    /** Stores the SIMD register to a pointer, assuming the pointer is not aligned.
    
        @param ptr The pointer to store the elements. May be unaligned.
    */
    forcedinline void storeUnaligned (T* ptr) const noexcept
    {
        store (ptr, false);
    }

    //==============================================================================
    /** Accesses an element of the SIMD register by index.
    
        @param i The index of the element to access. Must be in the range [0, N).

        @returns The value of the element at the specified index.
    */
    forcedinline T operator[] (int i) const noexcept
    {
        jassert (i >= 0 && i < N);

        alignas (64) T lanes[nativeSize];
        data[static_cast<std::size_t> (i) / nativeSize].store_unaligned (lanes);
        return lanes[static_cast<std::size_t> (i) % nativeSize];
    }

    //==============================================================================
    /** Adds two SIMD registers element-wise.
    
        @param other The SIMD register to add.

        @returns A SIMD register containing the element-wise sum.
    */
    forcedinline SIMDRegister operator+ (SIMDRegister other) const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = data[i] + other.data[i];
        return result;
    }

    /** Subtracts two SIMD registers element-wise.
    
        @param other The SIMD register to subtract.

        @returns A SIMD register containing the element-wise difference.
    */
    forcedinline SIMDRegister operator- (SIMDRegister other) const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = data[i] - other.data[i];
        return result;
    }

    /** Multiplies two SIMD registers element-wise.
    
        @param other The SIMD register to multiply.

        @returns A SIMD register containing the element-wise product.
    */
    forcedinline SIMDRegister operator* (SIMDRegister other) const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = data[i] * other.data[i];
        return result;
    }

    /** Divides two SIMD registers element-wise.
    
        @param other The SIMD register to divide by.

        @returns A SIMD register containing the element-wise quotient.
    */
    forcedinline SIMDRegister operator/ (SIMDRegister other) const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = data[i] / other.data[i];
        return result;
    }

    /** Adds another SIMD register to this one element-wise.
    
        @param other The SIMD register to add.

        @returns A reference to this SIMD register after the addition.
    */
    forcedinline SIMDRegister& operator+= (SIMDRegister other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    /** Multiplies another SIMD register with this one element-wise.
    
        @param other The SIMD register to multiply.

        @returns A reference to this SIMD register after the multiplication.
    */
    forcedinline SIMDRegister& operator*= (SIMDRegister other) noexcept
    {
        *this = *this * other;
        return *this;
    }

    //==============================================================================
    /** Performs a fused multiply-add operation: this + (a * b).
    
        @param a The first SIMD register to multiply.
        @param b The second SIMD register to multiply.

        @returns A SIMD register containing the result of the fused multiply-add.
    */
    forcedinline SIMDRegister mulAdd (SIMDRegister a, SIMDRegister b) const noexcept
    {
        return *this + a * b;
    }

    //==============================================================================
    /** Computes the absolute value of each element in the SIMD register.
    
        @returns A SIMD register containing the absolute values of the elements.
    */
    forcedinline SIMDRegister abs() const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = xsimd::abs (data[i]);
        return result;
    }

    //==============================================================================
    /** Computes the element-wise minimum of two SIMD registers.
    
        @param other The SIMD register to compare with.

        @returns A SIMD register containing the element-wise minimum.
    */
    forcedinline SIMDRegister min (SIMDRegister other) const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = xsimd::min (data[i], other.data[i]);
        return result;
    }

    /** Computes the element-wise maximum of two SIMD registers.
    
        @param other The SIMD register to compare with.

        @returns A SIMD register containing the element-wise maximum.
    */
    forcedinline SIMDRegister max (SIMDRegister other) const noexcept
    {
        SIMDRegister result;
        for (std::size_t i = 0; i < numBatches; ++i)
            result.data[i] = xsimd::max (data[i], other.data[i]);
        return result;
    }

    //==============================================================================
    /** Computes the sum of all elements in the SIMD register.
    
        @returns The sum of the elements.
    */
    forcedinline T sum() const noexcept
    {
        T result {};
        for (int i = 0; i < N; ++i)
            result += (*this)[i];
        return result;
    }

    //==============================================================================
    /** Computes the maximum value among all elements in the SIMD register.
    
        @returns The maximum value among the elements.
    */
    forcedinline T hmax() const noexcept
    {
        T result = (*this)[0];
        for (int i = 1; i < N; ++i)
            result = jmax (result, (*this)[i]);
        return result;
    }

private:
    using Batch = xsimd::batch<T>;

    static constexpr std::size_t nativeSize = Batch::size;
    static constexpr std::size_t numBatches = (static_cast<std::size_t> (N) + nativeSize - 1) / nativeSize;

    forcedinline void load (const T* ptr, bool aligned) noexcept
    {
        std::size_t offset = 0;

        for (std::size_t batchIndex = 0; batchIndex < numBatches; ++batchIndex)
        {
            if (offset + nativeSize <= static_cast<std::size_t> (N))
            {
                data[batchIndex] = aligned ? Batch::load_aligned (ptr + offset)
                                           : Batch::load_unaligned (ptr + offset);
            }
            else
            {
                alignas (64) T lanes[nativeSize] = {};
                const auto remaining = static_cast<std::size_t> (N) - offset;

                for (std::size_t i = 0; i < remaining; ++i)
                    lanes[i] = ptr[offset + i];

                data[batchIndex] = Batch::load_unaligned (lanes);
            }

            offset += nativeSize;
        }
    }

    forcedinline void store (T* ptr, bool aligned) const noexcept
    {
        std::size_t offset = 0;

        for (std::size_t batchIndex = 0; batchIndex < numBatches; ++batchIndex)
        {
            if (offset + nativeSize <= static_cast<std::size_t> (N))
            {
                if (aligned)
                    data[batchIndex].store_aligned (ptr + offset);
                else
                    data[batchIndex].store_unaligned (ptr + offset);
            }
            else
            {
                alignas (64) T lanes[nativeSize];
                data[batchIndex].store_unaligned (lanes);

                const auto remaining = static_cast<std::size_t> (N) - offset;

                for (std::size_t i = 0; i < remaining; ++i)
                    ptr[offset + i] = lanes[i];
            }

            offset += nativeSize;
        }
    }

    std::array<Batch, numBatches> data;
};

using Float4 = SIMDRegister<float, 4>;
using Float8 = SIMDRegister<float, 8>;
using Double2 = SIMDRegister<double, 2>;
using Double4 = SIMDRegister<double, 4>;

} // namespace yup
