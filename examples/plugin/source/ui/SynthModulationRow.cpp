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

#include "SynthModulationRow.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

SynthModulationRow::SynthModulationRow (int slotToEdit,
                                        SynthRoutes& routesToEdit,
                                        const std::vector<SynthRouteDestination>& destinationsToOffer,
                                        yup::AudioParameter::Ptr depthParameter)
    : slot (slotToEdit)
    , routes (routesToEdit)
    , destinations (destinationsToOffer)
    , depthField ({}, std::move (depthParameter), SynthSliderField::Look::bar, true)
{
    setOpaque (false);

    const auto sourceNames = getSynthModulationSourceShortNames();

    for (int index = 0; index < sourceNames.size(); ++index)
        sourceBox.addItem (sourceNames[index], index + 1);

    for (std::size_t index = 0; index < destinations.size(); ++index)
        destinationBox.addItem (destinations[index].name, static_cast<int> (index) + 1);

    for (auto* box : { &sourceBox, &destinationBox })
    {
        box->setTextWhenNothingSelected ("-");
        box->onSelectedItemChanged = [this] { writeRoute(); };
        addAndMakeVisible (*box);
    }

    removeButton.onClick = [this]
    {
        const auto before = readRoute();
        routes.clearRoute (slot);
        syncFromRoutes();
        recordRoute (before);
    };
    addAndMakeVisible (removeButton);

    addAndMakeVisible (depthField);

    syncFromRoutes();
}

void SynthModulationRow::syncFromRoutes()
{
    depthField.syncFromParameter();

    const auto route = routes.getRoute (slot);
    removeButton.setVisible (route.isActive());
    repaint();

    if (! route.isActive())
    {
        // A half picked row keeps its source while the destination is being chosen.
        if (destinationBox.getSelectedId() > 0)
            destinationBox.setSelectedId (0, yup::dontSendNotification);

        return;
    }

    auto destinationId = 0;

    for (std::size_t index = 0; index < destinations.size(); ++index)
        if (destinations[index].index == route.destination)
            destinationId = static_cast<int> (index) + 1;

    sourceBox.setSelectedId (static_cast<int> (route.source) + 1, yup::dontSendNotification);
    destinationBox.setSelectedId (destinationId, yup::dontSendNotification);
}

void SynthModulationRow::setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames)
{
    // Item n is source n, and the macros come last.
    const auto first = static_cast<int> (SynthModulationSource::macro1);

    for (std::size_t macro = 0; macro < macroNames.size(); ++macro)
        sourceBox.changeItemText (first + static_cast<int> (macro), macroNames[macro]);
}

void SynthModulationRow::writeRoute()
{
    const auto source = sourceBox.getSelectedId() - 1;
    const auto destination = destinationBox.getSelectedId() - 1;

    if (source < 0 || ! yup::isPositiveAndBelow (destination, static_cast<int> (destinations.size())))
        return;

    const auto before = readRoute();
    const auto& target = destinations[static_cast<std::size_t> (destination)];
    routes.setRoute (slot, static_cast<SynthModulationSource> (source), target.id, target.index);
    removeButton.setVisible (isUsed());
    recordRoute (before);
}

SynthModulationRow::RouteState SynthModulationRow::readRoute() const
{
    const auto route = routes.getRoute (slot);
    return { static_cast<int> (route.source), route.destination, routes.getDestinationID (slot) };
}

void SynthModulationRow::recordRoute (const RouteState& before)
{
    const auto after = readRoute();

    if (after == before)
        return;

    SynthUndoHistory::record<RouteState> (*this, before, after, [&routes = routes, slot = slot] (const RouteState& state)
    {
        if (state.destination < 0)
            routes.clearRoute (slot);
        else
            routes.setRoute (slot, static_cast<SynthModulationSource> (state.source), state.destinationID, state.destination);
    });
}

void SynthModulationRow::resized()
{
    auto bounds = getLocalBounds().reduced (spacing, 0.0f);

    removeButton.setBounds (bounds.removeFromRight (buttonWidth).withSizeKeepingCenter (buttonWidth, buttonWidth));
    bounds.removeFromRight (spacing);
    depthField.setBounds (bounds.removeFromRight (depthWidth));
    bounds.removeFromRight (spacing);
    bounds.removeFromLeft (dotSize + spacing);
    sourceBox.setBounds (bounds.removeFromLeft (sourceWidth).reduced (0.0f, 6.0f));
    arrowBounds = bounds.removeFromLeft (arrowWidth);
    destinationBox.setBounds (bounds.reduced (0.0f, 6.0f));
}

void SynthModulationRow::paint (yup::Graphics& g)
{
    SynthTheme::paintWell (g, *this, getLocalBounds());

    const auto selected = sourceBox.getSelectedId() - 1;
    const auto source = static_cast<SynthModulationSource> (yup::jmax (0, selected));
    const auto dot = yup::Rectangle<float> (spacing, (getHeight() - dotSize) * 0.5f, dotSize, dotSize);

    g.setFillColor (selected >= 0 ? getSourceColor (*this, source) : SynthTheme::textSecondary (*this).withAlpha (0.4f));
    g.fillRoundedRect (dot, 2.0f);

    const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (10.0f);
    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText (yup::String::fromUTF8 (YUP_ICON_ARROW_RIGHT_LONG), font, arrowBounds, yup::Justification::center);
}
