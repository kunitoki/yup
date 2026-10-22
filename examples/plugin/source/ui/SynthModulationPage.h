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

#include "../dsp/SynthParameters.h"
#include "SynthModulationRow.h"

#include <array>
#include <memory>

//==============================================================================
/** The modulation routes as an editable list.

    Shows every used route and one empty row to add the next. An interim list until
    drag-to-assign modulation and the route drawer replace it.

    @see SynthRoutes, SynthModulationRow
*/
class SynthModulationPage : public yup::Component
{
public:
    SynthModulationPage (const SynthParameters& parametersToEdit,
                         SynthRoutes& routesToEdit,
                         yup::Span<const yup::AudioParameter::Ptr> processorParameters,
                         const yup::Font& font);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float rowHeight = 44.0f;
    static constexpr float spacing = 4.0f;

    std::vector<SynthRouteDestination> destinations;
    const SynthRoutes& routes;
    int shownGeneration = -1;

    yup::Label titleLabel;
    std::array<std::unique_ptr<SynthModulationRow>, SynthExample::modulationSlots> rows;
};
