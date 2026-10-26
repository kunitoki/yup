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
#include <functional>
#include <memory>

class PresetBrowseState;

//==============================================================================
/** The middle column of the preset browser: the search, the sort and the rescan above
    a list of the presets with their favorite star, name, category, author, location,
    date and rating.

    One click on a row loads its preset and the up and down keys load as they move; a
    click on the star or on the rating only marks the preset. The row of the loaded
    preset stays highlighted whatever loaded it, and a search, a filter or a rescan never
    loads anything.

    @see SynthPresetBrowser, PresetBrowseState
*/
class SynthPresetList
    : public yup::Component
    , private yup::ListBoxModel
{
public:
    explicit SynthPresetList (RetraceProcessor& processorToControl);
    ~SynthPresetList() override;

    /** Follows the preset the processor has loaded. Call at the display rate. */
    void syncFromProcessor();

    /** Gives the list the keyboard focus, so the arrows move through it. */
    void focusList();

    /** Called on a right click on a row (row >= 0) or the list background (-1), with the
        position in this component.
    */
    std::function<void (int row, yup::Point<float> position)> onMenuRequested;

    /** Splits a row into its columns: favorite, name, category, author, location,
        modified and rating, in the order of PresetQuery::SortKey.
    */
    static std::array<yup::Rectangle<float>, 7> layoutColumns (yup::Rectangle<float> row);

    void resized() override;
    void paint (yup::Graphics& g) override;
    void paintOverChildren (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    class Rows;
    class Row;

    static constexpr float topBarHeight = 30.0f;
    static constexpr float columnHeaderHeight = 22.0f;
    static constexpr float rowHeight = 24.0f;

    int getNumRows() override;
    void refreshRowComponent (int row, bool isSelected, std::unique_ptr<yup::Component>& component) override;

    bool handleRowPress (int row, const yup::MouseEvent& event);
    void loadRow (int row);
    void showLoadedPreset();
    void sortBy (int column);
    void showSortMenu();
    void updateSortButton();

    RetraceProcessor& processor;
    PresetBrowseState& browser;
    SynthFieldEditor searchField;
    yup::TextButton sortButton;
    SynthIconButton rescanButton { "RescanPresets", YUP_ICON_ARROWS_ROTATE, 12.0f };
    std::unique_ptr<Rows> rows;
    yup::Rectangle<float> searchArea;
    yup::Rectangle<float> listArea;
    yup::Rectangle<float> columnHeader;
    yup::String shownKey;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthPresetList)
};
