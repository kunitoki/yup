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
/**
    IIR antiderivative antialiaser for static nonlinear functions.

    Implements the AA-IIR method from the paper "Arbitrary-Order IIR Antiderivative
    Antialiasing" (La Pastina, D'Angelo, Gabrielli, DAFx 2021). The method discretizes
    a nonlinear static function using a fictitious continuous-time domain with an IIR
    antialiasing filter, yielding substantially better aliasing reduction than simple
    oversampling or rectangular-kernel AA-FIR at comparable cost.

    The IIR filter is specified via its Laplace-domain partial fraction decomposition
    into complex conjugate pole pairs and real poles. Each pole contributes one
    recursive state variable updated per sample according to closed-form expressions
    derived from the chosen NonlinearTraits type.

    The algorithm introduces one sample of inherent latency.

    @tparam SampleType      Audio sample type (float or double).
    @tparam NonlinearTraits Policy class providing the nonlinear function f(x) and
                            its closed-form integral computations.
    @tparam CoeffType       Internal arithmetic precision (defaults to double).
*/
template <typename SampleType, typename NonlinearTraits, typename CoeffType = double>
class AaIirAntialiaser
{
public:
    //==============================================================================
    /**
        Laplace-domain pole configuration for the AA-IIR filter kernel.

        Poles must all lie in the left half-plane (poleReal < 0, Re(β) < 0)
        to ensure filter stability. Complex poles must be specified as
        conjugate pairs; the conjugate is added automatically.
    */
    struct PoleConfig
    {
        /** A complex conjugate pole pair with its partial-fraction residue. */
        struct ComplexPair
        {
            CoeffType poleReal;    ///< Real part of pole (must be < 0)
            CoeffType poleImag;    ///< Imaginary part of pole (positive half)
            CoeffType residueReal; ///< Re(B), residue from partial fractions
            CoeffType residueImag; ///< Im(B), residue from partial fractions
        };

        /** A simple real pole with its partial-fraction residue. */
        struct RealPole
        {
            CoeffType pole;    ///< Pole location (must be < 0)
            CoeffType residue; ///< Residue A from partial fractions
        };

        std::vector<ComplexPair> complexPairs;
        std::vector<RealPole> realPoles;

        /** Direct (Dirac delta) term A0; contributes A0 * f(x) to the output.
            Zero for proper low-pass filters. */
        CoeffType constantTerm = CoeffType (0);
    };

    //==============================================================================
    /** Constructs the processor with the given pole configuration.

        Call prepare() before processing audio.

        @param config  Pole configuration. Defaults to a 2nd-order Butterworth
                       lowpass at 0.45*Fs (the AA-IIR-1 preset from the paper).
    */
    explicit AaIirAntialiaser (PoleConfig config = makeChebyshevTypeIIOrder10()) noexcept
    {
        configure (std::move (config));
    }

    //==============================================================================
    /** Reconfigures the processor with new pole specifications.

        Must be followed by a call to prepare() before processing audio.

        @param config  New pole configuration.
    */
    void configure (PoleConfig config)
    {
        complexPoles.clear();
        for (const auto& pair : config.complexPairs)
        {
            ComplexPoleState s;
            s.poleReal = pair.poleReal;
            s.poleImag = pair.poleImag;
            s.residueReal = pair.residueReal;
            s.residueImag = pair.residueImag;
            complexPoles.push_back (s);
        }

        realPoles.clear();
        for (const auto& rp : config.realPoles)
        {
            RealPoleState s;
            s.pole = rp.pole;
            s.residue = rp.residue;
            realPoles.push_back (s);
        }

        constantTerm = config.constantTerm;
        storedConfig = std::move (config);
    }

    //==============================================================================
    /** Prepares the processor for playback.

        Precomputes per-pole exponentials e^beta (sample-rate independent for
        poles given in normalized rad/sample) and clears all internal state.

        @param sampleRate        Sample rate (unused for normalized poles but
                                 accepted for API consistency).
        @param maximumBlockSize  Maximum expected block size (unused).
    */
    void prepare (double /*sampleRate*/, int /*maximumBlockSize*/) noexcept
    {
        for (auto& p : complexPoles)
        {
            const CoeffType mag = std::exp (p.poleReal);
            p.expReal = mag * std::cos (p.poleImag);
            p.expImag = mag * std::sin (p.poleImag);
        }

        for (auto& p : realPoles)
            p.expAlpha = std::exp (p.pole);

        reset();
    }

