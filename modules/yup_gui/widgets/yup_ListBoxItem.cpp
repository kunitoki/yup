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

namespace
{

struct ListBoxItemIds
{
    static inline const Identifier text { "text" };
    static inline const Identifier iconPosition { "iconPosition" };
    static inline const Identifier selected { "selected" };
    static inline const Identifier hovered { "hovered" };
    static inline const Identifier hasIcon { "hasIcon" };
    static inline const Identifier iconBounds { "iconBounds" };
    static inline const Identifier textBounds { "textBounds" };
};

} // namespace

//==============================================================================
const Identifier ListBoxItem::Style::textColorId { "listBoxItemText" };
const Identifier ListBoxItem::Style::textColorSelectedId { "listBoxItemTextSelected" };
const Identifier ListBoxItem::Style::backgroundColorId { "listBoxItemBackground" };
const Identifier ListBoxItem::Style::backgroundColorSelectedId { "listBoxItemBackgroundSelected" };
const Identifier ListBoxItem::Style::backgroundColorHoveredId { "listBoxItemBackgroundHovered" };
const Identifier ListBoxItem::Style::iconPartId { "icon" };
const Identifier ListBoxItem::Style::textPartId { "text" };

//==============================================================================
const ComponentTypeInfo& ListBoxItem::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ListBoxItem> (Identifier ("ListBoxItem"))
                                 .color ("textColor", Style::textColorId)
                                 .color ("textSelectedColor", Style::textColorSelectedId)
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("backgroundSelectedColor", Style::backgroundColorSelectedId)
                                 .color ("backgroundHoveredColor", Style::backgroundColorHoveredId)
                                 .property (ListBoxItemIds::text, &ListBoxItem::getText, &ListBoxItem::setText, String(), "The text displayed by the list item.")
                                 .enumeration (ListBoxItemIds::iconPosition, { "left", "right", "above", "below" }, &ListBoxItem::getIconPosition, &ListBoxItem::setIconPosition, "Where the icon is positioned relative to the text.")
                                 .state (ListBoxItemIds::selected, &ListBoxItem::isSelected, &ListBoxItem::setSelected, false, "Whether this list item is selected.")
                                 .state (ListBoxItemIds::hovered, &ListBoxItem::isHovered, &ListBoxItem::setHovered, false, "Whether this list item is hovered.")
                                 .readOnly (ListBoxItemIds::hasIcon, [] (const ListBoxItem& i) { return i.getIconDrawable() != nullptr || i.getIconImage().isValid(); }, false, "Whether the item was given an icon, which the part 'icon' paints.")
                                 .readOnly (ListBoxItemIds::iconBounds, &ListBoxItem::getIconBounds, {}, "The area of the icon, empty for an item without one.")
                                 .readOnly (ListBoxItemIds::textBounds, &ListBoxItem::getTextBounds, {}, "The area of the text.")
                                 .part (Style::iconPartId, &ListBoxItem::getIconBounds, "The drawable or the image the item was given as its icon.")
                                 .part (Style::textPartId, &ListBoxItem::getTextBounds, "The text, laid out by the theme for the area it is given and filled with the fill color.")
                                 .build();

    return info;
}

//==============================================================================
ListBoxItem::ListBoxItem()
{
    setOpaque (false);
    setWantsMouseEvents (false, false);
}

ListBoxItem::~ListBoxItem() = default;

//==============================================================================
void ListBoxItem::setText (const String& newText)
{
    if (text != newText)
    {
        text = newText;
        notifyPropertyChanged (ListBoxItemIds::text);
        calculateLayout();
        repaint();
    }
}

String ListBoxItem::getText() const
{
    return text;
}

//==============================================================================
void ListBoxItem::setIconDrawable (std::shared_ptr<Drawable> newIcon)
{
    if (iconDrawable != newIcon)
    {
        iconDrawable = newIcon;
        iconImage = {};
        calculateLayout();
        repaint();
    }
}

void ListBoxItem::setIcon (const Image& newIcon)
{
    iconImage = newIcon;
    iconDrawable = nullptr;

    calculateLayout();
    repaint();
}

