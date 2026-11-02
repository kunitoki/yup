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

#include "../dsp/SynthRoutes.h"

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_gui/yup_gui.h>

//==============================================================================
/** Route edits made from the interface, each recorded into the undo history.

    @see SynthRoutes, SynthUndoHistory
*/
namespace SynthRouteEdits
{
/** What a slot holds, as an undo step keeps it. */
struct RouteState
{
    int source = 0;
    int destination = -1; /**< Parameter index, -1 when the slot is unused. */
    yup::String destinationID;

    bool operator== (const RouteState&) const = default;
};

/** Returns what a slot holds. */
RouteState read (const SynthRoutes& routes, int slot);

/** Writes a slot's route, or unroutes it, and records the change.

    @param component     A component inside the interface, to find the history from
    @param routes        The routes to edit
    @param slot          The slot to write
    @param state         The route to write; a negative destination unroutes the slot
    @param joinsLastStep True to undo it together with the step recorded just before
*/
void write (yup::Component& component, SynthRoutes& routes, int slot, const RouteState& state, bool joinsLastStep = false);

/** Unroutes a slot and returns its depth to the default, as one undo step.

    @param component     A component inside the interface, to find the history from
    @param routes        The routes to edit
    @param slot          The slot to clear
    @param depth         The slot's depth parameter
    @param joinsLastStep True to undo it together with the step recorded just before
*/
void clear (yup::Component& component, SynthRoutes& routes, int slot, const yup::AudioParameter::Ptr& depth, bool joinsLastStep = false);
} // namespace SynthRouteEdits
