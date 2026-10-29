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
#include <functional>

//==============================================================================
/** One route as the audio thread plays it. */
struct SynthRoute
{
    SynthModulationSource source = SynthModulationSource::env1;
    int destination = -1; /**< Parameter index in the processor, -1 when the slot is unused. */

    bool isActive() const noexcept { return destination >= 0; }
};

//==============================================================================
/** The sources and destinations of the modulation routes: patch state, not parameters.

    A destination is stored by parameter ID, so a route keeps pointing at the same
    control whatever the parameter order, and resolved to the parameter's index for the
    audio thread. Only continuous parameters can be destinations: the resolver decides.

    The message thread edits routes; the audio thread reads them with plain atomic loads
    once per block. A route changing while it is read can be seen half updated for one
    block, which only ever plays a valid source into a valid destination.

    @see SynthModulationParameters
*/
class SynthRoutes
{
public:
    /** Turns a parameter ID into an index, or -1 when it is not a valid destination. */
    using Resolver = std::function<int (const yup::String& parameterID)>;

    SynthRoutes();

    //==============================================================================
    /** Routes a source into a destination. Message thread. */
    void setRoute (int slot, SynthModulationSource source, const yup::String& destinationID, int destinationIndex);

    /** Unroutes a slot. Message thread. */
    void clearRoute (int slot);

    /** Unroutes every slot. Message thread. */
    void clearAll();

    /** Returns the source of a slot. */
    SynthModulationSource getSource (int slot) const noexcept;

    /** Returns the destination parameter ID of a slot, empty when unused. Message thread. */
    yup::String getDestinationID (int slot) const;

    /** Returns the first unused slot, or -1. Message thread. */
    int findFreeSlot() const;

    /** Returns the slot routing a source into a destination, or -1. Message thread. */
    int findRoute (SynthModulationSource source, const yup::String& destinationID) const;

    //==============================================================================
    /** Returns a slot as the audio thread plays it. */
    SynthRoute getRoute (int slot) const noexcept;

    /** Bumped on every change, so the user interface knows to refresh. */
    int getGeneration() const noexcept { return generation.load(); }

    //==============================================================================
    /** Writes every used slot into a "Routes" tree. */
    yup::DataTree createTree() const;

    /** Replaces every slot from a "Routes" tree; an invalid tree clears them all. Message thread. */
    void readFrom (const yup::DataTree& tree, const Resolver& resolve);

    /** The type of the tree createTree() returns. */
    static const yup::Identifier treeType;

private:
    std::array<std::atomic<int>, SynthExample::modulationSlots> sources;
    std::array<std::atomic<int>, SynthExample::modulationSlots> destinations;
    std::array<yup::String, SynthExample::modulationSlots> destinationIDs;
    std::atomic<int> generation { 0 };

    YUP_DECLARE_NON_COPYABLE (SynthRoutes)
};
