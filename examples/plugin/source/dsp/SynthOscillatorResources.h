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

#include "SynthTypes.h"

#include <array>

//==============================================================================
/** Immutable waveform data, prepared once and read by every voice.

    Building a FourierSeries allocates, so it belongs at construction time. Once
    prepared the resources are read-only and safe to share across voices.

    @see SynthOscillator
*/
class SynthOscillatorResources
{
public:
    /** Builds the series of every Waveform preset. */
    SynthOscillatorResources();

    /** Returns the series of one of the Waveform presets. */
    const yup::FourierSeries<double>& getFrame (yup::Waveform waveform) const noexcept
    {
        return frames[static_cast<std::size_t> (waveform)];
    }

private:
    std::array<yup::FourierSeries<double>, 6> frames;
};
