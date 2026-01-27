/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#if YUP_MODULE_AVAILABLE_yup_audio_gui
#include <yup_audio_gui/yup_audio_gui.h>
#endif

namespace yup
{

//==============================================================================

extern const uint8_t RobotoFlexFont_data[];
extern const std::size_t RobotoFlexFont_size;
extern const uint8_t FontAwesome7Font_data[];
extern const std::size_t FontAwesome7Font_size;

//==============================================================================

struct SliderColors
{
    Color background;
    Color track;
    Color thumb;
    Color thumbOver;
    Color thumbDown;
    Color text;
};

SliderColors getSliderColors (const ApplicationTheme& theme, const Slider& slider)
{
    SliderColors colors;
    colors.background = slider.findColor (Slider::Style::backgroundColorId).value_or (Color (0xff3d3d3d));
    colors.track = slider.findColor (Slider::Style::trackColorId).value_or (Color (0xff636363));
    colors.thumb = slider.findColor (Slider::Style::thumbColorId).value_or (Color (0xff4ebfff));
    colors.thumbOver = slider.findColor (Slider::Style::thumbOverColorId).value_or (colors.thumb.brighter (0.3f));
    colors.thumbDown = slider.findColor (Slider::Style::thumbDownColorId).value_or (colors.thumb.darker (0.2f));
    colors.text = slider.findColor (Slider::Style::textColorId).value_or (Colors::white);
    return colors;
}

void paintRotarySlider (Graphics& g, const ApplicationTheme& theme, const Slider& slider, Rectangle<float> sliderBounds, float rotaryStartAngle, float rotaryEndAngle, float sliderValue, bool isMouseOver, bool isMouseDown)
{
    const auto colors = getSliderColors (theme, slider);

    auto bounds = sliderBounds.reduced (slider.proportionOfWidth (0.1f));
    const auto center = bounds.getCenter();

    const auto fromRadians = rotaryStartAngle;
    const auto toRadians = rotaryEndAngle;
    const auto toCurrentRadians = fromRadians + (toRadians - fromRadians) * sliderValue;

    Path backgroundPath;
    backgroundPath.addEllipse (bounds.reduced (slider.proportionOfWidth (0.105f)));

    g.setFillColor (colors.background);
    g.fillPath (backgroundPath);

    g.setStrokeColor (colors.background.darker (0.3f));
    g.setStrokeWidth (slider.proportionOfWidth (0.0175f));
    g.strokePath (backgroundPath);

    const auto reducedBounds = bounds.reduced (slider.proportionOfWidth (0.175f));
    const auto pos = center.getPointOnCircumference (
        reducedBounds.getWidth() / 2.0f,
        reducedBounds.getHeight() / 2.0f,
        toCurrentRadians);

    Path foregroundLine;
    foregroundLine.addLine (Line<float> (pos, center).keepOnlyStart (0.25f));

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (colors.text);
    g.setStrokeWidth (slider.proportionOfWidth (0.03f));
    g.strokePath (foregroundLine);

    Path backgroundArc;
    backgroundArc.addCenteredArc (center,
                                  bounds.getWidth() / 2.0f,
                                  bounds.getHeight() / 2.0f,
                                  0.0f,
                                  fromRadians,
                                  toRadians,
                                  true);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (colors.track);
    g.setStrokeWidth (slider.proportionOfWidth (0.075f));
    g.strokePath (backgroundArc);

    Path foregroundArc;
    foregroundArc.addCenteredArc (center,
                                  bounds.getWidth() / 2.0f,
                                  bounds.getHeight() / 2.0f,
                                  0.0f,
                                  fromRadians,
                                  toCurrentRadians,
                                  true);

    auto thumbColor = slider.isMouseOver() ? colors.thumbOver : colors.thumb;
    if (! slider.isEnabled())
        thumbColor = thumbColor.withAlpha (0.3f);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (thumbColor);
    g.setStrokeWidth (slider.proportionOfWidth (0.075f));
    g.strokePath (foregroundArc);

    if (slider.hasKeyboardFocus())
    {
        Path focusPath;
        focusPath.addEllipse (slider.getLocalBounds().reduced (2));

        g.setStrokeColor (Colors::cornflowerblue); // TODO - findColor
        g.setStrokeWidth (2.0f);
        g.strokePath (focusPath);
    }
}

void paintLinearSlider (Graphics& g, const ApplicationTheme& theme, const Slider& slider, Rectangle<float> sliderBounds, Rectangle<float> thumbBounds, bool isHorizontal, float sliderValue, bool isMouseOver, bool isMouseDown)
{
    const auto colors = getSliderColors (theme, slider);

    // Draw track background
    g.setFillColor (colors.background);
    if (isHorizontal)
        g.fillRoundedRect (sliderBounds.getX(), sliderBounds.getCenterY() - 2.0f, sliderBounds.getWidth(), 4.0f, 2.0f);
    else
        g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f, sliderBounds.getY(), 4.0f, sliderBounds.getHeight(), 2.0f);

    // Draw value track for bar sliders
    const auto sliderType = slider.getSliderType();
    if (sliderType == Slider::LinearBarHorizontal || sliderType == Slider::LinearBarVertical)
    {
        g.setFillColor (colors.track);
        if (isHorizontal)
            g.fillRoundedRect (sliderBounds.getX(), sliderBounds.getCenterY() - 2.0f, sliderValue * sliderBounds.getWidth(), 4.0f, 2.0f);
        else
            g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f,
                               sliderBounds.getBottom() - (sliderValue * sliderBounds.getHeight()),
                               4.0f,
                               sliderValue * sliderBounds.getHeight(),
                               2.0f);
    }

    // Draw thumb
    g.setFillColor (isMouseDown ? colors.thumbDown : (isMouseOver ? colors.thumbOver : colors.thumb));
    g.fillEllipse (thumbBounds);

    // Draw focus outline if needed
    if (slider.hasKeyboardFocus())
    {
        g.setStrokeColor (Colors::cornflowerblue);
        g.setStrokeWidth (2.0f);
        g.strokeRoundedRect (slider.getLocalBounds().reduced (2), 2.0f);
    }
}

void paintTwoValueSlider (Graphics& g, const ApplicationTheme& theme, const Slider& slider, Rectangle<float> sliderBounds, Rectangle<float> minThumbBounds, Rectangle<float> maxThumbBounds, bool isHorizontal, float minValue, float maxValue, bool isMouseOverMinThumb, bool isMouseOverMaxThumb, bool isMouseDown)
{
    const auto colors = getSliderColors (theme, slider);

    // Draw track background
    g.setFillColor (colors.background);
    if (isHorizontal)
        g.fillRoundedRect (sliderBounds.getX(), sliderBounds.getCenterY() - 2.0f, sliderBounds.getWidth(), 4.0f, 2.0f);
    else
        g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f, sliderBounds.getY(), 4.0f, sliderBounds.getHeight(), 2.0f);

    // Draw selected range
    g.setFillColor (colors.track);
    if (isHorizontal)
    {
        const float startX = sliderBounds.getX() + (minValue * sliderBounds.getWidth());
        const float endX = sliderBounds.getX() + (maxValue * sliderBounds.getWidth());
        g.fillRoundedRect (startX, sliderBounds.getCenterY() - 2.0f, endX - startX, 4.0f, 2.0f);
    }
    else
    {
        const float startY = sliderBounds.getBottom() - (minValue * sliderBounds.getHeight());
        const float endY = sliderBounds.getBottom() - (maxValue * sliderBounds.getHeight());
        g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f, endY, 4.0f, startY - endY, 2.0f);
    }

    // Draw min thumb
    g.setFillColor (isMouseDown ? colors.thumbDown : (isMouseOverMinThumb ? colors.thumbOver : colors.thumb));
    g.fillEllipse (minThumbBounds);

    // Draw max thumb
    g.setFillColor (isMouseDown ? colors.thumbDown : (isMouseOverMaxThumb ? colors.thumbOver : colors.thumb));
    g.fillEllipse (maxThumbBounds);

    // Draw focus outline if needed
    if (slider.hasKeyboardFocus())
    {
        g.setStrokeColor (Colors::cornflowerblue);
        g.setStrokeWidth (2.0f);
        g.strokeRoundedRect (slider.getLocalBounds().reduced (2), 2.0f);
    }
}

