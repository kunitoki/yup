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

#include <yup_data_model/yup_data_model.h>

#include "SynthOscillatorParameters.h"

#include <array>
#include <atomic>

//==============================================================================
/** Sums a Fourier series at one point of its period.

    yup::FourierSeries stores coefficients rather than samples, so the waveform display
    and the partials' level measurement reconstruct them on demand. Only the message
    thread calls this.

    @param series      The coefficients to sum
    @param phase       The position in the period, normalized to 0 to 1
    @param maxHarmonic The highest harmonic to include

    @returns The value of the series at that phase.
*/
float evaluateFourierSeries (const yup::FourierSeries<double>& series, double phase, int maxHarmonic) noexcept;

//==============================================================================
/** The drawn partials of one oscillator: state saved with the patch, not parameters.

    The waveform editor writes partials continuously while the mouse is down but bumps
    the generation at most once per user interface frame. The audio thread rebuilds its
    series only when that counter moves, which keeps a drag from forcing an inverse FFT
    per mouse event on every sounding voice.

    Every partial keeps both its cosine and sine coefficient, so a drawn cycle comes back
    with the phase it was drawn with rather than as a sum of sines.

    @see SynthOscillatorValues, WaveformEditor, SynthPatch
*/
class SynthPartials
{
public:
    /** Creates silent partials, with the oscillator playing its waveform preset. */
    SynthPartials();

    //==============================================================================
    /** Rebuilds a prepared series from the partials, without allocating.

        Nothing stops the editor from asking for every harmonic at once, which would sum
        to many times full scale, so the caller passes the scale that brings the
        reconstruction back to a peak of one: getHarmonicScale(), measured on the
        message thread by updateHarmonicScale().

        @param series The prepared series to overwrite
        @param scale  The factor to apply to every coefficient
    */
    void copyHarmonicsInto (yup::FourierSeries<double>& series, float scale) const noexcept;

    /** Seeds the partials from a series, so editing starts at the visible shape. */
    void seedHarmonicsFrom (const yup::FourierSeries<double>& series) noexcept;

    /** Sets the magnitude of one partial while keeping its phase.

        A partial that is currently silent has no phase to keep, so it starts as a sine.

        @param index     The zero based partial, 0 being the fundamental
        @param magnitude The new magnitude
    */
    void setHarmonicMagnitude (int index, float magnitude) noexcept;

    //==============================================================================
    /** True while the oscillator plays these partials instead of its waveform preset. */
    bool usesCustomSeries() const noexcept { return customSeries.load(); }

    /** Switches between these partials and the waveform preset. */
    void setUsesCustomSeries (bool shouldUseCustomSeries) noexcept { customSeries.store (shouldUseCustomSeries); }

    /** Measures the reconstruction's peak and stores the scale that brings it to one.

        Allocates, so it runs on the message thread, never on the audio thread; call it
        after editing and before bumpGeneration().
    */
    void updateHarmonicScale();

    /** Returns the scale updateHarmonicScale() measured. */
    float getHarmonicScale() const noexcept { return harmonicScale.load(); }

    //==============================================================================
    /** Returns the counter the audio thread watches for edits. */
    int getGeneration() const noexcept { return generation.load(); }

    /** Publishes every edit made since the last bump to the audio thread. */
    void bumpGeneration() noexcept { generation.fetch_add (1); }

    /** Copies the partial related fields into an oscillator snapshot. */
    void fillValues (SynthOscillatorValues& values) const noexcept;

    //==============================================================================
    /** Writes the partials into the properties of a state tree. */
    void writeTo (yup::DataTree& tree) const;

    /** Reads the partials back, falling back to the waveform preset when the tree is invalid.

        Updates the harmonic scale but does not bump the generation: the caller does once
        everything else of the patch is in place.
    */
    void readFrom (const yup::DataTree& tree);

private:
    std::array<std::atomic<float>, SynthExample::editableHarmonics> harmonicCosines;
    std::array<std::atomic<float>, SynthExample::editableHarmonics> harmonicSines;
    std::atomic<float> harmonicScale { 1.0f };
    std::atomic<bool> customSeries { false };
    std::atomic<int> generation { 0 };

    YUP_DECLARE_NON_COPYABLE (SynthPartials)
};
