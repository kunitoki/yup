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

namespace
{

struct JoystickComponentIds
{
    static inline const Identifier grabMode { "grabMode" };
    static inline const Identifier thumbSize { "thumbSize" };
    static inline const Identifier snapBack { "snapBack" };
    static inline const Identifier dragging { "dragging" };
};

Identifier axisId (JoystickComponent::Axis axis, StringRef suffix)
{
    return Identifier (String (axis == JoystickComponent::Axis::x ? "x" : "y") + suffix);
}

void setRangeBound (JoystickComponent& joystick, JoystickComponent::Axis axis, double bound, bool isMinimum)
{
    if (! std::isfinite (bound))
        return;

    auto range = joystick.getRange (axis);
    const auto length = range.end - range.start;

    if (isMinimum)
    {
        range.start = bound;
        range.end = range.end > bound ? range.end : bound + length;
    }
    else
    {
        range.end = bound;
        range.start = range.start < bound ? range.start : bound - length;
    }

    joystick.setRange (axis, range);
}

void addAxisProperties (ComponentTypeBuilder<JoystickComponent>& builder, JoystickComponent::Axis axis)
{
    using J = JoystickComponent;

    // The range comes first so that the default and the value land inside it.
    builder.property (axisId (axis, "Minimum"), [axis] (const J& j) { return j.getRange (axis).start; }, [axis] (J& j, double v) { setRangeBound (j, axis, v, true); }, 0.0, "The lowest value of the axis.")
        .property (axisId (axis, "Maximum"), [axis] (const J& j) { return j.getRange (axis).end; }, [axis] (J& j, double v) { setRangeBound (j, axis, v, false); }, 1.0, "The highest value of the axis.")
        .property (axisId (axis, "Interval"), [axis] (const J& j) { return j.getRange (axis).interval; }, [axis] (J& j, double v)
        {
            if (v < 0.0)
                return;

            auto range = j.getRange (axis);
            range.interval = v;
            j.setRange (axis, range);
        }, 0.0, "The step between values of the axis, 0 for continuous.")
        .property (axisId (axis, "Skew"), [axis] (const J& j) { return j.getRange (axis).skew; }, [axis] (J& j, double v)
        {
            if (v <= 0.0)
                return;

            auto range = j.getRange (axis);
            range.skew = v;
            j.setRange (axis, range);
        }, 1.0, "The skew of the axis, below 1 to give the low end more room.")
        .property (axisId (axis, "Logarithmic"), [axis] (const J& j) { return j.isLogarithmic (axis); }, [axis] (J& j, bool v) { j.setLogarithmic (axis, v); }, false, "Whether each decade of the axis takes the same room, for frequencies and ratios.")
        .property (axisId (axis, "Default"), [axis] (const J& j) { return j.getDefaultValue (axis); }, [axis] (J& j, double v) { j.setDefaultValue (axis, v); }, 0.5, "The value restored on double click or snap back.")
        .property (axisId (axis, "Value"), [axis] (const J& j) { return j.getValue (axis); }, [axis] (J& j, double v) { j.setValue (axis, v); }, 0.5, "The current value of the axis.")
        .readOnly (axisId (axis, "Normalized"), [axis] (const J& j) { return j.getNormalizedValue (axis); }, 0.5, "The current value mapped to 0..1, 1 being the right or top edge.");
}

} // namespace

//==============================================================================
const Identifier JoystickComponent::Style::backgroundColorId { "joystickBackground" };
const Identifier JoystickComponent::Style::outlineColorId { "joystickOutline" };
const Identifier JoystickComponent::Style::guideColorId { "joystickGuide" };
const Identifier JoystickComponent::Style::thumbColorId { "joystickThumb" };
const Identifier JoystickComponent::Style::thumbOverColorId { "joystickThumbOver" };
const Identifier JoystickComponent::Style::thumbDownColorId { "joystickThumbDown" };

