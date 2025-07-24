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
// Style identifier definitions

const Identifier Slider::Style::backgroundColorId { "Slider_backgroundColorId" };
const Identifier Slider::Style::trackColorId { "Slider_trackColorId" };
const Identifier Slider::Style::thumbColorId { "Slider_thumbColorId" };
const Identifier Slider::Style::thumbOverColorId { "Slider_thumbOverColorId" };
const Identifier Slider::Style::thumbDownColorId { "Slider_thumbDownColorId" };
const Identifier Slider::Style::textColorId { "Slider_textColorId" };

//==============================================================================

Slider::Slider (SliderType sliderType, StringRef componentID)
    : Component (componentID)
    , sliderType (sliderType)
    , currentValue (defaultValue)
{
    setMouseCursor (MouseCursor::Hand);
    setWantsKeyboardFocus (true);
    setOpaque (false);

    if (sliderType == Rotary || sliderType == RotaryHorizontalDrag || sliderType == RotaryVerticalDrag)
        setMouseDragSensitivity (0.3f);

    setValue (defaultValue, dontSendNotification);
}

Slider::Slider (SliderType sliderType)
    : Slider (sliderType, {})
{
}

//==============================================================================

void Slider::setValue (double newValue, NotificationType notification)
{
    newValue = constrainValue (newValue);

    if (! approximatelyEqual (currentValue, newValue))
    {
        currentValue = newValue;
        sendValueChanged (notification);
        repaint();
    }
}

double Slider::getValue() const
{
    return currentValue;
}

void Slider::setValueNormalised (double newValue, NotificationType notification)
{
    setValue (range.convertFrom0to1 (std::clamp (newValue, 0.0, 1.0)), notification);
}

double Slider::getValueNormalised() const
{
    return range.convertTo0to1 (currentValue);
}

void Slider::valueChanged() {}

//==============================================================================

void Slider::setMinValue (double newMinValue, NotificationType notification, bool allowNudgingOfOtherValues)
{
    newMinValue = constrainValue (newMinValue);

    if (allowNudgingOfOtherValues)
    {
        if (newMinValue > maxValue)
            setMaxValue (newMinValue, notification, false);

        if (newMinValue > currentValue)
            setValue (newMinValue, notification);
    }

    if (! approximatelyEqual (minValue, newMinValue))
    {
        minValue = newMinValue;
        sendMinValueChanged (notification);
        repaint();
    }
}

double Slider::getMinValue() const
{
    return minValue;
}

void Slider::setMaxValue (double newMaxValue, NotificationType notification, bool allowNudgingOfOtherValues)
{
    newMaxValue = constrainValue (newMaxValue);

    if (allowNudgingOfOtherValues)
    {
        if (newMaxValue < minValue)
            setMinValue (newMaxValue, notification, false);

        if (newMaxValue < currentValue)
            setValue (newMaxValue, notification);
    }

    if (! approximatelyEqual (maxValue, newMaxValue))
    {
        maxValue = newMaxValue;
        sendMaxValueChanged (notification);
        repaint();
    }
}

double Slider::getMaxValue() const
{
    return maxValue;
}

void Slider::minValueChanged() {}

void Slider::maxValueChanged() {}

//==============================================================================

void Slider::setDefaultValue (double newDefaultValue)
{
    defaultValue = constrainValue (newDefaultValue);
}

double Slider::getDefaultValue() const
{
    return defaultValue;
}

//==============================================================================

void Slider::setRange (const NormalisableRange<double>& newRange)
{
    range = newRange;
    setDefaultValue (constrainValue (defaultValue));
    setValue (constrainValue (currentValue), dontSendNotification);
    setMinValue (constrainValue (minValue), dontSendNotification);
    setMaxValue (constrainValue (maxValue), dontSendNotification);
}

void Slider::setRange (double minValue, double maxValue, double stepSize)
{
    setRange (NormalisableRange<double> (minValue, maxValue, stepSize));
}

