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

#include "SynthCurveState.h"
#include "SynthParameters.h"
#include "SynthPartials.h"
#include "SynthRoutes.h"

//==============================================================================
/** Saving, loading and generating whole patches.

    A patch is one DataTree: every parameter value as a property keyed by its ID, plus
    one child per oscillator holding its drawn partials, one child with the envelope and
    LFO curves and one with the modulation routes. Session state, user preset files,
    factory presets and randomized patches all use this format, and all of them are
    applied through loadState(), so there is a single path that changes a patch.

    A property that is missing from the tree loads the parameter's default, and missing
    curves or routes load theirs, which lets a factory preset list only what it changes.
    Patches from before the four operator engine (version 1) are rejected.
*/
namespace SynthPatch
{
/** The partials of every oscillator. */
using Partials = std::array<SynthPartials, SynthExample::oscillatorCount>;

/** The patch state that is not a parameter. */
struct State
{
    Partials& partials;
    SynthCurveState& curves;
    SynthRoutes& routes;
};

/** What a patch says about itself, stored with it. Missing fields read as empty. */
struct PresetInfo
{
    yup::String name;
    yup::String bank;
    yup::String author;
    yup::String tags; /**< Comma separated. */

    bool operator== (const PresetInfo&) const = default;
};

/** Returns the description stored in a patch. */
PresetInfo readInfo (const yup::DataTree& state);

/** Captures the current patch.

    @param parameters   The processor's parameters
    @param state        The partials, curves and routes
    @param presetIndex  The factory preset the patch started from, or -1
    @param info         The description stored with the patch
*/
yup::DataTree createState (yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state, int presetIndex, const PresetInfo& info);

/** Applies a patch: parameters by ID, then partials, curves and routes, then one
    generation bump per oscillator.

    Message thread only; the caller notifies the host afterwards.
*/
yup::Result loadState (const yup::DataTree& tree, yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state);

/** Returns the factory preset index stored in a patch, or -1. */
int getPresetIndex (const yup::DataTree& state);

/** Returns the index of the parameter a route may reach, or -1.

    Only continuous parameters the voices play are destinations: modulating a choice or
    a count would jump between steps, the quality would change the latency, and volume,
    glide and the route depths are not applied per voice.
*/
int findRouteDestination (yup::Span<const yup::AudioParameter::Ptr> parameters, const yup::String& parameterID);

/** Creates a random patch that keeps the current volume, voice mode, glide and quality. */
yup::DataTree createRandomState (const SynthParameters& parameters, yup::Random& random);

//==============================================================================
/** Returns how many factory presets there are. */
int getNumFactoryPresets();

/** Returns the name of a factory preset. */
yup::String getFactoryPresetName (int index);

/** Returns the description of a factory preset, empty for an index out of range. */
PresetInfo getFactoryPresetInfo (int index);

/** Returns a factory preset as a patch, or an invalid tree for an index out of range. */
yup::DataTree createFactoryState (int index);
} // namespace SynthPatch
