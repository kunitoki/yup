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
        calculateLayout();
        repaint();
    }
}

void ListBoxItem::setIcon (const Image& newIcon)
{
    if (newIcon.isValid())
    {
        // TODO: Create a Drawable from the Image once Drawable supports Image rendering
        // For now, just clear the iconDrawable
        iconDrawable = nullptr;
    }
    else
    {
        iconDrawable = nullptr;
    }

    calculateLayout();
    repaint();
}

std::shared_ptr<Drawable> ListBoxItem::getIconDrawable() const
{
    return iconDrawable;
}

//==============================================================================
void ListBoxItem::setIconPosition (IconPosition position)
{
    if (iconPosition != position)
    {
        iconPosition = position;
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

    // If no icon, text takes full content area
    if (iconDrawable == nullptr)
    {
        textBounds = contentBounds;
        iconBounds = {};
        return;
    }

    // Get the drawable's bounds to determine aspect ratio
    auto drawableBounds = iconDrawable->getBounds();
    auto drawableWidth = drawableBounds.getWidth();
    auto drawableHeight = drawableBounds.getHeight();

    // If drawable has no size, use text bounds only
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