NormalisableRange<double> Slider::getRange() const
{
    return range;
}

double Slider::getInterval() const
{
    return range.interval;
}

//==============================================================================

void Slider::setSkewFactor (double skewFactor)
{
    if (skewFactor <= 0.0)
    {
        jassertfalse; // Skew factor must be positive
        return;
    }

    if (! approximatelyEqual (range.skew, skewFactor))
    {
        range.skew = skewFactor;

        // Reapply constraints to current values with new skew
        setDefaultValue (constrainValue (defaultValue));
        setValue (constrainValue (currentValue), dontSendNotification);
        setMinValue (constrainValue (minValue), dontSendNotification);
        setMaxValue (constrainValue (maxValue), dontSendNotification);

        repaint();
    }
}

void Slider::setSkewFactorFromMidpoint (double midpointValue)
{
    midpointValue = jlimit (range.getRange().getStart(), range.getRange().getEnd(), midpointValue);

    range.setSkewForCentre (midpointValue);

    // Reapply constraints to current values with new skew
    setDefaultValue (constrainValue (defaultValue));
    setValue (constrainValue (currentValue), dontSendNotification);
    setMinValue (constrainValue (minValue), dontSendNotification);
    setMaxValue (constrainValue (maxValue), dontSendNotification);

    repaint();
}

double Slider::getSkewFactor() const
{
    return range.skew;
}

//==============================================================================

void Slider::setNumDecimalPlacesToDisplay (int decimalPlaces)
{
    numDecimalPlaces = decimalPlaces;
    repaint();
}

int Slider::getNumDecimalPlacesToDisplay() const
{
    return numDecimalPlaces;
}

//==============================================================================

void Slider::setSliderType (SliderType newType)
{
    if (sliderType != newType)
    {
        sliderType = newType;
        repaint();
        resized();
    }
}

Slider::SliderType Slider::getSliderType() const
{
    return sliderType;
}

void Slider::setTextBoxStyle (TextEntryBoxPosition position, bool isReadOnly, int textEntryBoxWidth, int textEntryBoxHeight)
{
    textBoxPosition = position;
    textBoxIsReadOnly = isReadOnly;
    textBoxWidth = textEntryBoxWidth;
    textBoxHeight = textEntryBoxHeight;

    resized();
    repaint();
}

Slider::TextEntryBoxPosition Slider::getTextBoxPosition() const
{
    return textBoxPosition;
}

bool Slider::isTextBoxReadOnly() const
{
    return textBoxIsReadOnly;
}

//==============================================================================

void Slider::setPopupDisplayEnabled (bool shouldShowBubble, Component* bubbleComponent)
{
    popupDisplayEnabled = shouldShowBubble;

    if (bubbleComponent != nullptr)
        popupBubbleComponent.reset (bubbleComponent);
}

void Slider::setPopupMenuEnabled (bool shouldShowMenu)
{
    popupMenuEnabled = shouldShowMenu;
}

//==============================================================================

bool Slider::isMouseOver() const
{
    return isMouseOverSlider;
}

bool Slider::isCurrentlyBeingDragged() const
{
    return dragMode != notDragging;
}

//==============================================================================

void Slider::setMouseDragSensitivity (double sensitivity)
{
    mouseDragSensitivity = std::max (0.001, sensitivity);
}

double Slider::getMouseDragSensitivity() const
{
    return mouseDragSensitivity;
}

void Slider::setVelocityModeParameters (double sensitivity, double threshold, double offsetThreshold)
{
    velocitySensitivity = std::max (0.001, sensitivity);
    velocityThreshold = std::max (0.001, threshold);
    velocityOffsetThreshold = std::max (0.0, offsetThreshold);
}

//==============================================================================

void Slider::resized()
{
    auto bounds = getLocalBounds();

    if (textBoxPosition != NoTextBox)
    {
        auto textBounds = getTextBoxBounds();

        if (textEditor != nullptr)
            textEditor->setBounds (textBounds);
    }
}

