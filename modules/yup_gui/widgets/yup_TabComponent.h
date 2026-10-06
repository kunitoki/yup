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

namespace yup
{

//==============================================================================
/** A TabBar with a page of content for each tab.

    Only the page of the selected tab is visible. All pages stay children of the component, so
    they keep their state while hidden. The bar is available from getTabBar() for every setting
    that is not about pages, and pages follow changes made to the bar directly: removing a tab
    from the bar also drops its page.

    @code
    tabs.addTab ("general", "General", std::make_unique<GeneralPage>());
    tabs.addTab ("audio", "Audio", audioPage);
    tabs.getTabBar().setVariant (TabBar::Variant::underline);
    @endcode

    @see TabBar, TabButton
*/
class YUP_API TabComponent
    : public Component
    , private TabBar::Listener
{
public:
    //==============================================================================
    /** Where the bar sits around the content. */
    enum class Placement
    {
        top,    /**< A horizontal bar above the content. */
        bottom, /**< A horizontal bar below the content. */
        left,   /**< A vertical bar left of the content. */
        right   /**< A vertical bar right of the content. */
    };

    //==============================================================================
    /** Creates an empty tab component. */
    TabComponent (StringRef componentID = {});

    /** Destructor. */
    ~TabComponent() override;

    //==============================================================================
    /** Adds a tab that owns its page.

        @param tabId        A unique identifier for the tab.
        @param text         The text shown by the tab.
        @param content      The page, which is deleted when the tab is removed.
        @param insertIndex  Where to insert the tab, or -1 to add it at the end.

        @returns The new tab, to configure its content.
    */
    TabButton& addTab (const Identifier& tabId, const String& text, std::unique_ptr<Component> content, int insertIndex = -1);

    /** Adds a tab showing a page owned elsewhere.

        @param tabId        A unique identifier for the tab.
        @param text         The text shown by the tab.
        @param content      The page, which must outlive the tab.
        @param insertIndex  Where to insert the tab, or -1 to add it at the end.

        @returns The new tab, to configure its content.
    */
    TabButton& addTab (const Identifier& tabId, const String& text, Component& content, int insertIndex = -1);

    /** Removes a tab and its page, deleting the page if the tab owns it.

        @param tabId  The tab to remove.
    */
    void removeTab (const Identifier& tabId);

    /** Returns the page of a tab, or nullptr if there is no such tab. */
    Component* getTabContent (const Identifier& tabId) const;

    /** Returns the bar, to configure it or to change its tabs. */
    TabBar& getTabBar() noexcept { return tabBar; }

    /** Returns the bar. */
    const TabBar& getTabBar() const noexcept { return tabBar; }

    //==============================================================================
    /** Sets where the bar sits around the content. The default is top.

        This also sets the orientation of the bar, and puts the underline on the edge facing the content.

        @param newPlacement  The new placement.
    */
    void setTabBarPlacement (Placement newPlacement);

    /** Returns where the bar sits around the content. */
    Placement getTabBarPlacement() const noexcept { return placement; }

    /** Sets the thickness of the bar across its tabs. The default is 36.

        @param newThickness  The height of a horizontal bar, or the width of a vertical one.
    */
    void setTabBarThickness (float newThickness);

    /** Returns the thickness of the bar across its tabs. */
    float getTabBarThickness() const noexcept { return tabBarThickness; }

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    struct Page
    {
        Identifier tabId;
        Component* content = nullptr;
        std::unique_ptr<Component> ownedContent;
    };

    TabButton& addPage (const Identifier& tabId, const String& text, Component& content, std::unique_ptr<Component> ownedContent, int insertIndex);
    Rectangle<float> getContentArea() const;
    void updatePageVisibility();

    void tabSelectionChanged (TabBar& bar, const Identifier& tabId) override;
    void tabRemoved (TabBar& bar, const Identifier& tabId) override;

    TabBar tabBar;
    std::vector<Page> pages;
    Placement placement = Placement::top;
    float tabBarThickness = 36.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabComponent)
};

} // namespace yup