void paintSlider (Graphics& g, const ApplicationTheme& theme, const Slider& s)
{
    auto sliderBounds = s.getSliderBounds();
    const auto sliderType = s.getSliderType();
    const bool isMouseOver = s.isMouseOver();
    const bool isMouseDown = false; // s.isMouseButtonDown();

    switch (sliderType)
    {
        case Slider::RotaryHorizontalDrag:
        case Slider::RotaryVerticalDrag:
        case Slider::Rotary:
        default:
        {
            constexpr float rotaryStartAngle = degreesToRadians (135.0f);
            constexpr float rotaryEndAngle = rotaryStartAngle + degreesToRadians (270.0f);
            paintRotarySlider (g, theme, s, sliderBounds, rotaryStartAngle, rotaryEndAngle, static_cast<float> (s.getValueNormalised()), isMouseOver, isMouseDown);
            break;
        }

        case Slider::LinearHorizontal:
        case Slider::LinearVertical:
        case Slider::LinearBarHorizontal:
        case Slider::LinearBarVertical:
        {
            Rectangle<float> thumbBounds;
            const bool isHorizontal = (sliderType == Slider::LinearHorizontal || sliderType == Slider::LinearBarHorizontal);
            const float sliderValue = static_cast<float> (s.getValueNormalised());

            if (isHorizontal)
            {
                const float thumbX = sliderBounds.getX() + (sliderValue * sliderBounds.getWidth()) - 8.0f;
                thumbBounds = Rectangle<float> (thumbX, sliderBounds.getCenterY() - 8.0f, 16.0f, 16.0f);
            }
            else
            {
                const float thumbY = sliderBounds.getBottom() - (sliderValue * sliderBounds.getHeight()) - 8.0f;
                thumbBounds = Rectangle<float> (sliderBounds.getCenterX() - 8.0f, thumbY, 16.0f, 16.0f);
            }

            paintLinearSlider (g, theme, s, sliderBounds, thumbBounds, isHorizontal, sliderValue, isMouseOver, isMouseDown);
            break;
        }

        case Slider::TwoValueHorizontal:
        case Slider::TwoValueVertical:
        {
            Rectangle<float> minThumbBounds, maxThumbBounds;
            const bool isHorizontal = (sliderType == Slider::TwoValueHorizontal);
            const float minNorm = 0.0f; // static_cast<float>(s.getMinValueNormalised());
            const float maxNorm = 1.0f; // static_cast<float>(s.getMaxValueNormalised());

            if (isHorizontal)
            {
                const float minThumbX = sliderBounds.getX() + (minNorm * sliderBounds.getWidth()) - 8.0f;
                const float maxThumbX = sliderBounds.getX() + (maxNorm * sliderBounds.getWidth()) - 8.0f;
                minThumbBounds = Rectangle<float> (minThumbX, sliderBounds.getCenterY() - 8.0f, 16.0f, 16.0f);
                maxThumbBounds = Rectangle<float> (maxThumbX, sliderBounds.getCenterY() - 8.0f, 16.0f, 16.0f);
            }
            else
            {
                const float minThumbY = sliderBounds.getBottom() - (minNorm * sliderBounds.getHeight()) - 8.0f;
                const float maxThumbY = sliderBounds.getBottom() - (maxNorm * sliderBounds.getHeight()) - 8.0f;
                minThumbBounds = Rectangle<float> (sliderBounds.getCenterX() - 8.0f, minThumbY, 16.0f, 16.0f);
                maxThumbBounds = Rectangle<float> (sliderBounds.getCenterX() - 8.0f, maxThumbY, 16.0f, 16.0f);
            }

            // For two-value sliders, check which thumb the mouse is over
            bool isMouseOverMinThumb = false;
            bool isMouseOverMaxThumb = false;
            if (isMouseOver)
            {
                const auto mousePos = Point<float>(); // s.getMousePosition();
                const Point<float> mousePosFloat (static_cast<float> (mousePos.getX()), static_cast<float> (mousePos.getY()));
                isMouseOverMinThumb = minThumbBounds.contains (mousePosFloat);
                isMouseOverMaxThumb = maxThumbBounds.contains (mousePosFloat);
            }

            paintTwoValueSlider (g, theme, s, sliderBounds, minThumbBounds, maxThumbBounds, isHorizontal, minNorm, maxNorm, isMouseOverMinThumb, isMouseOverMaxThumb, isMouseDown);
            break;
        }
    }
}

//==============================================================================

void paintTextEditor (Graphics& g, const ApplicationTheme& theme, const TextEditor& t)
{
    auto bounds = t.getLocalBounds();
    auto textBounds = t.getTextBounds();
    auto scrollOffset = t.getScrollOffset();
    constexpr auto cornerRadius = 6.0f;

    // Draw background
    auto backgroundColor = t.findColor (TextEditor::Style::backgroundColorId).value_or (Colors::white);
    g.setFillColor (backgroundColor);
    g.fillRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw outline
    auto outlineColor = t.hasKeyboardFocus()
                          ? t.findColor (TextEditor::Style::focusedOutlineColorId).value_or (Colors::cornflowerblue)
                          : t.findColor (TextEditor::Style::outlineColorId).value_or (Colors::gray);
    g.setStrokeColor (outlineColor);

    float strokeWidth = t.hasKeyboardFocus() ? 2.0f : 1.0f;
    g.setStrokeWidth (strokeWidth);

    g.strokeRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw selection background
    if (t.hasSelection())
    {
        auto selectionColor = t.findColor (TextEditor::Style::selectionColorId).value_or (Colors::cornflowerblue.withAlpha (0.5f));
        g.setFillColor (selectionColor);

        // Get all selection rectangles for proper multiline selection rendering
        auto selectionRects = t.getSelectedTextAreas();
        for (const auto& rect : selectionRects)
        {
            // Adjust each rectangle for scroll offset and text bounds
            auto adjustedRect = rect.translated (textBounds.getTopLeft() - scrollOffset);
            g.fillRect (adjustedRect);
        }
    }

    // Draw text with scroll offset
    auto textColor = t.findColor (TextEditor::Style::textColorId).value_or (Colors::gray);
    g.setFillColor (textColor);

    auto scrolledTextBounds = textBounds.translated (-scrollOffset.getX(), -scrollOffset.getY());
    g.fillFittedText (t.getStyledText(), scrolledTextBounds);

    // Draw caret
    if (t.hasKeyboardFocus() && t.isCaretVisible())
    {
        auto caretColor = t.findColor (TextEditor::Style::caretColorId).value_or (yup::Colors::black);
        g.setFillColor (caretColor);

        auto caretBounds = t.getCaretBounds();
        g.fillRect (caretBounds);
    }
}

//==============================================================================

