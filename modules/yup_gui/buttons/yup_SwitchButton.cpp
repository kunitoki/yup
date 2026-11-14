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

const Identifier SwitchButton::Style::switchColorId = "switchButtonSwitch";
const Identifier SwitchButton::Style::switchOffBackgroundColorId = "switchButtonOffBackground";
const Identifier SwitchButton::Style::switchOnBackgroundColorId = "switchButtonOnBackground";
const Identifier SwitchButton::Style::cornerRadiusId = "switchButtonCornerRadius";
const Identifier SwitchButton::Style::thumbInsetId = "switchButtonThumbInset";

//==============================================================================

const ComponentTypeInfo& SwitchButton::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (makeColorProperty (Identifier ("switchColor"), Style::switchColorId));
        props.push_back (makeColorProperty (Identifier ("switchOffBackgroundColor"), Style::switchOffBackgroundColorId));
        props.push_back (makeColorProperty (Identifier ("switchOnBackgroundColor"), Style::switchOnBackgroundColorId));

        props.push_back (makeNumberProperty (Identifier ("cornerRadius"), Style::cornerRadiusId, {}, "The corner radius, unset for a pill."));
        props.push_back (makeNumberProperty (Identifier ("thumbInset"), Style::thumbInsetId, 4.0, "The inset of the handle inside its slot."));

        props.push_back (ComponentProperty {
            .name = Identifier ("checked"),
            .type = ComponentProperty::Type::boolean,
            .getter = [] (const Component& c) { return var (static_cast<const SwitchButton&> (c).getToggleState()); },
            .setter = [] (Component& c, const var& v) { static_cast<SwitchButton&> (c).setToggleState (static_cast<bool> (v)); },
            .isState = true,
            .isStyleable = false });

        props.push_back (ComponentProperty {
            .name = Identifier ("vertical"),
            .type = ComponentProperty::Type::boolean,
            .getter = [] (const Component& c) { return var (static_cast<const SwitchButton&> (c).isVertical()); },
            .setter = [] (Component& c, const var& v) { static_cast<SwitchButton&> (c).setVertical (static_cast<bool> (v)); },
            .isStyleable = false });

        return ComponentTypeInfo { Identifier ("SwitchButton"), &Component::getClassTypeInfo(), std::move (props), { Button::getClickSignal() }, [] { return std::make_unique<SwitchButton>(); } };
    }();

    return info;
}

const ComponentTypeInfo& SwitchButton::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

SwitchButton::SwitchButton (StringRef componentID, bool isVertical)
    : Button (componentID)
    , isVerticalValue (isVertical)
{
    setOpaque (false);
}

//==============================================================================

void SwitchButton::setToggleState (bool shouldBeToggled, NotificationType notification)
{
    if (toggleState != shouldBeToggled)
    {
        toggleState = shouldBeToggled;

        updateSwitchCirclePosition();

        sendChangeNotification (notification, [this]
        {
            toggleStateChanged();

            if (onClick)
                onClick();
        });

        repaint();
    }
}

//==============================================================================

void SwitchButton::setVertical (bool shouldBeVertical) noexcept
{
    if (isVerticalValue != shouldBeVertical)
    {
        isVerticalValue = shouldBeVertical;

        isAnimating = false;

        updateSwitchCirclePosition();
    }
}

//==============================================================================

void SwitchButton::setMillisecondsToSpendMoving (int newValue) noexcept
{
    millisecondsToSpendMoving = newValue;
}

//==============================================================================

void SwitchButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void SwitchButton::refreshDisplay (double lastFrameTimeSeconds)
{
    if (! isAnimating)
        return;

    auto currentTime = Time::getCurrentTime();
    auto elapsedMs = (currentTime - animationStartTime).inMilliseconds();

    if (elapsedMs >= millisecondsToSpendMoving)
    {
        isAnimating = false;

        switchCircleBounds = animationTargetBounds;
    }
    else
    {
        float progress = jlimit (0.0f, 1.0f, static_cast<float> (elapsedMs) / static_cast<float> (millisecondsToSpendMoving));

        progress = progress * progress * (3.0f - 2.0f * progress);

        auto startPos = animationStartBounds.getPosition();
        auto targetPos = animationTargetBounds.getPosition();
        auto currentPos = startPos + (targetPos - startPos) * progress;

        switchCircleBounds = Rectangle<float> (currentPos, animationTargetBounds.getSize());
    }

    repaint();
}

//==============================================================================

void SwitchButton::resized()
{
    Button::resized();

    isAnimating = false;

    updateSwitchCirclePosition();
}

void SwitchButton::mouseUp (const MouseEvent& event)
{
    if (getLocalBounds().contains (event.getPosition()))
        setToggleState (! toggleState, sendNotification);

    Button::mouseUp (event);
}

//==============================================================================

void SwitchButton::updateSwitchCirclePosition()
{
    auto bounds = getLocalBounds();
    Rectangle<float> targetBounds;

    if (isVerticalValue)
    {
        targetBounds = Rectangle<float> (
            bounds.getX(),
            getToggleState() ? bounds.getBottom() - bounds.getWidth() : bounds.getY(),
            bounds.getWidth(),
            bounds.getWidth());
    }
    else
    {
        targetBounds = Rectangle<float> (
            getToggleState() ? bounds.getRight() - bounds.getHeight() : bounds.getX(),
            bounds.getY(),
            bounds.getHeight(),
            bounds.getHeight());
    }

    targetBounds = targetBounds.reduced (1).toNearestInt();

    if (millisecondsToSpendMoving <= 0 || switchCircleBounds.isEmpty())
    {
        switchCircleBounds = targetBounds;
        isAnimating = false;

        repaint();
    }
    else if (targetBounds != switchCircleBounds)
    {
        animationStartBounds = switchCircleBounds;
        animationTargetBounds = targetBounds;
        animationStartTime = Time::getCurrentTime();
        isAnimating = true;
    }
}

} // namespace yup
