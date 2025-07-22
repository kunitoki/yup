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

#pragma once

namespace yup
{

//==============================================================================
/**
    Window function types for spectral analysis and FIR filter design.

    This enumeration provides all commonly used window functions with
    optimal frequency and time domain characteristics for different applications.

    @see WindowFunctions
*/
enum class WindowType
{
    rectangular,    /**< Rectangular (no windowing) */
    hann,          /**< Hann window (raised cosine) */
    hamming,       /**< Hamming window */
    blackman,      /**< Blackman window */
    blackmanHarris,/**< Blackman-Harris window (4-term) */
    kaiser,        /**< Kaiser window (parameterizable) */
    gaussian,      /**< Gaussian window */
    tukey,         /**< Tukey window (tapered cosine) */
    bartlett,      /**< Bartlett window (triangular) */
    welch,         /**< Welch window (parabolic) */
    flattop,       /**< Flat-top window */
    cosine,        /**< Cosine window */
    lanczos,       /**< Lanczos window (sinc) */
    nuttall,       /**< Nuttall window */
    blackmanNuttall/**< Blackman-Nuttall window */
};

//==============================================================================
/**
    Comprehensive window function implementation with optimized single-value
    and buffer processing capabilities.

    Features:
    - Single sample window value calculation
    - In-place and out-of-place buffer windowing
    - Enum-based and method-based APIs
    - All standard window functions for audio DSP
    - Optimized implementations with minimal overhead

    Usage Examples:
    @code
    // Single value access
    auto value = WindowFunctions<float>::getValue(WindowType::hann, 128, 64);

    // Generate window buffer
    std::vector<float> window(512);
    WindowFunctions<float>::generateWindow(WindowType::kaiser, window, 8.0f);

    // Apply window to signal (in-place)
    WindowFunctions<float>::applyWindow(WindowType::blackman, signal);

    // Apply window to signal (out-of-place)
    std::vector<float> windowed(512);
    WindowFunctions<float>::applyWindow(WindowType::hann, signal, windowed);
    @endcode
*/
template <typename FloatType = double>
class WindowFunctions
{
public:
    //==============================================================================
    /**
        Calculates a single window function value.

        @param type      The window type to calculate
        @param n         The sample index (0 to N-1)
        @param N         The window length
        @param parameter Optional parameter for parameterizable windows (Kaiser beta, Gaussian sigma, etc.)
        @returns         The window value at sample n
    */
    static FloatType getValue (WindowType type, int n, int N, FloatType parameter = FloatType (8)) noexcept
    {
        jassert (n >= 0 && n < N && N > 0);

        switch (type)
        {
            case WindowType::rectangular:    return rectangular (n, N);
            case WindowType::hann:          return hann (n, N);
            case WindowType::hamming:       return hamming (n, N);
            case WindowType::blackman:      return blackman (n, N);
            case WindowType::blackmanHarris: return blackmanHarris (n, N);
            case WindowType::kaiser:        return kaiser (n, N, parameter);
            case WindowType::gaussian:      return gaussian (n, N, parameter);
            case WindowType::tukey:         return tukey (n, N, parameter);
            case WindowType::bartlett:      return bartlett (n, N);
            case WindowType::welch:         return welch (n, N);
            case WindowType::flattop:       return flattop (n, N);
            case WindowType::cosine:        return cosine (n, N);
            case WindowType::lanczos:       return lanczos (n, N);
            case WindowType::nuttall:       return nuttall (n, N);
            case WindowType::blackmanNuttall: return blackmanNuttall (n, N);
            default:                        return rectangular (n, N);
        }
    }

    //==============================================================================
    /**
        Generates a complete window function into a buffer.

        @param type      The window type to generate
        @param buffer    The output buffer to fill
        @param parameter Optional parameter for parameterizable windows
    */
    static void generateWindow (WindowType type, std::vector<FloatType>& buffer, FloatType parameter = FloatType (8)) noexcept
    {
        const auto N = static_cast<int> (buffer.size());
        for (int n = 0; n < N; ++n)
            buffer[static_cast<size_t> (n)] = getValue (type, n, N, parameter);
    }

    /**
        Generates a complete window function and returns it as a vector.

        @param type      The window type to generate
        @param length    The window length
        @param parameter Optional parameter for parameterizable windows
        @returns         Vector containing the window values
    */
    static std::vector<FloatType> generateWindow (WindowType type, int length, FloatType parameter = FloatType (8)) noexcept
    {
        std::vector<FloatType> window (static_cast<size_t> (length));
        generateWindow (type, window, parameter);
        return window;
    }

