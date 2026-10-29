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
constexpr int oscillatorCount = 4;
constexpr int maxHarmonics = 256;
constexpr int maxBlockSize = 2048;
constexpr int envelopeCount = 4;
constexpr int lfoCount = 4;
constexpr int filterCount = 2;
constexpr int modulationSlots = 64;
constexpr int macroCount = 8;

/** Samples between control updates: modulation, filter targets, FM ramps and host parameter changes. */
constexpr int controlChunk = 16;

/** Samples between per-voice spectrum derivations while a route reaches a spectral parameter.
    Each one costs inverse FFTs, so it runs slower than the control chunk. A multiple of it. */
constexpr int spectralChunk = 128;

/** Copies of the whole operator network a voice stacks for unison. */
constexpr int maxUnisonVoices = 7;

/** Harmonics the partial editor exposes, a subset of the maxHarmonics the engine renders. */
constexpr int editableHarmonics = 128;
static_assert (editableHarmonics <= maxHarmonics);

/** Harmonics the waveform display sums and a drawn stroke keeps: all its 256 points can carry. */
constexpr int displayHarmonics = 127;

constexpr double levelRampSeconds = 0.01;
constexpr double enableFadeSeconds = 0.005; /**< How long switching an operator on or off fades it. */
} // namespace SynthExample
