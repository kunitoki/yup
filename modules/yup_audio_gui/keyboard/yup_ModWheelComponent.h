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
    A vertical-drag control for a MIDI modulation wheel.

    ModWheelComponent reports a unipolar value in the fixed range 0.0 to 1.0,
    matching the MIDI-range-agnostic convention of Slider::getValue() - callers
    are responsible for mapping it onto a 7-bit or 14-bit MIDI CC message.

    Dragging vertically moves the value away from wherever it was when the drag
    started, scaled by getMouseDragSensitivity(). Unlike PitchWheelComponent, the
    value never springs back on release - a mod wheel holds wherever it was left.
    Double-clicking resets it to getDefaultValue().

    The actual drawing is delegated to the ApplicationTheme system.

    The component follows a MidiKeyboardState like MidiKeyboardComponent does: it
    registers as a listener and the modulation wheel (MIDI CC 1) value of the
    watched midi channel (see setMidiChannel()) is applied asynchronously on the
    message thread, so the state can be updated from an audio or MIDI callback.
    Multiple updates arriving between message-thread passes are coalesced and
    only the latest value is applied, and no update is applied while the user is
    dragging the wheel.

    @see Component, Slider, PitchWheelComponent, MidiKeyboardComponent

    @tags{GUI, Keyboard}
*/
class YUP_API ModWheelComponent
    : public Component
    , public MidiKeyboardState::Listener
    , private AsyncUpdater
{
public:
    //==============================================================================
    /** Creates a ModWheelComponent.

        @param state          the MidiKeyboardState whose modulation wheel value this
                              component will follow, and which the component will
                              listen to for changes
        @param componentID    an optional identifier for this component

        Note that the state must be destroyed after any components that are
        listening to it.
    */
    ModWheelComponent (MidiKeyboardState& state, StringRef componentID = {});

    /** Destructor. */
    ~ModWheelComponent() override;

    //==============================================================================
    /** Sets the current value.

        @param newValue      the new value, clamped to 0.0 to 1.0
        @param notification  whether to notify onValueChanged
    */
    void setValue (double newValue, NotificationType notification = sendNotification);

    /** Returns the current value, in the range 0.0 to 1.0. */
    double getValue() const noexcept { return value; }

    //==============================================================================
    /** Changes the midi channel whose modulation wheel value this component follows.

        @param midiChannelNumber  the midi channel (1 to 16)
    */
    void setMidiChannel (int midiChannelNumber);

    /** Returns the midi channel that this component follows. */
    int getMidiChannel() const noexcept { return midiChannel.load(); }

    //==============================================================================
    /** Sets the value applied by mouseDoubleClick().

        @param newDefaultValue  the new default value, clamped to 0.0 to 1.0
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
    /** Color identifiers used by the mod wheel component. */
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

    /** Called when a drag gesture ends. */
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
    void handleControllerMoved (MidiKeyboardState*, int midiChannel, int controllerNumber, int newControllerValue) override;
    /** @internal */
    void handleAsyncUpdate() override;

private:
    //==============================================================================
    friend void paintModWheel (Graphics&, const ApplicationTheme&, const ModWheelComponent&);

    double constrainValue (double newValue) const noexcept;

    MidiKeyboardState& state;
    std::atomic<int> midiChannel { 1 };

    double value = 0.0;
    double defaultValue = 0.0;
    double mouseDragSensitivity = 1.0;

    bool isDragging = false;
    bool isMouseOverWheel = false;
    float mouseDragStartY = 0.0f;
    double valueOnMouseDown = 0.0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModWheelComponent)
};

} // namespace yup