    //==============================================================================
    /**
        Applies a window function to a signal buffer (in-place).

        @param type      The window type to apply
        @param buffer    The signal buffer to window (modified in-place)
        @param parameter Optional parameter for parameterizable windows
    */
    static void applyWindow (WindowType type, std::vector<FloatType>& buffer, FloatType parameter = FloatType (8)) noexcept
    {
        const auto N = static_cast<int> (buffer.size());
        for (int n = 0; n < N; ++n)
        {
            const auto windowValue = getValue (type, n, N, parameter);
            buffer[static_cast<size_t> (n)] *= windowValue;
        }
    }

    /**
        Applies a window function to a signal buffer (out-of-place).

        @param type      The window type to apply
        @param input     The input signal buffer
        @param output    The output windowed buffer
        @param parameter Optional parameter for parameterizable windows
    */
    static void applyWindow (WindowType type, const std::vector<FloatType>& input, std::vector<FloatType>& output, FloatType parameter = FloatType (8)) noexcept
    {
        jassert (input.size() == output.size());

        const auto N = static_cast<int> (input.size());
        for (int n = 0; n < N; ++n)
        {
            const auto windowValue = getValue (type, n, N, parameter);
            output[static_cast<size_t> (n)] = input[static_cast<size_t> (n)] * windowValue;
        }
    }

    /**
        Applies a window function to raw arrays (in-place).

        @param type      The window type to apply
        @param buffer    The signal buffer to window (modified in-place)
        @param length    The buffer length
        @param parameter Optional parameter for parameterizable windows
    */
    static void applyWindow (WindowType type, FloatType* buffer, int length, FloatType parameter = FloatType (8)) noexcept
    {
        jassert (buffer != nullptr && length > 0);

        for (int n = 0; n < length; ++n)
        {
            const auto windowValue = getValue (type, n, length, parameter);
            buffer[n] *= windowValue;
        }
    }

    /**
        Applies a window function to raw arrays (out-of-place).

        @param type      The window type to apply
        @param input     The input signal buffer
        @param output    The output windowed buffer
        @param length    The buffer length
        @param parameter Optional parameter for parameterizable windows
    */
    static void applyWindow (WindowType type, const FloatType* input, FloatType* output, int length, FloatType parameter = FloatType (8)) noexcept
    {
        jassert (input != nullptr && output != nullptr && length > 0);

        for (int n = 0; n < length; ++n)
        {
            const auto windowValue = getValue (type, n, length, parameter);
            output[n] = input[n] * windowValue;
        }
    }

    //==============================================================================
    /** Method-based API for backwards compatibility and direct access */

    static FloatType rectangular (int n, int N) noexcept
    {
        ignoreUnused (n, N);
        return FloatType (1);
    }

    static FloatType hann (int n, int N) noexcept
    {
        return FloatType (0.5) * (FloatType (1) - std::cos (MathConstants<FloatType>::twoPi * n / (N - 1)));
    }

    static FloatType hamming (int n, int N) noexcept
    {
        return FloatType (0.54) - FloatType (0.46) * std::cos (MathConstants<FloatType>::twoPi * n / (N - 1));
    }

    static FloatType blackman (int n, int N) noexcept
    {
        const auto a0 = FloatType (0.42);
        const auto a1 = FloatType (0.5);
        const auto a2 = FloatType (0.08);
        const auto factor = MathConstants<FloatType>::twoPi * n / (N - 1);

        return a0 - a1 * std::cos (factor) + a2 * std::cos (FloatType (2) * factor);
    }

    static FloatType blackmanHarris (int n, int N) noexcept
    {
        const auto a0 = FloatType (0.35875);
        const auto a1 = FloatType (0.48829);
        const auto a2 = FloatType (0.14128);
        const auto a3 = FloatType (0.01168);
        const auto factor = MathConstants<FloatType>::twoPi * n / (N - 1);

        return a0 - a1 * std::cos (factor) + a2 * std::cos (FloatType (2) * factor) - a3 * std::cos (FloatType (3) * factor);
    }

