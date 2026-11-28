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

/**
    A flexible slider component that supports multiple types and orientations.

    This class provides a comprehensive slider implementation similar supporting rotary
    knobs, linear sliders with various orientations, and two-value range sliders. It
    integrates with the YUP theming system for customizable appearance.

    @see Component, NormalisableRange, ComponentStyle
 */
class YUP_API Slider : public Component
{
public:
    //==============================================================================
    /** Defines the different types of slider that can be created. */
    enum SliderType
    {
        LinearHorizontal,     /**< A horizontal linear slider. */
        LinearVertical,       /**< A vertical linear slider. */
        LinearBarHorizontal,  /**< A horizontal linear slider with a filled bar. */
        LinearBarVertical,    /**< A vertical linear slider with a filled bar. */
        Rotary,               /**< A rotary knob slider. */
        RotaryHorizontalDrag, /**< A rotary knob that responds to horizontal dragging. */
        RotaryVerticalDrag,   /**< A rotary knob that responds to vertical dragging. */
        IncDecButtons,        /**< A slider with increment/decrement buttons. */
        TwoValueHorizontal,   /**< A horizontal two-value range slider. */
        TwoValueVertical      /**< A vertical two-value range slider. */
    };

    /** The thumbs a slider can have. */
    enum class Thumb
    {
        none,     /**< No thumb. */
        value,    /**< The thumb of the value of a linear slider. */
        minValue, /**< The thumb of the lower value of a two-value slider. */
        maxValue  /**< The thumb of the upper value of a two-value slider. */
    };

    /** Defines the text entry box position for sliders with text boxes. */
    enum TextEntryBoxPosition
    {
        NoTextBox,    /**< No text entry box. */
        TextBoxLeft,  /**< Text box positioned to the left. */
        TextBoxRight, /**< Text box positioned to the right. */
        TextBoxAbove, /**< Text box positioned above. */
        TextBoxBelow  /**< Text box positioned below. */
    };

    //==============================================================================
    /** Creates a slider with the specified type.

        @param sliderType The type of slider to create
        @param componentID Optional component identifier
    */
    Slider (SliderType sliderType, StringRef componentID);

    /** Creates a slider with the specified type using enum construction.

        @param sliderType The type of slider to create
    */
    explicit Slider (SliderType sliderType);

    //==============================================================================
    /** Sets the slider's current value.

        @param newValue The new value to set
        @param notification Whether to send change notifications
    */
    void setValue (double newValue, NotificationType notification = sendNotification);

    /** Returns the slider's current value. */
    double getValue() const;

    /** Sets the value as a normalised position (0.0 to 1.0).
        @param newValue The normalised value to set
        @param notification Whether to send change notifications
    */
    void setValueNormalised (double newValue, NotificationType notification = sendNotification);

    /** Returns the value as a normalised position (0.0 to 1.0). */
    double getValueNormalised() const;

    /** Called when the slider's value changes. Override to implement custom behavior. */
    virtual void valueChanged();

    //==============================================================================
    /** Sets the minimum value for two-value sliders.

        @param newMinValue The new minimum value
        @param notification Whether to send change notifications
        @param allowNudgingOfOtherValues Whether to adjust other values if they conflict
    */
    void setMinValue (double newMinValue,
                      NotificationType notification = sendNotification,
                      bool allowNudgingOfOtherValues = false);

    /** Returns the minimum value for two-value sliders. */
    double getMinValue() const;

    /** Returns the minimum value as a normalised position (0.0 to 1.0) along the range. */
    double getMinValueNormalised() const;

    /** Sets the maximum value for two-value sliders.

        @param newMaxValue The new maximum value
        @param notification Whether to send change notifications
        @param allowNudgingOfOtherValues Whether to adjust other values if they conflict
    */
    void setMaxValue (double newMaxValue,
                      NotificationType notification = sendNotification,
                      bool allowNudgingOfOtherValues = false);

    /** Returns the maximum value for two-value sliders. */
    double getMaxValue() const;

    /** Returns the maximum value as a normalised position (0.0 to 1.0) along the range. */
    double getMaxValueNormalised() const;

    /** Called when the minimum value changes for multi-value sliders. */
    virtual void minValueChanged();

    /** Called when the maximum value changes for multi-value sliders. */
    virtual void maxValueChanged();

    //==============================================================================
    /** Sets the slider's default value, which a double click and a click with the reset modifier go back to.

        @param newDefaultValue The new default value

        @see setResetModifiers
    */
    void setDefaultValue (double newDefaultValue);

    /** Returns the slider's default value. */
    double getDefaultValue() const;

