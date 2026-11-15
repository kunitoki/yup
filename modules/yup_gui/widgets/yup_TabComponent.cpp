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

namespace
{

struct TabComponentIds
{
    static inline const Identifier placement { "placement" };
    static inline const Identifier tabBarThickness { "tabBarThickness" };
    static inline const Identifier selectedTab { "selectedTab" };
    static inline const Identifier title { "title" };
};

} // namespace

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
    {
        page.content->removeComponentListener (this);
        removeChildComponent (*page.content);
    }
}

//==============================================================================
const ComponentTypeInfo& TabComponent::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        static const StringArray placementNames { "top", "bottom", "left", "right" };

        props.push_back (ComponentProperty {
            .name = TabComponentIds::placement,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = placementNames[0],
            .getter = [] (const Component& c) { return var (placementNames[static_cast<int> (static_cast<const TabComponent&> (c).getTabBarPlacement())]); },
            .setter = [] (Component& c, const var& v)
            {
                if (const auto index = placementNames.indexOf (v.toString()); index >= 0)
                    static_cast<TabComponent&> (c).setTabBarPlacement (static_cast<Placement> (index));
            },
            .isStyleable = false,
            .enumValues = placementNames,
            .description = "Where the bar sits around the content." });

        props.push_back (ComponentProperty {
            .name = TabComponentIds::tabBarThickness,
            .type = ComponentProperty::Type::number,
            .defaultValue = 36.0,
            .getter = [] (const Component& c) { return var (static_cast<double> (static_cast<const TabComponent&> (c).getTabBarThickness())); },
            .setter = [] (Component& c, const var& v) { static_cast<TabComponent&> (c).setTabBarThickness (static_cast<float> (static_cast<double> (v))); },
            .isStyleable = false,
            .description = "The thickness of the bar across its tabs." });

        props.push_back (ComponentProperty {
            .name = TabComponentIds::selectedTab,
            .type = ComponentProperty::Type::string,
            .getter = [] (const Component& c) { return var (static_cast<const TabComponent&> (c).getTabBar().getSelectedTabId().toString()); },
            .setter = [] (Component& c, const var& v) { static_cast<TabComponent&> (c).getTabBar().setSelectedTab (Identifier (v.toString())); },
            .isStyleable = false,
            .description = "The id of the selected tab." });

        ComponentSignal tabChanged {
            .name = Identifier ("onTabChanged"),
            .connect = [] (Component& component, ComponentSignal::Handler handler)
            {
                if (auto* tabs = dynamic_cast<TabComponent*> (&component))
                    tabs->getTabBar().onSelectionChanged = [handler = std::move (handler)] (const Identifier& tabId)
                    {
                        handler ({ var (tabId.toString()) });
                    };
            },
            .description = "Emitted with its id when another tab is selected",
            .arguments = { { Identifier ("tab"), ComponentProperty::Type::string } }
        };

        auto attachChild = [] (Component& parent, Component& child)
        {
            auto& tabs = static_cast<TabComponent&> (parent);
            auto tabId = child.getComponentID();

            if (tabId.isEmpty())
                tabId = "tab" + String (tabs.pages.size() + 1);

            tabs.addTab (Identifier (tabId), child.getTitle(), child);
        };

        return ComponentTypeInfo { Identifier ("TabComponent"), &Component::getClassTypeInfo(), std::move (props), { std::move (tabChanged) }, [] { return std::make_unique<TabComponent>(); }, std::move (attachChild) };
    }();

    return info;
}

const ComponentTypeInfo& TabComponent::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
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
    content.addComponentListener (this);
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
    if (placement == newPlacement)
        return;

    placement = newPlacement;

    const auto vertical = placement == Placement::left || placement == Placement::right;
    tabBar.setOrientation (vertical ? TabBar::Orientation::vertical : TabBar::Orientation::horizontal);
    tabBar.setFlipped (placement == Placement::bottom || placement == Placement::right);

    resized();
    notifyPropertyChanged (TabComponentIds::placement);
}

void TabComponent::setTabBarThickness (float newThickness)
{
    const auto clamped = jmax (0.0f, newThickness);
    if (approximatelyEqual (tabBarThickness, clamped))
        return;

    tabBarThickness = clamped;
    resized();
    notifyPropertyChanged (TabComponentIds::tabBarThickness);
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
    notifyPropertyChanged (TabComponentIds::selectedTab);
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

    it->content->removeComponentListener (this);
    removeChildComponent (*it->content);
    pages.erase (it);
}

void TabComponent::componentPropertyChanged (Component& component, const Identifier& name)
{
    if (name != TabComponentIds::title)
        return;

    for (const auto& page : pages)
    {
        if (page.content != &component)
            continue;

        if (auto* button = tabBar.getTabButton (page.tabId))
            button->setText (component.getTitle());

        return;
    }
}

} // namespace yup
