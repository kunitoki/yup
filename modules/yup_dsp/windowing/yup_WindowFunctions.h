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
    rectangular,     /**< Rectangular (no windowing) */
    hann,            /**< Hann window (raised cosine) */
    hamming,         /**< Hamming window */
    blackman,        /**< Blackman window */
    blackmanHarris,  /**< Blackman-Harris window (4-term) */
    kaiser,          /**< Kaiser window (parameterizable) */
    gaussian,        /**< Gaussian window */
    tukey,           /**< Tukey window (tapered cosine) */
    bartlett,        /**< Bartlett window (triangular) */
    welch,           /**< Welch window (parabolic) */
    flattop,         /**< Flat-top window */
    cosine,          /**< Cosine window */
    lanczos,         /**< Lanczos window (sinc) */
    nuttall,         /**< Nuttall window */
    blackmanNuttall, /**< Blackman-Nuttall window */
    rakshitUllah     /**< Rakshit-Ullah adjustable window (novel) */
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
    auto value = WindowFunctions<float>::getValue(WindowType::hann, 64, 128);

    // Generate window buffer
    std::vector<float> window(512);
    WindowFunctions<float>::generate(WindowType::kaiser, window, 8.0f);

    // Apply window to signal (in-place)
    WindowFunctions<float>::apply(WindowType::blackman, signal.begin(), signal.end());

    // Apply window to signal (out-of-place)
    std::vector<float> windowed(512);
    WindowFunctions<float>::apply(WindowType::hann, signal.data(), windowed.data(), windowed.size());
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
            case WindowType::rectangular:
                return rectangular (n, N);
            case WindowType::hann:
                return hann (n, N);
            case WindowType::hamming:
                return hamming (n, N);
            case WindowType::blackman:
                return blackman (n, N);
            case WindowType::blackmanHarris:
                return blackmanHarris (n, N);
            case WindowType::kaiser:
                return kaiser (n, N, parameter);
            case WindowType::gaussian:
                return gaussian (n, N, parameter);
            case WindowType::tukey:
                return tukey (n, N, parameter);
            case WindowType::bartlett:
                return bartlett (n, N);
            case WindowType::welch:
                return welch (n, N);
            case WindowType::flattop:
                return flattop (n, N);
            case WindowType::cosine:
                return cosine (n, N);
            case WindowType::lanczos:
                return lanczos (n, N);
            case WindowType::nuttall:
                return nuttall (n, N);
            case WindowType::blackmanNuttall:
                return blackmanNuttall (n, N);
            case WindowType::rakshitUllah:
                return rakshitUllah (n, N, parameter);
            default:
                return rectangular (n, N);
        }
    }

    //==============================================================================
    /**
        Generates a complete window function into a buffer.

        @param type      The window type to generate
        @param output    The output buffer to fill
        @param parameter Optional parameter for parameterizable windows
    */
    static void generate (WindowType type, Span<FloatType> output, FloatType parameter = FloatType (8)) noexcept
    {
        const auto N = static_cast<int> (output.size());

        for (int n = 0; n < N; ++n)
            output[static_cast<std::size_t> (n)] = getValue (type, n, N, parameter);
    }

    /**
        Generates a complete window function into a buffer.

        @param type      The window type to generate
        @param output    The output buffer to fill
        @param parameter Optional parameter for parameterizable windows
    */
    static void generate (WindowType type, FloatType* output, std::size_t length, FloatType parameter = FloatType (8)) noexcept
    {
        const auto N = static_cast<int> (length);

        for (int n = 0; n < N; ++n)
            *output++ = getValue (type, n, N, parameter);
    }

    //==============================================================================
    /**
        Applies a window function to a signal buffer (in-place).

        @param type      The window type to apply
        @param buffer    The signal buffer to window (modified in-place)
        @param parameter Optional parameter for parameterizable windows
    */
    static void apply (WindowType type, Span<FloatType> input, FloatType param = FloatType (8))
    {
        const int N = static_cast<int> (input.size());

        FloatType* inputData = input.data();

        for (int n = 0; n < N; ++n)
            *inputData++ *= getValue (type, n, N, param);
    }

    /**
        Applies a window function to raw arrays (out-of-place).

        @param type      The window type to apply
        @param input     The input signal buffer
        @param output    The output windowed buffer
        @param length    The buffer length
        @param parameter Optional parameter for parameterizable windows
    */
    static void apply (WindowType type, Span<const FloatType> input, Span<FloatType> output, FloatType parameter = FloatType (8)) noexcept
    {
        jassert (input.size() == output.size());

        const int N = static_cast<int> (jmin (input.size(), output.size()));

        const FloatType* inputData = input.data();
        FloatType* outputData = output.data();

        for (int n = 0; n < N; ++n)
            *outputData++ = *inputData++ * getValue (type, n, N, parameter);
    }

    /**
        Applies a window function to raw arrays (out-of-place).

        @param type      The window type to apply
        @param input     The input signal buffer
        @param output    The output windowed buffer
        @param length    The buffer length
        @param parameter Optional parameter for parameterizable windows
    */
    static void apply (WindowType type, const FloatType* input, FloatType* output, std::size_t length, FloatType parameter = FloatType (8)) noexcept
    {
        jassert (input != nullptr && output != nullptr);

        const int N = static_cast<int> (length);

        for (int n = 0; n < N && input != nullptr && output != nullptr; ++n)
            *output++ = *input++ * getValue (type, n, N, parameter);
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

    /**
        Rakshit-Ullah adjustable window function.

        A novel adjustable window combining hyperbolic tangent and weighted cosine functions.
        Proposed by Hrishi Rakshit and Muhammad Ahsan Ullah (2015).

        @param n Sample index (0 to N-1)
        @param N Window length
        @param r Controlling parameter (default 1.0). Higher values give better side-lobe roll-off.
               Common values: 0.0005, 1.18, 1.618, 30, 75
        @return Window value at sample n

        @note Reference: "FIR Filter Design Using An Adjustable Novel Window and Its Applications"
              International Journal of Engineering and Technology (IJET), 2015
    */
    static FloatType rakshitUllah (int n, int N, FloatType r = FloatType (1)) noexcept
    {
        if (N <= 1)
            return FloatType (1);

        // Constants from the paper
        constexpr auto alpha = FloatType (2);
        constexpr auto B = FloatType (2);

        // Hyperbolic tangent component (y1)
        const auto center = (N - 1) / FloatType (2);
        const auto coshAlpha = std::cosh (alpha);
        const auto coshAlphaSquared = coshAlpha * coshAlpha;

        const auto arg1 = (n - center + coshAlphaSquared) / B;
        const auto arg2 = (n - center - coshAlphaSquared) / B;

        const auto y1 = std::tanh (arg1) - std::tanh (arg2);

        // Weighted cosine component (y2)
        const auto factor = MathConstants<FloatType>::twoPi * n / (N - 1);
        const auto y2 = FloatType (0.375) - FloatType (0.5) * std::cos (factor)
                      + FloatType (0.125) * std::cos (FloatType (2) * factor);

        // Combined window with power parameter
        const auto window = y1 * y2;

        // Apply the controlling parameter r
        if (approximatelyEqual (r, FloatType (1)))
            return window;
        else
            return std::pow (std::abs (window), r) * (window >= FloatType (0) ? FloatType (1) : FloatType (-1));
    }

private:
    //==============================================================================
    /** Modified Bessel function of the first kind, order 0 */
    static constexpr FloatType modifiedBesselI0 (FloatType x) noexcept
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
