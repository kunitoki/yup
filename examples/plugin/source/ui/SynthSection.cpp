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
#include "SynthSection.h"
#include "SynthTheme.h"

SynthSection::SynthSection (std::vector<Tab> tabsToShow, SynthTheme::Section section, std::vector<StripControl> sharedStripControls)
    : tabs (std::move (tabsToShow))
    , sharedControls (std::move (sharedStripControls))
{
    setOpaque (false);
    SynthTheme::applySection (*this, section);

    const auto font = SynthTheme::captionFont (13.5f);

    for (auto& tab : tabs)
    {
        auto led = (tab.enable != nullptr || tab.lockedOn) ? std::make_unique<ParameterLed> (tab.enable) : nullptr;

        if (led != nullptr)
            addAndMakeVisible (*led);

        tabWidths.push_back (SynthTheme::textWidth (tab.name.toUpperCase(), font) + tabPadding * 2.0f + (led != nullptr ? ledSpace : 0.0f));
        leds.push_back (std::move (led));

        addChildComponent (*tab.page);

        for (const auto& control : tab.stripControls)
            addChildComponent (*control.component);
    }

    for (const auto& control : sharedControls)
        addAndMakeVisible (*control.component);

    shownOn.assign (tabs.size(), true);
    selectTab (0);
    syncFromParameters();
}

void SynthSection::selectTab (int index)
{
    selected = yup::jlimit (0, static_cast<int> (tabs.size()) - 1, index);

    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        const auto shown = tab == selected;
        tabs[static_cast<std::size_t> (tab)].page->setVisible (shown);

        for (const auto& control : tabs[static_cast<std::size_t> (tab)].stripControls)
            control.component->setVisible (shown);
    }

    resized();
    repaint();
}

bool SynthSection::isTabOn (int index) const
{
    const auto& led = leds[static_cast<std::size_t> (index)];
    return led == nullptr || led->isOn();
}

void SynthSection::updatePageLook()
{
    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        auto* page = tabs[static_cast<std::size_t> (tab)].page;
        page->setOpacity (shownOn[static_cast<std::size_t> (tab)] ? 1.0f : 0.5f);
        page->repaint();
    }
}

bool SynthSection::syncFromParameters()
{
    auto changed = false;

    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        if (auto& led = leds[static_cast<std::size_t> (tab)]; led != nullptr)
            led->syncFromParameter();

        const auto on = isTabOn (tab);

        if (on == shownOn[static_cast<std::size_t> (tab)])
            continue;

        shownOn[static_cast<std::size_t> (tab)] = on;
        changed = true;
    }

    if (changed)
    {
        updatePageLook();
        repaint();
    }

    return changed;
}

yup::Rectangle<float> SynthSection::getTabBounds (int index) const
{
    auto x = 0.0f;

    for (int tab = 0; tab < index; ++tab)
        x += tabWidths[static_cast<std::size_t> (tab)];

    return { x, 0.0f, tabWidths[static_cast<std::size_t> (index)], SynthTheme::stripHeight };
}

void SynthSection::resized()
{
    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        if (auto& led = leds[static_cast<std::size_t> (tab)]; led != nullptr)
            led->setBounds (getTabBounds (tab).withTrimmedLeft (tabPadding).withWidth (ledSpace - 4.0f));
    }

    auto strip = getLocalBounds().removeFromTop (SynthTheme::stripHeight);

    const auto place = [&strip] (const StripControl& control)
    {
        auto area = strip.removeFromRight (control.width + SynthTheme::panelPadding * 2.0f);
        control.component->setBounds (area.reduced (SynthTheme::panelPadding, 4.0f));
    };

    for (const auto& control : sharedControls)
        place (control);

    for (const auto& control : tabs[static_cast<std::size_t> (selected)].stripControls)
        place (control);

    const auto body = getLocalBounds().withTrimmedTop (SynthTheme::stripHeight).reduced (SynthTheme::panelPadding);

    for (auto& tab : tabs)
        tab.page->setBounds (body);

    updatePageLook();
}

void SynthSection::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintPanel (g, *this, bounds);

    const auto strip = bounds.withHeight (SynthTheme::stripHeight);
    const auto corner = SynthTheme::panelCorner;

    g.setFillColor (SynthTheme::stripColor (*this));
    g.fillRoundedRect (strip, corner, corner, 0.0f, 0.0f);

    g.setFillColor (yup::Colors::black.withAlpha (SynthTheme::isDark() ? 0.6f : 0.2f));
    g.fillRect (strip.getX(), strip.getBottom() - 1.0f, strip.getWidth(), 1.0f);

    const auto font = SynthTheme::captionFont (13.5f);

    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        const auto area = getTabBounds (tab);
        const auto isSelected = tab == selected;

        if (isSelected)
        {
            g.setFillColor (SynthTheme::panelColor (*this));
            g.fillRoundedRect (area, tab == 0 ? corner : 0.0f, 0.0f, 0.0f, 0.0f);
            g.setFillColor (shownOn[static_cast<std::size_t> (tab)] ? SynthTheme::accent (*this) : SynthTheme::textSecondary (*this));
            g.fillRoundedRect (area.withHeight (2.0f), tab == 0 ? corner : 0.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            SynthTheme::paintSeam (g, { area.getRight() - 1.0f, area.getY() + 1.0f }, { area.getRight() - 1.0f, area.getBottom() - 1.0f });
        }

        auto text = area.withTrimmedLeft (tabPadding).withTrimmedRight (tabPadding);

        if (leds[static_cast<std::size_t> (tab)] != nullptr)
            text.removeFromLeft (ledSpace);

        g.setFillColor (isSelected && shownOn[static_cast<std::size_t> (tab)] ? SynthTheme::textPrimary (*this) : SynthTheme::textSecondary (*this));
        g.fillFittedText (tabs[static_cast<std::size_t> (tab)].name.toUpperCase(), font, text, yup::Justification::centerLeft);
    }
}

void SynthSection::mouseDown (const yup::MouseEvent& event)
{
    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        if (getTabBounds (tab).contains (event.getPosition()))
        {
            selectTab (tab);
            return;
        }
    }
}
