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

#include "../RetraceProcessor.h"
#include "SynthIconButton.h"

#include <yup_gui/yup_gui.h>

#include <array>
#include <functional>

//==============================================================================
/** The preset tile of the header: the patch name with the browser toggle and the steps
    through the browser's list, and below it the patch's bank, author and tags.

    The description is the processor's, so it follows whatever loaded the patch; it is
    edited in the preset browser's info panel. User presets are files holding the same
    patch the host saves with the session, kept by default in the user's audio presets
    folder under the plugin's vendor and name. The arrows follow the browser's current search, filters and
    order, even while the browser is closed.

    @see RetraceProcessor, SynthPatch, SynthHeader
*/
class SynthPresetBar : public yup::Component
{
public:
    explicit SynthPresetBar (RetraceProcessor& processorToControl);

    /** Shows the patch the processor is on and its description. */
    void syncFromProcessor();

    /** Asks for a file and saves the patch to it. */
    void savePreset();

    /** Asks for a preset file and loads it. */
    void loadPreset();

    /** Called when the list button, the caret or the name is clicked, to open or close the browser. */
    std::function<void()> onBrowserToggled;

    /** Shows the list button and the caret in the accent color while the browser is open. */
    void setBrowserOpen (bool isOpen);

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseMove (const yup::MouseEvent& event) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    static constexpr float arrowWidth = 22.0f;
    static constexpr float padding = 12.0f;

    void stepPreset (int delta);
    void toggleBrowser();

    RetraceProcessor& processor;
    SynthIconButton listButton { "PresetList", YUP_ICON_LIST, 13.0f, false };
    SynthIconButton menuButton { "PresetMenu", YUP_ICON_CARET_DOWN, 12.0f, false };
    SynthIconButton previousButton { "PreviousPreset", YUP_ICON_CHEVRON_LEFT, 11.0f, false };
    SynthIconButton nextButton { "NextPreset", YUP_ICON_CHEVRON_RIGHT, 11.0f, false };
    std::array<yup::Rectangle<float>, 3> captionAreas;
    std::array<yup::Rectangle<float>, 3> valueAreas;
    yup::Rectangle<float> nameArea;
    SynthPatch::PresetInfo shownInfo;
};
