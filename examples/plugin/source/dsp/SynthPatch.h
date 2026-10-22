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

#include "SynthParameters.h"
#include "SynthPartials.h"

//==============================================================================
/** Saving, loading and generating whole patches.

    A patch is one DataTree: every parameter value as a property keyed by its ID, plus
    one child per oscillator holding its drawn partials. Session state, user preset
    files, factory presets and randomized patches all use this format, and all of them
    are applied through loadState(), so there is a single path that changes a patch.

    A property that is missing from the tree loads the parameter's default, which lets
    a factory preset list only what it changes.
*/
namespace SynthPatch
{
/** The partials of every oscillator. */
using Partials = std::array<SynthPartials, SynthExample::oscillatorCount>;

/** Captures the current patch.

    @param parameters   The processor's parameters
    @param partials     The drawn partials of every oscillator
    @param presetIndex  The factory preset the patch started from, or -1
*/
yup::DataTree createState (yup::Span<const yup::AudioParameter::Ptr> parameters, const Partials& partials, int presetIndex);

/** Applies a patch: parameters by ID, then partials, then one generation bump per oscillator.

    Message thread only; the caller notifies the host afterwards.
*/
yup::Result loadState (const yup::DataTree& state, yup::Span<const yup::AudioParameter::Ptr> parameters, Partials& partials);

/** Returns the factory preset index stored in a patch, or -1. */
int getPresetIndex (const yup::DataTree& state);

/** Creates a random patch that keeps the current volume, voice mode and glide. */
yup::DataTree createRandomState (const SynthParameters& parameters, yup::Random& random);

//==============================================================================
/** Returns how many factory presets there are. */
int getNumFactoryPresets();

/** Returns the name of a factory preset. */
yup::String getFactoryPresetName (int index);

/** Returns a factory preset as a patch, or an invalid tree for an index out of range. */
yup::DataTree createFactoryState (int index);
} // namespace SynthPatch
