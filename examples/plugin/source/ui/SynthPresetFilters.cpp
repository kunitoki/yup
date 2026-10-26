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

#include "SynthPresetFilters.h"
#include "SynthTheme.h"
#include "../browser/PresetBrowseState.h"

#include <algorithm>
#include <tuple>

namespace
{
constexpr float chipPadding = 8.0f;
constexpr float starWidth = 16.0f;

yup::Font getChipFont()
{
    return SynthTheme::valueFont (11.5f);
}
} // namespace

SynthPresetFilters::SynthPresetFilters (PresetBrowseState& stateToFilter)
    : browser (stateToFilter)
{
    setOpaque (false);
    browser.getLibrary().addChangeListener (this);
    refreshChips();
}

SynthPresetFilters::~SynthPresetFilters()
{
    browser.getLibrary().removeChangeListener (this);
}

void SynthPresetFilters::changeListenerCallback (yup::ChangeBroadcaster*)
{
    refreshChips();
}

void SynthPresetFilters::refreshChips()
{
    const auto& entries = browser.getLibrary().getEntries();
    auto texts = showingTags ? PresetQuery::collectTags (entries) : PresetQuery::collectCategories (entries);

    // A selected value no preset carries any more keeps its chip, so it can still be turned off.
    const auto& query = browser.getQuery();

    for (const auto& selected : showingTags ? query.tags : query.categories)
        texts.addIfNotAlreadyThere (selected, true);

    texts.sortNatural();

    chips.clear();

    for (const auto& text : texts)
        chips.push_back ({ text, {} });

    layoutChips();
    repaint();
}

void SynthPresetFilters::layoutChips()
{
    const auto width = chipArea.getWidth();
    const auto font = getChipFont();
    float x = 0.0f;
    float y = 0.0f;

    for (auto& chip : chips)
    {
        const auto chipWidth = std::min (width, SynthTheme::textWidth (chip.text, font) + chipPadding * 2.0f);

        if (x > 0.0f && x + chipWidth > width)
        {
            x = 0.0f;
            y += chipHeight + chipGap;
        }

        chip.bounds = { x, y, chipWidth, chipHeight };
        x += chipWidth + chipGap;
    }

    chipsHeight = chips.empty() ? 0.0f : y + chipHeight;
    scrollOffset = yup::jlimit (0.0f, std::max (0.0f, chipsHeight - chipArea.getHeight()), scrollOffset);
}

void SynthPresetFilters::resized()
{
    auto bounds = getLocalBounds();
    auto tabs = bounds.removeFromTop (tabHeight);
    categoriesTab = tabs.removeFromLeft (tabs.getWidth() * 0.5f);
    tagsTab = tabs;

    bounds.removeFromTop (SynthTheme::controlGap);
    ratingArea = bounds.removeFromBottom (ratingHeight);
    bounds.removeFromBottom (SynthTheme::controlGap);
    chipArea = bounds;

    layoutChips();
}

void SynthPresetFilters::paint (yup::Graphics& g)
{
    const auto accent = SynthTheme::accent (*this);
    const auto secondary = SynthTheme::textSecondary (*this);
    const auto captionFont = SynthTheme::captionFont (10.0f);

    for (const auto& [tab, isActive, title] : { std::tuple { categoriesTab, ! showingTags, "CATEGORIES" }, std::tuple { tagsTab, showingTags, "TAGS" } })
    {
        g.setFillColor (isActive ? accent : secondary);
        g.fillFittedText (title, captionFont, tab, yup::Justification::center);

        if (isActive)
            g.fillRect (tab.getX(), tab.getBottom() - 2.0f, tab.getWidth(), 2.0f);
    }

    {
        const auto savedState = g.saveState();
        g.setClipPath (chipArea);

        const auto font = getChipFont();

        for (const auto& chip : chips)
        {
            const auto bounds = chip.bounds.withPosition (chip.bounds.getX() + chipArea.getX(), chip.bounds.getY() + chipArea.getY() - scrollOffset);

            if (bounds.getBottom() < chipArea.getY() || bounds.getY() > chipArea.getBottom())
                continue;

            const auto on = isChipOn (chip.text);
            g.setFillColor (on ? accent : SynthTheme::wellColor (*this));
            g.fillRoundedRect (bounds, chipHeight * 0.5f);

            g.setFillColor (on ? SynthTheme::onAccent (*this) : SynthTheme::textPrimary (*this));
            g.fillFittedText (chip.text, font, bounds.reduced (chipPadding * 0.5f, 0.0f), yup::Justification::center);
        }
    }

    g.setFillColor (secondary);
    g.fillFittedText ("MIN RATING", captionFont, ratingArea, yup::Justification::centerLeft);

    const auto star = yup::String::fromUTF8 (YUP_ICON_STAR);
    const auto iconFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (10.0f);
    const auto minimumRating = browser.getQuery().minimumRating;

    for (int index = 0; index < 5; ++index)
    {
        g.setFillColor (index < minimumRating ? accent : secondary.withAlpha (0.35f));
        g.fillFittedText (star, iconFont, getStarArea (index), yup::Justification::center);
    }
}

void SynthPresetFilters::mouseDown (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();

    if (categoriesTab.contains (position) || tagsTab.contains (position))
    {
        showingTags = tagsTab.contains (position);
        scrollOffset = 0.0f;
        refreshChips();
        return;
    }

    for (int index = 0; index < 5; ++index)
    {
        if (getStarArea (index).contains (position))
        {
            auto query = browser.getQuery();
            query.minimumRating = query.minimumRating == index + 1 ? 0 : index + 1;
            browser.setQuery (std::move (query));
            repaint();
            return;
        }
    }

    if (! chipArea.contains (position))
        return;

    const auto chipPosition = position - chipArea.getPosition() + yup::Point<float> (0.0f, scrollOffset);

    for (const auto& chip : chips)
    {
        if (chip.bounds.contains (chipPosition))
        {
            toggleChip (chip.text);
            return;
        }
    }
}

void SynthPresetFilters::mouseWheel (const yup::MouseEvent&, const yup::MouseWheelData& wheelData)
{
    const auto maximum = std::max (0.0f, chipsHeight - chipArea.getHeight());

    if (maximum <= 0.0f)
        return;

    scrollOffset = yup::jlimit (0.0f, maximum, scrollOffset - wheelData.getDeltaY() * chipHeight * 3.0f);
    repaint();
}

bool SynthPresetFilters::isChipOn (const yup::String& text) const
{
    const auto& query = browser.getQuery();
    return (showingTags ? query.tags : query.categories).contains (text, true);
}

void SynthPresetFilters::toggleChip (const yup::String& text)
{
    auto query = browser.getQuery();
    auto& selected = showingTags ? query.tags : query.categories;

    if (selected.contains (text, true))
        selected.removeString (text, true);
    else
        selected.add (text);

    browser.setQuery (std::move (query));
    refreshChips();
}

yup::Rectangle<float> SynthPresetFilters::getStarArea (int star) const
{
    return { ratingArea.getRight() - starWidth * static_cast<float> (5 - star), ratingArea.getY(), starWidth, ratingArea.getHeight() };
}
