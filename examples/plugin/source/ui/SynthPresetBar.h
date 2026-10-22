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

#include <yup_gui/yup_gui.h>

//==============================================================================
/** Picks a factory preset, steps through them, and saves or loads user presets.

    User presets are files holding the same patch the host saves with the session,
    kept by default in the user's application data folder under YUP/Prism/Presets.

    @see PrismProcessor, SynthPatch
*/
class SynthPresetBar : public yup::Component
{
public:
    explicit SynthPresetBar (PrismProcessor& processorToControl);

    /** Shows the preset the processor is on, which the host may have changed. */
    void syncFromProcessor();

    /** Names the patch shown while it does not come from a factory preset. */
    void setCustomPatchName (const yup::String& name);

    void resized() override;

private:
    static constexpr float buttonWidth = 26.0f;
    static constexpr float fileButtonWidth = 52.0f;
    static constexpr float spacing = 4.0f;

    void stepPreset (int delta);
    void savePreset();
    void loadPreset();

    static yup::File getPresetFolder();

    PrismProcessor& processor;

    yup::ComboBox presetCombo;
    yup::TextButton previousButton { "<" };
    yup::TextButton nextButton { ">" };
    yup::TextButton saveButton { "SAVE" };
    yup::TextButton loadButton { "LOAD" };
};