void paintTextButton (Graphics& g, const ApplicationTheme& theme, const TextButton& b)
{
    auto bounds = b.getLocalBounds();
    constexpr auto cornerRadius = 6.0f;

    Color backgroundColor, textColor;

    if (b.isButtonDown())
    {
        backgroundColor = b.findColor (TextButton::Style::backgroundPressedColorId).value_or (Colors::gray);
        textColor = b.findColor (TextButton::Style::textPressedColorId).value_or (Colors::dimgray);
    }
    else
    {
        backgroundColor = b.findColor (TextButton::Style::backgroundColorId).value_or (Colors::gray);
        textColor = b.findColor (TextButton::Style::textColorId).value_or (Colors::white);
    }

    if (b.isButtonOver())
    {
        backgroundColor = backgroundColor.brighter (0.2f);
        textColor = textColor.brighter (0.2f);
    }

    // Draw background with flat color (no gradient for modern flat design)
    g.setFillColor (backgroundColor);
    g.fillRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw modern outline
    Color outlineColor = b.hasKeyboardFocus()
                           ? b.findColor (TextButton::Style::outlineFocusedColorId).value_or (Colors::cornflowerblue)
                           : b.findColor (TextButton::Style::outlineColorId).value_or (Colors::dimgray);
    g.setStrokeColor (outlineColor);

    float strokeWidth = b.hasKeyboardFocus() ? 2.0f : 1.0f;
    g.setStrokeWidth (strokeWidth);

    g.strokeRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw text
    g.setFillColor (textColor);
    g.fillFittedText (b.getStyledText(), b.getTextBounds());
}

//==============================================================================

void paintToggleButton (Graphics& g, const ApplicationTheme& theme, const ToggleButton& b)
{
    auto bounds = b.getLocalBounds();
    constexpr auto cornerRadius = 6.0f;

    // Get colors based on toggle state
    auto bgColor = b.getToggleState()
                     ? b.findColor (ToggleButton::Style::backgroundToggledColorId).value_or (Colors::cornflowerblue)
                     : b.findColor (ToggleButton::Style::backgroundColorId).value_or (Color (0xfff0f0f0));

    auto textColor = b.getToggleState()
                       ? b.findColor (ToggleButton::Style::textToggledColorId).value_or (Color (0xffffffff))
                       : b.findColor (ToggleButton::Style::textColorId).value_or (Color (0xff333333));

    auto borderColor = b.getToggleState()
                         ? b.findColor (ToggleButton::Style::borderToggledColorId).value_or (Color (0xff357abd))
                         : b.findColor (ToggleButton::Style::borderColorId).value_or (Color (0xffcccccc));

    // Adjust colors for button state
    if (b.isButtonDown())
    {
        bgColor = bgColor.darker (0.1f);
        borderColor = borderColor.darker (0.1f);
    }
    else if (b.isButtonOver())
    {
        bgColor = bgColor.brighter (0.05f);
        borderColor = borderColor.brighter (0.05f);
    }

    // Draw background
    g.setFillColor (bgColor);
    g.fillRoundedRect (bounds, cornerRadius);

    // Draw border
    g.setStrokeColor (borderColor);
    g.setStrokeWidth (b.hasKeyboardFocus() ? 2.0f : 1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), cornerRadius);

    // Draw text
    if (b.getButtonText().isNotEmpty())
    {
        g.setFillColor (textColor);
        g.fillFittedText (b.getStyledText(), bounds);
    }
}

//==============================================================================

void paintSwitchButton (Graphics& g, const ApplicationTheme& theme, const SwitchButton& s)
{
    auto bounds = s.getLocalBounds().reduced (4);
    auto cornerSize = (s.isVertical() ? bounds.getWidth() : bounds.getHeight()) * 0.5f;

    // Draw shadow/outline
    g.setStrokeColor (Colors::black.withAlpha (0.1f));
    g.setStrokeWidth (2.0f);
    g.strokeRoundedRect (bounds, cornerSize);

    // Fill background based on switch state
    auto bgColor = s.getToggleState()
                     ? s.findColor (SwitchButton::Style::switchOnBackgroundColorId).value_or (Colors::cornflowerblue)
                     : s.findColor (SwitchButton::Style::switchOffBackgroundColorId).value_or (Color (0xff333333));

    g.setFillColor (bgColor);
    g.fillRoundedRect (bounds, cornerSize);

    // Draw handle
    auto circleBounds = s.getSwitchCircleBounds().reduced (4);
    auto circleColor = s.findColor (SwitchButton::Style::switchColorId).value_or (Colors::white);

    g.setFillColor (circleColor);
    g.fillRoundedRect (circleBounds, cornerSize);

    // Add a subtle shadow
    g.setStrokeColor (Colors::black.withAlpha (0.2f));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (circleBounds.reduced (0.5f), cornerSize - 0.5f);
}

//==============================================================================

