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
#include "SynthEffectChain.h"
#include "SynthParameters.h"
#include "SynthPartials.h"
#include "SynthRoutes.h"
#include "SynthTuning.h"

//==============================================================================
/** Saving, loading and generating whole patches.

    A patch is one DataTree: every parameter value as a property keyed by its ID, plus
    one child per oscillator holding its drawn partials, one child with the envelope and
    LFO curves, one with the modulation routes, one with the effects chain order and one with
    the tuning and pitch wheel ranges. Session state, user preset files,
    factory presets and randomized patches all use this format, and all of them are
    applied through loadState(), so there is a single path that changes a patch.

    A property that is missing from the tree loads the parameter's default, and missing
    curves, routes, chain order or tuning load theirs, which lets a factory preset list only what it changes.
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
    SynthEffectChain& effectChain;
    SynthTuning& tuning;
};

/** What a patch says about itself, stored with it. Missing fields read as empty. */
struct PresetInfo
{
    yup::String name;
    yup::String bank;
    yup::String author;
    yup::String tags; /**< Comma separated. */
    yup::String category; /**< The one main kind of sound, like Bass or Pad. */
    yup::String description;
    yup::String notes;
    yup::String id; /**< The preset's UUID, which stays with it through renames; empty for a patch that is not a preset. */
    std::array<yup::String, SynthExample::macroCount> macroNames; /**< Empty for a macro's default name. */

    bool operator== (const PresetInfo&) const = default;

    /** Returns what a macro is called: its name, or "Macro N" when it has none. */
    yup::String getMacroName (int index) const
    {
        const auto& name = macroNames[static_cast<std::size_t> (index)];
        return name.isNotEmpty() ? name : "Macro " + yup::String (index + 1);
    }
};

/** Returns the description stored in a patch. */
PresetInfo readInfo (const yup::DataTree& state);

/** Replaces the description stored in a patch, leaving the sound as it is. */
void writeInfo (yup::DataTree& state, const PresetInfo& info);

/** True when the tree is a patch, in any version. */
bool isPatch (const yup::DataTree& state);

/** Captures the current patch.

    @param parameters   The processor's parameters
    @param state        The partials, curves, routes, effects chain order and tuning
    @param presetIndex  The factory preset the patch started from, or -1
    @param info         The description stored with the patch
*/
yup::DataTree createState (yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state, int presetIndex, const PresetInfo& info);

/** Applies a patch: parameters by ID, then partials, curves, routes, effects order and tuning, then one
    generation bump per oscillator.

    Message thread only; the caller notifies the host afterwards.
*/
yup::Result loadState (const yup::DataTree& tree, yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state);

/** Returns the factory preset index stored in a patch, or -1. */
int getPresetIndex (const yup::DataTree& state);

/** Returns the index of the parameter a route may reach, or -1.

    Only continuous parameters are destinations: modulating a choice or a count would jump
    between steps, the quality would change the latency, and volume, glide, the route
    depths and the macros are not applied per voice. The master effects are destinations
    too, modulated monophonically (see HarmonicSynthEngine::getGlobalValues()).
*/
int findRouteDestination (yup::Span<const yup::AudioParameter::Ptr> parameters, const yup::String& parameterID);

/** Creates a random patch that keeps the current volume, voice mode, glide, quality,
    macro positions and effects, named "Random" with the macro names given.

    The patch aims for a kind of sound, a pluck, keys, a pad, a lead, a bass or a bell,
    which shapes its amplitude envelope. Only the modules it uses are switched on, always
    the first ones of each kind: every operator is heard or modulates another, every filter
    receives sound, and every envelope and LFO drives at least one route.
*/
yup::DataTree createRandomState (const SynthParameters& parameters,
                                 yup::Random& random,
                                 const std::array<yup::String, SynthExample::macroCount>& macroNames);

/** Moves the sound of a patch made by createState() a little, keeping its structure.

    The continuous controls of the modules that are on move by a few percent of their
    range, and the depths of the routes in use by up to 30% of themselves, never changing
    sign. Pitch, the switches, every choice, every control at its minimum, like an unused
    FM link or send, and every spectral control at its neutral value keep their values.
*/
void varyState (yup::DataTree& state, const SynthParameters& parameters, yup::Random& random);

//==============================================================================
/** Returns how many factory presets there are. */
int getNumFactoryPresets();

/** Returns the UUID of a factory preset, which stays the same when it is renamed. */
yup::String getFactoryPresetId (int index);

/** Returns the name of a factory preset. */
yup::String getFactoryPresetName (int index);

/** Returns the description of a factory preset, empty for an index out of range. */
PresetInfo getFactoryPresetInfo (int index);

/** Returns a factory preset as a patch, or an invalid tree for an index out of range. */
yup::DataTree createFactoryState (int index);
} // namespace SynthPatch