    //==============================================================================
    /** Sets the slider's range using a NormalisableRange.

        A range that is empty or reversed has no position to give the values: it is refused, and the
        slider keeps the range it has.

        @param newRange The new range with optional skew and step parameters
    */
    void setRange (const NormalisableRange<double>& newRange);

    /** Sets the slider's range with explicit parameters.

        @param minValue The minimum value
        @param maxValue The maximum value
        @param stepSize The step size (0 for continuous)
    */
    void setRange (double minValue, double maxValue, double stepSize = 0.0);

    /** Returns the slider's current range. */
    NormalisableRange<double> getRange() const;

    /** Returns the interval/step size for the slider. */
    double getInterval() const;

    //==============================================================================
    /** Sets the skew factor for the slider's range.

        The skew factor affects how values are distributed across the slider:
        - A value of 1.0 creates a linear distribution (no skewing)
        - Values < 1.0 allocate more space to the lower end of the range
        - Values > 1.0 allocate more space to the upper end of the range

        This is particularly useful for parameters like frequency which benefit
        from logarithmic scaling.

        @param skewFactor The skew factor to apply (must be > 0.0)
    */
    void setSkewFactor (double skewFactor);

    void setSkewFactorFromMidpoint (double midpointValue);

    /** Returns the current skew factor for the slider's range. */
    double getSkewFactor() const;

    //==============================================================================
    /** Returns the area of the button lowering the value of a slider of type IncDecButtons, in
        its coordinates. Empty for a slider of another type.

        The buttons are at the ends of the slider with the value between them: on a line when the
        slider is wider than tall, the one raising the value above the other otherwise. A press
        on a button steps the value by the interval of the range, or by a hundredth of the range
        when it has none, and keeps stepping while it is held.
    */
    Rectangle<float> getDecrementBounds() const;

    /** Returns the area of the button raising the value of a slider of type IncDecButtons.

        @see getDecrementBounds
    */
    Rectangle<float> getIncrementBounds() const;

    /** Returns the area the value of a slider of type IncDecButtons is shown in, as a text: what its
        two buttons leave of the slider between them. Empty for a slider of another type.

        @see getDecrementBounds, getIncrementBounds
    */
    Rectangle<float> getTextBounds() const;

    /** Returns which button of a slider of type IncDecButtons is pressed: -1 for the one lowering
        the value, 1 for the one raising it and 0 for none.
    */
    int getPressedStep() const noexcept { return pressedStep; }

    //==============================================================================
    /** Returns the area the value moves in, in the coordinates of the slider.

        For a linear slider it is the track, which stops short of both ends of the slider to leave
        room for the thumb drawn around its own. For a rotary slider it is the area the dial is drawn
        in the middle of. For a slider of type IncDecButtons it is the area its buttons and its value
        share, which keeps the thickness of a button in the middle of a larger slider. A drag is
        measured against it.

        @see getThumbBounds, getDecrementBounds
    */
    Rectangle<float> getTrackBounds() const;

    /** Returns the area of the thumb of a linear slider, in its coordinates. Empty for a slider of another type.

        The thumb is centered where the value is along the track, and has the size the `thumbSize` property
        gives it. It is where the theme paints it and what a press grabs: a press on the track beside it
        brings it under the pointer first. A slider of a bar type paints none, and ends its bar at the
        center of this area.

        @see getTrackBounds, getMinThumbBounds, getMaxThumbBounds
    */
    Rectangle<float> getThumbBounds() const;

    /** Returns the area of the thumb of the lower value of a two-value slider. Empty for a slider of another type.

        @see getThumbBounds
    */
    Rectangle<float> getMinThumbBounds() const;

    /** Returns the area of the thumb of the upper value of a two-value slider. Empty for a slider of another type.

        @see getThumbBounds
    */
    Rectangle<float> getMaxThumbBounds() const;

    /** Returns the thumb the pointer is over or, while the slider is dragged, the one that is dragged. */
    Thumb getHoveredThumb() const;

    //==============================================================================
    /** Sets the number of decimal places to use when displaying values.

        The default theme shows the value of a slider of type IncDecButtons with 3 decimal places at most.

        @param decimalPlaces Number of decimal places (0 or less for as many as the value needs)
    */
    void setNumDecimalPlacesToDisplay (int decimalPlaces);

    /** Returns the number of decimal places used for display. */
    int getNumDecimalPlacesToDisplay() const;

    //==============================================================================
    /** Sets the slider type.

        @param newType The new slider type
    */
    void setSliderType (SliderType newType);

