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
/** Spectral shaper placing ridges, dispersion and pulse width on a Fourier series.

    Every stage scales or rotates a harmonic that the source already contains, so no
    stage can produce a frequency the source did not have. That is the rule any further
    stage has to obey to stay alias-free: c[h] may be multiplied by anything, but a
    harmonic's frequency is fixed at h times the fundamental and cannot be moved - a
    stretched or inharmonic partial is not expressible in a periodic series at all. Rendering the result with
    a bandlimited backend therefore keeps the source's antialiasing guarantee, and
    the shape parameters can be swept without an antialiasing strategy of their own.

    Applied to each harmonic h, writing u = log2 (h) and c for the harmonic's complex
    coefficient (cosine + i sine):

    1. Ridge - a raised-cosine comb in u, periodic with ridgeSpacing octaves and slid
       along by color: gain = 0.15 + 0.85 * ridge^2 with
       ridge = 0.5 + 0.5 * cos (2 pi (u / ridgeSpacing - color)).
    1b. Tilt - an overall slope, gain is multiplied by 2^(-tilt * u), which the ridge
       comb cannot express because it is periodic in u rather than monotonic in it.
    1c. Odd/even - odd and even harmonics scaled against each other, the cheapest
       stage here and the one that walks between sawtooth-like and square-like.
    1d. Formant - a single movable resonance, gain multiplied by
       2^(formant * exp (-((u - formantPosition) / formantWidth)^2)). Where the ridge
       is a periodic comb this is one peak or notch, so the two stack into vowel-like
       shapes rather than duplicating each other.
    2. Squash - magnitude companding, |c| becomes |c|^squash with the phase kept.
       It follows the four gain stages so that it compands the shaped spectrum.
    3. Squeeze - exact pulse-width modulation, c is multiplied by 1 - d * cis (-2 pi h w).
       Subtracting a phase-shifted copy of a bandlimited signal stays bandlimited, so
       this is PWM without the usual aliasing. The depth d opens from 0 to 1 over the
       first squeezeFadeWidth of the width, which is what makes a width of zero a true
       bypass: the raw factor tends to i * h * 2 pi w as w falls, and renormalizing that
       leaves a differentiator rather than the original spectrum, so fading the depth is
       the only way the control can pass continuously through zero.
    4. Dispersion and scatter - one rotation carrying both angles, quadratic in u for
       dispersion plus a fixed pseudo-random offset per harmonic for scatter. Neither
       changes any magnitude, so they recolour the waveform and its transient without
       touching the timbre's brightness; dispersion sweeps like a chirp where scatter
       diffuses. Sharing a rotation makes scatter free on top of dispersion.
    5. Normalize - the whole series is scaled so that sum |c| matches the source's.

    The normalization is what keeps the shape parameters level-safe: the ridge gain,
    the companding and the pulse-width factor (whose magnitude reaches 2) all change
    level, and sum |c| bounds the waveform's peak, so preserving it means the output
    can never exceed the source's worst case. The sum runs over harmonics only. DC is
    not carried across; the destination's DC is always zero.

    Shaping is O (numHarmonics) with a handful of transcendentals per harmonic and no
    transform, so it is cheap enough to re-run once per audio block, which is what makes
    the shape controls modulatable. Feed the result to AdditiveOscillator to hear it
    immediately, or to WavetableOscillator / WaveformBank to trade one inverse FFT per
    update for a much cheaper per-sample cost - the usual choice between the two
    backends in this folder, unchanged by the shaping in front of them.

    The class holds no DSP state and the shape is a property of a patch rather than of
    a note, so shape once and let every voice play the result: doing it per voice is the
    one thing that makes it expensive.

    @tparam CoeffType  Precision of the series being shaped.

    @see FourierSeries, AdditiveOscillator, WavetableOscillator
*/
template <typename CoeffType = double>
class PrismSpectrum
{
public:
    //==============================================================================
    /** The shape controls, all independent of the ridge offset (color).

        tilt, squeeze, squash, formant and scatter each have a true bypass value and are
        continuous through it, so they can be modulated without a step there; oddEven is
        continuous through its neutral 0.5. The ridge has no bypass - its gain is always
        applied, and ridgeSpacing merely widens the comb until it is nearly flat over the
        source's range - so ridgeSpacing and color are shape rather than amount.
    */
    struct Shape
    {
        double ridgeSpacing = 1.0;    /**< Ridge period in octaves, clamped to [0.25, 8]. */
        double dispersion = 0.5;      /**< Quadratic phase in log2 space; 0.5 is flat. */
        double squeeze = 0.0;         /**< Pulse width in periods, clamped to [0, 0.5]; 0 is a true bypass. */
        double squash = 1.0;          /**< Magnitude exponent, clamped to [0.1, 4]; 1 is bypass. */
        double tilt = 0.0;            /**< Spectral slope, gain octaves per harmonic octave, clamped to +/-4; 0 is flat. */
        double oddEven = 0.5;         /**< Odd/even balance, clamped to [0, 1]; 0 keeps only odd harmonics, 1 only even, 0.5 is neutral. */
        double formant = 0.0;         /**< Signed resonance depth in gain octaves, clamped to +/-4; 0 is bypass. */
        double formantPosition = 2.0; /**< Resonance center in harmonic octaves (log2 of the harmonic index), clamped to [0, 12]. */
        double scatter = 0.0;         /**< Pseudo-random phase spread in periods, clamped to [0, 1]; 0 is bypass. */
    };

