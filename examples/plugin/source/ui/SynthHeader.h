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
#include "ParameterKnob.h"
#include "SynthIconButton.h"
#include "SynthLevelMeter.h"
#include "SynthPresetBar.h"

#include <yup_gui/yup_gui.h>

#include <functional>

//==============================================================================
/** The header of the instrument, a row of tiles: the logo, the preset with its
    description, the file, undo, redo, random and panic buttons, the voice and
    load readouts, and the master volume with its meter.

    @see SynthPresetBar, SynthInterface
*/
class SynthHeader : public yup::Component
{
public:
    explicit SynthHeader (RetraceProcessor& processorToControl);

    /** Called with the click position, in the header, when the logo is right clicked, to show the global settings. */
    std::function<void (yup::Point<float>)> onSettingsRequested;

    /** Called when the preset bar asks to open or close the preset browser. */
    std::function<void()> onBrowserToggled;

    /** Called when the undo or redo button is clicked, with true for redo. */
    std::function<void (bool)> onUndoRequested;

    /** Shows whether the preset browser is open on the preset bar. */
    void setBrowserOpen (bool isOpen) { presetBar.setBrowserOpen (isOpen); }

    /** Shows the processor's preset, undo history, voices, load and output level. Call at the display rate. */
    void syncFromProcessor();

    /** Sets the width of the tiles right of the preset, so they line up with the column below. */
    void setRightGroupWidth (float width);

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void styleChanged() override;

private:
    static constexpr float logoWidth = 190.0f;
    static constexpr float iconTileSize = 46.0f;
    static constexpr float volumeWidth = 120.0f;
    static constexpr int readoutInterval = 8;

    void colorLogo();

    RetraceProcessor& processor;
    yup::Drawable logo;
    bool logoNeedsColors = true;
    yup::Rectangle<float> logoTile;
    yup::Rectangle<float> logoArea;
    yup::Rectangle<float> statsTile;
    yup::Rectangle<float> volumeTile;
    SynthPresetBar presetBar;
    SynthIconButton saveButton { "SavePreset", YUP_ICON_FLOPPY_DISK };
    SynthIconButton loadButton { "LoadPreset", YUP_ICON_FOLDER_OPEN };
    SynthIconButton undoButton { "Undo", YUP_ICON_ARROW_ROTATE_LEFT };
    SynthIconButton redoButton { "Redo", YUP_ICON_ARROW_ROTATE_RIGHT };
    SynthIconButton randomButton { "Randomize", YUP_ICON_DICE };
    SynthIconButton panicButton { "AllNotesOff", YUP_ICON_SKULL };
    ParameterKnob volumeKnob;
    SynthLevelMeter meter;
    float rightGroupWidth = 381.0f;
    int readoutCalls = 0;
    yup::String voices;
    yup::String load;
    yup::String xruns;
    yup::Font captionFont;
    yup::Font valueFont;
};