void Slider::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void Slider::mouseEnter (const MouseEvent& event)
{
    isMouseOverSlider = true;
    repaint();
}

void Slider::mouseExit (const MouseEvent& event)
{
    isMouseOverSlider = false;
    repaint();
}

void Slider::mouseDown (const MouseEvent& event)
{
    // Check for right-click instead of isPopupTrigger()
    if (event.isRightButtonDown() && popupMenuEnabled)
    {
        showPopupMenu();
        return;
    }

    mouseDownPosition = event.getPosition(); // Already returns Point<float>, no toFloat() needed
    mouseDragStartPosition = mouseDownPosition;

    dragMode = getDragModeForMousePosition (mouseDownPosition);

    if (dragMode != notDragging)
    {
        // For linear sliders, implement improved click behavior
        bool shouldJumpToClickPosition = false;

        switch (sliderType)
        {
            case LinearHorizontal:
            case LinearBarHorizontal:
            case LinearVertical:
            case LinearBarVertical:
            {
                // Check if mouse is over thumb - if not, jump to click position
                if (! isMouseOverThumb (mouseDownPosition))
                {
                    shouldJumpToClickPosition = true;

                    // Calculate new value based on click position
                    const auto sliderBounds = getSliderBounds();
                    const float thumbSize = getThumbSize();

                    double newNormalizedValue = 0.0;

                    if (sliderType == LinearHorizontal || sliderType == LinearBarHorizontal)
                    {
                        const float clickX = mouseDownPosition.getX();
                        const float availableWidth = sliderBounds.getWidth() - thumbSize;
                        const float relativeX = clickX - sliderBounds.getX() - (thumbSize * 0.5f);
                        newNormalizedValue = std::clamp (static_cast<double> (relativeX / availableWidth), 0.0, 1.0);
                    }
                    else // Vertical sliders
                    {
                        const float clickY = mouseDownPosition.getY();
                        const float availableHeight = sliderBounds.getHeight() - thumbSize;
                        const float relativeY = sliderBounds.getBottom() - clickY - (thumbSize * 0.5f);
                        newNormalizedValue = std::clamp (static_cast<double> (relativeY / availableHeight), 0.0, 1.0);
                    }

                    // Set the new value (this will move the thumb to the click position)
                    setValueNormalised (newNormalizedValue, sendNotification);

                    // Update the drag start position to the new thumb center for smooth dragging
                    const auto newThumbBounds = getThumbBounds();
                    mouseDragStartPosition = Point<float> (newThumbBounds.getCenterX(), newThumbBounds.getCenterY());
                }
                break;
            }

            default:
                // For non-linear sliders, use existing behavior
                break;
        }

        valueOnMouseDown = currentValue;
        minValueOnMouseDown = minValue;
        maxValueOnMouseDown = maxValue;

        if (onDragStart)
            onDragStart (event);
    }

    takeKeyboardFocus();
    repaint();
}

void Slider::mouseUp (const MouseEvent& event)
{
    if (dragMode != notDragging)
    {
        if (onDragEnd)
            onDragEnd (event);

        dragMode = notDragging;
        repaint();
    }
}

void Slider::mouseDrag (const MouseEvent& event)
{
    if (dragMode == notDragging)
        return;

    updateValueFromMousePosition (event.getPosition(), dragMode); // Already returns Point<float>
}

void Slider::mouseWheel (const MouseEvent& event, const MouseWheelData& data)
{
    if (! isEnabled())
        return;

    const double multiplier = event.getModifiers().isShiftDown() ? 0.001 : 0.01;
    const double distance = (data.getDeltaX() + data.getDeltaY()) * multiplier * mouseDragSensitivity;

    setValueNormalised (getValueNormalised() + distance);
}

void Slider::mouseDoubleClick (const MouseEvent& event)
{
    if (isEnabled())
        resetToDefaultValue();
}

//==============================================================================