    /** Width over which the pulse-width stage fades in, so that zero bypasses it. */
    static constexpr double squeezeFadeWidth = 0.05;

    /** Half-width of the formant resonance, in harmonic octaves. */
    static constexpr double formantWidth = 0.75;

    //==============================================================================
    /** Precomputes the per-harmonic log2 tables the ridge and dispersion stages read.

        Allocates, and must be called before any shaping. Harmonics above the prepared
        count are ignored rather than shaped, so size this to the largest source.

        @param maxHarmonics  Largest harmonic index the shaper will handle.
    */
    void prepare (int maxHarmonics)
    {
        jassert (maxHarmonics >= 0);

        const auto count = static_cast<std::size_t> (jmax (0, maxHarmonics));

        logHarmonic.resize (count);
        logHarmonicSquared.resize (count);
        scatterAngle.resize (count);

        // A fixed generator rather than a shared one, so a given harmonic always gets
        // the same offset: scatter has to be a stable property of the shape, otherwise
        // re-deriving it every block would sound like noise rather than like a timbre.
        uint32 state = 0x9e3779b9u;

        for (std::size_t index = 0; index < count; ++index)
        {
            const auto u = std::log2 (static_cast<double> (index + 1));

            logHarmonic[index] = u;
            logHarmonicSquared[index] = u * u;

            state = state * 1664525u + 1013904223u;
            scatterAngle[index] = MathConstants<double>::twoPi * (static_cast<double> (state >> 8) / 16777216.0);
        }
    }

    /** Returns the largest harmonic index prepare() sized the tables for. */
    int getMaxHarmonics() const noexcept { return static_cast<int> (logHarmonic.size()); }