//==============================================================================
const ComponentTypeInfo& JoystickComponent::getClassTypeInfo()
{
    static const auto info = []
    {
        auto builder = ComponentTypeBuilder<JoystickComponent> (Identifier ("JoystickComponent"));

        builder.color ("backgroundColor", Style::backgroundColorId)
            .color ("outlineColor", Style::outlineColorId)
            .color ("guideColor", Style::guideColorId)
            .color ("thumbColor", Style::thumbColorId)
            .color ("thumbOverColor", Style::thumbOverColorId)
            .color ("thumbDownColor", Style::thumbDownColorId);

        addAxisProperties (builder, Axis::x);
        addAxisProperties (builder, Axis::y);

        return builder.enumeration (JoystickComponentIds::grabMode, { "anywhere", "thumb" }, &JoystickComponent::getGrabMode, &JoystickComponent::setGrabMode, "Which part of the component reacts to the mouse.")
            .property (JoystickComponentIds::thumbSize, &JoystickComponent::getThumbSize, &JoystickComponent::setThumbSize, 16.0f, "The diameter of the thumb, in points.")
            .property (JoystickComponentIds::snapBack, &JoystickComponent::isSnapBack, &JoystickComponent::setSnapBack, false, "Whether the thumb returns to the defaults when released.")
            .state (JoystickComponentIds::dragging, &JoystickComponent::isDragging)
            .signal ("onValueChanged", &JoystickComponent::onValueChanged, "Emitted with both values when either changes.", { { Identifier ("x"), ComponentProperty::Type::number }, { Identifier ("y"), ComponentProperty::Type::number } })
            .signal ("onDragStarted", &JoystickComponent::onDragStarted, "Emitted when a drag starts.")
            .signal ("onDragEnded", &JoystickComponent::onDragEnded, "Emitted when a drag ends.")
            .build();
    }();

    return info;
}

//==============================================================================
JoystickComponent::JoystickComponent (StringRef componentID)
    : Component (componentID)
{
    setMouseCursor (MouseCursor::Hand);
    setWantsKeyboardFocus (true);
    setOpaque (false);
}

//==============================================================================
void JoystickComponent::setRange (Axis axis, const NormalisableRange<double>& newRange)
{
    auto& state = axes[index (axis)];
    const auto previousNormalized = getNormalizedValue (axis);

    state.range = newRange;

    if (state.logarithmic && newRange.start > 0.0)
    {
        state.range = { newRange.start, newRange.end, [] (double start, double end, double v)
        {
            return mapToLog10 (v, start, end);
        }, [] (double start, double end, double v)
        {
            return mapFromLog10 (v, start, end);
        } };

        state.range.interval = newRange.interval;
    }
    state.defaultValue = state.range.snapToLegalValue (state.defaultValue);

    const auto previousValue = state.value;
    state.value = state.range.snapToLegalValue (state.value);

    if (! approximatelyEqual (previousValue, state.value))
    {
        valuesChanged (axis == Axis::x, axis == Axis::y, sendNotification);
    }
    else if (! approximatelyEqual (previousNormalized, getNormalizedValue (axis)))
    {
        repaint();
        notifyPropertyChanged (axisId (axis, "Normalized"));
    }
}

void JoystickComponent::setLogarithmic (Axis axis, bool shouldBeLogarithmic)
{
    auto& state = axes[index (axis)];
    if (state.logarithmic == shouldBeLogarithmic)
        return;

    state.logarithmic = shouldBeLogarithmic;

    const auto& range = state.range;
    setRange (axis, { range.start, range.end, range.interval, range.skew, range.symmetricSkew });
}

void JoystickComponent::setValue (Axis axis, double newValue, NotificationType notification)
{
    if (axis == Axis::x)
        setValues (newValue, getValue (Axis::y), notification);
    else
        setValues (getValue (Axis::x), newValue, notification);
}

void JoystickComponent::setValues (double newX, double newY, NotificationType notification)
{
    auto& x = axes[0];
    auto& y = axes[1];

    newX = x.range.snapToLegalValue (newX);
    newY = y.range.snapToLegalValue (newY);

    const auto xChanged = ! approximatelyEqual (x.value, newX);
    const auto yChanged = ! approximatelyEqual (y.value, newY);

    if (! xChanged && ! yChanged)
        return;

    x.value = newX;
    y.value = newY;
    valuesChanged (xChanged, yChanged, notification);
}

double JoystickComponent::getNormalizedValue (Axis axis) const
{
    const auto& state = axes[index (axis)];
    if (state.range.end <= state.range.start)
        return 0.0;

    return jlimit (0.0, 1.0, state.range.convertTo0to1 (state.value));
}

void JoystickComponent::setDefaultValue (Axis axis, double newDefault)
{
    auto& state = axes[index (axis)];
    state.defaultValue = state.range.snapToLegalValue (newDefault);
}

void JoystickComponent::resetToDefaults (NotificationType notification)
{
    setValues (axes[0].defaultValue, axes[1].defaultValue, notification);
}

void JoystickComponent::setThumbSize (float newSize)
{
    newSize = jmax (0.0f, newSize);
    if (approximatelyEqual (thumbSize, newSize))
        return;

    thumbSize = newSize;
    repaint();
}

