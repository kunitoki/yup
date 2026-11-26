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
    getDefaultValue() - the reset fires onValueChanged before onDragEnd, so
    a host recording the drag as a single gesture captures the spring-back
    inside it. Set it to false to build a "hold" pitch wheel instead. A wheel
    disabled or hidden while it is dragged ends the drag the same way, without
    waiting for the release.

    The actual drawing is delegated to the ApplicationTheme system.

    The component follows a MidiKeyboardState like MidiKeyboardComponent does: it
    registers as a listener and the pitch-wheel position of the watched midi
    channel (see setMidiChannel()) is applied asynchronously on the message
    thread, so the state can be updated from an audio or MIDI callback. Multiple
    updates arriving between message-thread passes are coalesced and only the
    latest position is applied. No update is applied while the user is dragging
    the wheel: the latest position reported during the drag is applied when the
    drag ends, after the spring-back.

    The state can be given after construction with setState(), which is how a wheel
    created by a `.yui` document as `PitchWheel` gets it (see YuiDocument::setInitializer).
    Without a state the wheel is moved by the user alone.

    @see MidiWheelComponent, ModWheelComponent, MidiKeyboardComponent

    @tags{GUI, Keyboard}
*/
class YUP_API PitchWheelComponent : public MidiWheelComponent
{
public:
    //==============================================================================
    /** Creates a PitchWheelComponent with no state, to be given one with setState().

        @param componentID    an optional identifier for this component
    */
    explicit PitchWheelComponent (StringRef componentID = {});

    /** Creates a PitchWheelComponent.

        @param state          the MidiKeyboardState whose pitch-wheel position this
                              component will follow, and which the component will
                              listen to for changes
        @param componentID    an optional identifier for this component

        Note that the state must be destroyed after any components that are
        listening to it.
    */
    PitchWheelComponent (MidiKeyboardState& state, StringRef componentID = {});

    //==============================================================================
    /** Returns the static type info describing the PitchWheelComponent type, `PitchWheel` in `.yui`. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** Sets whether releasing the mouse springs the value back to the default value:
        true (the default) for a spring-back pitch wheel, false to hold.
    */
    using MidiWheelComponent::setResetOnRelease;

    /** Returns whether releasing the mouse springs the value back to the default value. */
    using MidiWheelComponent::getResetOnRelease;

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
    /** @internal */
    void paint (Graphics& g) override;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchWheelComponent)
};

} // namespace yup