void paintComboBox (Graphics& g, const ApplicationTheme& theme, const ComboBox& c)
{
    auto& styledText = c.getStyledText();
    const auto bounds = c.getLocalBounds();

    // Draw background
    auto bgColor = c.findColor (ComboBox::Style::backgroundColorId).value_or (Color (0xffffffff));
    g.setFillColor (bgColor);
    g.fillRoundedRect (bounds, 4.0f);

    // Draw border
    const bool hasFocus = c.hasKeyboardFocus() || c.isPopupShown();
    auto outlineColor = hasFocus
                          ? c.findColor (ComboBox::Style::focusedBorderColorId).value_or (Colors::cornflowerblue)
                          : c.findColor (ComboBox::Style::borderColorId).value_or (Colors::dimgray);

    g.setStrokeColor (outlineColor);
    g.setStrokeWidth (hasFocus ? 2.0f : 1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

    // Calculate text and arrow areas
    auto arrowWidth = 20.0f;
    auto arrowSize = 4.0f;
    auto textBounds = bounds.reduced (8.0f, 4.0f);
    textBounds.removeFromRight (arrowWidth);

    auto arrowBounds = bounds.reduced (4.0f);
    arrowBounds.removeFromLeft (bounds.getWidth() - arrowWidth);

    // Draw text
    if (! styledText.isEmpty())
    {
        auto textColor = c.findColor (ComboBox::Style::textColorId).value_or (Color (0xff333333));
        g.setFillColor (textColor);
        g.fillFittedText (styledText, textBounds);
    }

    // Draw arrow
    auto arrowColor = c.findColor (ComboBox::Style::arrowColorId).value_or (Color (0xff666666));
    g.setFillColor (arrowColor);

    auto center = arrowBounds.getCenter();

    // Draw simple triangle using lines instead of Path
    g.setStrokeColor (arrowColor);
    g.setStrokeWidth (2.0f);

    // Draw downward arrow as lines
    g.strokeLine (center.getX() - arrowSize, center.getY() - arrowSize * 0.5f, center.getX(), center.getY() + arrowSize * 0.5f);
    g.strokeLine (center.getX() + arrowSize, center.getY() - arrowSize * 0.5f, center.getX(), center.getY() + arrowSize * 0.5f);
}

//==============================================================================

void paintLabel (Graphics& g, const ApplicationTheme& theme, const Label& l)
{
    auto& styledText = l.getStyledText();
    const auto bounds = l.getLocalBounds();

    if (const auto backgroundColor = l.findColor (Label::Style::backgroundColorId); backgroundColor && ! backgroundColor->isTransparent())
    {
        g.setFillColor (*backgroundColor);
        g.fillRoundedRect (bounds, 4.0f);
    }

    if (const auto outlineColor = l.findColor (Label::Style::outlineColorId); outlineColor && ! outlineColor->isTransparent())
    {
        g.setStrokeColor (*outlineColor);
        g.setStrokeWidth (2.0f);
        g.strokeRoundedRect (bounds, 4.0f);
    }

    if (const auto strokeColor = l.findColor (Label::Style::textStrokeColorId); strokeColor && ! strokeColor->isTransparent())
    {
        g.setStrokeColor (*strokeColor);
        g.setStrokeWidth (l.getStrokeWidth());
        g.strokeFittedText (styledText, bounds);
    }

    if (! styledText.isEmpty())
    {
        const auto fillColor = l.findColor (Label::Style::textFillColorId).value_or (Colors::white);
        g.setFillColor (fillColor);
        g.fillFittedText (styledText, bounds);
    }
}

//==============================================================================

void paintPopupMenu (Graphics& g, const ApplicationTheme& theme, const PopupMenu& p)
{
    auto localBounds = p.getLocalBounds();

    // TODO: Draw drop shadow if enabled
    if (false) // (p.getOptions().parentComponent != nullptr)
    {
        auto shadowRadius = static_cast<float> (8.0f);
        localBounds = localBounds.reduced (shadowRadius);

        g.setFillColor (Color (0, 0, 0));
        g.setFeather (shadowRadius);
        g.fillRoundedRect (localBounds.translated (0.0f, 2.0f), 4.0f);
        g.setFeather (0.0f);
    }

    // Draw menu background
    g.setFillColor (p.findColor (PopupMenu::Style::menuBackground).value_or (Color (0xff2a2a2a)));
    g.fillRoundedRect (localBounds, 4.0f);

    // Draw border
    g.setStrokeColor (p.findColor (PopupMenu::Style::menuBorder).value_or (Color (0xff555555)));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (localBounds.reduced (0.5f), 4.0f);

    // Draw items
    bool anyItemIsTicked = false;
    for (const auto& item : p)
    {
        if (item->isTicked)
        {
            anyItemIsTicked = true;
            break;
        }
    }

    int itemIndex = -1;
    auto itemFont = theme.getDefaultFont();

    for (const auto& item : p)
    {
        ++itemIndex;
        const auto rect = item->area;

        // Skip custom components as they render themselves
        if (item->isCustomComponent())
            continue;

        g.setOpacity (1.0f);

        // Draw hover background or active submenu background
        if (item->isHovered && ! item->isSeparator() && item->isEnabled)
        {
            Color highlightColor;

            // Check if this item is currently showing its submenu (active submenu)
            if (p.isItemShowingSubmenu (itemIndex))
            {
                highlightColor = p.findColor (PopupMenu::Style::menuItemBackgroundActiveSubmenu)
                                     .value_or (Colors::darkgray.darker (0.3f));
            }
            else
            {
                highlightColor = p.findColor (PopupMenu::Style::menuItemBackgroundHighlighted)
                                     .value_or (Colors::cornflowerblue);
            }

            g.setFillColor (highlightColor);
            g.fillRoundedRect (rect.reduced (2.0f, 1.0f), 2.0f);
        }
        // Also highlight active submenu items even when not hovered
        else if (! item->isSeparator() && item->isEnabled && p.isItemShowingSubmenu (itemIndex))
        {
            auto activeSubmenuColor = p.findColor (PopupMenu::Style::menuItemBackgroundActiveSubmenu)
                                          .value_or (Colors::darkgray.darker (0.3f));

            g.setFillColor (activeSubmenuColor);
            g.fillRoundedRect (rect.reduced (2.0f, 1.0f), 2.0f);
        }

        if (item->isSeparator())
        {
            // Draw separator line
            auto lineY = rect.getCenterY();
            g.setStrokeColor (p.findColor (PopupMenu::Style::menuBorder).value_or (Color (0xff555555)));
            g.setStrokeWidth (1.0f);
            g.strokeLine (rect.getX() + 8.0f, lineY, rect.getRight() - 8.0f, lineY);
        }
        else
        {
            // Draw menu item text
            auto textColor = item->textColor.value_or (p.findColor (PopupMenu::Style::menuItemText).value_or (Color (0xffffffff)));
            if (! item->isEnabled)
                textColor = p.findColor (PopupMenu::Style::menuItemTextDisabled).value_or (Color (0xff808080));

            g.setFillColor (textColor);

            auto textRect = rect.reduced (12.0f, 2.0f);
            if (anyItemIsTicked)
                textRect.setX (textRect.getX() + 8.0f);

            {
                auto styledText = yup::StyledText();
                {
                    auto modifier = styledText.startUpdate();
                    modifier.appendText (item->text, itemFont.withHeight (14.0f));
                }

                g.fillFittedText (styledText, textRect);
            }

            // Draw checkmark if ticked
            if (item->isTicked)
            {
                auto checkRect = Rectangle<float> (rect.getX() + 4.0f, rect.getY() + 4.0f, 12.0f, 12.0f);
                g.setStrokeColor (textColor);
                g.setStrokeWidth (2.0f);
                g.strokeLine (checkRect.getX() + 2.0f, checkRect.getCenterY(), checkRect.getCenterX(), checkRect.getBottom() - 2.0f);
                g.strokeLine (checkRect.getCenterX(), checkRect.getBottom() - 2.0f, checkRect.getRight() - 2.0f, checkRect.getY() + 2.0f);
            }

            // Draw shortcut text
            if (item->shortcutKeyText.isNotEmpty())
            {
                auto shortcutRect = Rectangle<float> (rect.getRight() - 80.0f, rect.getY() + 2.0f, 75.0f, rect.getHeight() - 2.0f);

                auto styledText = yup::StyledText();
                {
                    auto modifier = styledText.startUpdate();
                    modifier.setHorizontalAlign (yup::StyledText::right);
                    modifier.appendText (item->shortcutKeyText, itemFont.withHeight (13.0f));
                }

                g.setOpacity (0.7f);
                g.setFillColor (textColor);
                g.fillFittedText (styledText, shortcutRect);
                g.setOpacity (1.0f);
            }

            // Draw submenu arrow
            if (item->isSubMenu())
            {
                auto arrowRect = Rectangle<float> (rect.getRight() - 16.0f, rect.getY() + 4.0f, 8.0f, rect.getHeight() - 8.0f);
                g.setStrokeColor (textColor);
                g.setStrokeWidth (1.5f);
                g.strokeLine (arrowRect.getX() + 2.0f, arrowRect.getY() + 2.0f, arrowRect.getRight() - 2.0f, arrowRect.getCenterY());
                g.strokeLine (arrowRect.getRight() - 2.0f, arrowRect.getCenterY(), arrowRect.getX() + 2.0f, arrowRect.getBottom() - 2.0f);
            }
        }
    }

    // Paint scroll indicators if needed
    if (p.needsScrolling())
    {
        g.setFillColor (p.findColor (PopupMenu::Style::menuItemText).value_or (Colors::white));

        // Up arrow
        if (p.canScrollUp())
        {
            auto upBounds = p.getScrollUpIndicatorBounds();
            auto center = upBounds.getCenter();
            auto arrowSize = 4.0f;

            Path upArrow;
            upArrow.moveTo (center.getX(), center.getY() - arrowSize * 0.5f);
            upArrow.lineTo (center.getX() - arrowSize, center.getY() + arrowSize * 0.5f);
            upArrow.lineTo (center.getX() + arrowSize, center.getY() + arrowSize * 0.5f);
            upArrow.close();

            g.fillPath (upArrow);
        }

        // Down arrow
        if (p.canScrollDown())
        {
            auto downBounds = p.getScrollDownIndicatorBounds();
            auto center = downBounds.getCenter();
            auto arrowSize = 4.0f;

            Path downArrow;
            downArrow.moveTo (center.getX(), center.getY() + arrowSize * 0.5f);
            downArrow.lineTo (center.getX() - arrowSize, center.getY() - arrowSize * 0.5f);
            downArrow.lineTo (center.getX() + arrowSize, center.getY() - arrowSize * 0.5f);
            downArrow.close();

            g.fillPath (downArrow);
        }
    }
}

//==============================================================================

void paintScrollBar (Graphics& g, const ApplicationTheme& theme, const ScrollBar& scrollBar)
{
    // Don't paint if hidden
    if (scrollBar.getVisibilityMode() == ScrollBar::VisibilityMode::alwaysHidden)
        return;

    // Don't paint in auto-hide mode if scrolling is not needed
    if (scrollBar.getVisibilityMode() == ScrollBar::VisibilityMode::autoHide && ! scrollBar.isScrollingNeeded())
        return;

    auto trackBounds = scrollBar.getTrackBoundsForRendering();
    auto thumbBounds = scrollBar.getThumbBoundsForRendering().reduced (3);
    auto cornerSize = scrollBar.getScrollBarWidth() * 0.5f;

    // Draw track (optional, usually invisible on macOS)
    if (const auto trackColor = scrollBar.findColor (ScrollBar::Style::trackColorId); trackColor && ! trackColor->isTransparent())
    {
        g.setFillColor (*trackColor);
        g.fillRect (trackBounds);
    }

    // Draw thumb with rounded caps
    Color thumbColor;
    if (scrollBar.isDragging())
        thumbColor = scrollBar.findColor (ScrollBar::Style::thumbDraggingColorId).value_or (Color (0x99000000));
    else if (scrollBar.isThumbHovered())
        thumbColor = scrollBar.findColor (ScrollBar::Style::thumbHoverColorId).value_or (Color (0x77000000));
    else
        thumbColor = scrollBar.findColor (ScrollBar::Style::thumbColorId).value_or (Color (0x55000000));

    g.setFillColor (thumbColor);
    g.fillRoundedRect (thumbBounds, cornerSize);
}

//==============================================================================

void paintProgressBar (Graphics& g, const ApplicationTheme& theme, const ProgressBar& progressBar)
{
    const auto bounds = progressBar.getLocalBounds();
    const auto cornerSize = bounds.getHeight() * 0.5f;

    // Get colors
    const auto backgroundColor = progressBar.findColor (ProgressBar::Style::backgroundColorId)
                                     .value_or (Color (0xff3d3d3d));
    const auto foregroundColor = progressBar.findColor (ProgressBar::Style::foregroundColorId)
                                     .value_or (Color (0xff4ebfff));

    // Draw background track
    g.setFillColor (backgroundColor);
    g.fillRoundedRect (bounds, cornerSize);

    const auto progress = progressBar.getProgress();

    if (progressBar.isIndeterminate())
    {
        // Indeterminate mode - draw animated diagonal stripes (JUCE-style)
        const auto stripeWidth = bounds.getHeight() * 2.0f;
        const auto halfStripeWidth = stripeWidth * 0.5f;
        // Reverse animation direction (right to left becomes left to right)
        const auto position = static_cast<float> (static_cast<int> (stripeWidth) - (static_cast<int> (Time::getCurrentTime().toMilliseconds() / 15) % static_cast<int> (stripeWidth)));

        auto state = g.saveState();

        // Create a rounded rect clip path (setClipPath requires global coordinates)
        const auto globalBounds = progressBar.getBoundsRelativeToTopLevelComponent();
        Path clipPath;
        clipPath.addRoundedRectangle (globalBounds, cornerSize);
        g.setClipPath (clipPath);

        // Build two separate paths for alternating solid color shades
        Path stripesLight;
        Path stripesDark;
        int stripeIndex = 0;

        // Use half stripe width for spacing so stripes are closer together
        for (auto x = -position; x < bounds.getWidth() + stripeWidth; x += halfStripeWidth)
        {
            // Alternate between the two paths
            Path& currentPath = (stripeIndex % 2 == 0) ? stripesLight : stripesDark;

            currentPath.addQuadrilateral (
                x, 0.0f, x + halfStripeWidth, 0.0f, x, bounds.getHeight(), x - halfStripeWidth, bounds.getHeight());

            ++stripeIndex;
        }

        // Draw all stripes with solid colors (no alpha blending)
        g.setFillColor (foregroundColor);
        g.fillPath (stripesLight);

        g.setFillColor (foregroundColor.darker (0.15f));
        g.fillPath (stripesDark);
    }
    else
    {
        // Normal mode - draw filled portion with rounded rect clipping
        const auto filledWidth = bounds.getWidth() * static_cast<float> (jlimit (0.0, 1.0, progress));

        if (filledWidth > 0.0f)
        {
            auto state = g.saveState();

            // Create a rounded rect clip path for the filled portion (setClipPath requires global coordinates)
            const auto globalBounds = progressBar.getBoundsRelativeToTopLevelComponent();
            Path clipPath;
            clipPath.addRoundedRectangle (globalBounds, cornerSize);
            g.setClipPath (clipPath);

            // Draw the filled bar
            auto filledBounds = bounds.withWidth (filledWidth);
            g.setFillColor (foregroundColor);
            g.fillRect (filledBounds);
        }
    }
}

//==============================================================================

void paintListBoxItem (Graphics& g, const ApplicationTheme& theme, const ListBoxItem& item)
{
    // Determine background color
    Color backgroundColor;

    if (item.isSelected())
        backgroundColor = item.findColor (ListBoxItem::Style::backgroundColorSelectedId).value_or (Color (0xff3a7ebf));
    else if (item.isHovered())
        backgroundColor = item.findColor (ListBoxItem::Style::backgroundColorHoveredId).value_or (Color (0x22ffffff));
    else
        backgroundColor = item.findColor (ListBoxItem::Style::backgroundColorId).value_or (Color (0x00000000));

    // Fill background
    if (backgroundColor.getAlpha() > 0)
    {
        g.setFillColor (backgroundColor);
        g.fillRect (item.getLocalBounds());
    }

    // Draw icon
    auto iconDrawable = item.getIconDrawable();
    auto iconBounds = item.getIconBoundsForRendering();
    if (iconDrawable != nullptr && ! iconBounds.isEmpty())
        iconDrawable->paint (g, iconBounds, Fitting::scaleToFit, Justification::center);

    // Draw text
    auto text = item.getText();
    auto textBounds = item.getTextBoundsForRendering();
    if (text.isNotEmpty() && ! textBounds.isEmpty())
    {
        Color textColor;

        if (item.isSelected())
            textColor = item.findColor (ListBoxItem::Style::textColorSelectedId).value_or (Color (0xffffffff));
        else
            textColor = item.findColor (ListBoxItem::Style::textColorId).value_or (Color (0xff000000));

        g.setFillColor (textColor);

        auto font = theme.getDefaultFont();
        auto fontSize = std::min (textBounds.getHeight() * 0.6f, 16.0f);

        auto styledText = yup::StyledText();
        {
            auto modifier = styledText.startUpdate();
            modifier.appendText (text, font.withHeight (fontSize));
            modifier.setVerticalAlign (StyledText::middle);
        }

        g.fillFittedText (styledText, textBounds);
    }
}

//==============================================================================
#if YUP_MODULE_AVAILABLE_yup_audio_gui
void paintMidiKeyboard (Graphics& g, const ApplicationTheme& theme, const MidiKeyboardComponent& keyboard)
{
    auto bounds = keyboard.getLocalBounds();

    if (bounds.isEmpty())
        return;

    auto keyWidth = keyboard.getKeyStartRange().getLength();
    keyWidth /= keyboard.getNumWhiteKeysInRange (keyboard.getLowestVisibleKey(), keyboard.getHighestVisibleKey() + 1);

    // Draw keyboard background with subtle gradient shadow
    auto keyboardWidth = keyboard.getKeyStartRange().getEnd();
    auto shadowColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::whiteKeyShadowColorId);

    if (! shadowColor.isTransparent())
    {
        // Draw subtle top shadow gradient for depth
        ColorGradient shadowGradient;
        shadowGradient.addColorStop (shadowColor, Point<float> (0.0f, 0.0f), 0.0f);
        shadowGradient.addColorStop (shadowColor.withAlpha (0.0f), Point<float> (0.0f, 5.0f), 1.0f);

        g.setFillColorGradient (shadowGradient);
        g.fillRect (Rectangle<float> (0.0f, 0.0f, keyboardWidth, 5.0f));
    }

    // Draw separator line at bottom
    auto lineColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::keyOutlineColorId);
    if (! lineColor.isTransparent())
    {
        g.setFillColor (lineColor);
        g.fillRect (Rectangle<float> (0.0f, bounds.getHeight() - 1.0f, keyboardWidth, 1.0f));
    }

    // Paint white keys first
    for (int note = keyboard.getLowestVisibleKey(); note <= keyboard.getHighestVisibleKey(); ++note)
    {
        if (keyboard.isBlackKey (note))
            continue;

        bool isBlack;
        Rectangle<float> keyArea;
        keyboard.getKeyPosition (note, keyWidth, keyArea, isBlack);

        auto isPressed = keyboard.isNoteOn (note);
        auto isOver = keyboard.isMouseOverNote (note);

        // Base colors from theme
        auto whiteKeyColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::whiteKeyColorId);
        auto pressedColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::whiteKeyPressedColorId);
        auto outlineColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::keyOutlineColorId);

        // Determine fill color based on state
        Color fillColor = whiteKeyColor;
        if (isPressed)
            fillColor = pressedColor;
        if (isOver && ! isPressed)
            fillColor = whiteKeyColor.blendedWith (pressedColor.withAlpha (0.3f), BlendMode::SrcOver);

        // Fill the key
        g.setFillColor (fillColor);
        g.fillRect (keyArea);

        // Draw key separator line on the left edge
        if (! outlineColor.isTransparent())
        {
            g.setFillColor (outlineColor);
            g.fillRect (keyArea.removeFromLeft (1.0f));

            // Draw right edge for the last key
            if (note == keyboard.getHighestVisibleKey())
                g.fillRect (keyArea.removeFromRight (1.0f).translated (keyArea.getWidth(), 0.0f));
        }

        // Draw note text if there's space
        if (keyboard.getWidth() > 100 && keyArea.getWidth() > 15.0f)
        {
            auto noteText = String();
            int noteInOctave = note % 12;
            switch (noteInOctave)
            {
                case 0:
                    noteText = "C";
                    break;
                case 2:
                    noteText = "D";
                    break;
                case 4:
                    noteText = "E";
                    break;
                case 5:
                    noteText = "F";
                    break;
                case 7:
                    noteText = "G";
                    break;
                case 9:
                    noteText = "A";
                    break;
                case 11:
                    noteText = "B";
                    break;
                default:
                    break;
            }

            if (noteText.isNotEmpty())
            {
                auto textColor = outlineColor.contrasting (0.8f);
                if (isPressed)
                    textColor = pressedColor.contrasting (0.8f);

                g.setFillColor (textColor);

                StyledText styledText;
                {
                    auto modifier = styledText.startUpdate();
                    modifier.appendText (noteText, theme.getDefaultFont().withHeight (11.0f));
                    modifier.setHorizontalAlign (StyledText::center);
                }

                auto textArea = keyArea.reduced (2.0f).removeFromBottom (16.0f);
                g.fillFittedText (styledText, textArea);
            }
        }
    }

    // Paint black keys on top
    for (int note = keyboard.getLowestVisibleKey(); note <= keyboard.getHighestVisibleKey(); ++note)
    {
        if (! keyboard.isBlackKey (note))
            continue;

        bool isBlack;
        Rectangle<float> keyArea;
        keyboard.getKeyPosition (note, keyWidth, keyArea, isBlack);

        auto isPressed = keyboard.isNoteOn (note);
        auto isOver = keyboard.isMouseOverNote (note);

        // Base colors from theme
        auto blackKeyColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::blackKeyColorId);
        auto blackPressedColor = ApplicationTheme::findColor (MidiKeyboardComponent::Style::blackKeyPressedColorId);

        // Determine fill color based on state
        Color fillColor = blackKeyColor;
        if (isPressed)
            fillColor = blackPressedColor;
        if (isOver && ! isPressed)
            fillColor = blackKeyColor.blendedWith (blackPressedColor.withAlpha (0.3f), BlendMode::SrcOver);

        // Fill the key
        g.setFillColor (fillColor);
        g.fillRect (keyArea);

        if (isPressed)
        {
            // Draw pressed outline
            g.setStrokeColor (blackKeyColor);
            g.setStrokeWidth (1.0f);
            g.strokeRect (keyArea);
        }
        else
        {
            // Draw 3D highlight effect for unpressed keys
            auto highlightColor = fillColor.brighter (0.4f);
            g.setFillColor (highlightColor);

            // Create highlight area - top portion and side edges
            auto sideIndent = keyArea.getWidth() * 0.125f;
            auto topIndent = keyArea.getHeight() * 0.875f;
            auto highlightArea = keyArea.reduced (sideIndent, 0).removeFromTop (topIndent);

            g.fillRect (highlightArea);
        }
    }
}