std::shared_ptr<Drawable> ListBoxItem::getIconDrawable() const
{
    return iconDrawable;
}

Image ListBoxItem::getIconImage() const
{
    return iconImage;
}

//==============================================================================
void ListBoxItem::setIconPosition (IconPosition position)
{
    if (iconPosition != position)
    {
        iconPosition = position;
        notifyPropertyChanged (ListBoxItemIds::iconPosition);
        calculateLayout();
        repaint();
    }
}

ListBoxItem::IconPosition ListBoxItem::getIconPosition() const
{
    return iconPosition;
}

//==============================================================================
void ListBoxItem::setSelected (bool shouldBeSelected)
{
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        notifyPropertyChanged (ListBoxItemIds::selected);
        repaint();
    }
}

bool ListBoxItem::isSelected() const
{
    return selected;
}

//==============================================================================
void ListBoxItem::setHovered (bool shouldBeHovered)
{
    if (hovered != shouldBeHovered)
    {
        hovered = shouldBeHovered;
        notifyPropertyChanged (ListBoxItemIds::hovered);
        repaint();
    }
}

bool ListBoxItem::isHovered() const
{
    return hovered;
}

//==============================================================================
void ListBoxItem::calculateLayout()
{
    auto bounds = getLocalBounds();

    // Apply padding (4% of dimensions)
    auto paddingX = proportionOfWidth (0.04f);
    auto paddingY = proportionOfHeight (0.04f);
    auto contentBounds = bounds.reduced (paddingX, paddingY);

    // Get the icon's size to determine aspect ratio
    float drawableWidth = 0.0f;
    float drawableHeight = 0.0f;

    if (iconDrawable != nullptr)
    {
        auto drawableBounds = iconDrawable->getBounds();
        drawableWidth = drawableBounds.getWidth();
        drawableHeight = drawableBounds.getHeight();
    }
    else if (iconImage.isValid())
    {
        drawableWidth = static_cast<float> (iconImage.getWidth());
        drawableHeight = static_cast<float> (iconImage.getHeight());
    }

    // If there is no icon, or it has no size, use text bounds only
    if (drawableWidth <= 0.0f || drawableHeight <= 0.0f)
    {
        textBounds = contentBounds;
        iconBounds = {};
        return;
    }

    // Calculate icon size (keep aspect ratio)
    auto iconSize = std::min (contentBounds.getWidth(), contentBounds.getHeight()) * 0.6f;
    auto iconAspect = drawableWidth / drawableHeight;

    float iconWidth = iconSize;
    float iconHeight = iconSize;

    if (iconAspect > 1.0f)
    {
        iconHeight = iconSize / iconAspect;
    }
    else if (iconAspect < 1.0f)
    {
        iconWidth = iconSize * iconAspect;
    }

    // Layout based on icon position
    switch (iconPosition)
    {
        case IconPosition::left:
        {
            iconBounds = contentBounds.removeFromLeft (iconWidth).withSizeKeepingCenter (iconWidth, iconHeight);
            contentBounds.removeFromLeft (paddingX);
            textBounds = contentBounds;
            break;
        }

        case IconPosition::right:
        {
            iconBounds = contentBounds.removeFromRight (iconWidth).withSizeKeepingCenter (iconWidth, iconHeight);
            contentBounds.removeFromRight (paddingX);
            textBounds = contentBounds;
            break;
        }

        case IconPosition::above:
        {
            iconBounds = contentBounds.removeFromTop (iconHeight).withSizeKeepingCenter (iconWidth, iconHeight);
            contentBounds.removeFromTop (paddingY);
            textBounds = contentBounds;
            break;
        }

        case IconPosition::below:
        {
            iconBounds = contentBounds.removeFromBottom (iconHeight).withSizeKeepingCenter (iconWidth, iconHeight);
            contentBounds.removeFromBottom (paddingY);
            textBounds = contentBounds;
            break;
        }
    }
}

//==============================================================================
void ListBoxItem::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================
void ListBoxItem::resized()
{
    calculateLayout();
}

} // namespace yup
