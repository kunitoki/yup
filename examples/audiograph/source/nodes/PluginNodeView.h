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

class PluginNodeView final : public yup::AudioGraphNodeView
{
public:
    PluginNodeView (yup::AudioGraphNodeID nodeID,
                    const yup::AudioPluginDescription& descIn)
        : AudioGraphNodeView (nodeID)
        , desc (descIn)
    {
    }

    yup::String getNodeTitle() const override
    {
        return desc.name.isEmpty() ? "Plugin" : desc.name;
    }

    yup::String getNodeSubtitle() const override
    {
        return desc.vendor;
    }

    int getNumInputPorts() const override
    {
        int count = 0;
        if (desc.numInputChannels > 0)
            ++count;
        if (desc.numMidiInputPorts > 0)
            ++count;
        return count;
    }

    int getNumOutputPorts() const override
    {
        int count = 0;
        if (desc.numOutputChannels > 0)
            ++count;
        if (desc.numMidiOutputPorts > 0)
            ++count;
        return count;
    }

    int getPreferredWidth() const override
    {
        return yup::jmax (180, static_cast<int> (desc.name.length()) * 10 + 60);
    }

    yup::Color getNodeColor() const override
    {
        return desc.isInstrument ? yup::Color (0xffe11d48) : yup::Color (0xff0891b2);
    }

    PortInfo getInputPortInfo (int portIndex) const override
    {
        if (desc.numInputChannels > 0 && portIndex == 0)
            return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };

        return { "MIDI", getPortKindColor (PortKind::midi), PortKind::midi };
    }

    PortInfo getOutputPortInfo (int portIndex) const override
    {
        if (desc.numOutputChannels > 0 && portIndex == 0)
            return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };

        return { "MIDI", getPortKindColor (PortKind::midi), PortKind::midi };
    }

    int getNumParameterRows() const override { return 0; }

    const yup::AudioPluginDescription& getDescription() const noexcept { return desc; }

private:
    yup::AudioPluginDescription desc;
};