    /** Clears all internal filter state.

        Does not affect pole configuration or precomputed exponentials.
    */
    void reset() noexcept
    {
        prevX = CoeffType (0);

        for (auto& p : complexPoles)
        {
            p.stateY = CoeffType (0);
            p.stateZ = CoeffType (0);
        }

        for (auto& p : realPoles)
            p.state = CoeffType (0);
    }

    //==============================================================================
    /** Processes a single input sample through the AA-IIR nonlinearity.

        Implements the per-sample update from the paper (Eq. 20-21 for complex
        poles, Eq. 9 for real poles). The output at step n uses the previous
        input x_{n-1} and the current input x_n to compute the antialiased
        nonlinear output y_n.

        @param inputSample  The current input sample.
        @returns            The antialiased nonlinear output.
    */
    SampleType processSample (SampleType inputSample) noexcept
    {
        const CoeffType x0 = prevX;
        const CoeffType x1 = static_cast<CoeffType> (inputSample);
        prevX = x1;

        CoeffType output = constantTerm != CoeffType (0)
                             ? constantTerm * NonlinearTraits::f (x0)
                             : CoeffType (0);

        for (auto& p : complexPoles)
        {
            const auto [IR, II] = computeComplexIntegral (x0, x1, p.poleReal, p.poleImag);

            const CoeffType newY = p.expReal * p.stateY - p.expImag * p.stateZ
                                 + CoeffType (2) * (p.residueReal * IR - p.residueImag * II);
            const CoeffType newZ = p.expImag * p.stateY + p.expReal * p.stateZ
                                 + CoeffType (2) * (p.residueImag * IR + p.residueReal * II);

            p.stateY = newY;
            p.stateZ = newZ;
            output += newY;
        }

        for (auto& p : realPoles)
        {
            const CoeffType I = computeRealIntegral (x0, x1, p.pole);
            const CoeffType newY = p.expAlpha * p.state + p.residue * I;
            p.state = newY;
            output += newY;
        }

        return static_cast<SampleType> (output);
    }

    /** Processes a block of samples.

        @param inputBuffer   Pointer to the input samples.
        @param outputBuffer  Pointer to the output buffer (may alias inputBuffer).
        @param numSamples    Number of samples to process.
    */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /** Processes a block of samples in-place.

        @param buffer      Pointer to the buffer to process.
        @param numSamples  Number of samples to process.
    */
    void processInPlace (SampleType* buffer, int numSamples) noexcept
    {
        processBlock (buffer, buffer, numSamples);
    }

    //==============================================================================
    /** Creates a pole configuration for a 2nd-order Butterworth lowpass antialiasing filter.

        This is the AA-IIR-1 configuration from the paper, with cutoff at 0.45*Fs.
        The poles are at beta = omega_c * (-1 + j) / sqrt(2) in rad/sample (Fs = 1
        normalisation), with residue B = -j * omega_c / sqrt(2).

        @param cutoffNormalized  Filter cutoff as a fraction of the sample rate
                                 (0 < cutoffNormalized < 0.5). Default: 0.45.
    */
    static PoleConfig makeButterworthOrder2 (CoeffType cutoffNormalized = CoeffType (0.45))
    {
        // 2nd-order Butterworth lowpass: H(s) = wc^2 / (s^2 + sqrt(2)*wc*s + wc^2)
        // Poles: beta = wc * (-1 +/- j) / sqrt(2)
        // Residue: B = wc^2 / (beta - conj(beta)) = -j * wc / sqrt(2)
        const CoeffType wc = CoeffType (2) * CoeffType (MathConstants<CoeffType>::pi) * cutoffNormalized;
        const CoeffType sq = std::sqrt (CoeffType (2));

        typename PoleConfig::ComplexPair pair;
        pair.poleReal = -wc / sq;
        pair.poleImag = +wc / sq;
        pair.residueReal = CoeffType (0);
        pair.residueImag = -wc / sq;

        PoleConfig config;
        config.complexPairs.push_back (pair);
        return config;
    }