void Slider::keyDown (const KeyPress& key, const Point<float>& position)
{
    if (! isEnabled())
        return;

    updateValueFromKeypress (key);
}

void Slider::focusGained()
{
    repaint();
}

void Slider::focusLost()
{
    hideTextEditor (false);
    repaint();
}

//==============================================================================

void Slider::sendValueChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        valueChanged();

        if (onValueChanged)
            onValueChanged (getValue());
    });
}

void Slider::sendMinValueChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        minValueChanged();

        if (onMinValueChanged)
            onMinValueChanged (getMinValue());
    });
}

void Slider::sendMaxValueChanged (NotificationType notification)
{
    sendChangeNotification (notification, [this]
    {
        maxValueChanged();

        if (onMaxValueChanged)
            onMaxValueChanged (getMaxValue());
    });
}

//==============================================================================

void Slider::updateValueFromMousePosition (Point<float> mousePos, DragMode dragMode)
{
    const auto sliderBounds = getSliderBounds();

    if (sliderBounds.isEmpty())
        return;

    float mouseDelta = 0.0f;
    float totalRange = 1.0f;

    // Calculate mouse delta based on slider type
    switch (sliderType)
    {
        case LinearVertical:
        case LinearBarVertical:
            mouseDelta = mouseDragStartPosition.getY() - mousePos.getY();
            totalRange = sliderBounds.getHeight();
            break;

        case LinearHorizontal:
        case LinearBarHorizontal:
            mouseDelta = mousePos.getX() - mouseDragStartPosition.getX();
            totalRange = sliderBounds.getWidth();
            break;

        case RotaryVerticalDrag:
            mouseDelta = mouseDragStartPosition.getY() - mousePos.getY();
            totalRange = sliderBounds.getHeight();
            break;

        case RotaryHorizontalDrag:
            mouseDelta = mousePos.getX() - mouseDragStartPosition.getX();
            totalRange = sliderBounds.getWidth();
            break;

        case Rotary:
        {
            // For regular rotary, use both horizontal and vertical movement
            const float deltaX = mousePos.getX() - mouseDragStartPosition.getX();
            const float deltaY = mouseDragStartPosition.getY() - mousePos.getY();
            mouseDelta = deltaX + deltaY; // Combine both axes
            totalRange = std::max (sliderBounds.getWidth(), sliderBounds.getHeight());
            break;
        }

        default:
            // For other slider types, default to horizontal behavior
            mouseDelta = mousePos.getX() - mouseDragStartPosition.getX();
            totalRange = sliderBounds.getWidth();
            break;
    }

    if (totalRange <= 0.0f)
        return;

    const double normalizedDelta = (mouseDelta / totalRange) * mouseDragSensitivity;

    switch (dragMode)
    {
        case draggingForValue:
        {
            const double newNormalisedValue = range.convertTo0to1 (valueOnMouseDown) + normalizedDelta;
            setValueNormalised (newNormalisedValue);
            break;
        }

        case draggingForMinValue:
        {
            const double newNormalisedValue = range.convertTo0to1 (minValueOnMouseDown) + normalizedDelta;
            setMinValue (range.convertFrom0to1 (std::clamp (newNormalisedValue, 0.0, 1.0)));
            break;
        }

        case draggingForMaxValue:
        {
            const double newNormalisedValue = range.convertTo0to1 (maxValueOnMouseDown) + normalizedDelta;
            setMaxValue (range.convertFrom0to1 (std::clamp (newNormalisedValue, 0.0, 1.0)));
            break;
        }

        default:
            break;
    }
}

