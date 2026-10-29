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

#include <vector>

class PresetBrowseState;

//==============================================================================
/** The filters of the preset browser: a CATEGORIES and a TAGS tab of chips, and a
    minimum rating.

    The chips are the categories and tags found in the library. Several categories list
    the presets in any of them, several tags the presets carrying all of them, and the
    two combine. Clicking a chip toggles it; clicking the minimum rating's star k sets
    it to k, or back to 0 when it already was k.

    @see SynthPresetBrowser, PresetQuery
*/
class SynthPresetFilters
    : public yup::Component
    , private yup::ChangeListener
{
public:
    explicit SynthPresetFilters (PresetBrowseState& stateToFilter);
    ~SynthPresetFilters() override;

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseMove (const yup::MouseEvent& event) override;
    void mouseDown (const yup::MouseEvent& event) override;
    bool mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& wheelData) override;

private:
    struct Chip
    {
        yup::String text;
        yup::Rectangle<float> bounds; /**< In the chip area, before scrolling. */
    };

    static constexpr float tabHeight = 22.0f;
    static constexpr float ratingHeight = 22.0f;
    static constexpr float chipHeight = 20.0f;
    static constexpr float chipGap = 4.0f;

    void changeListenerCallback (yup::ChangeBroadcaster* source) override;
    void refreshChips();
    void layoutChips();
    const Chip* findChipAt (yup::Point<float> position) const;
    bool isChipOn (const yup::String& text) const;
    void toggleChip (const yup::String& text);
    yup::Rectangle<float> getStarArea (int star) const;

    PresetBrowseState& browser;
    bool showingTags = false;
    std::vector<Chip> chips;
    float chipsHeight = 0.0f;
    float scrollOffset = 0.0f;
    yup::Rectangle<float> categoriesTab;
    yup::Rectangle<float> tagsTab;
    yup::Rectangle<float> chipArea;
    yup::Rectangle<float> ratingArea;
    yup::Font captionFont;
    yup::Font chipFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetFilters)
};
