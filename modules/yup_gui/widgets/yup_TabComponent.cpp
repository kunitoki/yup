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

namespace yup
{

//==============================================================================
TabComponent::TabComponent (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);

    tabBar.addListener (this);
    addAndMakeVisible (tabBar);
}

TabComponent::~TabComponent()
{
    tabBar.removeListener (this);

    for (auto& page : pages)
        removeChildComponent (*page.content);
}

//==============================================================================
TabButton& TabComponent::addTab (const Identifier& tabId, const String& text, std::unique_ptr<Component> content, int insertIndex)
{
    jassert (content != nullptr);

    auto& page = *content;
    return addPage (tabId, text, page, std::move (content), insertIndex);
}

TabButton& TabComponent::addTab (const Identifier& tabId, const String& text, Component& content, int insertIndex)
{
    return addPage (tabId, text, content, nullptr, insertIndex);
}

TabButton& TabComponent::addPage (const Identifier& tabId, const String& text, Component& content, std::unique_ptr<Component> ownedContent, int insertIndex)
{
    if (auto* existing = tabBar.getTabButton (tabId))
    {
        jassertfalse; // Tab identifiers must be unique
        return *existing;
    }

    // The page must exist before the bar is told, as adding the first tab selects it.
    addChildComponent (content);
    content.setBounds (getContentArea());
    pages.push_back ({ tabId, &content, std::move (ownedContent) });

    auto& button = tabBar.addTab (tabId, text, insertIndex);
    updatePageVisibility();
    return button;
}

void TabComponent::removeTab (const Identifier& tabId)
{
    tabBar.removeTab (tabId);
}

Component* TabComponent::getTabContent (const Identifier& tabId) const
{
    for (const auto& page : pages)
    {
        if (page.tabId == tabId)
            return page.content;
    }

    return nullptr;
}

//==============================================================================
void TabComponent::setTabBarPlacement (Placement newPlacement)
{
    placement = newPlacement;

    const auto vertical = placement == Placement::left || placement == Placement::right;
    tabBar.setOrientation (vertical ? TabBar::Orientation::vertical : TabBar::Orientation::horizontal);
    tabBar.setFlipped (placement == Placement::bottom || placement == Placement::right);

    resized();
}

void TabComponent::setTabBarThickness (float newThickness)
{
    tabBarThickness = jmax (0.0f, newThickness);
    resized();
}

//==============================================================================
void TabComponent::resized()
{
    auto bounds = getLocalBounds();

    switch (placement)
    {
        case Placement::top:
            tabBar.setBounds (bounds.removeFromTop (tabBarThickness));
            break;

        case Placement::bottom:
            tabBar.setBounds (bounds.removeFromBottom (tabBarThickness));
            break;

        case Placement::left:
            tabBar.setBounds (bounds.removeFromLeft (tabBarThickness));
            break;

        case Placement::right:
            tabBar.setBounds (bounds.removeFromRight (tabBarThickness));
            break;
    }

    for (auto& page : pages)
        page.content->setBounds (bounds);
}

Rectangle<float> TabComponent::getContentArea() const
{
    auto bounds = getLocalBounds();

    switch (placement)
    {
        case Placement::top:
            return bounds.withTrimmedTop (tabBarThickness);

        case Placement::bottom:
            return bounds.withTrimmedBottom (tabBarThickness);

        case Placement::left:
            return bounds.withTrimmedLeft (tabBarThickness);

        case Placement::right:
            return bounds.withTrimmedRight (tabBarThickness);
    }

    return bounds;
}

void TabComponent::updatePageVisibility()
{
    const auto selectedTabId = tabBar.getSelectedTabId();

    for (auto& page : pages)
        page.content->setVisible (page.tabId == selectedTabId);
}

//==============================================================================
void TabComponent::tabSelectionChanged (TabBar& bar, const Identifier& tabId)
{
    ignoreUnused (bar, tabId);
    updatePageVisibility();
}

void TabComponent::tabRemoved (TabBar& bar, const Identifier& tabId)
{
    ignoreUnused (bar);

    const auto it = std::find_if (pages.begin(), pages.end(), [&tabId] (const Page& page)
    {
        return page.tabId == tabId;
    });

    if (it == pages.end())
        return;

    removeChildComponent (*it->content);
    pages.erase (it);
}

} // namespace yup