    /** Creates a pole configuration for a 10th-order Chebyshev Type II lowpass antialiasing filter.

        This is the AA-IIR-2 configuration from the paper, with stopband edge at 0.61*Fs
        and 60 dB stopband attenuation by default.

        Chebyshev Type II has a flat passband and equiripple stopband.  The filter is
        biproper (numerator degree = denominator degree = 10), so the partial-fraction
        decomposition includes a non-zero constant term A₀ in addition to the five
        complex conjugate pole pairs.

        Design steps (all computed in double precision):
          1. ε = 1 / sqrt(10^(As/10) − 1)  from the stopband attenuation As.
          2. β = asinh(1/ε) / N  (Chebyshev I parameter).
          3. Chebyshev I prototype poles (upper half-plane, unit cutoff):
                p̄ₖ = −sinh(β)·sin(θₖ) + j·cosh(β)·cos(θₖ),  θₖ = π(2k−1)/(2N).
          4. Chebyshev II poles (upper half-plane, scaled to stopband ωs):
                βₖ = ωs / conj(p̄ₖ).
          5. Transmission zeros (imaginary axis): zₖ = ωs / cos(θₖ).
          6. DC-gain constant: K = ∏|βₖ|² / ∏zₖ²  so that H(0) = 1.
          7. Residues via the partial-fraction formula.
          8. Constant term A₀ = K  (biproper limit H(∞) = K).

        @param stopbandNormalized     Stopband edge as a fraction of sample rate (default 0.61).
        @param stopbandAttenuationdB  Stopband attenuation in dB (default 60).
    */
    static PoleConfig makeChebyshevTypeIIOrder10 (CoeffType stopbandNormalized = CoeffType (0.61),
                                                  CoeffType stopbandAttenuationdB = CoeffType (60))
    {
        using C = std::complex<double>;
        constexpr int N = 10;
        constexpr int Nh = N / 2; // number of conjugate pairs

        const double ws = 2.0 * MathConstants<double>::pi * static_cast<double> (stopbandNormalized);
        const double As = static_cast<double> (stopbandAttenuationdB);

        // Chebyshev I parameter
        const double eps = 1.0 / std::sqrt (std::pow (10.0, As / 10.0) - 1.0);
        const double beta = std::asinh (1.0 / eps) / N;

        std::array<C, Nh> poles;
        std::array<double, Nh> zeros;

        for (int k = 0; k < Nh; ++k)
        {
            const double theta = MathConstants<double>::pi * (2 * (k + 1) - 1) / (2 * N);

            // Chebyshev I upper half-plane prototype pole (unit cutoff)
            const C p1 (-std::sinh (beta) * std::sin (theta),
                        std::cosh (beta) * std::cos (theta));

            // Chebyshev II upper half-plane pole: ωs / conj(p1)
            poles[k] = C (ws) / std::conj (p1);

            // Transmission zero frequency (positive imaginary axis): ωs / cos(θ)
            zeros[k] = ws / std::cos (theta);
        }

        // DC-gain normalisation: K = ∏|βₖ|² / ∏zₖ²  →  H(0) = 1.
        double K = 1.0;
        for (int k = 0; k < Nh; ++k)
        {
            K *= std::norm (poles[k]); // |βₖ|²
            K /= zeros[k] * zeros[k];  //  zₖ²
        }

        // Residue at each upper half-plane pole βₖ:
        //   Bₖ = K · N(βₖ) / [(βₖ − β̄ₖ) · ∏_{j≠k}(βₖ−βⱼ)(βₖ−β̄ⱼ)]
        // where N(s) = ∏ⱼ(s² + zⱼ²)  is the numerator polynomial.
        std::array<C, Nh> residues;
        for (int k = 0; k < Nh; ++k)
        {
            const C s = poles[k];

            // Numerator N(s) = K · ∏ⱼ(s² + zⱼ²)
            C num (K);
            for (int j = 0; j < Nh; ++j)
                num *= s * s + C (zeros[j] * zeros[j]);

            // Denominator derivative at s: (s − s̄) · ∏_{j≠k}(s−βⱼ)(s−β̄ⱼ)
            C den = s - std::conj (s); // = 2j · Im(βₖ)
            for (int j = 0; j < Nh; ++j)
                if (j != k)
                    den *= (s - poles[j]) * (s - std::conj (poles[j]));

            residues[k] = num / den;
        }

        PoleConfig config;
        for (int k = 0; k < Nh; ++k)
        {
            typename PoleConfig::ComplexPair pair;
            pair.poleReal = static_cast<CoeffType> (poles[k].real());
            pair.poleImag = static_cast<CoeffType> (poles[k].imag()); // > 0 by construction
            pair.residueReal = static_cast<CoeffType> (residues[k].real());
            pair.residueImag = static_cast<CoeffType> (residues[k].imag());
            config.complexPairs.push_back (pair);
        }

        // A₀ = K: biproper limit H(∞) = K (numerator and denominator share degree N).
        config.constantTerm = static_cast<CoeffType> (K);
        return config;
    }

private:
    //==============================================================================
    // Detection helper: true when NonlinearTraits provides fillBreakpoints<T>.
    // Traits that satisfy this get piecewise-accurate integration (needed for
    // hard clipping). Traits that do not (smooth functions like tanh) use a
    // single affine segment over [x0, x1], which is exact for smooth nonlinearities.
    template <typename Tr, typename T, typename = void>
    struct BreakpointHelper : std::false_type
    {
        static void fill (T, T, T*, int&) noexcept {}
    };

