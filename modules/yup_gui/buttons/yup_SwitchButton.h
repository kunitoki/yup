/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
/** A toggle switch button with a sliding indicator.

    The SwitchButton is a specialized button that displays as a toggle switch
    with a circular indicator that slides between on/off positions. It maintains
    a toggle state and provides visual feedback for the current state.

    @see Button, ToggleButton
*/
class YUP_API SwitchButton : public Button
{
public:
    //==============================================================================
    /** Creates a new SwitchButton.

        @param componentID    The component identifier for this button
        @param isVertical     Whether the switch should be oriented vertically
    */
    SwitchButton (StringRef componentID = {}, bool isVertical = false);

    //==============================================================================
    /** Returns true if the button is currently toggled on.

        @returns true if the button is in its toggled state
    */
    bool getToggleState() const noexcept { return toggleState; }

    /** Sets the button's toggle state.

        @param shouldBeToggled    Whether the button should be toggled on
        @param notification      Whether to send a notification about the change
    */
    void setToggleState (bool shouldBeToggled, NotificationType notification = sendNotification);

    //==============================================================================

    /** Sets whether the switch should be oriented vertically.

        @param shouldBeVertical whether the switch should be vertical
    */
    void setVertical (bool shouldBeVertical);

    /** Returns whether the switch is oriented vertically.

        @returns true if the switch is vertical
    */
    bool isVertical() const noexcept { return isVerticalValue; }

    //==============================================================================
    /** Sets the time in milliseconds for the switch animation.

        @param newValue the animation time in milliseconds
    */
    void setMillisecondsToSpendMoving (int newValue) noexcept;

    //==============================================================================
    /** Called when the toggle state changes.

        Override this to perform custom actions when the switch is toggled.
    */
    virtual void toggleStateChanged() {}

    //==============================================================================
    /** Color identifiers used by the switch button. */
    struct Style
    {
        static const Identifier switchColorId;
        static const Identifier switchOffBackgroundColorId;
        static const Identifier switchOnBackgroundColorId;

        static const Identifier cornerRadiusId; /**< Corner radius, unset for a pill */
        static const Identifier thumbInsetId;   /**< Inset of the handle inside its slot, 4 by default */
        static const Identifier thumbSizeId;    /**< Thickness of the switch, which is twice as long: 28 by default */
    };

    //==============================================================================
    /** Returns the static type info describing the SwitchButton type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** Returns the area of the switch in the component.

        The switch keeps the thickness its style gives it (Style::thumbSizeId) and is twice as long, whatever the
        size of the component: it is centered in it, and only gets smaller in a component with less room. It is
        what the pointer hits: a press beside it in a larger component is not a press of the switch.
    */
    Rectangle<float> getTrackBounds() const;

    /** Returns the area of the handle in the switch, before the inset of its style: it slides from one end of
        getTrackBounds() to the other when the switch is toggled.
    */
    Rectangle<float> getThumbBounds() const;

    /** @internal */
    bool hitTest (float x, float y) override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds);
    /** @internal */
    void paintButton (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void visibilityChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal */
    void clicked() override;

private:
    //==============================================================================
    void updateSwitchCirclePosition();
    void updateFrameAttachment();

    //==============================================================================
    bool toggleState = false;
    bool isVerticalValue = false;
    int millisecondsToSpendMoving = 50;

    // Where the handle is between the end it has when off (0) and the one it has when on (1)
    float handlePosition = 0.0f;

    // Animation state
    float animationStartPosition = 0.0f;
    double animationSeconds = 0.0;
    bool isAnimating = false;
    FrameAttachment frameAttachment { *this, bindFront (&SwitchButton::refreshDisplay, this), false };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwitchButton)
};

} // namespace yup
