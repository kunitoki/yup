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

namespace
{

struct LabelIds
{
    static inline const Identifier text { "text" };
    static inline const Identifier strokeWidth { "strokeWidth" };
    static inline const Identifier justification { "justification" };
};

} // namespace

//==============================================================================

const Identifier Label::Style::textFillColorId { "labelTextFillColorId" };
const Identifier Label::Style::textStrokeColorId { "labelTextStrokeColorId" };
const Identifier Label::Style::backgroundColorId { "labelBackgroundColorId" };
const Identifier Label::Style::outlineColorId { "labelOutlineColorId" };
const Identifier Label::Style::textHeightProportionId { "labelTextHeightProportionId" };

//==============================================================================

const ComponentTypeInfo& Label::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<Label> (Identifier ("Label"))
                                 .color ("textFillColor", Style::textFillColorId)
                                 .color ("textStrokeColor", Style::textStrokeColorId)
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("outlineColor", Style::outlineColorId)
                                 .number ("textHeightProportion", Style::textHeightProportionId, {}, "The height of the text as a proportion of the label height. Unset, the font keeps its own height.")
                                 .property (LabelIds::text, &Label::getText, [] (Label& l, const String& v) { l.setText (v); })
                                 .property (LabelIds::strokeWidth, &Label::getStrokeWidth, &Label::setStrokeWidth)
                                 .enumeration<Justification::Flags> (
                                     LabelIds::justification,
                                     { { "left", Justification::left },
                                       { "right", Justification::right },
                                       { "topLeft", Justification::topLeft },
                                       { "centerTop", Justification::centerTop },
                                       { "topRight", Justification::topRight },
                                       { "centerLeft", Justification::centerLeft },
                                       { "center", Justification::center },
                                       { "centerRight", Justification::centerRight },
                                       { "bottomLeft", Justification::bottomLeft },
                                       { "centerBottom", Justification::centerBottom },
                                       { "bottomRight", Justification::bottomRight } },
                                     [] (const Label& l) { return l.getJustification().getFlags(); },
                                     [] (Label& l, Justification::Flags f) { l.setJustification (f); })
                                 .build();

    return info;
}

//==============================================================================

Label::Label()
    : Label (StringRef {})
{
}

Label::Label (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
}

//==============================================================================

String Label::getText() const
{
    return text;
}

void Label::setText (String newText, NotificationType notification)
{
    ignoreUnused (notification);

    if (text == newText)
        return;

    text = std::move (newText);
    textDirty = true;
    invalidateCache();

    notifyPropertyChanged (LabelIds::text);
}

//==============================================================================

std::optional<Font> Label::getFont() const
{
    return font;
}

void Label::setFont (Font newFont)
{
    if (font && *font == newFont)
        return;

    font = std::move (newFont);
    invalidateCache();
}

void Label::resetFont()
{
    if (! font.has_value())
        return;

    font.reset();
    invalidateCache();
}

//==============================================================================

void Label::setStrokeWidth (float newWidth)
{
    if (strokeWidth == newWidth)
        return;

    strokeWidth = newWidth;
    notifyPropertyChanged (LabelIds::strokeWidth);
    repaint();
}

void Label::setJustification (Justification newJustification)
{
    if (justification == newJustification)
        return;

    justification = newJustification;
    invalidateCache();
    notifyPropertyChanged (LabelIds::justification);
}

//==============================================================================

void Label::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
    {
        if (needsUpdate)
            prepareText();

        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
    }
}

//==============================================================================

void Label::resized()
{
    invalidateCache();
}

void Label::styleChanged()
{
    invalidateCache();
}

//==============================================================================

void Label::prepareText()
{
    if (! needsUpdate)
        return;

    auto fontToUse = font.value_or (ApplicationTheme::getGlobalTheme()->getDefaultFont());

    static const Identifier textHeightProportion ("textHeightProportion");
    if (const auto proportion = findProperty (textHeightProportion); ! proportion.isVoid())
        fontToUse = fontToUse.withHeight (static_cast<float> (getHeight()) * static_cast<float> (static_cast<double> (proportion)));

    const auto currentSize = getSize();
    if (! textDirty && fontToUse == preparedFont && currentSize == preparedSize && justification == preparedJustification)
    {
        needsUpdate = false;
        return;
    }

    auto setup = [&] (const Font& f)
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (currentSize);
        modifier.setHorizontalAlign (StyledText::toHorizontalAlign (justification));
        modifier.setVerticalAlign (StyledText::toVerticalAlign (justification));
        modifier.setOverflow (StyledText::visible);
        modifier.setWrap (StyledText::noWrap);
        modifier.clear();
        if (text.isNotEmpty())
            modifier.appendText (text, f);
    };

    setup (fontToUse);

    if (text.isNotEmpty())
    {
        const float availableWidth = currentSize.getWidth();
        const float textWidth = styledText.getComputedTextBounds().getWidth();
        if (availableWidth > 0.0f && textWidth > availableWidth)
            setup (fontToUse.withHeight (fontToUse.getHeight() * (availableWidth / textWidth)));
    }

    preparedFont = fontToUse;
    preparedSize = currentSize;
    preparedJustification = justification;
    textDirty = false;
    needsUpdate = false;
}

void Label::invalidateCache()
{
    needsUpdate = true;
    repaint();
}

} // namespace yup
