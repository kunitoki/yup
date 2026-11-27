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

#pragma once

namespace yup
{

//==============================================================================
/**
    A two dimensional controller, an XY pad, moving a thumb over its area.

    Each axis has its own NormalisableRange (bounds, interval and skew), a default value and
    a current value. The thumb center spans the whole local bounds: the left edge is the start
    of the x range and the bottom edge the start of the y range.

    With GrabMode::anywhere a click moves the thumb under the pointer; with GrabMode::thumb only
    the thumb reacts to the mouse and drags relatively, which lets several joysticks overlap
    on the same area (the band nodes of an equalizer, for example). The values return to the
    defaults on double click, or on release with snap back enabled, and the arrow keys nudge
    them (by one interval, or 1% of a continuous range, and ten times that with Shift).

    @code
    joystick.setRange (JoystickComponent::Axis::x, { 20.0, 20000.0, 0.0, 0.2 });
    joystick.setRange (JoystickComponent::Axis::y, { -24.0, 24.0 });
    joystick.onValueChanged = [this] (double frequency, double gain) { updateBand (frequency, gain); };
    addAndMakeVisible (joystick);
    @endcode

    @see Slider

    @tags{UI}
*/
class YUP_API JoystickComponent : public Component
{
public:
    //==============================================================================
    /** The two axes of the joystick. */
    enum class Axis
    {
        x, /**< The horizontal axis, growing to the right. */
        y  /**< The vertical axis, growing to the top. */
    };

    /** Which part of the component reacts to the mouse. */
    enum class GrabMode
    {
        anywhere, /**< The whole area: a click moves the thumb under the pointer. */
        thumb     /**< Only the thumb, which drags relatively from where it was grabbed. */
    };

    //==============================================================================
    /** Creates a joystick with both axes ranging from 0 to 1 and centered defaults.

        @param componentID  An optional identifier for this component.
    */
    explicit JoystickComponent (StringRef componentID = {});

    //==============================================================================
    /** Sets the range of an axis, constraining its default and current values to it. */
    void setRange (Axis axis, const NormalisableRange<double>& newRange);

    /** Returns the range of an axis. */
    const NormalisableRange<double>& getRange (Axis axis) const noexcept { return axes[index (axis)].range; }

    /** Maps an axis logarithmically, so each decade of its range takes the same room.

        This suits frequencies and other ratios. It needs a range above zero: while the range
        starts at or below zero the axis stays linear. The skew is ignored while enabled.
    */
    void setLogarithmic (Axis axis, bool shouldBeLogarithmic);

    /** Returns true if an axis is mapped logarithmically. */
    bool isLogarithmic (Axis axis) const noexcept { return axes[index (axis)].logarithmic; }

    /** Sets the value of an axis, snapped to the range. */
    void setValue (Axis axis, double newValue, NotificationType notification = sendNotification);

    /** Returns the value of an axis. */
    double getValue (Axis axis) const noexcept { return axes[index (axis)].value; }

    /** Sets both values at once, notifying a single change. */
    void setValues (double newX, double newY, NotificationType notification = sendNotification);

    /** Returns the value of an axis mapped to 0..1 through its range (1 is the right or top edge). */
    double getNormalizedValue (Axis axis) const;

    /** Sets the value an axis returns to on double click or snap back. */
    void setDefaultValue (Axis axis, double newDefault);

    /** Returns the value an axis returns to on double click or snap back. */
    double getDefaultValue (Axis axis) const noexcept { return axes[index (axis)].defaultValue; }

    /** Moves both values back to their defaults. */
    void resetToDefaults (NotificationType notification = sendNotification);

    //==============================================================================
    /** Makes the thumb return to the defaults when released. Defaults to false. */
    void setSnapBack (bool shouldSnapBack) noexcept { snapBack = shouldSnapBack; }

    /** Returns true if the thumb returns to the defaults when released. */
    bool isSnapBack() const noexcept { return snapBack; }

    /** Sets which part of the component reacts to the mouse. Defaults to GrabMode::anywhere. */
    void setGrabMode (GrabMode newGrabMode) noexcept { grabMode = newGrabMode; }

    /** Returns which part of the component reacts to the mouse. */
    GrabMode getGrabMode() const noexcept { return grabMode; }

    /** Sets the diameter of the thumb, in points. Defaults to 16. */
    void setThumbSize (float newSize);

    /** Returns the diameter of the thumb, in points. */
    float getThumbSize() const noexcept { return thumbSize; }

    /** Returns the center of the thumb in local coordinates. */
    Point<float> getThumbPosition() const;

    /** Returns the area of the thumb in local coordinates: as large as getThumbSize() and centered on getThumbPosition().

        It is where the theme paints the thumb. With GrabMode::thumb a press grabs the thumb within half its
        size of its center, or within 4 points of it when the thumb is smaller than that.
    */
    Rectangle<float> getThumbBounds() const;

    /** Returns true while the thumb is being dragged. */
    bool isDragging() const noexcept { return dragging; }

    //==============================================================================
    /** Called with the x and y values when either changes. */
    std::function<void (double, double)> onValueChanged;

    /** Called when a drag starts. */
    std::function<void()> onDragStarted;

    /** Called when a drag ends. */
    std::function<void()> onDragEnded;

    //==============================================================================
    /** Color identifiers used by the joystick. */
    struct Style
    {
        static const Identifier backgroundColorId;
        static const Identifier outlineColorId;
        static const Identifier guideColorId;
        static const Identifier thumbColorId;
        static const Identifier thumbOverColorId;
        static const Identifier thumbDownColorId;
    };

    //==============================================================================
    /** Returns the type description of JoystickComponent. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    bool hitTest (float x, float y) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    bool keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;

private:
    struct AxisState
    {
        NormalisableRange<double> range;
        double defaultValue = 0.5;
        double value = 0.5;
        bool logarithmic = false;
    };

    static constexpr std::size_t index (Axis axis) noexcept { return axis == Axis::x ? 0 : 1; }

    void moveThumbTo (Point<float> center);
    void nudge (Axis axis, double steps);
    void setDragging (bool shouldBeDragging);
    void valuesChanged (bool xChanged, bool yChanged, NotificationType notification);

    std::array<AxisState, 2> axes;
    GrabMode grabMode = GrabMode::anywhere;
    float thumbSize = 16.0f;
    Point<float> grabOffset;
    bool snapBack = false;
    bool dragging = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoystickComponent)
};

} // namespace yup
