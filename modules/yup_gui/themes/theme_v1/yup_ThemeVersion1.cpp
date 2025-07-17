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

void paintRotarySlider (Graphics& g, const ApplicationTheme& theme, const Slider& slider,
                        Rectangle<float> sliderBounds, float rotaryStartAngle, float rotaryEndAngle,
                        float sliderValue, bool isMouseOver, bool isMouseDown)
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
    g.setStrokeWidth (slider.proportionOfWidth (0.035f));
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

void paintLinearSlider (Graphics& g, const ApplicationTheme& theme, const Slider& slider,
                        Rectangle<float> sliderBounds, Rectangle<float> thumbBounds,
                        bool isHorizontal, float sliderValue, bool isMouseOver, bool isMouseDown)
{
    const auto colors = getSliderColors (theme, slider);

    // Draw track background
    g.setFillColor (colors.background);
    if (isHorizontal)
        g.fillRoundedRect (sliderBounds.getX(), sliderBounds.getCenterY() - 2.0f,
                           sliderBounds.getWidth(), 4.0f, 2.0f);
    else
        g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f, sliderBounds.getY(),
                           4.0f, sliderBounds.getHeight(), 2.0f);

    // Draw value track for bar sliders
    const auto sliderType = slider.getSliderType();
    if (sliderType == Slider::LinearBarHorizontal || sliderType == Slider::LinearBarVertical)
    {
        g.setFillColor (colors.track);
        if (isHorizontal)
            g.fillRoundedRect (sliderBounds.getX(), sliderBounds.getCenterY() - 2.0f,
                               sliderValue * sliderBounds.getWidth(), 4.0f, 2.0f);
        else
            g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f,
                               sliderBounds.getBottom() - (sliderValue * sliderBounds.getHeight()),
                               4.0f, sliderValue * sliderBounds.getHeight(), 2.0f);
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

void paintTwoValueSlider (Graphics& g, const ApplicationTheme& theme, const Slider& slider,
                          Rectangle<float> sliderBounds, Rectangle<float> minThumbBounds,
                          Rectangle<float> maxThumbBounds, bool isHorizontal, float minValue, float maxValue,
                          bool isMouseOverMinThumb, bool isMouseOverMaxThumb, bool isMouseDown)
{
    const auto colors = getSliderColors (theme, slider);

    // Draw track background
    g.setFillColor (colors.background);
    if (isHorizontal)
        g.fillRoundedRect (sliderBounds.getX(), sliderBounds.getCenterY() - 2.0f,
                          sliderBounds.getWidth(), 4.0f, 2.0f);
    else
        g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f, sliderBounds.getY(),
                          4.0f, sliderBounds.getHeight(), 2.0f);

    // Draw selected range
    g.setFillColor (colors.track);
    if (isHorizontal)
    {
        const float startX = sliderBounds.getX() + (minValue * sliderBounds.getWidth());
        const float endX = sliderBounds.getX() + (maxValue * sliderBounds.getWidth());
        g.fillRoundedRect (startX, sliderBounds.getCenterY() - 2.0f,
                          endX - startX, 4.0f, 2.0f);
    }
    else
    {
        const float startY = sliderBounds.getBottom() - (minValue * sliderBounds.getHeight());
        const float endY = sliderBounds.getBottom() - (maxValue * sliderBounds.getHeight());
        g.fillRoundedRect (sliderBounds.getCenterX() - 2.0f, endY,
                          4.0f, startY - endY, 2.0f);
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
            paintRotarySlider (g, theme, s, sliderBounds, rotaryStartAngle, rotaryEndAngle,
                               static_cast<float>(s.getValueNormalised()), isMouseOver, isMouseDown);
            break;
        }

        case Slider::LinearHorizontal:
        case Slider::LinearVertical:
        case Slider::LinearBarHorizontal:
        case Slider::LinearBarVertical:
        {
            Rectangle<float> thumbBounds;
            const bool isHorizontal = (sliderType == Slider::LinearHorizontal || sliderType == Slider::LinearBarHorizontal);
            const float sliderValue = static_cast<float>(s.getValueNormalised());

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

            paintLinearSlider (g, theme, s, sliderBounds, thumbBounds, isHorizontal,
                               sliderValue, isMouseOver, isMouseDown);
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
                const Point<float> mousePosFloat (static_cast<float>(mousePos.getX()), static_cast<float>(mousePos.getY()));
                isMouseOverMinThumb = minThumbBounds.contains (mousePosFloat);
                isMouseOverMaxThumb = maxThumbBounds.contains (mousePosFloat);
            }

            paintTwoValueSlider (g, theme, s, sliderBounds, minThumbBounds, maxThumbBounds,
                                 isHorizontal, minNorm, maxNorm, isMouseOverMinThumb, isMouseOverMaxThumb, isMouseDown);
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
        if (! keyboard.isBlackKey (note))
        {
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
                fillColor = whiteKeyColor.overlaidWith (pressedColor.withAlpha (0.3f));

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
    }

    // Paint black keys on top
    for (int note = keyboard.getLowestVisibleKey(); note <= keyboard.getHighestVisibleKey(); ++note)
    {
        if (keyboard.isBlackKey (note))
        {
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
                fillColor = blackKeyColor.overlaidWith (blackPressedColor.withAlpha (0.3f));

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

#if YUP_MODULE_AVAILABLE_yup_audio_gui
    theme->setComponentStyle<MidiKeyboardComponent> (ComponentStyle::createStyle<MidiKeyboardComponent> (paintMidiKeyboard));
    theme->setColor (MidiKeyboardComponent::Style::whiteKeyColorId, Color (0xfff0f0f0));
    theme->setColor (MidiKeyboardComponent::Style::whiteKeyPressedColorId, Color (0xff4ebfff));
    theme->setColor (MidiKeyboardComponent::Style::whiteKeyShadowColorId, Color (0x40000000));
    theme->setColor (MidiKeyboardComponent::Style::blackKeyColorId, Color (0xff2a2a2a));
    theme->setColor (MidiKeyboardComponent::Style::blackKeyPressedColorId, Color (0xff4ebfff));
    theme->setColor (MidiKeyboardComponent::Style::blackKeyShadowColorId, Color (0x80000000));
    theme->setColor (MidiKeyboardComponent::Style::keyOutlineColorId, Color (0xff888888));
#endif

    return theme;
}

} // namespace yup