Point<float> JoystickComponent::getThumbPosition() const
{
    return { static_cast<float> (getNormalizedValue (Axis::x)) * getWidth(),
             static_cast<float> (1.0 - getNormalizedValue (Axis::y)) * getHeight() };
}

Rectangle<float> JoystickComponent::getThumbBounds() const
{
    return Rectangle<float> (0.0f, 0.0f, thumbSize, thumbSize).withCenter (getThumbPosition());
}

//==============================================================================
bool JoystickComponent::hitTest (float x, float y)
{
    if (grabMode == GrabMode::anywhere)
        return Component::hitTest (x, y);

    const auto radius = jmax (4.0f, thumbSize * 0.5f);
    return getThumbPosition().distanceTo ({ x, y }) <= radius;
}

void JoystickComponent::mouseDown (const MouseEvent& event)
{
    grabOffset = grabMode == GrabMode::thumb ? getThumbPosition() - event.getPosition() : Point<float>();

    setDragging (true);

    const BailOutChecker bailOutChecker (this);

    if (onDragStarted)
        onDragStarted();

    if (! bailOutChecker.shouldBailOut())
        moveThumbTo (event.getPosition() + grabOffset);
}

void JoystickComponent::mouseDrag (const MouseEvent& event)
{
    if (dragging)
        moveThumbTo (event.getPosition() + grabOffset);
}

void JoystickComponent::mouseUp (const MouseEvent& event)
{
    ignoreUnused (event);

    if (! dragging)
        return;

    setDragging (false);

    const BailOutChecker bailOutChecker (this);

    if (snapBack)
        resetToDefaults();

    if (! bailOutChecker.shouldBailOut() && onDragEnded)
        onDragEnded();
}

void JoystickComponent::mouseDoubleClick (const MouseEvent& event)
{
    ignoreUnused (event);

    resetToDefaults();
}

bool JoystickComponent::keyDown (const KeyPress& key, const Point<float>& position)
{
    ignoreUnused (position);

    const auto steps = key.getModifiers().isShiftDown() ? 10.0 : 1.0;

    switch (key.getKey())
    {
        case KeyPress::leftKey:
            nudge (Axis::x, -steps);
            return true;

        case KeyPress::rightKey:
            nudge (Axis::x, steps);
            return true;

        case KeyPress::downKey:
            nudge (Axis::y, -steps);
            return true;

        case KeyPress::upKey:
            nudge (Axis::y, steps);
            return true;

        default:
            return false;
    }
}

void JoystickComponent::focusGained()
{
    repaint();
}

void JoystickComponent::focusLost()
{
    repaint();
}

//==============================================================================
void JoystickComponent::moveThumbTo (Point<float> center)
{
    const auto width = getWidth();
    const auto height = getHeight();
    if (width <= 0.0f || height <= 0.0f)
        return;

    const auto nx = jlimit (0.0, 1.0, static_cast<double> (center.getX()) / width);
    const auto ny = jlimit (0.0, 1.0, 1.0 - static_cast<double> (center.getY()) / height);

    setValues (axes[0].range.convertFrom0to1 (nx), axes[1].range.convertFrom0to1 (ny));
}

void JoystickComponent::nudge (Axis axis, double steps)
{
    const auto& state = axes[index (axis)];

    if (state.range.interval > 0.0)
    {
        setValue (axis, state.value + state.range.interval * steps);
        return;
    }

    const auto normalized = jlimit (0.0, 1.0, getNormalizedValue (axis) + 0.01 * steps);
    setValue (axis, state.range.convertFrom0to1 (normalized));
}

void JoystickComponent::setDragging (bool shouldBeDragging)
{
    if (dragging == shouldBeDragging)
        return;

    dragging = shouldBeDragging;
    repaint();
    notifyPropertyChanged (JoystickComponentIds::dragging);
}

void JoystickComponent::valuesChanged (bool xChanged, bool yChanged, NotificationType notification)
{
    repaint();

    const BailOutChecker bailOutChecker (this);

    for (const auto axis : { Axis::x, Axis::y })
    {
        if (! (axis == Axis::x ? xChanged : yChanged))
            continue;

        notifyPropertyChanged (axisId (axis, "Value"));
        notifyPropertyChanged (axisId (axis, "Normalized"));

        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (notification != dontSendNotification && (xChanged || yChanged) && onValueChanged)
        onValueChanged (axes[0].value, axes[1].value);
}

} // namespace yup
