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

namespace
{

constexpr float tabButtonPadding = 12.0f;
constexpr float tabButtonGap = 6.0f;
constexpr float tabButtonIconSize = 16.0f;
constexpr float tabButtonCloseSize = 16.0f;
constexpr float tabButtonArrowSize = 10.0f;
constexpr float tabButtonVerticalLength = 32.0f;
constexpr float tabButtonCustomMargin = 8.0f;
constexpr float tabButtonSelectedWeight = 600.0f;

} // namespace

//==============================================================================
TabButton::TabButton (TabBar& owner, const Identifier& tabId)
    : Button (tabId.toString())
    , owner (owner)
    , tabId (tabId)
{
    setOpaque (false);
    setWantsKeyboardFocus (false);
}

TabButton::~TabButton() = default;

//==============================================================================
void TabButton::setText (const String& newText)
{
    if (text == newText)
        return;

    text = newText;
    contentChanged();
}

void TabButton::setIconGlyph (const String& newGlyph)
{
    if (iconGlyph == newGlyph && ! iconImage.isValid())
        return;

    iconGlyph = newGlyph;
    iconImage = {};
    contentChanged();
}

void TabButton::setIconGlyph (const char* utf8Glyph)
{
    setIconGlyph (String::fromUTF8 (utf8Glyph));
}

void TabButton::setIconImage (const Image& newImage)
{
    iconImage = newImage;
    iconGlyph = {};
    contentChanged();
}

bool TabButton::hasIcon() const noexcept
{
    return iconGlyph.isNotEmpty() || iconImage.isValid();
}

void TabButton::setClosable (bool shouldBeClosable)
{
    if (closable == shouldBeClosable)
        return;

    closable = shouldBeClosable;
    closeButtonOver = false;
    contentChanged();
}

void TabButton::setCustomComponent (std::unique_ptr<Component> newComponent)
{
    if (customComponent != nullptr)
        removeChildComponent (*customComponent);

    customComponent = std::move (newComponent);
    customComponentSize = {};

    if (customComponent != nullptr)
    {
        customComponentSize = { customComponent->getWidth(), customComponent->getHeight() };
        customComponent->setWantsMouseEvents (false, false);
        addAndMakeVisible (*customComponent);
    }

    contentChanged();
}

//==============================================================================
float TabButton::getPreferredLength() const
{
    if (owner.getOrientation() == TabBar::Orientation::vertical)
        return jmax (tabButtonVerticalLength, customComponent != nullptr ? customComponentSize.getHeight() + tabButtonCustomMargin : 0.0f);

    float length = 0.0f;
    int numParts = 0;

    const auto addPart = [&] (float partLength)
    {
        if (partLength <= 0.0f)
            return;

        length += partLength;
        ++numParts;
    };

    if (customComponent != nullptr)
        addPart (customComponentSize.getWidth());
    else
    {
        addPart (hasIcon() ? tabButtonIconSize : 0.0f);
        addPart (text.isNotEmpty() ? getTextWidth() : 0.0f);
    }

    addPart (closable ? tabButtonCloseSize : 0.0f);
    addPart (overflowButton ? tabButtonArrowSize : 0.0f);

    return tabButtonPadding * 2.0f + length + tabButtonGap * static_cast<float> (jmax (0, numParts - 1));
}

Font TabButton::getFont (bool forSelectedTab) const
{
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

    if (forSelectedTab && font.getAxisDescription ("wght").has_value())
        font = font.withAxisValue ("wght", tabButtonSelectedWeight);

    return font;
}

float TabButton::getTextWidth() const
{
    auto styledText = StyledText();
    {
        auto modifier = styledText.startUpdate();
        modifier.setWrap (StyledText::noWrap);
        modifier.appendText (text, getFont (true));
    }

    // Rounded up so the text never gets shortened in a tab of exactly its preferred length.
    return std::ceil (styledText.getComputedTextBounds().getWidth()) + 1.0f;
}

//==============================================================================
void TabButton::setSelected (bool shouldBeSelected)
{
    if (selected == shouldBeSelected)
        return;

    selected = shouldBeSelected;
    repaint();
}

void TabButton::contentChanged()
{
    updateLayout();
    repaint();

    // The overflow button is updated by the bar while it lays itself out.
    if (! overflowButton)
        owner.updateLayout (true, false);
}

void TabButton::updateLayout()
{
    iconBounds = {};
    textBounds = {};
    closeButtonBounds = {};
    arrowBounds = {};

    const auto bounds = getLocalBounds();

    if (bounds.isEmpty())
        return;

    const auto content = bounds.reduced (tabButtonPadding, 0.0f);
    const auto hasText = customComponent == nullptr && text.isNotEmpty();
    const auto leadingLength = customComponent != nullptr ? customComponentSize.getWidth()
                                                          : (hasIcon() ? tabButtonIconSize : 0.0f);
    const auto trailingLength = closable ? tabButtonCloseSize : (overflowButton ? tabButtonArrowSize : 0.0f);

    const auto numParts = (leadingLength > 0.0f ? 1 : 0) + (hasText ? 1 : 0) + (trailingLength > 0.0f ? 1 : 0);
    const auto gaps = tabButtonGap * static_cast<float> (jmax (0, numParts - 1));
    const auto fixedLength = leadingLength + trailingLength + gaps;
    const auto textLength = hasText ? jlimit (0.0f, getTextWidth(), content.getWidth() - fixedLength) : 0.0f;
    const auto totalLength = fixedLength + textLength;

    // Vertical bars read as a list, so their text starts at the leading edge.
    const auto centered = owner.getOrientation() == TabBar::Orientation::horizontal || ! hasText;
    auto x = content.getX() + (centered ? jmax (0.0f, (content.getWidth() - totalLength) * 0.5f) : 0.0f);
    auto placedAny = false;

    const auto place = [&] (float length, float height)
    {
        if (placedAny)
            x += tabButtonGap;

        const auto area = Rectangle<float> (x, bounds.getCenterY() - height * 0.5f, length, height);
        x += length;
        placedAny = true;
        return area;
    };

    if (customComponent != nullptr)
        customComponent->setBounds (place (customComponentSize.getWidth(), customComponentSize.getHeight()));
    else if (hasIcon())
        iconBounds = place (tabButtonIconSize, tabButtonIconSize);

    if (hasText)
        textBounds = place (textLength, bounds.getHeight());

    if (closable)
        closeButtonBounds = place (tabButtonCloseSize, tabButtonCloseSize);
    else if (overflowButton)
        arrowBounds = place (tabButtonArrowSize, tabButtonArrowSize);
}

//==============================================================================
void TabButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void TabButton::resized()
{
    updateLayout();
}

void TabButton::enablementChanged()
{
    repaint();
}

void TabButton::mouseExit (const MouseEvent& event)
{
    closeButtonOver = false;
    Button::mouseExit (event);
}

void TabButton::mouseMove (const MouseEvent& event)
{
    const auto over = closable && closeButtonBounds.contains (event.getPosition());

    if (closeButtonOver == over)
        return;

    closeButtonOver = over;
    repaint();
}

// The bar owns selection, dragging and closing, and may delete this tab while handling an event.
void TabButton::mouseDown (const MouseEvent& event)
{
    owner.buttonMouseDown (*this, event);
}

void TabButton::mouseDrag (const MouseEvent& event)
{
    owner.buttonMouseDrag (*this, event);
}

void TabButton::mouseUp (const MouseEvent& event)
{
    owner.buttonMouseUp (*this, event);
}

void TabButton::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    owner.mouseWheel (event.withPosition (owner.getLocalPoint (this, event.getPosition())), wheelData);
}

} // namespace yup
