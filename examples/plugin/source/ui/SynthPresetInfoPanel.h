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
#include "SynthFieldEditor.h"
#include "SynthIconButton.h"

#include <yup_gui/yup_gui.h>

#include <array>

//==============================================================================
/** The right column of the preset browser: the loaded preset's name, category, author,
    bank, description, notes and tags.

    A user preset's fields are edited in place: a finished field is written into the
    preset file at once, without the sound, and a new name renames the file. A factory
    preset's fields, and those of a patch that is not a listed preset, are read only.
    The category offers the categories found in the library.

    @see SynthPresetBrowser, PresetFileOperations
*/
class SynthPresetInfoPanel : public yup::Component
{
public:
    explicit SynthPresetInfoPanel (RetraceProcessor& processorToControl);

    /** Shows the loaded preset. Call at the display rate. */
    void syncFromProcessor();

    void resized() override;
    void paint (yup::Graphics& g) override;
    void paintOverChildren (yup::Graphics& g) override;

private:
    enum Field
    {
        name,
        category,
        author,
        bank,
        description,
        notes,
        tags,
        fieldCount
    };

    static constexpr float captionHeight = 14.0f;
    static constexpr float fieldHeight = 24.0f;
    static constexpr float textHeight = 56.0f;

    static yup::String getValue (const PresetEntry& entry, Field field);
    void showEntry (const PresetEntry* entry);
    void finishField (Field field);
    void showCategoryMenu();

    RetraceProcessor& processor;
    std::array<SynthFieldEditor, fieldCount> editors;
    std::array<yup::Rectangle<float>, fieldCount> captionAreas;
    std::array<yup::Rectangle<float>, fieldCount> wellAreas;
    std::array<yup::String, fieldCount> shownValues;
    SynthIconButton categoryButton { "PresetCategories", YUP_ICON_CARET_DOWN, 11.0f, false };
    yup::String shownKey;
    bool hasEntry = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetInfoPanel)
};
