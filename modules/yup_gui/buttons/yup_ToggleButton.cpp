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

const Identifier ToggleButton::Style::backgroundColorId = "toggleButtonBackground";
const Identifier ToggleButton::Style::backgroundToggledColorId = "toggleButtonBackgroundToggled";
const Identifier ToggleButton::Style::textColorId = "toggleButtonText";
const Identifier ToggleButton::Style::textToggledColorId = "toggleButtonTextToggled";
const Identifier ToggleButton::Style::borderColorId = "toggleButtonBorder";
const Identifier ToggleButton::Style::borderToggledColorId = "toggleButtonBorderToggled";

//==============================================================================

const ComponentTypeInfo& ToggleButton::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (makeColorProperty (Identifier ("backgroundColor"), Style::backgroundColorId));
        props.push_back (makeColorProperty (Identifier ("backgroundToggledColor"), Style::backgroundToggledColorId));
        props.push_back (makeColorProperty (Identifier ("textColor"), Style::textColorId));
        props.push_back (makeColorProperty (Identifier ("textToggledColor"), Style::textToggledColorId));
        props.push_back (makeColorProperty (Identifier ("borderColor"), Style::borderColorId));
        props.push_back (makeColorProperty (Identifier ("borderToggledColor"), Style::borderToggledColorId));

        props.push_back (ComponentProperty {
            .name = Identifier ("checked"),
            .type = ComponentProperty::Type::boolean,
            .getter = [] (const Component& c) { return var (static_cast<const ToggleButton&> (c).getToggleState()); },
            .setter = [] (Component& c, const var& v) { static_cast<ToggleButton&> (c).setToggleState (static_cast<bool> (v)); },
            .isState = true,
            .isStyleable = false });

        props.push_back (ComponentProperty {
            .name = Identifier ("text"),
            .type = ComponentProperty::Type::string,
            .getter = [] (const Component& c) { return var (static_cast<const ToggleButton&> (c).getButtonText()); },
            .setter = [] (Component& c, const var& v) { static_cast<ToggleButton&> (c).setButtonText (v.toString()); },
            .isStyleable = false });

        return ComponentTypeInfo { Identifier ("ToggleButton"), &Component::getClassTypeInfo(), std::move (props), { Button::getClickSignal() }, [] { return std::make_unique<ToggleButton>(); } };
    }();

    return info;
}

const ComponentTypeInfo& ToggleButton::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

ToggleButton::ToggleButton (StringRef componentID)
    : Button (componentID)
{
    setOpaque (false);
}

//==============================================================================

void ToggleButton::setToggleState (bool shouldBeToggled, NotificationType notification)
{
    if (toggleState != shouldBeToggled)
    {
        toggleState = shouldBeToggled;

        sendChangeNotification (notification, [this]
        {
            toggleStateChanged();

            if (onClick)
                onClick();
        });

        repaint();
    }
}

void ToggleButton::setButtonText (String newText)
{
    if (buttonText != newText)
    {
        buttonText = newText;
        resized();
    }
}

//==============================================================================

void ToggleButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void ToggleButton::resized()
{
    if (buttonText.isNotEmpty())
    {
        auto bounds = getLocalBounds();
        auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

        {
            auto modifier = styledText.startUpdate();
            modifier.setMaxSize (bounds.getSize());
            modifier.setHorizontalAlign (StyledText::center);
            modifier.setVerticalAlign (StyledText::middle);
            modifier.clear();
            modifier.appendText (buttonText, font.withHeight (jmin (font.getHeight(), getHeight() * 0.6f)));
        }
    }
}

//==============================================================================

void ToggleButton::mouseUp (const MouseEvent& event)
{
    if (getLocalBounds().contains (event.getPosition()))
        setToggleState (! toggleState, sendNotification);

    Button::mouseUp (event);
}

//==============================================================================

void ToggleButton::focusGained()
{
    hasFocus = true;
    repaint();
}

void ToggleButton::focusLost()
{
    hasFocus = false;
    repaint();
}

} // namespace yup
