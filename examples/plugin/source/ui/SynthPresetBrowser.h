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
#include "SynthPresetFilters.h"
#include "SynthPresetInfoPanel.h"
#include "SynthPresetList.h"
#include "SynthPresetSources.h"

#include <yup_gui/yup_gui.h>

//==============================================================================
/** The preset browser, shown over the instrument's panels while the header and the
    keyboard stay playable: the sources and filters on the left, the preset list in the
    middle and the selected preset's details on the right.

    A right click on a user preset renames, duplicates, deletes or reveals it; on the list
    background or a folder it saves the current patch there. Factory presets offer the
    same items disabled.

    @see SynthPresetList, SynthInterface
*/
class SynthPresetBrowser : public yup::Component
{
public:
    explicit SynthPresetBrowser (RetraceProcessor& processorToControl);

    /** Follows the preset the processor has loaded. Call at the display rate. */
    void syncFromProcessor();

    /** Gives the preset list the keyboard focus. */
    void focusList();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float leftColumnWidth = 180.0f;
    static constexpr float rightColumnWidth = 200.0f;
    static constexpr float sourcesShare = 0.6f;

    void showRowMenu (int row, yup::Point<float> position);
    void renamePreset (const PresetEntry& entry);
    void duplicatePreset (const PresetEntry& entry);
    void deletePreset (const PresetEntry& entry);
    void saveIntoFolder (const yup::File& folder);

    RetraceProcessor& processor;
    SynthPresetSources sources;
    SynthPresetFilters filters;
    SynthPresetList list;
    SynthPresetInfoPanel info;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetBrowser)
};
