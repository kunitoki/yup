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

#include <memory>

//==============================================================================
class GraphInputNodeView final : public yup::AudioGraphNodeView
{
public:
    GraphInputNodeView (std::shared_ptr<yup::AudioGraphProcessor> graphIn, yup::StringRef subtitleIn)
        : AudioGraphNodeView (yup::AudioGraphNodeID::invalid())
        , graph (std::move (graphIn))
        , subtitle (subtitleIn)
    {
    }

    yup::String getNodeTitle() const override { return "INPUT"; }

    yup::String getNodeSubtitle() const override { return subtitle; }

    int getNumInputPorts() const override { return 0; }

    int getNumOutputPorts() const override
    {
        return graph != nullptr ? static_cast<int> (graph->getBusLayout().getInputBuses().size()) : 0;
    }

    int getPreferredWidth() const override { return 160; }

    yup::Color getNodeColor() const override { return yup::Color (0xfff97316); }

    PortInfo getOutputPortInfo (int busIndex) const override
    {
        if (graph == nullptr)
            return { "?", getPortKindColor (PortKind::audio), PortKind::audio };

        return getPortInfo (graph->getBusLayout().getInputBuses(), busIndex);
    }

private:
    static PortInfo getPortInfo (yup::Span<const yup::AudioBus> buses, int busIndex)
    {
        if (busIndex < 0 || busIndex >= static_cast<int> (buses.size()))
            return { "?", getPortKindColor (PortKind::audio), PortKind::audio };

        const auto& bus = buses[static_cast<size_t> (busIndex)];
        const auto kind = bus.getType() == yup::AudioBus::Type::Audio ? PortKind::audio : PortKind::midi;
        return { bus.getName(), getPortKindColor (kind), kind };
    }

    std::shared_ptr<yup::AudioGraphProcessor> graph;
    yup::String subtitle;
};
