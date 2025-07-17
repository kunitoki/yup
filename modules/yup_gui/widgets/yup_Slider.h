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

    This class provides a comprehensive slider implementation similar to JUCE's Slider,
    supporting rotary knobs, linear sliders with various orientations, and two-value
    range sliders. It integrates with the YUP theming system for customizable appearance.

    @see Component, NormalisableRange, ComponentStyle
 */
class YUP_API Slider : public Component
{
public:
    //==============================================================================
    /** Defines the different types of slider that can be created. */
    enum SliderType
    {
        LinearHorizontal,           /**< A horizontal linear slider. */
        LinearVertical,             /**< A vertical linear slider. */
        LinearBarHorizontal,        /**< A horizontal linear slider with a filled bar. */
        LinearBarVertical,          /**< A vertical linear slider with a filled bar. */
        Rotary,                     /**< A rotary knob slider. */
        RotaryHorizontalDrag,       /**< A rotary knob that responds to horizontal dragging. */
        RotaryVerticalDrag,         /**< A rotary knob that responds to vertical dragging. */
        IncDecButtons,              /**< A slider with increment/decrement buttons. */
        TwoValueHorizontal,         /**< A horizontal two-value range slider. */
        TwoValueVertical,           /**< A vertical two-value range slider. */
        ThreeValueHorizontal,       /**< A horizontal three-value slider (min, mid, max). */
        ThreeValueVertical          /**< A vertical three-value slider (min, mid, max). */
    };

    /** Defines the text entry box position for sliders with text boxes. */
    enum TextEntryBoxPosition
    {
        NoTextBox,                  /**< No text entry box. */
        TextBoxLeft,                /**< Text box positioned to the left. */
        TextBoxRight,               /**< Text box positioned to the right. */
        TextBoxAbove,               /**< Text box positioned above. */
        TextBoxBelow                /**< Text box positioned below. */
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
    /** Sets the minimum value for two-value and three-value sliders.

        @param newMinValue The new minimum value
        @param notification Whether to send change notifications
        @param allowNudgingOfOtherValues Whether to adjust other values if they conflict
    */
    void setMinValue (double newMinValue,
                      NotificationType notification = sendNotification,
                      bool allowNudgingOfOtherValues = false);

    /** Returns the minimum value for two-value and three-value sliders. */
    double getMinValue() const;

    /** Sets the maximum value for two-value and three-value sliders.

        @param newMaxValue The new maximum value
        @param notification Whether to send change notifications
        @param allowNudgingOfOtherValues Whether to adjust other values if they conflict
    */
    void setMaxValue (double newMaxValue,
                      NotificationType notification = sendNotification,
                      bool allowNudgingOfOtherValues = false);

    /** Returns the maximum value for two-value and three-value sliders. */
    double getMaxValue() const;

    /** Called when the minimum value changes for multi-value sliders. */
    virtual void minValueChanged();

    /** Called when the maximum value changes for multi-value sliders. */
    virtual void maxValueChanged();

    //==============================================================================
    /** Sets the slider's default value (used for double-click reset).
        @param newDefaultValue The new default value
    */
    void setDefaultValue (double newDefaultValue);

    /** Returns the slider's default value. */
    double getDefaultValue() const;

    //==============================================================================
    /** Sets the slider's range using a NormalisableRange.
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

    /** Sets the number of decimal places to use when displaying values.

        @param decimalPlaces Number of decimal places (negative for automatic)
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

    /** Sets the velocity-based sensitivity for mouse wheel and fine dragging.

        @param sensitivity Multiplier for velocity-based movement
        @param threshold Minimum velocity threshold
        @param offsetThreshold Minimum offset threshold
    */
    void setVelocityModeParameters (double sensitivity = 1.0,
                                    double threshold = 1.0,
                                    double offsetThreshold = 0.0);

    //==============================================================================
    /**
        Style color IDs for customizing Slider appearance.
    */
    struct Style
    {
        static const Identifier backgroundColorId;     /**< Background color for slider track/circle */
        static const Identifier trackColorId;          /**< Color for active track or value indicator */
        static const Identifier thumbColorId;          /**< Color for slider thumb/knob */
        static const Identifier thumbOverColorId;      /**< Color for thumb when mouse is over */
        static const Identifier thumbDownColorId;      /**< Color for thumb when pressed */
        static const Identifier textColorId;           /**< Color for text labels */
    };

    //==============================================================================
    /** Callback function objects for value changes. */
    std::function<void (double)> onValueChanged;
    std::function<void (double)> onMinValueChanged;
    std::function<void (double)> onMaxValueChanged;

    /** Callback function objects for drag events. */
    std::function<void (const MouseEvent&)> onDragStart;
    std::function<void (const MouseEvent&)> onDragEnd;

    //==============================================================================
    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& data) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;
    /** @internal */
    Rectangle<float> getSliderBounds() const;

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

    void updateValueFromMousePosition (Point<float> mousePos, DragMode dragMode);
    void updateValueFromKeypress (const KeyPress& key);
    void resetToDefaultValue();

    double constrainValue (double valueToConstrain) const;
    double snapToLegalValue (double valueToSnap) const;

    Rectangle<float> getTextBoxBounds() const;

    DragMode getDragModeForMousePosition (Point<float> mousePos) const;
    bool isMouseOverSliderArea (Point<float> mousePos) const;

    // New methods for improved linear slider handling
    Rectangle<float> getThumbBounds() const;
    bool isMouseOverThumb (Point<float> mousePos) const;
    float getThumbSize() const;

    void showPopupMenu();
    void createTextEditor();
    void hideTextEditor (bool discardCurrentEditorContents);

    //==============================================================================
    SliderType sliderType = LinearHorizontal;
    NormalisableRange<double> range { 0.0, 1.0 };

    double currentValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;

    int numDecimalPlaces = 7;
    double mouseDragSensitivity = 1.0;
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
    Point<float> mouseDownPosition;
    Point<float> mouseDragStartPosition;
    double valueOnMouseDown = 0.0;
    double minValueOnMouseDown = 0.0;
    double maxValueOnMouseDown = 0.0;

    std::unique_ptr<Component> popupBubbleComponent;
    std::unique_ptr<TextEditor> textEditor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slider)
};

} // namespace yup