void paintKMeter (Graphics& g, const ApplicationTheme& theme, const KMeterComponent& meter)
{
    const auto bounds = meter.getLocalBounds();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();

    if (bounds.isEmpty())
        return;

    // Get colors from theme
    const auto backgroundColor = meter.findColor (KMeterComponent::Style::backgroundColorId)
                                     .value_or (Color (0xff1a1a1a));
    const auto greenColor = meter.findColor (KMeterComponent::Style::greenZoneColorId)
                                .value_or (Color (0xff00cc00));
    const auto amberColor = meter.findColor (KMeterComponent::Style::amberZoneColorId)
                                .value_or (Color (0xffffaa00));
    const auto redColor = meter.findColor (KMeterComponent::Style::redZoneColorId)
                              .value_or (Color (0xffcc0000));
    const auto averageColor = meter.findColor (KMeterComponent::Style::averageLevelColorId)
                                  .value_or (Color (0xccffffff));
    const auto peakColor = meter.findColor (KMeterComponent::Style::peakLevelColorId)
                               .value_or (Color (0xffffffff));
    const auto peakClipColor = meter.findColor (KMeterComponent::Style::peakLevelClipColorId)
                                   .value_or (Color (0xffff0000));
    const auto peakHoldColor = meter.findColor (KMeterComponent::Style::peakHoldColorId)
                                   .value_or (Color (0xffffff00));

    // Draw background with subtle depth
    {
        ColorGradient backgroundGradient;
        backgroundGradient.addColorStop (backgroundColor.darker (0.15f),
                                         Point<float> (bounds.getX(), bounds.getY()),
                                         0.0f);
        backgroundGradient.addColorStop (backgroundColor,
                                         Point<float> (bounds.getX(), bounds.getY() + height * 0.5f),
                                         0.5f);
        backgroundGradient.addColorStop (backgroundColor.darker (0.25f),
                                         Point<float> (bounds.getX(), bounds.getBottom()),
                                         1.0f);

        g.setFillColorGradient (backgroundGradient);
        g.fillRect (bounds);
    }

    // Get K-System scale range for the meter state
    const auto scale = meter.meterState.getScale();
    const float rangeMin = KMeterState::rangeMinForScale (scale);
    const float rangeMax = KMeterState::rangeMaxForScale (scale);
    const float rangeSpan = rangeMax - rangeMin;

    auto linearDbToY = [&] (float db) -> float
    {
        const float clampedDb = jlimit (rangeMin, rangeMax, db);
        const float normalized = (clampedDb - rangeMin) / rangeSpan;
        return bounds.getY() + height * (1.0f - normalized);
    };

    std::function<float (float)> dbToY = linearDbToY;

    if (meter.getScaleMapping() == KMeterComponent::ScaleMapping::segmented)
    {
        struct MeterSegment
        {
            float topDb = 0.0f;
            float bottomDb = 0.0f;
            float topY = 0.0f;
            float bottomY = 0.0f;
            float heightUnits = 1.0f;
            Color color;
        };

        const float limitRedBars = 4.0f;
        const float limitAmberBars = 0.0f;
        float limitTopBars = 18.0f;
        float limitGreenBars = -24.0f;
        float limitLinearArea = -30.0f;
        bool useFiveUnitBottom = false;

        switch (scale)
        {
            case KMeterState::Scale::k20:
                limitTopBars = 18.0f;
                limitGreenBars = -24.0f;
                limitLinearArea = -30.0f;
                useFiveUnitBottom = false;
                break;
            case KMeterState::Scale::k14:
                limitTopBars = 12.0f;
                limitGreenBars = -30.0f;
                limitLinearArea = -30.0f;
                useFiveUnitBottom = false;
                break;
            case KMeterState::Scale::k12:
                limitTopBars = 10.0f;
                limitGreenBars = -30.0f;
                limitLinearArea = -30.0f;
                useFiveUnitBottom = true;
                break;
            default:
                break;
        }

        const Color nonLinearColor = greenColor.darker (0.35f);
        std::vector<MeterSegment> segments;
        segments.reserve (64);

        float currentDb = rangeMax;
        while (currentDb > rangeMin)
        {
            float segmentRange = 1.0f;

            if (currentDb > limitTopBars)
                segmentRange = 0.5f;
            else if (currentDb > limitGreenBars)
                segmentRange = 1.0f;
            else if (currentDb > limitLinearArea)
                segmentRange = 6.0f;
            else
                segmentRange = 10.0f;

            float bottomDb = currentDb - segmentRange;
            if (bottomDb < rangeMin)
                bottomDb = rangeMin;

            float heightUnits = 1.0f;
            if (currentDb > limitTopBars)
                heightUnits = 1.0f;
            else if (currentDb > limitGreenBars)
                heightUnits = 2.0f;
            else if (currentDb > limitLinearArea)
                heightUnits = 3.0f;
            else if (bottomDb <= rangeMin)
                heightUnits = useFiveUnitBottom ? 5.0f : 4.0f;
            else
                heightUnits = 3.0f;

            Color segmentColor = nonLinearColor;
            if (currentDb > limitRedBars)
                segmentColor = redColor;
            else if (currentDb > limitAmberBars)
                segmentColor = amberColor;
            else if (currentDb > limitGreenBars)
                segmentColor = greenColor;

            segments.push_back ({ currentDb, bottomDb, 0.0f, 0.0f, heightUnits, segmentColor });
            currentDb = bottomDb;
        }

        float totalUnits = 0.0f;
        for (const auto& segment : segments)
            totalUnits += segment.heightUnits;

        const float unitHeight = totalUnits > 0.0f ? height / totalUnits : height;
        float currentY = bounds.getY();

        for (size_t index = 0; index < segments.size(); ++index)
        {
            auto& segment = segments[index];
            const bool isLast = (index + 1 == segments.size());
            const float segmentHeight = isLast ? (bounds.getBottom() - currentY) : (segment.heightUnits * unitHeight);

            segment.topY = currentY;
            segment.bottomY = currentY + segmentHeight;
            currentY = segment.bottomY;
        }

        dbToY = [segments, rangeMin, rangeMax, bounds] (float db) -> float
        {
            const float clampedDb = jlimit (rangeMin, rangeMax, db);

            for (const auto& segment : segments)
            {
                if (clampedDb <= segment.topDb && clampedDb >= segment.bottomDb)
                {
                    const float range = segment.topDb - segment.bottomDb;
                    const float normalized = range > 0.0f ? (clampedDb - segment.bottomDb) / range : 0.0f;
                    return segment.bottomY - (segment.bottomY - segment.topY) * normalized;
                }
            }

            return bounds.getBottom();
        };
    }

    // Get current levels (thread-safe)
    const float averageDb = meter.currentAverageDb.get();
    const float peakDb = meter.currentPeakDb.get();
    const float peakHoldDb = meter.currentPeakHoldDb.get();
    const bool isClipping = meter.currentClipping.get();

    // Draw average level (filled bar from bottom with gradient)
    if (averageDb > rangeMin)
    {
        const float clampedAverageDb = jlimit (rangeMin, rangeMax, averageDb);
        const float barLeft = bounds.getX() + 2.0f;
        const float barWidth = width - 4.0f;
        const float barBottom = dbToY (rangeMin);
        const float barTop = dbToY (clampedAverageDb);
        const float fillHeight = barBottom - barTop;

        if (fillHeight > 0.0f)
        {
            const float barCenterX = barLeft + barWidth * 0.5f;
            const Point<float> gradientStart (barCenterX, bounds.getBottom());
            const Point<float> gradientEnd (barCenterX, bounds.getY());

            auto positionForDb = [&] (float db) -> float
            {
                const float y = dbToY (db);
                return jlimit (0.0f, 1.0f, (bounds.getBottom() - y) / height);
            };

            std::vector<ColorGradient::ColorStop> gradientStops;
            gradientStops.emplace_back (greenColor, gradientStart, positionForDb (rangeMin));

            if (rangeMax >= 0.0f)
            {
                const float zeroPos = positionForDb (0.0f);
                if (zeroPos > 0.0f && zeroPos < 1.0f)
                    gradientStops.emplace_back (amberColor, Point<float> (barCenterX, dbToY (0.0f)), zeroPos);
            }

            if (rangeMax >= 4.0f)
            {
                const float fourPos = positionForDb (4.0f);
                if (fourPos > 0.0f && fourPos < 1.0f)
                    gradientStops.emplace_back (redColor, Point<float> (barCenterX, dbToY (4.0f)), fourPos);
            }

            gradientStops.emplace_back (redColor, gradientEnd, positionForDb (rangeMax));

            ColorGradient gradient (ColorGradient::Type::Linear, std::move (gradientStops));
            g.setFillColorGradient (gradient.withMultipliedAlpha (averageColor.getAlphaFloat()));
            g.fillRect (barLeft, barTop, barWidth, fillHeight);
        }
    }

    // Draw peak hold marker first
    if (meter.getShowPeakHold() && peakHoldDb > rangeMin)
    {
        const float holdY = dbToY (peakHoldDb);
        if (holdY >= bounds.getY() && holdY <= bounds.getBottom())
        {
            g.setFillColor (peakHoldColor);
            const float lineTop = jlimit (bounds.getY(), bounds.getBottom() - 2.0f, holdY - 1.0f);
            g.fillRect (bounds.getX(), lineTop, width, 2.0f);
        }
    }

    // Draw peak level indicator
    if (meter.getShowPeak() && peakDb > rangeMin)
    {
        const float peakY = dbToY (peakDb);
        if (peakY >= bounds.getY() && peakY <= bounds.getBottom())
        {
            g.setFillColor (isClipping ? peakClipColor : peakColor);
            const float lineTop = jlimit (bounds.getY(), bounds.getBottom() - 2.0f, peakY - 1.0f);
            g.fillRect (bounds.getX(), lineTop, width, 2.0f);
        }
    }

    // Draw scale markers and labels
    {
        const auto& font = theme.getDefaultFont();
        g.setFillColor (Color (0xffffffff).withAlpha (0.7f));

        // Determine tick interval based on meter height and range
        const float dbPerPixel = rangeSpan / height;
        int tickInterval = 5; // Default: 5dB ticks

        // Adjust tick interval for better visibility
        if (height < 200)
            tickInterval = 10; // Fewer ticks for small meters

        // Draw tick marks and labels
        // Start from a nice round number
        const int startDb = static_cast<int> (std::ceil (rangeMin / tickInterval)) * tickInterval;
        const int endDb = static_cast<int> (std::floor (rangeMax / tickInterval)) * tickInterval;

        for (int db = startDb; db <= endDb; db += tickInterval)
        {
            const float y = dbToY (static_cast<float> (db));

            // Skip if outside visible area
            if (y < bounds.getY() || y > bounds.getBottom())
                continue;

            // Draw tick mark
            g.fillRect (bounds.getX(), y - 0.5f, 4.0f, 1.0f);

            // Draw label for important values
            bool shouldLabel = false;

            // Always label 0dB (the reference)
            if (db == 0)
                shouldLabel = true;
            // Label every 10dB
            else if (db % 10 == 0)
                shouldLabel = true;
            // Label -20dB (important for K-20)
            else if (db == -20)
                shouldLabel = true;

            if (shouldLabel)
            {
                const String label = (db > 0 ? "+" : "") + String (db);
                const float labelHeight = 12.0f;
                const Rectangle<float> labelRect (bounds.getX() + 5.0f, y - 6.0f, 22.0f, labelHeight);

                // Only draw label if it's fully visible (not clipped at top or bottom)
                if (labelRect.getY() >= bounds.getY() && labelRect.getBottom() <= bounds.getBottom())
                {
                    g.fillFittedText (label, font, labelRect, Justification::left);
                }
            }
        }
    }

    // Draw 0dB reference line
    {
        const float zeroDbY = dbToY (0.0f);
        g.setStrokeColor (Color (0xffffffff).withAlpha (0.5f));
        g.setStrokeWidth (1.5f);
        g.strokeLine (Point<float> (bounds.getX(), zeroDbY), Point<float> (bounds.getRight(), zeroDbY));
    }
}
#endif

