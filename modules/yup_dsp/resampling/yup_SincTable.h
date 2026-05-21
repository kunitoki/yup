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
    Pre-computed, windowed sinc lookup table for polyphase sample-rate conversion.

    Stores the positive half of a symmetric windowed sinc kernel.  The
    OversampleFactor parameter controls the fractional-phase resolution: table
    entry (tap, delta) corresponds to the sinc value at
    t = tap + delta / OversampleFactor.

    @tparam CoeffType         Precision for stored values (float or double).
    @tparam OversampleFactor Number of fractional phase sub-steps per tap.
    @tparam SincRadius            Number of taps on each side of the kernel centre.
*/
template <typename CoeffType, int OversampleFactor, int SincRadius>
class SincTable
{
public:
    static_assert (OversampleFactor > 0, "SincTable OversampleFactor must be > 0");
    static_assert (SincRadius > 0, "SincTable SincRadius must be > 0");

    /** Number of entries in the half-kernel storage. */
    static constexpr int tableSize = (SincRadius + 1) * OversampleFactor;

    //==============================================================================
    /** Default constructor. */
    SincTable() noexcept = default;

    //==============================================================================
    /**
        Computes the sinc kernel with cutoff at sampleRate / 2.

        Suitable for integer-factor upsampling where the anti-image cutoff
        equals the input Nyquist frequency.

        @param sampleRate  Input sample rate in Hz.
    */
    void configure (CoeffType sampleRate) noexcept
    {
        const CoeffType fc = sampleRate / CoeffType (2);
        const CoeffType T = CoeffType (1) / sampleRate;

        for (int i = 0; i <= SincRadius; ++i)
        {
            for (int delta = 0; delta < OversampleFactor; ++delta)
            {
                const CoeffType t = static_cast<CoeffType> (i)
                                  + static_cast<CoeffType> (delta) / static_cast<CoeffType> (OversampleFactor);

                if (t == CoeffType (0))
                    (*this) (i, delta) = CoeffType (1);
                else
                    (*this) (i, delta) = std::sin (MathConstants<CoeffType>::twoPi * fc * t * T)
                                       / (MathConstants<CoeffType>::twoPi * fc * t * T);
            }
        }
    }

    /**
        Computes the sinc kernel with an explicit cutoff frequency.

        Use this when the cutoff must be lower than sampleRate / 2, e.g. for
        downsampling where the target Nyquist is the cutoff.

        @param cutoff      Anti-aliasing cutoff in Hz (must be in (0, sampleRate/2]).
        @param sampleRate  Input sample rate in Hz.
    */
    void configureWithCutoff (CoeffType cutoff, CoeffType sampleRate) noexcept
    {
        jassert (cutoff > CoeffType (0) && cutoff <= sampleRate / CoeffType (2));

        const CoeffType T = CoeffType (1) / sampleRate;

        for (int i = 0; i <= SincRadius; ++i)
        {
            for (int delta = 0; delta < OversampleFactor; ++delta)
            {
                const CoeffType t = static_cast<CoeffType> (i)
                                  + static_cast<CoeffType> (delta) / static_cast<CoeffType> (OversampleFactor);

                if (t == CoeffType (0))
                    (*this) (i, delta) = CoeffType (1);
                else
                    (*this) (i, delta) = std::sin (MathConstants<CoeffType>::twoPi * cutoff * t * T)
                                       / (MathConstants<CoeffType>::twoPi * cutoff * t * T);
            }
        }
    }

    /**
        Multiplies the stored half-kernel by the second half of a Kaiser window.

        The full symmetric window has 2 * tableSize - 1 samples, so the stored
        center coefficient is exactly aligned with the window center and remains
        unchanged.

        @param beta  Kaiser window shape parameter (higher = more side-lobe suppression).
    */
    void applyKaiserWindow (CoeffType beta = CoeffType (5)) noexcept
    {
        constexpr int N = tableSize * 2 - 1;
        constexpr int center = tableSize - 1;

        for (int i = 0; i < tableSize; ++i)
            table[static_cast<std::size_t> (i)] *=
                WindowFunctions<CoeffType>::kaiser (center + i, N, beta);
    }

    //==============================================================================
    /** Returns the kernel value at absolute (mirrored) index i. */
    forcedinline CoeffType& operator[] (int i) noexcept
    {
        const int idx = (i < 0) ? -i : i;
        return table[static_cast<std::size_t> (idx)];
    }

    /** Returns the kernel value at absolute (mirrored) index i. */
    const forcedinline CoeffType& operator[] (int i) const noexcept
    {
        const int idx = (i < 0) ? -i : i;
        return table[static_cast<std::size_t> (idx)];
    }

    /**
        Returns the kernel value at (tap, delta), where the fractional phase is
        t = tap + delta / OversampleFactor.

        Negative tap indices are resolved by mirroring (symmetric kernel).
        Negative delta with tap == 0 is also resolved by symmetry.
    */
    forcedinline CoeffType& operator() (int tap, int delta) noexcept
    {
        if (tap < 0)
            return table[static_cast<std::size_t> ((-tap) * OversampleFactor - delta)];
        if (tap == 0 && delta < 0)
            return table[static_cast<std::size_t> (-delta)]; // sinc(-t) == sinc(t)
        return table[static_cast<std::size_t> (tap * OversampleFactor + delta)];
    }

    /** @copydoc operator()(int, int) */
    const forcedinline CoeffType& operator() (int tap, int delta) const noexcept
    {
        if (tap < 0)
            return table[static_cast<std::size_t> ((-tap) * OversampleFactor - delta)];
        if (tap == 0 && delta < 0)
            return table[static_cast<std::size_t> (-delta)]; // sinc(-t) == sinc(t)
        return table[static_cast<std::size_t> (tap * OversampleFactor + delta)];
    }

private:
    std::array<CoeffType, static_cast<std::size_t> (tableSize)> table {};
};

} // namespace yup
