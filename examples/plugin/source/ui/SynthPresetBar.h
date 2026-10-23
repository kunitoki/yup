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

#include "../PrismProcessor.h"
#include "SynthIconButton.h"

#include <yup_gui/yup_gui.h>

#include <array>
#include <functional>

//==============================================================================
/** The preset tile of the header: the patch name with the factory preset list and the
    steps through it, and below it the patch's bank, author and tags.

    The description is the processor's, so it follows whatever loaded the patch. A user
    patch's bank, author and tags are edited in place and saved with it; a factory
    preset's are read only. User presets are files holding the same patch the host saves
    with the session, kept by default in the user's application data folder under
    YUP/Prism/Presets.

    @see PrismProcessor, SynthPatch, SynthHeader
*/
class SynthPresetBar : public yup::Component
{
public:
    explicit SynthPresetBar (PrismProcessor& processorToControl);

    /** Shows the patch the processor is on and its description. */
    void syncFromProcessor();

    /** Asks for a file and saves the patch to it. */
    void savePreset();

    /** Asks for a preset file and loads it. */
    void loadPreset();

    /** Called when the description stops being edited, so the keyboard can take the focus back. */
    std::function<void()> onEditingFinished;

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float arrowWidth = 22.0f;
    static constexpr float padding = 12.0f;

    enum Field
    {
        bank,
        author,
        tags
    };

    /** A field of the description: takes the keyboard focus only while it is clicked to
        be edited, and gives it up on return or escape, so the computer keyboard keeps
        playing the notes otherwise. */
    class FieldEditor : public yup::TextEditor
    {
    public:
        FieldEditor();

        std::function<void()> onFinished;

        void mouseDown (const yup::MouseEvent& event) override;
        void focusLost() override;
        void keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override;
    };

    void stepPreset (int delta);
    void showPresetMenu();
    void writeField (Field field, const yup::String& text);
    void finishEditingWhenIdle();
    static yup::File getPresetFolder();

    PrismProcessor& processor;
    SynthIconButton listButton { "PresetList", YUP_ICON_LIST, 13.0f, false };
    SynthIconButton menuButton { "PresetMenu", YUP_ICON_CARET_DOWN, 12.0f, false };
    SynthIconButton previousButton { "PreviousPreset", YUP_ICON_CHEVRON_LEFT, 11.0f, false };
    SynthIconButton nextButton { "NextPreset", YUP_ICON_CHEVRON_RIGHT, 11.0f, false };
    std::array<FieldEditor, 3> editors;
    std::array<yup::Rectangle<float>, 3> captionAreas;
    yup::Rectangle<float> nameArea;
    SynthPatch::PresetInfo shownInfo;
};