    /** Returns the current slider type. */
    SliderType getSliderType() const;

    /** Sets the text entry box position and size.

        @param position Where to position the text box
        @param isReadOnly Whether the text box should be read-only
        @param textEntryBoxWidth Width of the text box
        @param textEntryBoxHeight Height of the text box
    */
    void setTextBoxStyle (TextEntryBoxPosition position,
                          bool isReadOnly = false,
                          int textEntryBoxWidth = 80,
                          int textEntryBoxHeight = 20);

    /** Returns the text entry box position. */
    TextEntryBoxPosition getTextBoxPosition() const;

    /** Returns true if the text box is read-only. */
    bool isTextBoxReadOnly() const;

    //==============================================================================
    /** Sets whether the slider should pop up a bubble when dragged.

        @param shouldShowBubble Whether to show the bubble
        @param bubbleComponent Optional custom bubble component
    */
    void setPopupDisplayEnabled (bool shouldShowBubble, Component* bubbleComponent = nullptr);

    /** Sets the menu items that appear when right-clicking the slider. */
    void setPopupMenuEnabled (bool shouldShowMenu);

    //==============================================================================
    /** Returns true if the mouse is currently over the slider. */
    bool isMouseOver() const;

    /** Returns true if the slider is currently being dragged. */
    bool isCurrentlyBeingDragged() const;

    //==============================================================================
    /** Sets the sensitivity of mouse movement for dragging.

        @param sensitivity Multiplier for mouse movement (default is 1.0)
    */
    void setMouseDragSensitivity (double sensitivity);

    /** Returns the current mouse drag sensitivity. */
    double getMouseDragSensitivity() const;

    /** Sets how much slower the value moves while the fine drag modifier is held.

        The sensitivity of a drag and of the mouse wheel is multiplied by it, so 0.1, the default,
        makes the value move ten times slower.

        @param scale The multiplier, above 0.

        @see setFineDragModifiers, setMouseDragSensitivity
    */
    void setFineDragScale (double scale);

    /** Returns how much slower the value moves while the fine drag modifier is held. */
    double getFineDragScale() const;

    /** Sets the modifier keys that make a drag and the mouse wheel fine while they are held, Shift by default.

        Pressing or releasing them during a drag goes on from the value the slider has, with no jump.

        @param modifiers The keys, all of which are to be held, such as KeyModifiers::shiftMask.
                         No key, a default KeyModifiers, for no fine drag.

        @see setFineDragScale
    */
    void setFineDragModifiers (KeyModifiers modifiers);

    /** Returns the modifier keys that make a drag and the mouse wheel fine. */
    KeyModifiers getFineDragModifiers() const;

    /** Sets the modifier keys a click is held with to bring the default value back, Alt by default.

        Such a click starts no drag. A double click does the same, unless a component listener
        takes it (see ComponentListener::componentDoubleClicked).

        @param modifiers The keys, all of which are to be held, such as KeyModifiers::altMask.
                         No key, a default KeyModifiers, for no such click.

        @see setDefaultValue
    */
    void setResetModifiers (KeyModifiers modifiers);

    /** Returns the modifier keys a click is held with to bring the default value back. */
    KeyModifiers getResetModifiers() const;

    /** Sets the velocity-based sensitivity for mouse wheel and fine dragging.

        @param sensitivity Multiplier for velocity-based movement
        @param threshold Minimum velocity threshold
        @param offsetThreshold Minimum offset threshold
    */
    void setVelocityModeParameters (double sensitivity = 1.0,
                                    double threshold = 1.0,
                                    double offsetThreshold = 0.0);

    //==============================================================================
    /** Callback function objects for value changes. */
    std::function<void (double)> onValueChanged;
    std::function<void (double)> onMinValueChanged;
    std::function<void (double)> onMaxValueChanged;

    /** Callback function objects for drag events. */
    std::function<void (const MouseEvent&)> onDragStart;
    std::function<void (const MouseEvent&)> onDragEnd;

    //==============================================================================
    /**
        Style color IDs for customizing Slider appearance.
    */
    struct Style
    {
        static const Identifier backgroundColorId; /**< Background color for slider track/circle */
        static const Identifier trackColorId;      /**< Color for the unfilled track behind the value indicator */
        static const Identifier thumbColorId;      /**< Color for slider thumb/knob or value indicator */
        static const Identifier thumbOverColorId;  /**< Color for thumb/knob or value indicator when mouse is over */
        static const Identifier thumbDownColorId;  /**< Color for thumb/knob or value indicator when pressed */
        static const Identifier textColorId;       /**< Color for text labels */

