/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
/**
    A list item component that can display text and an icon.

    This component is used by ListBox when the ListBoxModel's
    refreshComponentForRow() method returns nullptr. It provides a simple
    way to display text and/or an icon (as a Drawable) with configurable layout.

    The icon can be positioned to the left, right, above, or below the text.
    The component integrates with the YUP theme system for styling.

    @see ListBox, ListBoxModel
*/
class YUP_API ListBoxItem : public Component
{
public:
    //==============================================================================
    /** Defines where the icon should be positioned relative to the text. */
    enum class IconPosition
    {
        left,  /**< Icon appears to the left of the text. */
        right, /**< Icon appears to the right of the text. */
        above, /**< Icon appears above the text. */
        below  /**< Icon appears below the text. */
    };

    //==============================================================================
    /** Constructor. */
    ListBoxItem();

    /** Destructor. */
    ~ListBoxItem() override;

    //==============================================================================
    /** Sets the text to display.

        @param newText  The text string to display
    */
    void setText (const String& newText);

    /** Returns the current text.

        @return The text being displayed
    */
    String getText() const;

    //==============================================================================
    /** Sets the icon drawable to display.

        @param newIcon  The drawable to use as an icon (can be nullptr for no icon)
    */
    void setIconDrawable (std::shared_ptr<Drawable> newIcon);

    /** Sets the icon from an image.

        This creates a drawable from the image internally.

        @param newIcon  The image to use as an icon
    */
    void setIcon (const Image& newIcon);

    /** Returns the current icon drawable.

        @return The icon drawable being displayed
    */
    std::shared_ptr<Drawable> getIconDrawable() const;

    //==============================================================================
    /** Sets the position of the icon relative to the text.

        @param position  The icon position (left, right, above, or below)
    */
    void setIconPosition (IconPosition position);

    /** Returns the current icon position.

        @return The icon position
    */
    IconPosition getIconPosition() const;

    //==============================================================================
    /** Sets whether this item appears selected.

        @param shouldBeSelected  True to show as selected
    */
    void setSelected (bool shouldBeSelected);

    /** Returns whether this item is currently selected.

        @return True if selected
    */
    bool isSelected() const;

    //==============================================================================
    /** Sets whether this item appears hovered.

        @param shouldBeHovered  True to show as hovered
    */
    void setHovered (bool shouldBeHovered);

    /** Returns whether this item is currently hovered.

        @return True if hovered
    */
    bool isHovered() const;

    //==============================================================================
    /** Style identifiers for theming. */
    struct Style
    {
        static inline const Identifier textColorId { "listBoxItemText" };
        static inline const Identifier textColorSelectedId { "listBoxItemTextSelected" };
        static inline const Identifier backgroundColorId { "listBoxItemBackground" };
        static inline const Identifier backgroundColorSelectedId { "listBoxItemBackgroundSelected" };
        static inline const Identifier backgroundColorHoveredId { "listBoxItemBackgroundHovered" };
    };

    //==============================================================================
    /** Returns the calculated text bounds for rendering.

        This is used by the theme for rendering.

        @return The text bounds
    */
    Rectangle<float> getTextBoundsForRendering() const { return textBounds; }

    /** Returns the calculated icon bounds for rendering.

        This is used by the theme for rendering.

        @return The icon bounds
    */
    Rectangle<float> getIconBoundsForRendering() const { return iconBounds; }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;

private:
    //==============================================================================
    void calculateLayout();

    //==============================================================================
    String text;
    std::shared_ptr<Drawable> iconDrawable;
    IconPosition iconPosition = IconPosition::left;
    bool selected = false;
    bool hovered = false;

    Rectangle<float> textBounds;
    Rectangle<float> iconBounds;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListBoxItem)
};

} // namespace yup