//==============================================================================

ApplicationTheme::Ptr createThemeVersion1()
{
    ApplicationTheme::Ptr theme (new ApplicationTheme);

    {
        Font font;
        if (auto result = font.loadFromData (MemoryBlock (&RobotoFlexFont_data[0], RobotoFlexFont_size)); result.failed())
            yup::Logger::outputDebugString (result.getErrorMessage());

        theme->setDefaultFont (std::move (font));
    }

    {
        Font font;
        if (auto result = font.loadFromData (MemoryBlock (&FontAwesome7Font_data[0], FontAwesome7Font_size)); result.failed())
            yup::Logger::outputDebugString (result.getErrorMessage());

        theme->setDefaultIconFont (std::move (font));
    }

    theme->setComponentStyle<Slider> (ComponentStyle::createStyle<Slider> (paintSlider));
    theme->setColor (Slider::Style::backgroundColorId, Color (0xff3d3d3d));
    theme->setColor (Slider::Style::trackColorId, Color (0xff636363));
    theme->setColor (Slider::Style::thumbColorId, Color (0xff4ebfff));
    theme->setColor (Slider::Style::thumbOverColorId, Color (0xff4ebfff).brighter (0.3f));
    theme->setColor (Slider::Style::thumbDownColorId, Color (0xff4ebfff).darker (0.2f));
    theme->setColor (Slider::Style::textColorId, Colors::white);

    theme->setComponentStyle<TextButton> (ComponentStyle::createStyle<TextButton> (paintTextButton));
    theme->setComponentStyle<ToggleButton> (ComponentStyle::createStyle<ToggleButton> (paintToggleButton));
    theme->setComponentStyle<SwitchButton> (ComponentStyle::createStyle<SwitchButton> (paintSwitchButton));
    theme->setComponentStyle<TextEditor> (ComponentStyle::createStyle<TextEditor> (paintTextEditor));
    theme->setComponentStyle<ComboBox> (ComponentStyle::createStyle<ComboBox> (paintComboBox));

    theme->setComponentStyle<Label> (ComponentStyle::createStyle<Label> (paintLabel));
    theme->setColor (Label::Style::textFillColorId, Colors::white);
    theme->setColor (Label::Style::textStrokeColorId, Colors::transparentBlack);
    theme->setColor (Label::Style::backgroundColorId, Colors::transparentBlack);
    theme->setColor (Label::Style::outlineColorId, Colors::transparentBlack);

    theme->setComponentStyle<PopupMenu> (ComponentStyle::createStyle<PopupMenu> (paintPopupMenu));

    theme->setComponentStyle<ScrollBar> (ComponentStyle::createStyle<ScrollBar> (paintScrollBar));
    theme->setColor (ScrollBar::Style::trackColorId, Color (0xff3d3d3d));
    theme->setColor (ScrollBar::Style::thumbColorId, Color (0x55000000));
    theme->setColor (ScrollBar::Style::thumbHoverColorId, Color (0x77000000));
    theme->setColor (ScrollBar::Style::thumbDraggingColorId, Color (0x99000000));

    theme->setComponentStyle<ProgressBar> (ComponentStyle::createStyle<ProgressBar> (paintProgressBar));
    theme->setColor (ProgressBar::Style::backgroundColorId, Color (0xff3d3d3d));
    theme->setColor (ProgressBar::Style::foregroundColorId, Color (0xff4ebfff));

    theme->setComponentStyle<ListBoxItem> (ComponentStyle::createStyle<ListBoxItem> (paintListBoxItem));
    theme->setColor (ListBoxItem::Style::textColorId, Colors::black);
    theme->setColor (ListBoxItem::Style::textColorSelectedId, Colors::white);
    theme->setColor (ListBoxItem::Style::backgroundColorId, Colors::transparentBlack);
    theme->setColor (ListBoxItem::Style::backgroundColorSelectedId, Color (0xff3a7ebf));
    theme->setColor (ListBoxItem::Style::backgroundColorHoveredId, Color (0x22ffffff));

#if YUP_MODULE_AVAILABLE_yup_audio_gui
    theme->setComponentStyle<MidiKeyboardComponent> (ComponentStyle::createStyle<MidiKeyboardComponent> (paintMidiKeyboard));
    theme->setColor (MidiKeyboardComponent::Style::whiteKeyColorId, Color (0xfff0f0f0));
    theme->setColor (MidiKeyboardComponent::Style::whiteKeyPressedColorId, Color (0xff4ebfff));
    theme->setColor (MidiKeyboardComponent::Style::whiteKeyShadowColorId, Color (0x40000000));
    theme->setColor (MidiKeyboardComponent::Style::blackKeyColorId, Color (0xff2a2a2a));
    theme->setColor (MidiKeyboardComponent::Style::blackKeyPressedColorId, Color (0xff4ebfff));
    theme->setColor (MidiKeyboardComponent::Style::blackKeyShadowColorId, Color (0x80000000));
    theme->setColor (MidiKeyboardComponent::Style::keyOutlineColorId, Color (0xff888888));

    theme->setComponentStyle<KMeterComponent> (ComponentStyle::createStyle<KMeterComponent> (paintKMeter));
    theme->setColor (KMeterComponent::Style::backgroundColorId, Color (0xff1a1a1a));
    theme->setColor (KMeterComponent::Style::greenZoneColorId, Color (0xff00cc00));
    theme->setColor (KMeterComponent::Style::amberZoneColorId, Color (0xffffaa00));
    theme->setColor (KMeterComponent::Style::redZoneColorId, Color (0xffcc0000));
    theme->setColor (KMeterComponent::Style::averageLevelColorId, Color (0xccffffff));
    theme->setColor (KMeterComponent::Style::peakLevelColorId, Color (0xffffffff));
    theme->setColor (KMeterComponent::Style::peakLevelClipColorId, Color (0xffff0000));
    theme->setColor (KMeterComponent::Style::peakHoldColorId, Color (0xffffff00));
#endif

    return theme;
}

} // namespace yup