void Slider::updateValueFromKeypress (const KeyPress& key)
{
    const bool isUpKey = key.getKey() == KeyPress::upKey || key.getKey() == KeyPress::rightKey;
    const bool isDownKey = key.getKey() == KeyPress::downKey || key.getKey() == KeyPress::leftKey;
    const bool isPageUp = key.getKey() == KeyPress::pageUpKey;
    const bool isPageDown = key.getKey() == KeyPress::pageDownKey;
    const bool isHome = key.getKey() == KeyPress::homeKey;
    const bool isEnd = key.getKey() == KeyPress::endKey;

    if (! (isUpKey || isDownKey || isPageUp || isPageDown || isHome || isEnd))
        return;

    const double currentNormalisedValue = getValueNormalised();
    double newNormalisedValue = currentNormalisedValue;

    if (isHome)
        newNormalisedValue = 0.0;
    else if (isEnd)
        newNormalisedValue = 1.0;
    else
    {
        const double increment = (isPageUp || isPageDown) ? 0.1 : 0.01;
        const double direction = (isUpKey || isPageUp) ? 1.0 : -1.0;
        newNormalisedValue = currentNormalisedValue + (direction * increment);
    }

    setValueNormalised (newNormalisedValue);
}

void Slider::resetToDefaultValue()
{
    setValue (defaultValue);
}

//==============================================================================

double Slider::constrainValue (double valueToConstrain) const
{
    return snapToLegalValue (range.getRange().clipValue (valueToConstrain));
}

double Slider::snapToLegalValue (double valueToSnap) const
{
    return range.snapToLegalValue (valueToSnap);
}

//==============================================================================

Rectangle<float> Slider::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    if (textBoxPosition != NoTextBox)
    {
        const auto textBounds = getTextBoxBounds();

        switch (textBoxPosition)
        {
            case TextBoxLeft:
                bounds.removeFromLeft (textBounds.getWidth());
                break;
            case TextBoxRight:
                bounds.removeFromRight (textBounds.getWidth());
                break;
            case TextBoxAbove:
                bounds.removeFromTop (textBounds.getHeight());
                break;
            case TextBoxBelow:
                bounds.removeFromBottom (textBounds.getHeight());
                break;
            default:
                break;
        }
    }

    // Add margin and account for thumb size to prevent clipping
    float margin = 2.0f;

    // For linear sliders, add extra margin based on thumb size to prevent clipping
    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
        {
            const float thumbSize = std::min (20.0f, bounds.getHeight() * 0.8f);
            const float thumbMargin = thumbSize * 0.5f;
            return Rectangle<float> (bounds.getX() + thumbMargin, bounds.getY() + margin, bounds.getWidth() - (thumbMargin * 2), bounds.getHeight() - (margin * 2));
        }

        case LinearVertical:
        case LinearBarVertical:
        {
            const float thumbSize = std::min (20.0f, bounds.getWidth() * 0.8f);
            const float thumbMargin = thumbSize * 0.5f;
            return Rectangle<float> (bounds.getX() + margin, bounds.getY() + thumbMargin, bounds.getWidth() - (margin * 2), bounds.getHeight() - (thumbMargin * 2));
        }

        default:
            // For rotary and other types, use smaller margin
            return Rectangle<float> (bounds.getX() + margin, bounds.getY() + margin, bounds.getWidth() - (margin * 2), bounds.getHeight() - (margin * 2));
    }
}

Rectangle<float> Slider::getTextBoxBounds() const
{
    if (textBoxPosition == NoTextBox)
        return {};

    auto bounds = getLocalBounds();

    switch (textBoxPosition)
    {
        case TextBoxLeft:
            return bounds.removeFromLeft (textBoxWidth);
        case TextBoxRight:
            return bounds.removeFromRight (textBoxWidth);
        case TextBoxAbove:
            return bounds.removeFromTop (textBoxHeight);
        case TextBoxBelow:
            return bounds.removeFromBottom (textBoxHeight);
        default:
            return {};
    }
}

//==============================================================================

float Slider::getThumbSize() const
{
    // Return a reasonable thumb size based on slider type and bounds
    const auto bounds = getSliderBounds();

    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
            return std::min (20.0f, bounds.getHeight() * 0.8f);

        case LinearVertical:
        case LinearBarVertical:
            return std::min (20.0f, bounds.getWidth() * 0.8f);

        default:
            return 16.0f; // Default for rotary and other types
    }
}

