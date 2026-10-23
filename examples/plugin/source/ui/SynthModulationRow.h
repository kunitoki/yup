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
#include "SynthIconButton.h"
#include "SynthSliderField.h"

#include <vector>

//==============================================================================
/** A parameter a route can reach, as the route list offers it. */
struct SynthRouteDestination
{
    yup::String id;
    yup::String name;
    int index = -1;
};

//==============================================================================
/** One modulation route in a sunken strip: a dot in the source's color, which source
    drives which destination, how much, and a remove button.

    Picking a source and a destination writes the route; the depth is the slot's
    automatable depth parameter.

    @see SynthRoutes, SynthModulationPage
*/
class SynthModulationRow : public yup::Component
{
public:
    SynthModulationRow (int slotToEdit,
                        SynthRoutes& routesToEdit,
                        const std::vector<SynthRouteDestination>& destinationsToOffer,
                        yup::AudioParameter::Ptr depthParameter);

    /** Shows the slot's route and depth, wherever they were changed. */
    void syncFromRoutes();

    /** True when the slot holds a route. */
    bool isUsed() const noexcept { return routes.getRoute (slot).isActive(); }

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float depthWidth = 140.0f;
    static constexpr float buttonWidth = 22.0f;
    static constexpr float dotSize = 8.0f;
    static constexpr float spacing = 6.0f;

    void writeRoute();

    int slot = 0;
    SynthRoutes& routes;
    const std::vector<SynthRouteDestination>& destinations;

    yup::ComboBox sourceBox;
    yup::ComboBox destinationBox;
    SynthSliderField depthField;
    SynthIconButton removeButton { "RemoveRoute", YUP_ICON_XMARK, 11.0f };
};
