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

class TabBar;

//==============================================================================
/** A single tab of a TabBar.

    Tabs are created and owned by a TabBar, which also handles their selection, layout, dragging
    and keyboard navigation: a TabButton only holds what the tab shows. Get one from
    TabBar::addTab() or TabBar::getTabButton() and configure its content from there.

    A tab shows an optional icon (a glyph from the theme icon font, or an image), a text and an
    optional close button. A custom component can replace the icon and the text. A tab with an
    icon and no text is an icon-only tab.

    @code
    auto& tab = tabBar.addTab ("table", "Table");
    tab.setIconGlyph (YUP_ICON_TABLE);
    tab.setClosable (true);
    @endcode

    @see TabBar, TabComponent
*/
class YUP_API TabButton : public Button
{
public:
    //==============================================================================
    /** Creates a tab for a bar. Tabs are normally created by TabBar::addTab().

        @param owner   The bar the tab belongs to.
        @param tabId   The stable identifier of the tab.
    */
    TabButton (TabBar& owner, const Identifier& tabId);

    /** Destructor. */
    ~TabButton() override;

    //==============================================================================
    /** Returns the identifier of the tab. */
    const Identifier& getTabId() const noexcept { return tabId; }

    /** Returns the bar the tab belongs to. */
    TabBar& getTabBar() const noexcept { return owner; }

    //==============================================================================
    /** Sets the text shown by the tab.

        @param newText  The text, or an empty string for an icon-only tab.
    */
    void setText (const String& newText);

    /** Returns the text shown by the tab. */
    const String& getText() const noexcept { return text; }

    //==============================================================================
    /** Shows a glyph from the theme icon font as the icon, replacing any icon image.

        @param newGlyph  A glyph, or an empty string for no icon.
    */
    void setIconGlyph (const String& newGlyph);

    /** Shows a glyph from the theme icon font as the icon, replacing any icon image.

        @param utf8Glyph  A UTF-8 glyph such as YUP_ICON_TABLE, or nullptr for no icon.
    */
    void setIconGlyph (const char* utf8Glyph);

    /** Returns the icon glyph, or an empty string when there is none. */
    const String& getIconGlyph() const noexcept { return iconGlyph; }

    /** Shows an image as the icon, replacing any icon glyph.

        @param newImage  The image, or an invalid Image for no icon.
    */
    void setIconImage (const Image& newImage);

    /** Returns the icon image, or an invalid Image when there is none. */
    const Image& getIconImage() const noexcept { return iconImage; }

    /** Returns true if the tab shows an icon glyph or an icon image. */
    bool hasIcon() const noexcept;

    //==============================================================================
    /** Shows or hides a close button on the tab.

        Clicking the close button never selects the tab: it calls TabBar::onTabCloseRequested,
        or removes the tab when that callback is not set.

        @param shouldBeClosable  True to show the close button.
    */
    void setClosable (bool shouldBeClosable);

    /** Returns true if the tab shows a close button. */
    bool isClosable() const noexcept { return closable; }

    //==============================================================================
    /** Replaces the icon and the text with a custom component.

        The tab takes ownership of the component. The component does not receive mouse events,
        so clicking and dragging still select and move the tab. Its size when it is set is used as
        its preferred size, and it is centered in the tab.

        @param newComponent  The component to show, or nullptr to go back to the icon and text.
    */
    void setCustomComponent (std::unique_ptr<Component> newComponent);

    /** Returns the custom component, or nullptr when there is none. */
    Component* getCustomComponent() const noexcept { return customComponent.get(); }

    //==============================================================================
    /** Returns true if this is the selected tab of its bar.

        The overflow button of a bar reports itself as selected when it stands in for a selected
        tab that does not fit in the bar.
    */
    bool isSelected() const noexcept { return selected; }

    /** Returns true if the mouse is over the close button. */
    bool isCloseButtonOver() const noexcept { return closeButtonOver; }

    /** Returns true if this is the button a bar shows for the tabs that do not fit.

        @see TabBar::Overflow
    */
    bool isOverflowButton() const noexcept { return overflowButton; }

    //==============================================================================
    /** Returns the length the tab would like along its bar.

        For a horizontal bar this is the width that fits the content, measured with the selected
        font so the tab keeps its size when the selection changes. For a vertical bar it is the
        height of a row.
    */
    float getPreferredLength() const;

    /** Returns the font of the text, based on the theme default font.

        @param forSelectedTab  True to get the heavier font of the selected tab.
    */
    Font getFont (bool forSelectedTab) const;

    //==============================================================================
    /** Returns the area of the icon, or an empty rectangle when there is none. */
    Rectangle<float> getIconBounds() const noexcept { return iconBounds; }

    /** Returns the area of the text, or an empty rectangle when there is none. */
    Rectangle<float> getTextBounds() const noexcept { return textBounds; }

    /** Returns the area of the close button, or an empty rectangle when the tab is not closable. */
    Rectangle<float> getCloseButtonBounds() const noexcept { return closeButtonBounds; }

    /** Returns the area of the dropdown arrow of an overflow button, or an empty rectangle otherwise. */
    Rectangle<float> getArrowBounds() const noexcept { return arrowBounds; }

    //==============================================================================
    /** Color identifiers used by the tab. */
    struct Style
    {
        static inline const Identifier textColorId { "tabButtonText" };
        static inline const Identifier textSelectedColorId { "tabButtonTextSelected" };
        static inline const Identifier hoveredBackgroundColorId { "tabButtonHoveredBackground" };
        static inline const Identifier closeButtonColorId { "tabButtonCloseButton" };
    };

    //==============================================================================
    /** @internal */
    void paintButton (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void enablementChanged() override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;

private:
    friend class TabBar;

    void setSelected (bool shouldBeSelected);
    void contentChanged();
    void updateLayout();
    float getTextWidth() const;

    TabBar& owner;
    Identifier tabId;
    String text;
    String iconGlyph;
    Image iconImage;
    std::unique_ptr<Component> customComponent;
    Size<float> customComponentSize;
    bool closable = false;
    bool selected = false;
    bool closeButtonOver = false;
    bool overflowButton = false;

    Rectangle<float> iconBounds;
    Rectangle<float> textBounds;
    Rectangle<float> closeButtonBounds;
    Rectangle<float> arrowBounds;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabButton)
};

} // namespace yup