Rectangle<float> Slider::getThumbBounds() const
{
    const auto sliderBounds = getSliderBounds();
    const float thumbSize = getThumbSize();
    const double normalizedValue = getValueNormalised();

    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
        {
            const float thumbX = sliderBounds.getX() + (normalizedValue * (sliderBounds.getWidth() - thumbSize));
            const float thumbY = sliderBounds.getY() + (sliderBounds.getHeight() - thumbSize) * 0.5f;
            return Rectangle<float> (thumbX, thumbY, thumbSize, thumbSize);
        }

        case LinearVertical:
        case LinearBarVertical:
        {
            const float thumbX = sliderBounds.getX() + (sliderBounds.getWidth() - thumbSize) * 0.5f;
            const float thumbY = sliderBounds.getBottom() - thumbSize - (normalizedValue * (sliderBounds.getHeight() - thumbSize));
            return Rectangle<float> (thumbX, thumbY, thumbSize, thumbSize);
        }

        default:
            // For non-linear sliders, return empty bounds
            return Rectangle<float>();
    }
}

bool Slider::isMouseOverThumb (Point<float> mousePos) const
{
    // Only apply thumb hit detection for linear sliders
    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
        case LinearVertical:
        case LinearBarVertical:
            return getThumbBounds().contains (mousePos);

        default:
            // For other slider types, use existing behavior
            return isMouseOverSliderArea (mousePos);
    }
}

Slider::DragMode Slider::getDragModeForMousePosition (Point<float> mousePos) const
{
    switch (sliderType)
    {
        case LinearHorizontal:
        case LinearBarHorizontal:
        case LinearVertical:
        case LinearBarVertical:
        {
            // For linear sliders, allow dragging if mouse is over slider area
            // The improved click behavior is handled in mouseDown()
            return isMouseOverSliderArea (mousePos) ? draggingForValue : notDragging;
        }

        case TwoValueHorizontal:
        case TwoValueVertical:
        {
            // Determine which thumb is closer to the mouse position
            const auto sliderBounds = getSliderBounds();
            const bool isHorizontal = (sliderType == TwoValueHorizontal);

            const float minPos = isHorizontal
                                   ? sliderBounds.getX() + (range.convertTo0to1 (minValue) * sliderBounds.getWidth())
                                   : sliderBounds.getBottom() - (range.convertTo0to1 (minValue) * sliderBounds.getHeight());

            const float maxPos = isHorizontal
                                   ? sliderBounds.getX() + (range.convertTo0to1 (maxValue) * sliderBounds.getWidth())
                                   : sliderBounds.getBottom() - (range.convertTo0to1 (maxValue) * sliderBounds.getHeight());

            const float mouseCoord = isHorizontal ? mousePos.getX() : mousePos.getY(); // Use getX() and getY()
            const float distToMin = std::abs (mouseCoord - minPos);
            const float distToMax = std::abs (mouseCoord - maxPos);

            return (distToMin < distToMax) ? draggingForMinValue : draggingForMaxValue;
        }

        default:
            // For rotary and other slider types, use existing behavior
            return isMouseOverSliderArea (mousePos) ? draggingForValue : notDragging;
    }
}

bool Slider::isMouseOverSliderArea (Point<float> mousePos) const
{
    const auto sliderBounds = getSliderBounds();
    return sliderBounds.contains (mousePos);
}

//==============================================================================

void Slider::showPopupMenu()
{
    // TODO: Implement popup menu with reset to default, etc.
}

void Slider::createTextEditor()
{
    // TODO: Implement text editor creation
}

void Slider::hideTextEditor (bool discardCurrentEditorContents)
{
    if (textEditor != nullptr)
    {
        if (! discardCurrentEditorContents)
        {
            // TODO: Parse text and update value
        }

        textEditor.reset();
    }
}

} // namespace yup
