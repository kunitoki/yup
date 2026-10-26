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

#include "SynthPresetBrowser.h"
#include "SynthPresetLibrary.h"
#include "SynthTheme.h"

SynthPresetBrowser::SynthPresetBrowser (RetraceProcessor& processorToControl)
    : sources (processorToControl, processorToControl.getPresetLibrary())
    , filters (processorToControl.getPresetBrowser())
    , list (processorToControl)
    , info (processorToControl)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    // The lists, the tree and the fields sit in wells painted like the waveform display, so their own boxes go.
    for (const auto& colorId : { yup::ListBox::Style::backgroundColorId, yup::ListBox::Style::outlineColorId, yup::TextEditor::Style::backgroundColorId,
                                 yup::TextEditor::Style::outlineColorId, yup::TextEditor::Style::focusedOutlineColorId })
        setColor (colorId, yup::Colors::transparentBlack);

    for (auto* column : std::initializer_list<yup::Component*> { &sources, &filters, &list, &info })
        addAndMakeVisible (*column);
}

void SynthPresetBrowser::syncFromProcessor()
{
    list.syncFromProcessor();
    info.syncFromProcessor();
}

void SynthPresetBrowser::focusList()
{
    list.focusList();
}

void SynthPresetBrowser::resized()
{
    auto bounds = getLocalBounds().reduced (SynthTheme::panelPadding);
    auto left = bounds.removeFromLeft (leftColumnWidth);
    bounds.removeFromLeft (SynthTheme::panelPadding);
    info.setBounds (bounds.removeFromRight (rightColumnWidth));
    bounds.removeFromRight (SynthTheme::panelPadding);
    list.setBounds (bounds);

    sources.setBounds (left.removeFromTop (std::round (left.getHeight() * sourcesShare)));
    left.removeFromTop (SynthTheme::panelPadding);
    filters.setBounds (left);
}

void SynthPresetBrowser::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    g.setFillColor (SynthTheme::frameColor (*this));
    g.fillRect (bounds);
    SynthTheme::paintTile (g, *this, bounds);
}
