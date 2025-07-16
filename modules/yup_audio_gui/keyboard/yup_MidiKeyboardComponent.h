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
    A component that displays a virtual MIDI keyboard.

    This component renders a piano-style keyboard with white and black keys that
    responds to mouse interactions and updates a MidiKeyboardState object. It also
    monitors the state to visually show which keys are currently pressed.

    The actual drawing is delegated to the ApplicationTheme system.

    @tags{AudioGUI}
*/
class YUP_API MidiKeyboardComponent
    : public Component
    , public MidiKeyboardState::Listener
{
public:
    //==============================================================================
    /** The different orientations that the keyboard can have. */
    enum Orientation
    {
        horizontalKeyboard,
        verticalKeyboardFacingLeft,
        verticalKeyboardFacingRight
    };

    //==============================================================================
    /** Creates a MidiKeyboardComponent.

        @param state           the MidiKeyboardState object that this keyboard will use to show
                              which keys are down, and which the user can use to trigger key events
        @param orientation     whether the keyboard is horizontal or vertical
    */
    MidiKeyboardComponent (MidiKeyboardState& state, Orientation orientation);

    /** Destructor. */
    ~MidiKeyboardComponent() override;

    //==============================================================================
    /** Changes the velocity used in midi note-on messages that are triggered by clicking
        on the component.

        @param velocity   the new velocity, in the range 0 to 1.0
    */
    void setVelocity (float velocity);

    /** Returns the current velocity setting. */
    float getVelocity() const noexcept { return velocity; }

    //==============================================================================
    /** Changes the midi channel number that will be used for events triggered by clicking
        on the component.

        @param midiChannelNumber  the midi channel (1 to 16). Events with midi
                                 channel numbers outside this range are ignored
    */
    void setMidiChannel (int midiChannelNumber);

    /** Returns the midi channel that the keyboard is using for midi messages. */
    int getMidiChannel() const noexcept { return midiChannel; }

    //==============================================================================
    /** Changes the number of octaves displayed by the keyboard.

        @param numOctaves    the number of octaves to display
    */
    void setOctaveForMiddleC (int octaveNumber);

    /** Returns the number of octaves currently being displayed. */
    int getOctaveForMiddleC() const noexcept { return octaveNumForMiddleC; }

    //==============================================================================
    /** Changes the lowest visible key on the keyboard.

        @param noteNumber   the midi note number (0-127) of the lowest key to be shown
    */
    void setLowestVisibleKey (int noteNumber);

    /** Returns the lowest visible key. */
    int getLowestVisibleKey() const noexcept { return rangeStart; }

    /** Sets the range of keys that the keyboard will display.

        @param lowestNote   the lowest key (0-127)
        @param highestNote  the highest key (0-127)
    */
    void setAvailableRange (int lowestNote, int highestNote);

    /** Returns the highest key that is shown on the keyboard. */
    int getHighestVisibleKey() const noexcept { return rangeEnd; }

    Range<float> getKeyStartRange() const;

    //==============================================================================
    /** Returns the position within the component of a key.

        @param midiNoteNumber  the note to find the position of
        @returns               the key's rectangle, or an empty rectangle if the key isn't visible
    */
    Rectangle<float> getRectangleForKey (int midiNoteNumber) const;

    /** Returns the note number of the key at a given position within the component.

        @param position  the position to search
        @returns         the midi note number of the key, or -1 if there's no key there
    */
    int getNoteAtPosition (Point<float> position) const;

    //==============================================================================

    bool isNoteOn (int midiNoteNumber) const;

    //==============================================================================

    virtual bool isBlackKey (int midiNoteNumber) const;

    int getNumWhiteKeysInRange (int rangeStart, int rangeEnd) const;

    //==============================================================================
    /** Color identifiers used by the midi keyboard component. */
    struct Style
    {
        static const Identifier whiteKeyColorId;
        static const Identifier whiteKeyPressedColorId;
        static const Identifier whiteKeyShadowColorId;
        static const Identifier blackKeyColorId;
        static const Identifier blackKeyPressedColorId;
        static const Identifier blackKeyShadowColorId;
        static const Identifier keyOutlineColorId;
    };

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void mouseDown (const MouseEvent& e) override;
    /** @internal */
    void mouseDrag (const MouseEvent& e) override;
    /** @internal */
    void mouseUp (const MouseEvent& e) override;
    /** @internal */
    void mouseMove (const MouseEvent& e) override;
    /** @internal */
    void mouseEnter (const MouseEvent& e) override;
    /** @internal */
    void mouseExit (const MouseEvent& e) override;
    /** @internal */
    void mouseWheel (const MouseEvent& e, const MouseWheelData& wheel) override;
    /** @internal */
    void handleNoteOn (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    /** @internal */
    void handleNoteOff (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusLost () override;

    //==============================================================================
    /** @internal */
    void getKeyPosition (int midiNoteNumber, float keyWidth, Rectangle<float>& keyPos, bool& isBlack) const;
    /** @internal */
    bool isMouseOverNote (int midiNoteNumber) const { return midiNoteNumber == mouseOverNote; }

private:
    //==============================================================================
    MidiKeyboardState& state;

    int midiChannel = 1;
    int midiInChannelMask = 0xffff;
    float velocity = 1.0f;

    int rangeStart = 12;
    int rangeEnd = 96;
    int octaveNumForMiddleC = 3;

    Orientation orientation;

    Array<int> mouseDownNotes;
    int mouseOverNote = -1;
    bool shouldCheckState = false;

    String getWhiteNoteText (int midiNoteNumber);
    int xyToNote (Point<float> pos, float& mousePositionVelocity);
    int remappedXYToNote (Point<float> pos, float& mousePositionVelocity) const;
    void repaintNote (int midiNoteNumber);
    void updateNoteUnderMouse (Point<float> pos, bool isDown, int fingerNum);
    void updateNoteUnderMouse (const MouseEvent& e, bool isDown);
    void resetAnyKeysInUse();
    void updateShadowNoteUnderMouse (const MouseEvent& e);

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiKeyboardComponent)
};

} // namespace yup
