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

#include "SynthRouteEdits.h"
#include "ParameterGesture.h"
#include "SynthUndoHistory.h"

namespace SynthRouteEdits
{
namespace
{
void apply (SynthRoutes& routes, int slot, const RouteState& state)
{
    if (state.destination < 0)
        routes.clearRoute (slot);
    else
        routes.setRoute (slot, static_cast<SynthModulationSource> (state.source), state.destinationID, state.destination);
}
} // namespace

RouteState read (const SynthRoutes& routes, int slot)
{
    const auto route = routes.getRoute (slot);
    return { static_cast<int> (route.source), route.destination, routes.getDestinationID (slot) };
}

void write (yup::Component& component, SynthRoutes& routes, int slot, const RouteState& state, bool joinsLastStep)
{
    const auto before = read (routes, slot);
    apply (routes, slot, state);
    const auto after = read (routes, slot);

    if (after == before)
        return;

    SynthUndoHistory::record<RouteState> (component, before, after, [&routes, slot] (const RouteState& value)
    {
        apply (routes, slot, value);
    },
                                          joinsLastStep);
}

void clear (yup::Component& component, SynthRoutes& routes, int slot, const yup::AudioParameter::Ptr& depth, bool joinsLastStep)
{
    if (! joinsLastStep)
        SynthUndoHistory::beginStep (component);

    write (component, routes, slot, {}, true);

    if (depth->getValue() != depth->getDefaultValue())
        ParameterGesture (component, depth).setNormalized (depth->convertToNormalizedValue (depth->getDefaultValue()), true);
}
} // namespace SynthRouteEdits
