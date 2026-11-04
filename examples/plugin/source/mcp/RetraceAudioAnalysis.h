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

#include <yup_audio_basics/yup_audio_basics.h>

//==============================================================================
/** Describes rendered or captured audio in numbers a language model can reason about. */
namespace RetraceAudioAnalysis
{
/** Measures a buffer and returns a JSON object with:

    - `duration`, `sampleRate`, `channels`
    - `peakDb`, `rmsDb`, `crestDb`, `dcOffset`, `clippedSamples` (at or above -0.01 dBFS)
    - `onset` (first sample above -60 dBFS), `timeToPeak` and `silentTail` (trailing time
      below -80 dBFS), in seconds
    - `stereoCorrelation`, from -1 (out of phase) through 0 (unrelated) to 1 (mono)
    - `spectralCentroid` in Hz and `octaveBands`, the share of energy in each octave band
      from 31.5 Hz to 16 kHz, in dB relative to the loudest band

    Levels are dBFS, -144 for silence.
*/
yup::var analyse (const yup::AudioBuffer<float>& audio, double sampleRate);
} // namespace RetraceAudioAnalysis
