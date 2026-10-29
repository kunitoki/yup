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
    // Its own drawing, the chrome around the children, is drawn once and reused until
    // it repaints, since the post effect repaints the whole interface on every change.
    setCachedToTexture (true);

    setOpaque (false);
    SynthTheme::applySection (*this, section);

    for (auto& tab : tabs)
    {
        auto led = (tab.enable != nullptr || tab.lockedOn) ? std::make_unique<ParameterLed> (tab.enable) : nullptr;

        if (led != nullptr)
            addAndMakeVisible (*led);

        tabWidths.push_back (0.0f);
        leds.push_back (std::move (led));

        if (tab.trailingControl != nullptr)
            addChildComponent (*tab.trailingControl);

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

        if (auto* control = tabs[static_cast<std::size_t> (tab)].trailingControl; control != nullptr)
            control->setVisible (shown);

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

void SynthSection::syncFromParameters()
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
    auto strip = getLocalBounds().removeFromTop (SynthTheme::stripHeight);

    const auto place = [&strip] (const StripControl& control, float scale)
    {
        auto area = strip.removeFromRight ((control.width + SynthTheme::panelPadding * 2.0f) * scale);
        control.component->setBounds (area.reduced (SynthTheme::panelPadding * scale, 4.0f));
    };

    for (const auto& control : sharedControls)
        place (control, 1.0f);

    // Tabs are laid out before the selected tab's own controls, so they keep their widths
    // whichever tab is selected; those controls shrink into the space left instead
    const auto font = SynthTheme::captionFont (13.5f);
    auto contentWidth = 0.0f;

    for (std::size_t tab = 0; tab < tabs.size(); ++tab)
    {
        tabWidths[tab] = SynthTheme::textWidth (tabs[tab].name.toUpperCase(), font) + (leds[tab] != nullptr ? ledSpace : 0.0f);
        contentWidth += tabWidths[tab];
    }

    // Tight strips shrink the padding first, narrower ones squeeze the tabs and clip their names
    const auto paddingCount = 2.0f * static_cast<float> (tabs.size());
    tabInset = yup::jlimit (minimumTabPadding, tabPadding, (strip.getWidth() - contentWidth) / paddingCount);

    const auto naturalWidth = contentWidth + paddingCount * tabInset;
    const auto scale = naturalWidth > 0.0f ? yup::jlimit (0.0f, 1.0f, strip.getWidth() / naturalWidth) : 1.0f;

    for (int tab = 0; tab < static_cast<int> (tabs.size()); ++tab)
    {
        const auto index = static_cast<std::size_t> (tab);
        tabWidths[index] = (tabWidths[index] + tabInset * 2.0f) * scale;
        auto area = getTabBounds (tab).reduced (tabInset, 4.0f);

        if (auto& led = leds[index]; led != nullptr)
            led->setBounds (area.removeFromLeft (ledSpace).withWidth (ledSpace - 4.0f));
    }

    const auto& selectedTab = tabs[static_cast<std::size_t> (selected)];
    std::vector<StripControl> controls;

    if (selectedTab.trailingControl != nullptr)
        controls.push_back ({ selectedTab.trailingControl, trailingControlWidth });

    controls.insert (controls.end(), selectedTab.stripControls.begin(), selectedTab.stripControls.end());

    auto wantedWidth = 0.0f;

    for (const auto& control : controls)
        wantedWidth += control.width + SynthTheme::panelPadding * 2.0f;

    const auto tabsRight = getTabBounds (static_cast<int> (tabs.size()) - 1).getRight();
    const auto controlScale = wantedWidth > 0.0f ? yup::jlimit (0.0f, 1.0f, (strip.getRight() - tabsRight) / wantedWidth) : 1.0f;

    for (const auto& control : controls)
        place (control, controlScale);

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

    g.setFillColor (yup::Colors::black.withAlpha (SynthTheme::isDark (*this) ? 0.6f : 0.2f));
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
            SynthTheme::paintSeam (g, *this, { area.getRight() - 1.0f, area.getY() + 1.0f }, { area.getRight() - 1.0f, area.getBottom() - 1.0f });
        }

        auto text = area.withTrimmedLeft (tabInset).withTrimmedRight (tabInset);

        if (leds[static_cast<std::size_t> (tab)] != nullptr)
            text.removeFromLeft (ledSpace);

        const auto name = tabs[static_cast<std::size_t> (tab)].name.toUpperCase();
        const auto savedState = g.saveState();
        g.setClipPath (area);

        g.setFillColor (isSelected && shownOn[static_cast<std::size_t> (tab)] ? SynthTheme::textPrimary (*this) : SynthTheme::textSecondary (*this));
        g.fillFittedText (name, font, text.withWidth (yup::jmax (text.getWidth(), SynthTheme::textWidth (name, font))), yup::Justification::centerLeft);
    }
}

void SynthSection::mouseMove (const yup::MouseEvent& event)
{
    auto overTab = false;

    for (int tab = 0; tab < static_cast<int> (tabs.size()) && ! overTab; ++tab)
        overTab = getTabBounds (tab).contains (event.getPosition());

    setMouseCursor (overTab ? yup::MouseCursor::Hand : yup::MouseCursor::Default);
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
