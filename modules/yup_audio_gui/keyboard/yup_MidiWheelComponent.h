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
    The vertical-drag wheel that PitchWheelComponent and ModWheelComponent are made of.

    A wheel holds a value in a fixed range. Dragging vertically moves the value away
    from wherever it was when the drag started, scaled by getMouseDragSensitivity(),
    and double-clicking sets it to getDefaultValue(). onDragStart and onDragEnd mark
    the gesture. A wheel that resets on release springs back to the default value
    when it is let go - the reset fires onValueChanged before onDragEnd, so a host
    recording the drag as a single gesture captures the spring-back inside it.

    A drag also ends when the wheel is disabled or hidden while it is dragged, itself
    or through one of its parents: the wheel springs back if it resets on release and
    onDragEnd is called, with an event that has no buttons since no release happened.
    The release that comes later ends nothing.

    The wheel follows a MidiKeyboardState like MidiKeyboardComponent does: it
    registers as a listener and the value the state holds for the watched midi
    channel (see setMidiChannel()) is applied asynchronously on the message thread,
    without calling onValueChanged, so the state can be updated from an audio or MIDI
    callback. Multiple updates arriving between message-thread passes are coalesced
    and only the latest value is applied. The state does not move the wheel while the
    user drags it: the latest value it reported during the drag is applied when the
    drag ends, after any spring-back. The wheel never writes to the state.

    The class is not used on its own: PitchWheelComponent and ModWheelComponent choose
    what the wheel follows, with the range that goes with it, and whether it resets on
    release. Each one has a type info and a theme style of its own.

    @see PitchWheelComponent, ModWheelComponent, MidiKeyboardComponent

    @tags{GUI, Keyboard}
*/
class YUP_API MidiWheelComponent
    : public Component
    , public MidiKeyboardState::Listener
    , private AsyncUpdater
{
public:
    //==============================================================================
    /** Destructor. */
    ~MidiWheelComponent() override;

    //==============================================================================
    /** Changes the state the wheel follows.

        The wheel keeps its value until the state reports a new one. Call it on the message
        thread. The state must outlive the wheel, or be removed first by setting a null one.

        @param newState   the state to follow, or nullptr for none
    */
    void setState (MidiKeyboardState* newState);

    /** Returns the state the wheel follows, or nullptr if it has none. */
    MidiKeyboardState* getState() const noexcept { return state; }

    //==============================================================================
    /** Sets the current value.

        @param newValue      the new value, clamped to the range of the wheel
        @param notification  whether to notify onValueChanged
    */
    void setValue (double newValue, NotificationType notification = sendNotification);

    /** Returns the current value, in the range of the wheel. */
    double getValue() const noexcept { return value; }

    //==============================================================================
    /** Changes the midi channel of the state that this component follows.

        @param midiChannelNumber  the midi channel (1 to 16)
    */
    void setMidiChannel (int midiChannelNumber);

    /** Returns the midi channel that this component follows. */
    int getMidiChannel() const noexcept { return midiChannel.load(); }

    //==============================================================================
    /** Sets the value set by mouseDoubleClick(), which is also the one the wheel
        springs back to when it resets on release.

        @param newDefaultValue  the new default value, clamped to the range of the wheel
    */
    void setDefaultValue (double newDefaultValue);

    /** Returns the default value. */
    double getDefaultValue() const noexcept { return defaultValue; }

    //==============================================================================
    /** Sets how far the mouse must be dragged to sweep the full range.

        @param sensitivity  a multiplier applied to the normalized drag delta
    */
    void setMouseDragSensitivity (double sensitivity);

    /** Returns the current drag sensitivity. */
    double getMouseDragSensitivity() const noexcept { return mouseDragSensitivity; }

    //==============================================================================
    /** Returns true if the mouse is currently over the component. */
    bool isMouseOver() const noexcept { return isMouseOverWheel; }

    /** Returns true if the wheel is currently being dragged. */
    bool isCurrentlyBeingDragged() const noexcept { return isDragging; }

    //==============================================================================
    /** Called whenever the value changes, except when the wheel follows its state. */
    std::function<void (double)> onValueChanged;

    /** Called when a drag gesture begins. */
    std::function<void (const MouseEvent&)> onDragStart;

    /** Called when a drag gesture ends, after any spring-back reset.

        The event is the release that ended the drag. When the wheel was disabled or
        hidden while it was dragged, it has no buttons and the last position of the drag.
    */
    std::function<void (const MouseEvent&)> onDragEnd;

    //==============================================================================
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void enablementChanged() override;
    /** @internal */
    void visibilityChanged() override;
    /** @internal */
    void handleNoteOn (MidiKeyboardState*, int, int, float) override;
    /** @internal */
    void handleNoteOff (MidiKeyboardState*, int, int, float) override;
    /** @internal */
    void handlePitchWheelMoved (MidiKeyboardState*, int midiChannel, int wheelPosition) override;
    /** @internal */
    void handleControllerMoved (MidiKeyboardState*, int midiChannel, int controllerNumber, int newControllerValue) override;
    /** @internal */
    void handleAsyncUpdate() override;

protected:
    //==============================================================================
    /** What a wheel follows of its MidiKeyboardState, which sets its range. */
    enum class Source
    {
        pitchWheel,     /**< The pitch-wheel position, as a bipolar value from -1.0 to 1.0. */
        modulationWheel /**< The modulation wheel (MIDI CC 1) value, as a unipolar value from 0.0 to 1.0. */
    };

    /** Creates a wheel with no state, to be given one with setState().

        @param componentID            an optional identifier for this component
        @param source                 what the wheel follows of its state
        @param shouldResetOnRelease   whether the wheel springs back to its default value when released
    */
    MidiWheelComponent (StringRef componentID, Source source, bool shouldResetOnRelease);

    /** Sets whether releasing the mouse springs the value back to the default value.

        @param shouldReset  true for a spring-back wheel, false to hold
    */
    void setResetOnRelease (bool shouldReset) noexcept { resetOnRelease = shouldReset; }

    /** Returns whether releasing the mouse springs the value back to the default value. */
    bool getResetOnRelease() const noexcept { return resetOnRelease; }

private:
    //==============================================================================
    void endDrag (const MouseEvent& event);
    void cancelDrag();
    void applyStateValue();

    const Source source;
    const Range<double> valueRange;

    MidiKeyboardState* state = nullptr;
    std::atomic<int> midiChannel { 1 };

    double value = 0.0;
    double defaultValue = 0.0;
    bool resetOnRelease = false;
    double mouseDragSensitivity = 1.0;

    bool isDragging = false;
    bool isMouseOverWheel = false;
    bool stateChangedWhileDragging = false;
    float mouseDragStartY = 0.0f;
    Point<float> lastDragPosition;
    double valueOnMouseDown = 0.0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiWheelComponent)
};

} // namespace yup
