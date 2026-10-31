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

    The traits provide f(x), either as a static function or as a const member function
    of a traits instance the processor holds, so a curve chosen at runtime can be
    antialiased too (see setTraits()). The optional static fillBreakpoints() splits the
    integration where f has a kink; without it f is integrated as one affine segment
    per sample, which is exact for smooth curves.

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

        @param config           Pole configuration. Defaults to a 10th-order Chebyshev
                                Type II lowpass (the AA-IIR-2 preset from the paper).
        @param nonlinearTraits  The traits instance providing f(x).
    */
    explicit AaIirAntialiaser (PoleConfig config = makeChebyshevTypeIIOrder10(), NonlinearTraits nonlinearTraits = {})
        : traits (std::move (nonlinearTraits))
    {
        configure (std::move (config));
        prevF = traits.f (prevX);
    }

    //==============================================================================
    /** Replaces the traits instance providing f(x), keeping the filter state.

        Safe to call between samples: the next sample integrates from the previous
        input through the new curve.
    */
    void setTraits (NonlinearTraits newTraits) noexcept
    {
        traits = std::move (newTraits);
        prevF = traits.f (prevX);
    }

    /** Returns the traits instance providing f(x). */
    const NonlinearTraits& getTraits() const noexcept { return traits; }

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
        // Over a single affine segment the integral is P f(x0) + Q f(x1), with
        // P = (e^b - (e^b - 1) / b) / b and Q = ((e^b - 1) / b - 1) / b.
        for (auto& p : complexPoles)
        {
            using Complex = std::complex<CoeffType>;

            const Complex pole (p.poleReal, p.poleImag);
            const Complex e = std::exp (pole);
            const Complex inv = CoeffType (1) / pole;
            const Complex ramp = (e - CoeffType (1)) * inv;
            const Complex residue = CoeffType (2) * Complex (p.residueReal, p.residueImag);
            const Complex gain0 = residue * (e - ramp) * inv;
            const Complex gain1 = residue * (ramp - CoeffType (1)) * inv;

            p.expReal = e.real();
            p.expImag = e.imag();
            p.invReal = inv.real();
            p.invImag = inv.imag();
            p.gain0Real = gain0.real();
            p.gain0Imag = gain0.imag();
            p.gain1Real = gain1.real();
            p.gain1Imag = gain1.imag();
        }

        for (auto& p : realPoles)
        {
            p.expAlpha = std::exp (p.pole);
            p.invPole = CoeffType (1) / p.pole;

            const CoeffType ramp = (p.expAlpha - CoeffType (1)) * p.invPole;
            p.gain0 = p.residue * (p.expAlpha - ramp) * p.invPole;
            p.gain1 = p.residue * (ramp - CoeffType (1)) * p.invPole;
        }

        reset();
    }

    /** Clears all internal filter state.

        Does not affect pole configuration or precomputed exponentials.
    */
    void reset() noexcept
    {
        prevX = CoeffType (0);
        prevF = traits.f (prevX);

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

        // The segments and the curve values are shared by every pole; f(x0) is last sample's f(x1).
        const auto segments = makeSegments (x0, x1, prevF);
        const bool affine = segments.count == 2;
        const CoeffType f0 = segments.f[0];
        const CoeffType f1 = segments.f[segments.count - 1];
        prevF = f1;

        CoeffType output = constantTerm * f0;

        for (auto& p : complexPoles)
        {
            CoeffType drivenR, drivenI;

            if (affine)
            {
                drivenR = p.gain0Real * f0 + p.gain1Real * f1;
                drivenI = p.gain0Imag * f0 + p.gain1Imag * f1;
            }
            else
            {
                const auto [IR, II] = computeComplexIntegral (segments, p);
                drivenR = CoeffType (2) * (p.residueReal * IR - p.residueImag * II);
                drivenI = CoeffType (2) * (p.residueImag * IR + p.residueReal * II);
            }

            const CoeffType newY = p.expReal * p.stateY - p.expImag * p.stateZ + drivenR;
            const CoeffType newZ = p.expImag * p.stateY + p.expReal * p.stateZ + drivenI;

            p.stateY = newY;
            p.stateZ = newZ;
            output += newY;
        }

        for (auto& p : realPoles)
        {
            const CoeffType driven = affine ? p.gain0 * f0 + p.gain1 * f1
                                            : p.residue * computeRealIntegral (segments, p);
            p.state = p.expAlpha * p.state + driven;
            output += p.state;
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
    struct ComplexPoleState
    {
        CoeffType poleReal = CoeffType (0);
        CoeffType poleImag = CoeffType (0);
        CoeffType residueReal = CoeffType (0);
        CoeffType residueImag = CoeffType (0);
        CoeffType expReal = CoeffType (0); // Re(e^beta), computed in prepare()
        CoeffType expImag = CoeffType (0); // Im(e^beta), computed in prepare()
        CoeffType invReal = CoeffType (0); // Re(1/beta), computed in prepare()
        CoeffType invImag = CoeffType (0); // Im(1/beta), computed in prepare()
        CoeffType gain0Real = CoeffType (0); // 2 B P, weight of f(x0) over a single segment
        CoeffType gain0Imag = CoeffType (0);
        CoeffType gain1Real = CoeffType (0); // 2 B Q, weight of f(x1) over a single segment
        CoeffType gain1Imag = CoeffType (0);
        CoeffType stateY = CoeffType (0);  // Re(ŷ_n)
        CoeffType stateZ = CoeffType (0);  // Im(ŷ_n)
    };

    struct RealPoleState
    {
        CoeffType pole = CoeffType (0);
        CoeffType residue = CoeffType (0);
        CoeffType expAlpha = CoeffType (0); // e^pole, computed in prepare()
        CoeffType invPole = CoeffType (0);  // 1/pole, computed in prepare()
        CoeffType gain0 = CoeffType (0);    // A P, weight of f(x0) over a single segment
        CoeffType gain1 = CoeffType (0);    // A Q, weight of f(x1) over a single segment
        CoeffType state = CoeffType (0);    // y_n
    };

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
    /** The affine segments of f over [x0, x1], in u = (xi - x0) / (x1 - x0) space.

        Holds the breakpoints u[0] = 0 < ... < u[count - 1] = 1 and f at each of them.
        Traits breakpoints only ever fall strictly inside (0, 1).
    */
    struct Segments
    {
        CoeffType u[4] = { CoeffType (0), CoeffType (1), CoeffType (0), CoeffType (0) };
        CoeffType f[4] = {};
        int count = 2;
    };

    Segments makeSegments (CoeffType x0, CoeffType x1, CoeffType f0) const noexcept
    {
        Segments segments;
        segments.f[0] = f0;
        const CoeffType delta = x1 - x0;

        if (std::abs (delta) >= CoeffType (1e-7))
        {
            BreakpointHelper<NonlinearTraits, CoeffType>::fill (x0, delta, segments.u, segments.count);
            std::sort (segments.u, segments.u + segments.count);
        }

        for (int i = 1; i < segments.count; ++i)
            segments.f[i] = traits.f (x0 + segments.u[i] * delta);

        return segments;
    }

    /** Computes the mean definite integral used by the AA-IIR method for a complex pole.

        The paper uses the mean integral (eq. 3): ⁻∫_a^b = (1/(b-a)) * ∫_a^b.
        Returns the real and imaginary parts of:

            I_mean = (1/delta) * integral_{x0}^{x1} f(xi) * exp(beta*(1-(xi-x0)/delta)) dxi

        where delta = x1-x0, beta = poleReal + j*poleImag, and f = NonlinearTraits::f.

        Using u = (xi-x0)/delta ∈ [0,1], the mean integral per affine segment is:

            I_seg = (1/beta) * (exp_a*(fA + slopeU/beta) - exp_b*(fB + slopeU/beta))

        where slopeU = (fB-fA)/(ub-ua) is the slope of f in u-space (independent of delta).
        This formula has the correct finite limit as delta → 0: f(x0) * (exp(beta)-1)/beta.

        exp(beta*(1-u)) is e^beta at u = 0 and 1 at u = 1, both known in advance, so
        only the breakpoints inside the interval cost a complex exponential.
    */
    static std::pair<CoeffType, CoeffType> computeComplexIntegral (const Segments& segments, const ComplexPoleState& p) noexcept
    {
        const auto expAt = [&] (int i) -> std::pair<CoeffType, CoeffType>
        {
            if (i == 0)
                return { p.expReal, p.expImag };

            if (i == segments.count - 1)
                return { CoeffType (1), CoeffType (0) };

            const CoeffType v = CoeffType (1) - segments.u[i];
            const CoeffType mag = std::exp (p.poleReal * v);
            return { mag * std::cos (p.poleImag * v), mag * std::sin (p.poleImag * v) };
        };

        CoeffType resultR = CoeffType (0);
        CoeffType resultI = CoeffType (0);
        auto [eaR, eaI] = expAt (0);

        for (int i = 0; i < segments.count - 1; ++i)
        {
            const auto [ebR, ebI] = expAt (i + 1);
            const CoeffType fA = segments.f[i];
            const CoeffType fB = segments.f[i + 1];

            // Slope in u-space - independent of delta; ub > ua always.
            const CoeffType slopeU = (fB - fA) / (segments.u[i + 1] - segments.u[i]);

            // slopeU / beta (complex)
            const CoeffType sR = slopeU * p.invReal;
            const CoeffType sI = slopeU * p.invImag;

            // e_a*(fA + slopeU/beta) - e_b*(fB + slopeU/beta)
            const CoeffType tAR = fA + sR;
            const CoeffType tBR = fB + sR;
            const CoeffType diffPR = eaR * tAR - eaI * sI - ebR * tBR + ebI * sI;
            const CoeffType diffPI = eaR * sI + eaI * tAR - ebR * sI - ebI * tBR;

            // I_seg = (1/beta) * diff
            resultR += p.invReal * diffPR - p.invImag * diffPI;
            resultI += p.invReal * diffPI + p.invImag * diffPR;

            eaR = ebR;
            eaI = ebI;
        }

        return { resultR, resultI };
    }

    /** Computes the mean definite integral for a real pole (real arithmetic version).

        See computeComplexIntegral for derivation. Returns:

            I_mean = (1/delta) * integral_{x0}^{x1} f(xi) * exp(pole*(1-(xi-x0)/delta)) dxi
    */
    static CoeffType computeRealIntegral (const Segments& segments, const RealPoleState& p) noexcept
    {
        const auto expAt = [&] (int i) -> CoeffType
        {
            if (i == 0)
                return p.expAlpha;

            if (i == segments.count - 1)
                return CoeffType (1);

            return std::exp (p.pole * (CoeffType (1) - segments.u[i]));
        };

        CoeffType result = CoeffType (0);
        CoeffType ea = expAt (0);

        for (int i = 0; i < segments.count - 1; ++i)
        {
            const CoeffType eb = expAt (i + 1);
            const CoeffType fA = segments.f[i];
            const CoeffType fB = segments.f[i + 1];
            const CoeffType sOverP = (fB - fA) / (segments.u[i + 1] - segments.u[i]) * p.invPole;

            // I_seg = (1/pole) * (ea*(fA + slopeU/pole) - eb*(fB + slopeU/pole))
            result += p.invPole * (ea * (fA + sOverP) - eb * (fB + sOverP));
            ea = eb;
        }

        return result;
    }

    //==============================================================================
    std::vector<ComplexPoleState> complexPoles;
    std::vector<RealPoleState> realPoles;
    CoeffType constantTerm = CoeffType (0);
    CoeffType prevX = CoeffType (0);
    CoeffType prevF = CoeffType (0);
    PoleConfig storedConfig;
    NonlinearTraits traits;

    //==============================================================================
    YUP_LEAK_DETECTOR (AaIirAntialiaser)
};

} // namespace yup
