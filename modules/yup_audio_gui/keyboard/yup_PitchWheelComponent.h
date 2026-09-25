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

//==============================================================================
/**
    A vertical-drag control for MIDI pitch-bend.

    PitchWheelComponent reports a bipolar value in the fixed range -1.0 to 1.0,
    matching the MIDI-range-agnostic convention of Slider::getValue() - callers
    are responsible for mapping it onto a 14-bit pitch-wheel MIDI message (see
    MidiMessage::pitchWheel()).

    Dragging vertically moves the value away from wherever it was when the drag
    started, scaled by getMouseDragSensitivity(). When getResetOnRelease() is
    true (the default), releasing the mouse springs the value back to
    getDefaultValue() - the reset fires onValueChanged() before onDragEnd(), so
    a host recording the drag as a single gesture captures the spring-back
    inside it. Set it to false to build a "hold" pitch wheel instead.

    The actual drawing is delegated to the ApplicationTheme system.

    The component follows a MidiKeyboardState like MidiKeyboardComponent does: it
    registers as a listener and the pitch-wheel position of the watched midi
    channel (see setMidiChannel()) is applied asynchronously on the message
    thread, so the state can be updated from an audio or MIDI callback. Multiple
    updates arriving between message-thread passes are coalesced and only the
    latest position is applied, and no update is applied while the user is
    dragging the wheel.

    @see Component, Slider, MidiKeyboardComponent

    @tags{GUI, Keyboard}
*/
class YUP_API PitchWheelComponent
    : public Component
    , public MidiKeyboardState::Listener
    , private AsyncUpdater
{
public:
    //==============================================================================
    /** Creates a PitchWheelComponent.

        @param state          the MidiKeyboardState whose pitch-wheel position this
                              component will follow, and which the component will
                              listen to for changes
        @param componentID    an optional identifier for this component

        Note that the state must be destroyed after any components that are
        listening to it.
    */
    PitchWheelComponent (MidiKeyboardState& state, StringRef componentID = {});

    /** Destructor. */
    ~PitchWheelComponent() override;

    //==============================================================================
    /** Sets the current value.

        @param newValue      the new value, clamped to -1.0 to 1.0
        @param notification  whether to notify onValueChanged
    */
    void setValue (double newValue, NotificationType notification = sendNotification);

    /** Returns the current value, in the range -1.0 to 1.0. */
    double getValue() const noexcept { return value; }

    //==============================================================================
    /** Changes the midi channel whose pitch-wheel position this component follows.

        @param midiChannelNumber  the midi channel (1 to 16)
    */
    void setMidiChannel (int midiChannelNumber);

    /** Returns the midi channel that this component follows. */
    int getMidiChannel() const noexcept { return midiChannel.load(); }

    //==============================================================================
    /** Sets the value the wheel springs back to when getResetOnRelease() is true,
        and the value set by mouseDoubleClick().

        @param newDefaultValue  the new default value, clamped to -1.0 to 1.0
    */
    void setDefaultValue (double newDefaultValue);

    /** Returns the default value. */
    double getDefaultValue() const noexcept { return defaultValue; }

    //==============================================================================
    /** Sets whether releasing the mouse springs the value back to the default value.

        @param shouldReset  true for a spring-back pitch wheel, false to hold
    */
    void setResetOnRelease (bool shouldReset) noexcept { resetOnRelease = shouldReset; }

    /** Returns whether releasing the mouse springs the value back to the default value. */
    bool getResetOnRelease() const noexcept { return resetOnRelease; }

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
    /** Color identifiers used by the pitch wheel component. */
    struct Style
    {
        static const Identifier bodyTopColorId;
        static const Identifier bodyBottomColorId;
        static const Identifier outlineColorId;
        static const Identifier gripColorId;
        static const Identifier gripOverColorId;
        static const Identifier gripDownColorId;
    };

    //==============================================================================
    /** Called whenever the value changes. */
    std::function<void (double)> onValueChanged;

    /** Called when a drag gesture begins. */
    std::function<void (const MouseEvent&)> onDragStart;

    /** Called when a drag gesture ends, after any spring-back reset. */
    std::function<void (const MouseEvent&)> onDragEnd;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
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
    void handleNoteOn (MidiKeyboardState*, int, int, float) override;
    /** @internal */
    void handleNoteOff (MidiKeyboardState*, int, int, float) override;
    /** @internal */
    void handlePitchWheelMoved (MidiKeyboardState*, int midiChannel, int wheelPosition) override;
    /** @internal */
    void handleAsyncUpdate() override;

private:
    //==============================================================================
    friend void paintPitchWheel (Graphics&, const ApplicationTheme&, const PitchWheelComponent&);

    double constrainValue (double newValue) const noexcept;

    MidiKeyboardState& state;
    std::atomic<int> midiChannel { 1 };

    double value = 0.0;
    double defaultValue = 0.0;
    bool resetOnRelease = true;
    double mouseDragSensitivity = 1.0;

    bool isDragging = false;
    bool isMouseOverWheel = false;
    float mouseDragStartY = 0.0f;
    double valueOnMouseDown = 0.0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchWheelComponent)
};

} // namespace yup
