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

#include "SynthParameterFactory.h"
#include "SynthTypes.h"

//==============================================================================
/** A plain snapshot of one LFO's controls. The custom shape's curve is patch state. */
struct SynthLFOValues
{
    using Shape = yup::LFO<float>::Shape;
    using RetriggerMode = yup::LFO<float>::RetriggerMode;

    Shape shape = Shape::sine;
    float rate = 1.0f;
    bool tempoSync = false;
    int division = 5;
    float phase = 0.0f;
    float smoothing = 0.0f; /**< In seconds. */
    RetriggerMode retrigger = RetriggerMode::retrigger;
    float depth = 1.0f;
    bool enabled = true;
    bool inverted = false; /**< Negates the output, after smoothing. */

    bool operator== (const SynthLFOValues&) const = default;

    /** Returns the rate in Hz, following the tempo when synced and one is known. */
    float getFrequency (double bpm) const noexcept
    {
        if (! tempoSync || bpm <= 0.0)
            return rate;

        const auto& divisions = getSynthLFODivisions();
        const auto beats = divisions[static_cast<std::size_t> (yup::jlimit (0, static_cast<int> (divisions.size()) - 1, division))].beats;

        return static_cast<float> (bpm / (60.0 * beats));
    }
};

//==============================================================================
/** The parameters of one LFO, IDs prefixed "lfo1." to "lfo4.". */
class SynthLFOParameters
{
public:
    /** Creates the parameters of one LFO with the SynthLFOValues defaults.

        @param lfoIndex  The zero based LFO, which picks the ID prefix and host IDs
    */
    explicit SynthLFOParameters (int lfoIndex);

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot. */
    SynthLFOValues read (SynthParameterReader reader = {}) const noexcept;

    yup::AudioParameter::Ptr shape;
    yup::AudioParameter::Ptr rate;
    yup::AudioParameter::Ptr tempoSync; /**< Stepped, 0 or 1. */
    yup::AudioParameter::Ptr division;
    yup::AudioParameter::Ptr phase;
    yup::AudioParameter::Ptr smoothing;
    yup::AudioParameter::Ptr retrigger;
    yup::AudioParameter::Ptr depth;
    yup::AudioParameter::Ptr enabled; /**< Stepped, 0 or 1; off is not computed and sources 0. */
    yup::AudioParameter::Ptr inverted; /**< Stepped, 0 or 1. */
};