    //==============================================================================
    /** Shapes a source series into a destination series at one ridge offset.

        The destination is cleared first, so harmonics the source does not reach stay
        silent and the antialiasing guarantee survives a destination larger than the
        source. Harmonics beyond either series or beyond prepare()'s count are dropped.
        The two series may not alias each other.

        @param source       Series to shape; left untouched.
        @param destination  Series to write; cleared and rewritten.
        @param shape        Ridge spacing, dispersion, pulse width and companding.
        @param color        Ridge offset in ridge periods. Periodic with period 1.
    */
    void process (const FourierSeries<CoeffType>& source,
                  FourierSeries<CoeffType>& destination,
                  const Shape& shape,
                  double color) const noexcept
    {
        jassert (&source != &destination);

        destination.clear();

        const auto limit = jmin (source.getNumHarmonics(), destination.getNumHarmonics(), getMaxHarmonics());
        if (limit <= 0)
            return;

        const auto ridgeScale = MathConstants<double>::twoPi / jlimit (0.25, 8.0, shape.ridgeSpacing);
        const auto ridgeOffset = MathConstants<double>::twoPi * color;
        const auto width = jlimit (0.0, 0.5, shape.squeeze);
        const auto depth = jmin (1.0, width / squeezeFadeWidth);
        const auto exponent = jlimit (0.1, 4.0, shape.squash);
        const auto curvature = jlimit (0.0, 1.0, shape.dispersion) - 0.5;
        const auto slope = jlimit (-4.0, 4.0, shape.tilt);
        const auto balance = jlimit (0.0, 1.0, shape.oddEven);
        const auto oddGain = jmin (1.0, 2.0 - 2.0 * balance);
        const auto evenGain = jmin (1.0, 2.0 * balance);
        const auto resonance = jlimit (-4.0, 4.0, shape.formant);
        const auto center = jlimit (0.0, 12.0, shape.formantPosition);
        const auto spread = jlimit (0.0, 1.0, shape.scatter);

        auto sourceSum = 0.0;
        auto shapedSum = 0.0;

        for (int harmonic = 1; harmonic <= limit; ++harmonic)
        {
            const auto index = static_cast<std::size_t> (harmonic - 1);

            auto real = static_cast<double> (source.getCosine (harmonic));
            auto imaginary = static_cast<double> (source.getSine (harmonic));

            sourceSum += std::hypot (real, imaginary);

            const auto u = logHarmonic[index];
            const auto ridge = 0.5 + 0.5 * std::cos (ridgeScale * u - ridgeOffset);

            auto gain = 0.15 + 0.85 * ridge * ridge;

            if (slope != 0.0)
                gain *= std::exp2 (-slope * u);

            gain *= (harmonic % 2 == 1) ? oddGain : evenGain;

            if (resonance != 0.0)
            {
                const auto offset = (u - center) / formantWidth;

                gain *= std::exp2 (resonance * std::exp (-offset * offset));
            }

            real *= gain;
            imaginary *= gain;

            if (exponent != 1.0)
            {
                const auto magnitude = std::hypot (real, imaginary);

                if (magnitude > 0.0)
                {
                    const auto companded = std::pow (magnitude, exponent);

                    real = real / magnitude * companded;
                    imaginary = imaginary / magnitude * companded;
                }
            }

            if (depth > 0.0)
            {
                const auto angle = MathConstants<double>::twoPi * std::fmod (static_cast<double> (harmonic) * width, 1.0);

                rotate (real, imaginary, 1.0 - depth * std::cos (angle), depth * std::sin (angle));
            }

            // Both remaining stages are rotations, so their angles add and one sincos
            // serves both: scatter costs nothing on top of dispersion.
            if (curvature != 0.0 || spread > 0.0)
            {
                const auto angle = curvature * logHarmonicSquared[index] + spread * scatterAngle[index];

                rotate (real, imaginary, std::cos (angle), std::sin (angle));
            }

            shapedSum += std::hypot (real, imaginary);

            destination.setHarmonic (harmonic, static_cast<CoeffType> (real), static_cast<CoeffType> (imaginary));
        }

        if (shapedSum <= 0.0 || sourceSum <= 0.0 || shapedSum == sourceSum)
            return;

        const auto normalization = static_cast<CoeffType> (sourceSum / shapedSum);

        for (int harmonic = 1; harmonic <= limit; ++harmonic)
            destination.setHarmonic (harmonic,
                                     destination.getCosine (harmonic) * normalization,
                                     destination.getSine (harmonic) * normalization);
    }

private:
    //==============================================================================
    /** Multiplies the complex coefficient in place by cosine + i sine. */
    static void rotate (double& real, double& imaginary, double cosine, double sine) noexcept
    {
        const auto rotated = real * cosine - imaginary * sine;

        imaginary = real * sine + imaginary * cosine;
        real = rotated;
    }

    //==============================================================================
    std::vector<double> logHarmonic;
    std::vector<double> logHarmonicSquared;
    std::vector<double> scatterAngle;
};

} // namespace yup