    static FloatType kaiser (int n, int N, FloatType beta) noexcept
    {
        const auto arg = FloatType (2) * n / (N - 1) - FloatType (1);
        const auto x = beta * std::sqrt (FloatType (1) - arg * arg);

        return modifiedBesselI0 (x) / modifiedBesselI0 (beta);
    }

    static FloatType gaussian (int n, int N, FloatType sigma = FloatType (0.4)) noexcept
    {
        const auto arg = (n - (N - 1) / FloatType (2)) / (sigma * (N - 1) / FloatType (2));
        return std::exp (FloatType (-0.5) * arg * arg);
    }

    static FloatType tukey (int n, int N, FloatType alpha = FloatType (0.5)) noexcept
    {
        const auto halfAlphaN = alpha * (N - 1) / FloatType (2);

        if (n < halfAlphaN)
            return FloatType (0.5) * (FloatType (1) + std::cos (MathConstants<FloatType>::pi * (n / halfAlphaN - FloatType (1))));
        else if (n > (N - 1) - halfAlphaN)
            return FloatType (0.5) * (FloatType (1) + std::cos (MathConstants<FloatType>::pi * ((n - (N - 1) + halfAlphaN) / halfAlphaN)));
        else
            return FloatType (1);
    }

    static FloatType bartlett (int n, int N) noexcept
    {
        return FloatType (1) - FloatType (2) * std::abs (n - (N - 1) / FloatType (2)) / (N - 1);
    }

    static FloatType welch (int n, int N) noexcept
    {
        const auto arg = (n - (N - 1) / FloatType (2)) / ((N - 1) / FloatType (2));
        return FloatType (1) - arg * arg;
    }

    static FloatType flattop (int n, int N) noexcept
    {
        const auto a0 = FloatType (0.21557895);
        const auto a1 = FloatType (0.41663158);
        const auto a2 = FloatType (0.277263158);
        const auto a3 = FloatType (0.083578947);
        const auto a4 = FloatType (0.006947368);
        const auto factor = MathConstants<FloatType>::twoPi * n / (N - 1);

        return a0 - a1 * std::cos (factor) + a2 * std::cos (FloatType (2) * factor)
               - a3 * std::cos (FloatType (3) * factor) + a4 * std::cos (FloatType (4) * factor);
    }

    static FloatType cosine (int n, int N) noexcept
    {
        return std::sin (MathConstants<FloatType>::pi * n / (N - 1));
    }

    static FloatType lanczos (int n, int N) noexcept
    {
        const auto x = FloatType (2) * n / (N - 1) - FloatType (1);
        if (std::abs (x) < FloatType (1e-10))
            return FloatType (1);

        const auto px = MathConstants<FloatType>::pi * x;
        return std::sin (px) / px;
    }

    static FloatType nuttall (int n, int N) noexcept
    {
        const auto a0 = FloatType (0.355768);
        const auto a1 = FloatType (0.487396);
        const auto a2 = FloatType (0.144232);
        const auto a3 = FloatType (0.012604);
        const auto factor = MathConstants<FloatType>::twoPi * n / (N - 1);

        return a0 - a1 * std::cos (factor) + a2 * std::cos (FloatType (2) * factor) - a3 * std::cos (FloatType (3) * factor);
    }

    static FloatType blackmanNuttall (int n, int N) noexcept
    {
        const auto a0 = FloatType (0.3635819);
        const auto a1 = FloatType (0.4891775);
        const auto a2 = FloatType (0.1365995);
        const auto a3 = FloatType (0.0106411);
        const auto factor = MathConstants<FloatType>::twoPi * n / (N - 1);

        return a0 - a1 * std::cos (factor) + a2 * std::cos (FloatType (2) * factor) - a3 * std::cos (FloatType (3) * factor);
    }

private:
    //==============================================================================
    /** Modified Bessel function of the first kind, order 0 */
    static FloatType modifiedBesselI0 (FloatType x) noexcept
    {
        auto result = FloatType (1);
        auto term = FloatType (1);

        for (int k = 1; k < 25; ++k)
        {
            term *= (x / (FloatType (2) * k)) * (x / (FloatType (2) * k));
            result += term;

            if (term < result * FloatType (1e-12))
                break;
        }

        return result;
    }
};

//==============================================================================
/** Type aliases for convenience */
using WindowFunctionsFloat = WindowFunctions<float>;
using WindowFunctionsDouble = WindowFunctions<double>;

} // namespace yup