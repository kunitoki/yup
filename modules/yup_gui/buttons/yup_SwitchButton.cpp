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

const Identifier switchVerticalPropertyId ("vertical");
const Identifier switchThumbBoundsPropertyId ("thumbBounds");
const Identifier switchTrackBoundsPropertyId ("trackBounds");
const Identifier switchThumbSizePropertyId ("thumbSize");

} // namespace

//==============================================================================

const Identifier SwitchButton::Style::switchColorId = "switchButtonSwitch";
const Identifier SwitchButton::Style::switchOffBackgroundColorId = "switchButtonOffBackground";
const Identifier SwitchButton::Style::switchOnBackgroundColorId = "switchButtonOnBackground";
const Identifier SwitchButton::Style::cornerRadiusId = "switchButtonCornerRadius";
const Identifier SwitchButton::Style::thumbInsetId = "switchButtonThumbInset";
const Identifier SwitchButton::Style::thumbSizeId = "switchButtonThumbSize";

//==============================================================================

const ComponentTypeInfo& SwitchButton::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<SwitchButton> (Identifier ("SwitchButton"), Button::getClassTypeInfo())
                                 .color ("switchColor", Style::switchColorId)
                                 .color ("switchOffBackgroundColor", Style::switchOffBackgroundColorId)
                                 .color ("switchOnBackgroundColor", Style::switchOnBackgroundColorId)
                                 .number ("cornerRadius", Style::cornerRadiusId, {}, "The corner radius, unset for a pill.")
                                 .number ("thumbInset", Style::thumbInsetId, 4.0, "The inset of the handle inside its slot.")
                                 .number (switchThumbSizePropertyId, Style::thumbSizeId, 28.0, "The thickness of the switch, which is twice as long: it keeps this size in a larger component and is centered in it.")
                                 .state (checkedPropertyId, &SwitchButton::getToggleState, [] (SwitchButton& b, bool v) { b.setToggleState (v); })
                                 .property (switchVerticalPropertyId, &SwitchButton::isVertical, &SwitchButton::setVertical)
                                 .readOnly (switchTrackBoundsPropertyId, &SwitchButton::getTrackBounds, {}, "The area of the switch, of the size its style gives it and centered in the component.")
                                 .readOnly (switchThumbBoundsPropertyId, &SwitchButton::getThumbBounds, {}, "The area of the handle, before its inset: it slides from one end of the switch to the other when it is toggled.")
                                 .build();

    return info;
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

        auto bailOutChecker = BailOutChecker (this);

        notifyPropertyChanged (checkedPropertyId);
        if (bailOutChecker.shouldBailOut())
            return;

        if (toggleState)
        {
            turnOffOtherButtonsInGroup();
            if (bailOutChecker.shouldBailOut())
                return;
        }

        sendChangeNotification (notification, [this]
        {
            auto innerBailOut = BailOutChecker (this);

            toggleStateChanged();
            if (innerBailOut.shouldBailOut())
                return;

            if (onClick)
                onClick();
        });

        if (! bailOutChecker.shouldBailOut())
            repaint();
    }
}

//==============================================================================

void SwitchButton::setVertical (bool shouldBeVertical)
{
    if (isVerticalValue != shouldBeVertical)
    {
        isVerticalValue = shouldBeVertical;

        isAnimating = false;

        updateSwitchCirclePosition();
        notifyPropertyChanged (switchVerticalPropertyId);
        repaint();
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
    if (isAnimating)
    {
        animationSeconds += jmax (0.0, lastFrameTimeSeconds);

        const float targetPosition = toggleState ? 1.0f : 0.0f;
        const auto secondsToSpendMoving = millisecondsToSpendMoving / 1000.0;

        if (animationSeconds >= secondsToSpendMoving)
        {
            isAnimating = false;

            handlePosition = targetPosition;
        }
        else
        {
            float progress = static_cast<float> (animationSeconds / secondsToSpendMoving);

            progress = progress * progress * (3.0f - 2.0f * progress);

            handlePosition = animationStartPosition + (targetPosition - animationStartPosition) * progress;
        }

        repaint();
    }

    updateFrameAttachment();
}

//==============================================================================

void SwitchButton::resized()
{
    Button::resized();

    // The handle is where its state puts it in the new bounds, with nothing to slide from
    isAnimating = false;
    handlePosition = toggleState ? 1.0f : 0.0f;

    updateFrameAttachment();
}

void SwitchButton::visibilityChanged()
{
    updateFrameAttachment();
}

void SwitchButton::parentHierarchyChanged()
{
    updateFrameAttachment();
}

Rectangle<float> SwitchButton::getTrackBounds() const
{
    const auto bounds = getLocalBounds();
    const auto across = isVerticalValue ? bounds.getWidth() : bounds.getHeight();
    const auto along = isVerticalValue ? bounds.getHeight() : bounds.getWidth();

    const auto thickness = jmin (across, jmax (1.0f, findPropertyAs<float> (switchThumbSizePropertyId)));
    const auto length = jmin (along, thickness * 2.0f);

    return bounds.withSizeKeepingCenter (isVerticalValue ? thickness : length, isVerticalValue ? length : thickness);
}

Rectangle<float> SwitchButton::getThumbBounds() const
{
    const auto bounds = getTrackBounds();
    const auto size = isVerticalValue ? bounds.getWidth() : bounds.getHeight();
    const auto travel = jmax (0.0f, (isVerticalValue ? bounds.getHeight() : bounds.getWidth()) - size);

    const auto circle = isVerticalValue
                          ? Rectangle<float> (bounds.getX(), bounds.getY() + travel * handlePosition, size, size)
                          : Rectangle<float> (bounds.getX() + travel * handlePosition, bounds.getY(), size, size);

    return circle.reduced (jmin (1.0f, size * 0.5f));
}

bool SwitchButton::hitTest (float x, float y)
{
    return getTrackBounds().contains (x, y);
}

void SwitchButton::clicked()
{
    if (selectValueOnClick())
        return;

    // The button of a radio group that is on stays on when clicked. A change of state runs onClick itself
    if (toggleState && getRadioGroupId() != 0)
        Button::clicked();
    else
        setToggleState (! toggleState, sendNotification);
}

//==============================================================================

void SwitchButton::updateSwitchCirclePosition()
{
    const float targetPosition = toggleState ? 1.0f : 0.0f;

    // A switch out of sight, or with no room yet, has nowhere to slide
    if (millisecondsToSpendMoving <= 0 || getWidth() <= 0.0f || getHeight() <= 0.0f || ! isShowing() || getNativeComponent() == nullptr)
    {
        handlePosition = targetPosition;
        isAnimating = false;

        repaint();
    }
    else if (! approximatelyEqual (targetPosition, handlePosition))
    {
        animationStartPosition = handlePosition;
        animationSeconds = 0.0;
        isAnimating = true;
    }

    updateFrameAttachment();
}

void SwitchButton::updateFrameAttachment()
{
    frameAttachment.setActive (isAnimating && isShowing());
}

} // namespace yup
