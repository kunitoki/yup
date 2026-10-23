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

#include "SynthModulationPage.h"
#include "SynthTheme.h"

#include "../dsp/SynthPatch.h"

SynthModulationPage::SynthModulationPage (const SynthParameters& parametersToEdit,
                                          SynthRoutes& routesToEdit,
                                          yup::Span<const yup::AudioParameter::Ptr> processorParameters)
    : routes (routesToEdit)
{
    setOpaque (false);

    for (const auto& parameter : processorParameters)
    {
        const auto index = SynthPatch::findRouteDestination (processorParameters, parameter->getID());

        if (index >= 0)
            destinations.push_back ({ parameter->getID(), parameter->getName(), index });
    }

    for (std::size_t slot = 0; slot < rows.size(); ++slot)
    {
        rows[slot] = std::make_unique<SynthModulationRow> (static_cast<int> (slot), routesToEdit, destinations,
                                                           parametersToEdit.modulation.depths[slot]);
        addChildComponent (*rows[slot]);
    }

    syncFromParameters();
}

void SynthModulationPage::syncFromParameters()
{
    for (auto& row : rows)
        row->syncFromRoutes();

    if (const auto generation = routes.getGeneration(); generation != shownGeneration)
    {
        shownGeneration = generation;
        resized();
    }
}

void SynthModulationPage::setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames)
{
    if (macroNames == shownMacroNames)
        return;

    shownMacroNames = macroNames;

    for (auto& row : rows)
        row->setMacroNames (macroNames);
}

void SynthModulationPage::resized()
{
    auto bounds = getLocalBounds();

    // Every used route, then the first free slot to add the next one, as many as fit.
    auto offeredEmpty = false;

    for (auto& row : rows)
    {
        const auto show = (row->isUsed() || ! offeredEmpty) && bounds.getHeight() >= rowHeight;

        if (! row->isUsed() && show)
            offeredEmpty = true;

        row->setVisible (show);

        if (show)
        {
            row->setBounds (bounds.removeFromTop (rowHeight));
            bounds.removeFromTop (spacing);
        }
    }
}
