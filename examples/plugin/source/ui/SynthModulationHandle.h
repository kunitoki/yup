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

#include <yup_gui/yup_gui.h>

#include "../dsp/SynthTypes.h"

//==============================================================================
/** A small icon dragged from an envelope or LFO onto a control to modulate it.

    Dropped on a control that can be a destination, the interface routes the source into
    it, see SynthInterface. While dragging, a dot in the source's color follows the mouse.
*/
class SynthModulationHandle
    : public yup::Component
    , public yup::DragAndDropSource
{
public:
    explicit SynthModulationHandle (SynthModulationSource sourceToDrag);
    ~SynthModulationHandle() override;

    /** Returns the source a drop routes. */
    SynthModulationSource getSource() const noexcept { return source; }

    /** The text a modulation drag carries. */
    static const yup::String dragText;

    void paint (yup::Graphics& g) override;
    void mouseEnter (const yup::MouseEvent& event) override;
    void mouseExit (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void dragOperationEnded (const yup::DragAndDropData& data, yup::DragAndDropAction performed) override;

private:
    class Ghost : public yup::Component
    {
    public:
        void paint (yup::Graphics& g) override;

        yup::Color color;
        yup::Color iconColor;
    };

    static constexpr float dragThreshold = 4.0f;
    static constexpr float ghostSize = 22.0f;

    SynthModulationSource source;
    Ghost ghost;
    bool hovered = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthModulationHandle)
};
