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
#include "ParameterKnob.h"
#include "SynthIconButton.h"
#include "SynthLevelMeter.h"
#include "SynthPresetBar.h"

#include <yup_gui/yup_gui.h>

#include <functional>

//==============================================================================
/** The header of the instrument, a row of tiles: the logo, the preset with its
    description, the file, random and panic buttons, the voice and load
    readouts, and the master volume with its meter.

    @see SynthPresetBar, SynthInterface
*/
class SynthHeader : public yup::Component
{
public:
    explicit SynthHeader (PrismProcessor& processorToControl);

    /** Called when the preset description stops being edited. */
    std::function<void()> onEditingFinished;

    /** Called when the logo is right clicked, to show the global settings. */
    std::function<void()> onSettingsRequested;

    /** Shows the processor's preset, voices, load and output level. Call at the display rate. */
    void syncFromProcessor();

    /** Sets the width of the tiles right of the preset, so they line up with the column below. */
    void setRightGroupWidth (float width);

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    static constexpr float logoWidth = 214.0f;
    static constexpr float iconTileSize = 46.0f;
    static constexpr float volumeWidth = 120.0f;

    PrismProcessor& processor;
    yup::Drawable logo;
    yup::Rectangle<float> logoTile;
    yup::Rectangle<float> logoArea;
    yup::Rectangle<float> statsTile;
    yup::Rectangle<float> volumeTile;
    SynthPresetBar presetBar;
    SynthIconButton saveButton { "SavePreset", YUP_ICON_FLOPPY_DISK };
    SynthIconButton loadButton { "LoadPreset", YUP_ICON_FOLDER_OPEN };
    SynthIconButton randomButton { "Randomize", YUP_ICON_DICE };
    SynthIconButton panicButton { "AllNotesOff", YUP_ICON_SKULL };
    ParameterKnob volumeKnob;
    SynthLevelMeter meter;
    float rightGroupWidth = 381.0f;
    yup::String voices;
    yup::String load;
    yup::String xruns;
};
