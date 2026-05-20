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

//==============================================================================
class SoundCardOutputNodeView final : public yup::AudioGraphNodeView
{
public:
    explicit SoundCardOutputNodeView (yup::StringRef subtitleIn = "sound card")
        : AudioGraphNodeView (yup::AudioGraphNodeID::invalid())
        , subtitle (subtitleIn)
    {
    }

    yup::String getNodeTitle() const override { return "OUTPUT"; }

    yup::String getNodeSubtitle() const override { return subtitle; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 0; }

    int getPreferredWidth() const override { return 150; }

    yup::Color getNodeColor() const override { return yup::Color (0xff06b6d4); }

    PortInfo getInputPortInfo (int) const override
    {
        return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };
    }

private:
    yup::String subtitle;
};
