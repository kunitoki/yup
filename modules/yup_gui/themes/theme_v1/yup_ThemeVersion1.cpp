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

namespace yup
{

//==============================================================================

extern const uint8_t RobotoFlexFont_data[];
extern const std::size_t RobotoFlexFont_size;

//==============================================================================

void paintSlider (Graphics& g, const ApplicationTheme& theme, const Slider& s)
{
    auto bounds = s.getLocalBounds().reduced (s.proportionOfWidth (0.1f));
    const auto center = bounds.getCenter();

    constexpr auto fromRadians = degreesToRadians (135.0f);
    constexpr auto toRadians = fromRadians + degreesToRadians (270.0f);

    Path backgroundPath;
    backgroundPath.addEllipse (bounds.reduced (s.proportionOfWidth (0.105f)));

    g.setFillColor (Color (0xff3d3d3d)); // TODO - findColor
    g.fillPath (backgroundPath);

    g.setStrokeColor (Color (0xff2b2b2b)); // TODO - findColor
    g.setStrokeWidth (s.proportionOfWidth (0.0175f));
    g.strokePath (backgroundPath);

    Path backgroundArc;
    backgroundArc.addCenteredArc (center,
                                  bounds.getWidth() / 2.0f,
                                  bounds.getHeight() / 2.0f,
                                  0.0f,
                                  fromRadians,
                                  toRadians,
                                  true);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (Color (0xff636363)); // TODO - findColor
    g.setStrokeWidth (s.proportionOfWidth (0.075f));
    g.strokePath (backgroundArc);

    const auto toCurrentRadians = fromRadians + degreesToRadians (270.0f) * s.getValueNormalised();

    Path foregroundArc;
    foregroundArc.addCenteredArc (center,
                                  bounds.getWidth() / 2.0f,
                                  bounds.getHeight() / 2.0f,
                                  0.0f,
                                  fromRadians,
                                  toCurrentRadians,
                                  true);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (s.isMouseOver() ? Color (0xff4ebfff).brighter (0.3f) : Color (0xff4ebfff)); // TODO - findColor
    g.setStrokeWidth (s.proportionOfWidth (0.075f));
    g.strokePath (foregroundArc);

    const auto reducedBounds = bounds.reduced (s.proportionOfWidth (0.175f));
    const auto pos = center.getPointOnCircumference (
        reducedBounds.getWidth() / 2.0f,
        reducedBounds.getHeight() / 2.0f,
        toCurrentRadians);

    Path foregroundLine;
    foregroundLine.addLine (Line<float> (pos, center).keepOnlyStart (0.25f));

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (Color (0xffffffff)); // TODO - findColor
    g.setStrokeWidth (s.proportionOfWidth (0.03f));
    g.strokePath (foregroundLine);

    /*
    const auto& font = theme.getDefaultFont();
    StyledText text;
    text.appendText (font, s.proportionOfHeight (0.1f), s.proportionOfHeight (0.1f), String (s.getValue(), 3).toRawUTF8());
    text.layout (s.getLocalBounds().reduced (5).removeFromBottom (s.proportionOfWidth (0.1f)), StyledText::center);

    g.setStrokeColor (Color (0xffffffff));
    g.strokeFittedText (text, s.getLocalBounds().reduced (5).removeFromBottom (s.proportionOfWidth (0.1f)));
    */

    if (s.hasKeyboardFocus())
    {
        g.setStrokeColor (Color (0xffff5f2b)); // TODO - findColor
        g.setStrokeWidth (s.proportionOfWidth (0.0175f));
        g.strokeRect (s.getLocalBounds());
    }
}

void paintTextEditor (Graphics& g, const ApplicationTheme& theme, const TextEditor& t)
{
    auto bounds = t.getLocalBounds();
    auto textBounds = t.getTextBounds();
    auto scrollOffset = t.getScrollOffset();
    constexpr auto cornerRadius = 6.0f;

    // Draw background
    auto backgroundColor = t.findColor (TextEditor::Colors::backgroundColorId).value_or (Colors::white);
    g.setFillColor (backgroundColor);
    g.fillRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw outline
    auto outlineColor = t.hasKeyboardFocus()
                          ? t.findColor (TextEditor::Colors::focusedOutlineColorId).value_or (Colors::cornflowerblue)
                          : t.findColor (TextEditor::Colors::outlineColorId).value_or (Colors::gray);
    g.setStrokeColor (outlineColor);

    float strokeWidth = t.hasKeyboardFocus() ? 2.0f : 1.0f;
    g.setStrokeWidth (strokeWidth);

    g.strokeRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw selection background
    if (t.hasSelection())
    {
        auto selectionColor = t.findColor (TextEditor::Colors::selectionColorId).value_or (Colors::cornflowerblue.withAlpha (0.5f));
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
    auto textColor = t.findColor (TextEditor::Colors::textColorId).value_or (Colors::gray);
    g.setFillColor (textColor);

    auto scrolledTextBounds = textBounds.translated (-scrollOffset.getX(), -scrollOffset.getY());
    g.fillFittedText (t.getStyledText(), scrolledTextBounds);

    // Draw caret
    if (t.hasKeyboardFocus() && t.isCaretVisible())
    {
        auto caretColor = t.findColor (TextEditor::Colors::caretColorId).value_or (yup::Colors::black);
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
        backgroundColor = b.findColor (TextButton::Colors::backgroundPressedColorId).value_or (Colors::gray);
        textColor = b.findColor (TextButton::Colors::textPressedColorId).value_or (Colors::dimgray);
    }
    else
    {
        backgroundColor = b.findColor (TextButton::Colors::backgroundColorId).value_or (Colors::gray);
        textColor = b.findColor (TextButton::Colors::textColorId).value_or (Colors::white);
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
                           ? b.findColor (TextButton::Colors::outlineFocusedColorId).value_or (Colors::cornflowerblue)
                           : b.findColor (TextButton::Colors::outlineColorId).value_or (Colors::dimgray);
    g.setStrokeColor (outlineColor);

    float strokeWidth = b.hasKeyboardFocus() ? 2.0f : 1.0f;
    g.setStrokeWidth (strokeWidth);

    g.strokeRoundedRect (bounds.reduced (1.0f), cornerRadius);

    // Draw text
    g.setFillColor (textColor);
    g.fillFittedText (b.getStyledText(), b.getTextBounds());
}

//==============================================================================

void paintLabel (Graphics& g, const ApplicationTheme& theme, const Label& l)
{
    auto& styledText = l.getStyledText();
    const auto bounds = l.getLocalBounds();

    if (const auto strokeColor = l.findColor (Label::Colors::strokeColorId); strokeColor && ! strokeColor->isTransparent())
    {
        g.setStrokeColor (*strokeColor);
        g.setStrokeWidth (l.getStrokeWidth());
        g.strokeFittedText (styledText, bounds);
    }

    const auto fillColor = l.findColor (Label::Colors::fillColorId).value_or (Colors::white);
    g.setFillColor (fillColor);
    g.fillFittedText (styledText, bounds);
}

//==============================================================================

void paintPopupMenu (Graphics& g, const ApplicationTheme& theme, const PopupMenu& p)
{
    // Draw drop shadow if enabled
    if (false) // owner->options.addAsChildToTopmost)
    {
        auto shadowBounds = p.getLocalBounds().to<float>();
        auto shadowRadius = static_cast<float> (8.0f);

        g.setFillColor (Color (0, 0, 0));
        g.setFeather (shadowRadius);
        g.fillRoundedRect (shadowBounds.translated (0.0f, 2.0f).enlarged (2.0f), 4.0f);
        g.setFeather (0.0f);
    }

    // Draw menu background
    g.setFillColor (p.findColor (PopupMenu::Colors::menuBackground).value_or (Color (0xff2a2a2a)));
    g.fillRoundedRect (p.getLocalBounds().to<float>(), 4.0f);

    // Draw border
    g.setStrokeColor (p.findColor (PopupMenu::Colors::menuBorder).value_or (Color (0xff555555)));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (p.getLocalBounds().to<float>().reduced (0.5f), 4.0f);

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

        // Draw hover background
        if (item->isHovered && ! item->isSeparator() && item->isEnabled)
        {
            g.setFillColor (p.findColor (PopupMenu::Colors::menuItemBackgroundHighlighted).value_or (Color (0xff404040)));
            g.fillRoundedRect (rect.reduced (2.0f, 1.0f), 2.0f);
        }

        if (item->isSeparator())
        {
            // Draw separator line
            auto lineY = rect.getCenterY();
            g.setStrokeColor (p.findColor (PopupMenu::Colors::menuBorder).value_or (Color (0xff555555)));
            g.setStrokeWidth (1.0f);
            g.strokeLine (rect.getX() + 8.0f, lineY, rect.getRight() - 8.0f, lineY);
        }
        else
        {
            // Draw menu item text
            auto textColor = item->textColor.value_or (p.findColor (PopupMenu::Colors::menuItemText).value_or (Color (0xffffffff)));
            if (! item->isEnabled)
                textColor = p.findColor (PopupMenu::Colors::menuItemTextDisabled).value_or (Color (0xff808080));

            g.setFillColor (textColor);

            auto textRect = rect.reduced (12.0f, 2.0f);
            if (anyItemIsTicked)
                textRect.setX (textRect.getX() + 8.0f);

            {
                auto styledText = yup::StyledText();
                {
                    auto modifier = styledText.startUpdate();
                    modifier.appendText (item->text, itemFont, 14.0f);
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
                auto shortcutRect = Rectangle<float> (rect.getRight() - 80.0f, rect.getY(), 75.0f, rect.getHeight());

                auto styledText = yup::StyledText();
                {
                    auto modifier = styledText.startUpdate();
                    modifier.setHorizontalAlign (yup::StyledText::right);
                    modifier.appendText (item->shortcutKeyText, itemFont, 13.0f);
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
}

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
    theme->setComponentStyle<TextButton> (ComponentStyle::createStyle<TextButton> (paintTextButton));
    theme->setComponentStyle<TextEditor> (ComponentStyle::createStyle<TextEditor> (paintTextEditor));

    theme->setComponentStyle<Label> (ComponentStyle::createStyle<Label> (paintLabel));
    theme->setColor (Label::Colors::fillColorId, Colors::white);
    theme->setColor (Label::Colors::strokeColorId, Colors::transparentBlack);

    theme->setComponentStyle<PopupMenu> (ComponentStyle::createStyle<PopupMenu> (paintPopupMenu));

    return theme;
}

} // namespace yup
