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

#include "SynthRoutes.h"

namespace
{
const yup::Identifier routeType ("Route");
const yup::Identifier slotProperty ("slot");
const yup::Identifier sourceProperty ("source");
const yup::Identifier destinationProperty ("destination");
} // namespace

const yup::Identifier SynthRoutes::treeType ("Routes");

SynthRoutes::SynthRoutes()
{
    for (auto& source : sources)
        source.store (0);

    for (auto& destination : destinations)
        destination.store (-1);
}

void SynthRoutes::setRoute (int slot, SynthModulationSource source, const yup::String& destinationID, int destinationIndex)
{
    if (! yup::isPositiveAndBelow (slot, SynthExample::modulationSlots))
        return;

    const auto index = static_cast<std::size_t> (slot);

    destinationIDs[index] = destinationIndex >= 0 ? destinationID : yup::String();
    sources[index].store (static_cast<int> (source));
    destinations[index].store (destinationIndex);
    generation.fetch_add (1);
}

void SynthRoutes::clearRoute (int slot)
{
    setRoute (slot, SynthModulationSource::env1, {}, -1);
}

void SynthRoutes::clearAll()
{
    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
        clearRoute (slot);
}

SynthModulationSource SynthRoutes::getSource (int slot) const noexcept
{
    return getRoute (slot).source;
}

yup::String SynthRoutes::getDestinationID (int slot) const
{
    if (! yup::isPositiveAndBelow (slot, SynthExample::modulationSlots))
        return {};

    return destinationIDs[static_cast<std::size_t> (slot)];
}

int SynthRoutes::findFreeSlot() const
{
    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
        if (! getRoute (slot).isActive())
            return slot;

    return -1;
}

int SynthRoutes::findRoute (SynthModulationSource source, const yup::String& destinationID) const
{
    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
        if (getRoute (slot).isActive() && getSource (slot) == source && getDestinationID (slot) == destinationID)
            return slot;

    return -1;
}

SynthRoute SynthRoutes::getRoute (int slot) const noexcept
{
    if (! yup::isPositiveAndBelow (slot, SynthExample::modulationSlots))
        return {};

    const auto index = static_cast<std::size_t> (slot);
    const auto source = yup::jlimit (0, static_cast<int> (SynthModulationSource::count) - 1, sources[index].load());

    return { static_cast<SynthModulationSource> (source), destinations[index].load() };
}

yup::DataTree SynthRoutes::createTree() const
{
    yup::DataTree tree (treeType);
    auto transaction = tree.beginTransaction();

    const auto sourceNames = getSynthModulationSourceNames();

    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
    {
        const auto route = getRoute (slot);

        if (! route.isActive())
            continue;

        yup::DataTree child (routeType);

        {
            auto childTransaction = child.beginTransaction();
            childTransaction.setProperty (slotProperty, slot);
            childTransaction.setProperty (sourceProperty, sourceNames[static_cast<int> (route.source)]);
            childTransaction.setProperty (destinationProperty, getDestinationID (slot));
        }

        transaction.addChild (child);
    }

    return tree;
}

void SynthRoutes::readFrom (const yup::DataTree& tree, const Resolver& resolve)
{
    clearAll();

    if (! tree.isValid() || tree.getType() != treeType)
        return;

    const auto sourceNames = getSynthModulationSourceNames();

    for (int index = 0; index < tree.getNumChildren(); ++index)
    {
        const auto child = tree.getChild (index);

        if (child.getType() != routeType)
            continue;

        const auto slot = static_cast<int> (child.getProperty (slotProperty, -1));
        const auto source = sourceNames.indexOf (child.getProperty (sourceProperty, {}).toString());
        const auto destinationID = child.getProperty (destinationProperty, {}).toString();
        const auto destination = source >= 0 ? resolve (destinationID) : -1;

        if (destination >= 0)
            setRoute (slot, static_cast<SynthModulationSource> (source), destinationID, destination);
    }
}
