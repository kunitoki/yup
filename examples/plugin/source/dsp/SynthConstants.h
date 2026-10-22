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

//==============================================================================
/** Sizing shared by the polyphonic engine and its user interface. */
namespace SynthExample
{
constexpr int voiceCount = 8;
constexpr int oscillatorCount = 2;
constexpr int maxHarmonics = 128;
constexpr int maxBlockSize = 2048;
constexpr int envelopeCount = 2;
constexpr int lfoCount = 2;
constexpr int modulationSlots = 16;

/** Samples between control updates: modulation, filter targets, local spectra and host parameter changes. */
constexpr int controlChunk = 128;

/** Unison slots per oscillator, counting the one running the selected algorithm. */
constexpr int maxUnisonVoices = 5;

/** Harmonics the partial editor exposes, a subset of the maxHarmonics the engine renders. */
constexpr int editableHarmonics = 64;

/** Harmonics the waveform display sums, capped well below maxHarmonics to keep repaints cheap. */
constexpr int displayHarmonics = 64;

constexpr double levelRampSeconds = 0.01;
} // namespace SynthExample