        static const Identifier trackWidthId;             /**< Thickness of the linear track, 4 by default */
        static const Identifier thumbSizeId;              /**< Diameter of the linear thumbs, 16 by default */
        static const Identifier rotaryInsetId;            /**< Rotary dial inset as a proportion of the smaller side, 0.1 by default */
        static const Identifier rotaryDialInsetId;        /**< Rotary dial body inset inside the arc bounds as a proportion of the smaller side, 0.105 by default */
        static const Identifier rotaryDialOutlineWidthId; /**< Rotary dial outline stroke width as a proportion of the smaller side, 0.0175 by default */
        static const Identifier rotaryPointerInsetId;     /**< Rotary pointer tip inset inside the arc bounds as a proportion of the smaller side, 0.175 by default */
        static const Identifier rotaryPointerWidthId;     /**< Rotary pointer stroke width as a proportion of the smaller side, 0.03 by default */
        static const Identifier rotaryArcWidthId;         /**< Rotary track and value arc stroke width as a proportion of the smaller side, 0.075 by default */
    };

    //==============================================================================
    /** Returns the static type info describing the Slider type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** Returns where whoever paints the slider keeps the value it laid out as a text, to lay it out again only when
        it has to: the value of a slider of type IncDecButtons, shown in getTextBounds().

        The slider holds it and never looks into it: see TextLayoutCache.
    */
    TextLayoutCache& getTextLayoutCache() const noexcept { return textLayoutCache; }

    //==============================================================================
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    bool mouseWheel (const MouseEvent& event, const MouseWheelData& data) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    bool keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;
    /** @internal */
    void enablementChanged() override;
    /** @internal */
    void visibilityChanged() override;

private:
    //==============================================================================
    enum DragMode
    {
        notDragging,
        draggingForValue,
        draggingForMinValue,
        draggingForMaxValue
    };

    void sendValueChanged (NotificationType notification);
    void sendMinValueChanged (NotificationType notification);
    void sendMaxValueChanged (NotificationType notification);
    void sendRangeChanged (const NormalisableRange<double>& previousRange);

    void updateValueFromMousePosition (Point<float> mousePos, DragMode dragMode);
    double getDragScale (const KeyModifiers& modifiers) const;
    bool updateValueFromKeypress (const KeyPress& key);
    void resetToDefaultValue();

    double constrainValue (double valueToConstrain) const;
    double snapToLegalValue (double valueToSnap) const;

    Rectangle<float> getTextBoxBounds() const;

    DragMode getDragModeForMousePosition (Point<float> mousePos) const;
    bool isMouseOverSliderArea (Point<float> mousePos) const;

    Rectangle<float> getThumbBoundsAt (double normalisedValue) const;
    double getNormalisedValueAt (Point<float> position) const;
    void endInteraction (const MouseEvent& event);
    void cancelInteraction();

    Rectangle<float> getStepButtonBounds (int direction) const;
    void stepValue (int direction);
    void repeatStep();

    void showPopupMenu();
    void createTextEditor();
    void hideTextEditor (bool discardCurrentEditorContents);

    //==============================================================================
    SliderType sliderType = LinearHorizontal;
    NormalisableRange<double> range { 0.0, 1.0 };

    double defaultValue = 0.0;
    double currentValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;

    int numDecimalPlaces = 7;
    double mouseDragSensitivity = 1.0;
    double fineDragScale = 0.1;
    KeyModifiers fineDragModifiers { KeyModifiers::shiftMask };
    KeyModifiers resetModifiers { KeyModifiers::altMask };
    double velocitySensitivity = 1.0;
    double velocityThreshold = 1.0;
    double velocityOffsetThreshold = 0.0;

    TextEntryBoxPosition textBoxPosition = NoTextBox;
    bool textBoxIsReadOnly = false;
    int textBoxWidth = 80;
    int textBoxHeight = 20;

    bool popupDisplayEnabled = false;
    bool popupMenuEnabled = false;
    bool isMouseOverSlider = false;

    DragMode dragMode = notDragging;
    Point<float> pointerPosition;
    Point<float> mouseDownPosition;
    Point<float> mouseDragStartPosition;
    double dragScale = 1.0;
    double valueOnMouseDown = 0.0;
    double minValueOnMouseDown = 0.0;
    double maxValueOnMouseDown = 0.0;

    int pressedStep = 0;
    TimedCallback stepRepeater;
    mutable TextLayoutCache textLayoutCache;

    std::unique_ptr<Component> popupBubbleComponent;
    std::unique_ptr<TextEditor> textEditor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slider)
};

} // namespace yup