    template <typename Tr, typename T>
    struct BreakpointHelper<Tr, T, std::void_t<decltype (Tr::template fillBreakpoints<T> (T {}, T {}, (T*) nullptr, std::declval<int&>()))>>
        : std::true_type
    {
        static void fill (T x0, T delta, T* pts, int& nPts) noexcept
        {
            Tr::template fillBreakpoints<T> (x0, delta, pts, nPts);
        }
    };

    //==============================================================================
    /** Computes the mean definite integral used by the AA-IIR method for a complex pole.

        The paper uses the mean integral (eq. 3): ⁻∫_a^b = (1/(b-a)) * ∫_a^b.
        Returns the real and imaginary parts of:

            I_mean = (1/delta) * integral_{x0}^{x1} f(xi) * exp(beta*(1-(xi-x0)/delta)) dxi

        where delta = x1-x0, beta = poleReal + j*poleImag, and f = NonlinearTraits::f.

        Using u = (xi-x0)/delta ∈ [0,1], the mean integral per affine segment is:

            I_seg = (1/beta) * (exp_a*(fA + slopeU/beta) - exp_b*(fB + slopeU/beta))

        where slopeU = (fB-fA)/(ub-ua) is the slope of f in u-space (independent of delta).
        This formula has the correct finite limit as delta → 0: f(x0) * (exp(beta)-1)/beta.
    */
    template <typename T>
    static std::pair<T, T> computeComplexIntegral (T x0, T x1, T poleReal, T poleImag) noexcept
    {
        // 1/beta = conj(beta) / |beta|^2
        const T betaMagSq = poleReal * poleReal + poleImag * poleImag;
        const T ibR = poleReal / betaMagSq;  // Re(1/beta)
        const T ibI = -poleImag / betaMagSq; // Im(1/beta)

        // exp(beta) at u=0
        const T emag = std::exp (poleReal);
        const T e1R = emag * std::cos (poleImag); // Re(exp(beta))
        const T e1I = emag * std::sin (poleImag); // Im(exp(beta))

        const T delta = x1 - x0;

        // Build segment breakpoints in u-space via the traits (if provided).
        // Traits that implement fillBreakpoints<T> (e.g. HardClipperTraits) add
        // their characteristic breakpoints here; smooth nonlinearities don't and
        // the integral collapses to a single affine segment over [x0, x1].
        T pts[4] = { T (0), T (1), T (0), T (0) };
        int nPts = 2;

        if (std::abs (delta) >= T (1e-7))
        {
            BreakpointHelper<NonlinearTraits, T>::fill (x0, delta, pts, nPts);

            for (int i = 1; i < nPts; ++i)
            {
                const T key = pts[i];
                int j = i - 1;
                while (j >= 0 && pts[j] > key)
                {
                    pts[j + 1] = pts[j];
                    --j;
                }
                pts[j + 1] = key;
            }
        }

        T resultR = T (0);
        T resultI = T (0);

        for (int i = 0; i < nPts - 1; ++i)
        {
            const T ua = pts[i];
            const T ub = pts[i + 1];
            const T xiA = x0 + ua * delta;
            const T xiB = x0 + ub * delta;
            const T fA = NonlinearTraits::f (xiA);
            const T fB = NonlinearTraits::f (xiB);

            // Slope in u-space: slopeU = (fB-fA)/(ub-ua) — independent of delta.
            // ub > ua always, so no division by zero here.
            const T slopeU = (fB - fA) / (ub - ua);

            // slopeU / beta (complex)
            const T sR = slopeU * ibR;
            const T sI = slopeU * ibI;

            // exp(beta*(1-ua)) and exp(beta*(1-ub))
            const T magA = std::exp (poleReal * (T (1) - ua));
            const T eaR = magA * std::cos (poleImag * (T (1) - ua));
            const T eaI = magA * std::sin (poleImag * (T (1) - ua));

            const T magB = std::exp (poleReal * (T (1) - ub));
            const T ebR = magB * std::cos (poleImag * (T (1) - ub));
            const T ebI = magB * std::sin (poleImag * (T (1) - ub));

            // e_a*(fA + slopeU/beta) - e_b*(fB + slopeU/beta)
            const T tAR = fA + sR;
            const T tBR = fB + sR;
            const T diffPR = eaR * tAR - eaI * sI - ebR * tBR + ebI * sI;
            const T diffPI = eaR * sI + eaI * tAR - ebR * sI - ebI * tBR;

            // I_seg = (1/beta) * diff
            resultR += ibR * diffPR - ibI * diffPI;
            resultI += ibR * diffPI + ibI * diffPR;
        }

        return { resultR, resultI };
    }

