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

#include "SynthTypes.h"

#include <atomic>

//==============================================================================
/** The drawn curves of every envelope and LFO: patch state, not parameters.

    The message thread edits the curves under a spin lock and bumps the generation. The
    audio thread compares the generation once per block and copies the curves with a
    try-lock, so it never waits: a block that finds the lock taken keeps playing the
    previous curves and tries again on the next one.

    Envelope 1 drives the amplitude, so its curve always ends at zero and never
    sustains or loops on its last point, which would leave a voice sounding forever or
    end it on a click.

    @see yup::BreakpointCurve, yup::CurveEnvelope
*/
class SynthCurveState
{
public:
    using Curve = yup::BreakpointCurve<float>;
    using EnvelopeCurves = std::array<Curve, SynthExample::envelopeCount>;
    using LFOCurves = std::array<Curve, SynthExample::lfoCount>;

    /** Creates the default curves: an ADSR per envelope and a ramp per LFO. */
    SynthCurveState();

    //==============================================================================
    /** Returns a copy of an envelope's curve. Message thread. */
    Curve getEnvelope (int index) const;

    /** Replaces an envelope's curve and publishes it. Message thread. */
    void setEnvelope (int index, const Curve& curve);

    /** Returns a copy of an LFO's custom curve. Message thread. */
    Curve getLFO (int index) const;

    /** Replaces an LFO's custom curve and publishes it. Message thread. */
    void setLFO (int index, const Curve& curve);

    /** Bumped on every change. */
    int getGeneration() const noexcept { return generation.load(); }

    //==============================================================================
    /** Copies every curve when the generation moved since seenGeneration. Audio thread.

        @returns True when the curves were copied, in which case seenGeneration is updated.
    */
    bool copyIfChanged (EnvelopeCurves& envelopes, LFOCurves& lfos, int& seenGeneration) const noexcept;

    //==============================================================================
    /** Writes every curve into a "Curves" tree. */
    yup::DataTree createTree() const;

    /** Replaces every curve from a "Curves" tree; anything missing gets its default. Message thread. */
    void readFrom (const yup::DataTree& tree);

    /** The type of the tree createTree() returns. */
    static const yup::Identifier treeType;

    //==============================================================================
    /** Builds an attack, decay, sustain and release curve with times in seconds. */
    static Curve makeAdsr (float attack, float decay, float sustain, float release, float curvature = 0.0f);

    /** Returns the default curve of an envelope. */
    static Curve makeDefaultEnvelope (int index);

    /** Returns the default custom LFO curve, a triangle. */
    static Curve makeDefaultLFO();

    /** Makes a curve safe to drive the amplitude with, see the class description. */
    static Curve sanitizeAmplitude (Curve curve);

private:
    mutable yup::SpinLock lock;
    EnvelopeCurves envelopes;
    LFOCurves lfos;
    std::atomic<int> generation { 0 };

    YUP_DECLARE_NON_COPYABLE (SynthCurveState)
};
