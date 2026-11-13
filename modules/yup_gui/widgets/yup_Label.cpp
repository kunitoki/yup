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

const Identifier Label::Style::textFillColorId { "Label_textFillColorId" };
const Identifier Label::Style::textStrokeColorId { "Label_textStrokeColorId" };
const Identifier Label::Style::backgroundColorId { "Label_backgroundColorId" };
const Identifier Label::Style::outlineColorId { "Label_outlineColorId" };
const Identifier Label::Style::textHeightProportionId { "Label_textHeightProportionId" };

//==============================================================================

const ComponentTypeInfo& Label::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (makeColorProperty (Identifier ("textFillColor"), Style::textFillColorId));
        props.push_back (makeColorProperty (Identifier ("textStrokeColor"), Style::textStrokeColorId));
        props.push_back (makeColorProperty (Identifier ("backgroundColor"), Style::backgroundColorId));
        props.push_back (makeColorProperty (Identifier ("outlineColor"), Style::outlineColorId));

        props.push_back (ComponentProperty {
            .name = Identifier ("textHeightProportion"),
            .storageId = Style::textHeightProportionId,
            .type = ComponentProperty::Type::number,
            .defaultValue = 0.75,
            .description = "The proportion of the label height used for the text." });

        props.push_back (ComponentProperty {
            .name = Identifier ("text"),
            .type = ComponentProperty::Type::string,
            .getter = [] (const Component& c) { return var (static_cast<const Label&> (c).getText()); },
            .setter = [] (Component& c, const var& v) { static_cast<Label&> (c).setText (v.toString()); },
            .isStyleable = false });

        props.push_back (ComponentProperty {
            .name = Identifier ("strokeWidth"),
            .type = ComponentProperty::Type::number,
            .getter = [] (const Component& c) { return var (static_cast<double> (static_cast<const Label&> (c).getStrokeWidth())); },
            .setter = [] (Component& c, const var& v) { static_cast<Label&> (c).setStrokeWidth (static_cast<float> (static_cast<double> (v))); },
            .isStyleable = false });

        return ComponentTypeInfo { Identifier ("Label"), &Component::getClassTypeInfo(), std::move (props), { "onTextChanged" }, [] { return std::make_unique<Label>(); } };
    }();

    return info;
}

const ComponentTypeInfo& Label::getTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

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
    if (text == newText)
        return;

    text = std::move (newText);
    invalidateCache();

    if (notification != dontSendNotification)
        // TODO: Send notification
        ;
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
    font.reset();
}

//==============================================================================

void Label::setStrokeWidth (float newWidth)
{
    if (strokeWidth == newWidth)
        return;

    strokeWidth = newWidth;
    repaint();
}

void Label::setJustification (Justification newJustification)
{
    if (justification == newJustification)
        return;

    justification = newJustification;
    repaint();
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

//==============================================================================

void Label::prepareText()
{
    if (! needsUpdate)
        return;

    auto fontToUse = font.value_or (ApplicationTheme::getGlobalTheme()->getDefaultFont());

    auto setup = [&] (const Font& f)
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (getSize());
        modifier.setHorizontalAlign (StyledText::horizontalAlignFromJustification (justification));
        modifier.setVerticalAlign (StyledText::verticalAlignFromJustification (justification));
        modifier.setOverflow (StyledText::visible);
        modifier.setWrap (StyledText::noWrap);
        modifier.clear();
        if (text.isNotEmpty())
            modifier.appendText (text, f);
    };

    setup (fontToUse);

    if (text.isNotEmpty())
    {
        const float availableWidth = static_cast<float> (getWidth());
        const float textWidth = styledText.getComputedTextBounds().getWidth();
        if (availableWidth > 0.0f && textWidth > availableWidth)
            setup (fontToUse.withHeight (fontToUse.getHeight() * (availableWidth / textWidth)));
    }

    needsUpdate = false;
}

void Label::invalidateCache()
{
    needsUpdate = true;
    repaint();
}

} // namespace yup
