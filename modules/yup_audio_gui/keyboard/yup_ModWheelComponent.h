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
    Double-clicking resets it to getDefaultValue(). A wheel disabled or hidden
    while it is dragged ends the drag without waiting for the release.

    The actual drawing is delegated to the ApplicationTheme system.

    The component follows a MidiKeyboardState like MidiKeyboardComponent does: it
    registers as a listener and the modulation wheel (MIDI CC 1) value of the
    watched midi channel (see setMidiChannel()) is applied asynchronously on the
    message thread, so the state can be updated from an audio or MIDI callback.
    Multiple updates arriving between message-thread passes are coalesced and
    only the latest value is applied. No update is applied while the user is
    dragging the wheel: the latest value reported during the drag is applied
    when the drag ends.

    The state can be given after construction with setState(), which is how a wheel
    created by a `.yui` document as `ModWheel` gets it (see YuiDocument::setInitializer).
    Without a state the wheel is moved by the user alone.

    @see MidiWheelComponent, PitchWheelComponent, MidiKeyboardComponent

    @tags{GUI, Keyboard}
*/
class YUP_API ModWheelComponent : public MidiWheelComponent
{
public:
    //==============================================================================
    /** Creates a ModWheelComponent with no state, to be given one with setState().

        @param componentID    an optional identifier for this component
    */
    explicit ModWheelComponent (StringRef componentID = {});

    /** Creates a ModWheelComponent.

        @param state          the MidiKeyboardState whose modulation wheel value this
                              component will follow, and which the component will
                              listen to for changes
        @param componentID    an optional identifier for this component

        Note that the state must be destroyed after any components that are
        listening to it.
    */
    ModWheelComponent (MidiKeyboardState& state, StringRef componentID = {});

    //==============================================================================
    /** Returns the static type info describing the ModWheelComponent type, `ModWheel` in `.yui`. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

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

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModWheelComponent)
};

} // namespace yup