    /** Computes the mean definite integral for a real pole (real arithmetic version).

        See computeComplexIntegral for derivation. Returns:

            I_mean = (1/delta) * integral_{x0}^{x1} f(xi) * exp(pole*(1-(xi-x0)/delta)) dxi
    */
    template <typename T>
    static T computeRealIntegral (T x0, T x1, T pole) noexcept
    {
        const T invPole = T (1) / pole;
        const T delta = x1 - x0;

        T pts[4] = { T (0), T (1), T (0), T (0) };
        int nPts = 2;

        if (std::abs (delta) >= T (1e-7))
        {
            BreakpointHelper<NonlinearTraits, T>::fill (x0, delta, pts, nPts);

            for (int i = 1; i < nPts; ++i)
            {
                const T key = pts[i];
                int j = i - 1;
                while (j >= 0 && pts[j] > key)
                {
                    pts[j + 1] = pts[j];
                    --j;
                }
                pts[j + 1] = key;
            }
        }

        T result = T (0);

        for (int i = 0; i < nPts - 1; ++i)
        {
            const T ua = pts[i];
            const T ub = pts[i + 1];
            const T xiA = x0 + ua * delta;
            const T xiB = x0 + ub * delta;
            const T fA = NonlinearTraits::f (xiA);
            const T fB = NonlinearTraits::f (xiB);
            const T slopeU = (fB - fA) / (ub - ua);
            const T sOverP = slopeU * invPole;

            const T ea = std::exp (pole * (T (1) - ua));
            const T eb = std::exp (pole * (T (1) - ub));

            // I_seg = (1/pole) * (ea*(fA + slopeU/pole) - eb*(fB + slopeU/pole))
            result += invPole * (ea * (fA + sOverP) - eb * (fB + sOverP));
        }

        return result;
    }

    //==============================================================================
    struct ComplexPoleState
    {
        CoeffType poleReal = CoeffType (0);
        CoeffType poleImag = CoeffType (0);
        CoeffType residueReal = CoeffType (0);
        CoeffType residueImag = CoeffType (0);
        CoeffType expReal = CoeffType (0); // Re(e^beta), computed in prepare()
        CoeffType expImag = CoeffType (0); // Im(e^beta), computed in prepare()
        CoeffType stateY = CoeffType (0);  // Re(ŷ_n)
        CoeffType stateZ = CoeffType (0);  // Im(ŷ_n)
    };

    struct RealPoleState
    {
        CoeffType pole = CoeffType (0);
        CoeffType residue = CoeffType (0);
        CoeffType expAlpha = CoeffType (0); // e^pole, computed in prepare()
        CoeffType state = CoeffType (0);    // y_n
    };

    //==============================================================================
    std::vector<ComplexPoleState> complexPoles;
    std::vector<RealPoleState> realPoles;
    CoeffType constantTerm = CoeffType (0);
    CoeffType prevX = CoeffType (0);
    PoleConfig storedConfig;

    //==============================================================================
    YUP_LEAK_DETECTOR (AaIirAntialiaser)
};

} // namespace yup
