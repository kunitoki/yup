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
    responds to mouse and touch interactions and updates a MidiKeyboardState object.
    It also monitors the state to visually show which keys are currently pressed.

    Every pointer is tracked on its own: the mouse and each finger of a multitouch
    screen hold, slide and release their own note, so chords can be played with
    several fingers. The note-on velocity comes from where the key is pressed,
    growing from the back edge of the key to its front edge and scaled by
    setVelocity(), unless setMidiVelocitySensitive() turns that off.

    The keyboard shows a visible window (setVisibleRange()) into a wider available
    range (setAvailableRange()). When the window doesn't cover the whole available
    range, two scroll buttons at the ends of the keyboard shift it by an octave
    (see setScrollButtonsVisible()). The mouse wheel scrolls the window by single
    white keys, while Ctrl/Cmd + wheel zooms in and out, clamped to a minimum span
    of a single octave and a maximum span covering the whole available range.
    Every change of the visible window is reported through onVisibleRangeChanged.

    When the component has keyboard focus, notes can be played with the computer
    keyboard, using a configurable set of keys (see setKeyboardKeys()) mapped to
    consecutive semitones starting at the octave set by setKeyPressBaseOctave().
    The default layout is "zsxdcvgbhnjmq2w3er5t6y7ui9o0p"; releasing a key sends
    the matching note-off.

    The actual drawing is delegated to the ApplicationTheme system.

    The key state may be updated from any thread (for example from an audio or MIDI
    input callback via MidiKeyboardState::processNextMidiEvent()); note repaints are
    coalesced and applied on the message thread.

    @tags{AudioGUI}
*/
class YUP_API MidiKeyboardComponent
    : public Component
    , public MidiKeyboardState::Listener
    , private AsyncUpdater
{
public:
    //==============================================================================
    /** The different orientations that the keyboard can have. */
    enum Orientation
    {
        horizontalKeyboard,         /**< Keys hang from the top edge, low notes on the left. */
        verticalKeyboardFacingLeft, /**< Keys hang from the right edge, low notes at the top. */
        verticalKeyboardFacingRight /**< Keys hang from the left edge, low notes at the bottom. */
    };

    //==============================================================================
    /** A note number together with the velocity implied by a position on its key.

        @see getNoteAndVelocityAtPosition
    */
    struct NoteAndVelocity
    {
        /** The midi note number, or -1 if there's no key at the position. */
        int note = -1;

        /** The distance from the back edge of the key, as a proportion of its length (0 to 1). */
        float velocity = 0.0f;
    };

    //==============================================================================
    /** One of the two buttons that scroll the visible range of a MidiKeyboardComponent
        by an octave.

        The keyboard creates and lays out both buttons itself. The class is public so that
        a theme can register a ComponentStyle for it.
    */
    class YUP_API ScrollButton : public Button
    {
    public:
        /** Creates a scroll button for a keyboard.

            @param owner       the keyboard whose visible range the button scrolls
            @param scrollsUp   true to scroll towards higher notes, false towards lower notes
        */
        ScrollButton (MidiKeyboardComponent& owner, bool scrollsUp);

        /** Returns true if the button scrolls towards higher notes. */
        bool isScrollingUp() const noexcept { return scrollsUp; }

        /** Returns the keyboard that owns this button. */
        const MidiKeyboardComponent& getKeyboard() const noexcept { return owner; }

        /** @internal */
        void paintButton (Graphics& g) override;

    private:
        MidiKeyboardComponent& owner;
        bool scrollsUp;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollButton)
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

        When the keyboard is velocity sensitive, this is the velocity of a press at the
        front edge of a key, and presses closer to the back edge are scaled down from it.

        @param velocity   the new velocity, in the range 0 to 1.0

        @see setMidiVelocitySensitive
    */
    void setVelocity (float velocity);

    /** Returns the current velocity setting. */
    float getVelocity() const noexcept { return velocity; }

    /** Chooses whether the position of a mouse or touch press on a key sets its velocity.

        When enabled (the default), the velocity grows from the back edge of the key to its
        front edge and is scaled by getVelocity(). When disabled, every press uses
        getVelocity() as it is. Computer-keyboard notes always use getVelocity().

        @param isSensitive   true to derive the velocity from the press position
    */
    void setMidiVelocitySensitive (bool isSensitive) noexcept { useMousePositionForVelocity = isSensitive; }

    /** Returns true if the position of a press on a key sets its velocity. */
    bool isMidiVelocitySensitive() const noexcept { return useMousePositionForVelocity; }

    //==============================================================================
    /** Changes the midi channel number that will be used for events triggered by clicking
        on the component.

        Any notes currently held down are released before the channel changes.

        @param midiChannelNumber  the midi channel (1 to 16). Events with midi
                                  channel numbers outside this range are ignored
    */
    void setMidiChannel (int midiChannelNumber);

    /** Returns the midi channel that the keyboard is using for midi messages. */
    int getMidiChannel() const noexcept { return midiChannel; }

    /** Chooses which midi channels the keyboard shows as pressed.

        @param midiChannelMask   a bitmask where bit 0 is channel 1 and bit 15 is channel 16.
                                 The default is 0xffff, which shows every channel.
    */
    void setMidiChannelsToDisplay (int midiChannelMask);

    /** Returns the bitmask of the midi channels the keyboard shows as pressed. */
    int getMidiChannelsToDisplay() const noexcept { return midiInChannelMask; }

    //==============================================================================
    /** Sets the octave number used to name middle C (note 60) in the key labels.

        This only affects how octaves are named, see getWhiteNoteText(). The notes played
        from the computer keyboard are set with setKeyPressBaseOctave().

        @param octaveNumber   the octave number of middle C, 3 by default (so note 60 is "C3")
    */
    void setOctaveForMiddleC (int octaveNumber);

    /** Returns the octave number used to name middle C. */
    int getOctaveForMiddleC() const noexcept { return octaveNumForMiddleC; }

    //==============================================================================
    /** Sets the octave played by the first key of the computer-keyboard mapping.

        Any notes currently held down are released before the new octave takes effect,
        so a key held across the change can't get stuck.

        @param octaveNumber   the octave of the first mapped key, 3 by default (so "z" plays note 36)

        @see setKeyboardKeys
    */
    void setKeyPressBaseOctave (int octaveNumber);

    /** Returns the octave played by the first key of the computer-keyboard mapping. */
    int getKeyPressBaseOctave() const noexcept { return keyPressBaseOctave; }

    /** Sets the computer-keyboard keys used to trigger notes, one character per
        semitone offset from the octave base (offset 0 is the octave root set by
        setKeyPressBaseOctave()). Matching against key presses is case-insensitive.

        Any notes currently held down are released before the new mapping takes
        effect, so a key held across the change can't get stuck.

        @param keys   ordered characters, e.g. the default "zsxdcvgbhnjmq2w3er5t6y7ui9o0p"
                      maps 29 consecutive semitones starting at the base octave
    */
    void setKeyboardKeys (const String& keys);

    /** Returns the current computer-keyboard key mapping (always lower-case). */
    String getKeyboardKeys() const noexcept { return keyboardKeys; }

    //==============================================================================
    /** Sets the range of keys that the keyboard can show, and shows all of them.

        The visible range can then be narrowed with setVisibleRange(), and moved within
        this range with the scroll buttons, the mouse wheel or Ctrl/Cmd + wheel zoom.

        @param lowestNote   the lowest key (0-127)
        @param highestNote  the highest key (0-127)
    */
    void setAvailableRange (int lowestNote, int highestNote);

    /** Returns the lowest key of the available range. */
    int getRangeStart() const noexcept { return rangeStart; }

    /** Returns the highest key of the available range. */
    int getRangeEnd() const noexcept { return rangeEnd; }

    /** Sets the range of keys that are shown on the keyboard.

        Both ends are clamped to the available range, with the highest key never
        below the lowest one.

        @param lowestNote   the lowest key to show
        @param highestNote  the highest key to show

        @see setAvailableRange, onVisibleRangeChanged
    */
    void setVisibleRange (int lowestNote, int highestNote);

    /** Changes the lowest visible key, keeping the highest one.

        @param noteNumber   the midi note number of the lowest key to be shown, clamped
                            between the start of the available range and the highest visible key
    */
    void setLowestVisibleKey (int noteNumber);

    /** Returns the lowest visible key. */
    int getLowestVisibleKey() const noexcept { return lowestVisibleKey; }

    /** Changes the highest visible key, keeping the lowest one.

        @param noteNumber   the midi note number of the highest key to be shown, clamped
                            between the lowest visible key and the end of the available range
    */
    void setHighestVisibleKey (int noteNumber);

    /** Returns the highest key that is shown on the keyboard. */
    int getHighestVisibleKey() const noexcept { return highestVisibleKey; }

    /** Called whenever the visible range changes, whether from a setter, the scroll
        buttons, the mouse wheel or zooming.
    */
    std::function<void()> onVisibleRangeChanged;

    //==============================================================================
    /** Shows or hides the two buttons that scroll the visible range by an octave.

        Even when enabled, the buttons are only shown while the visible range doesn't
        cover the whole available range. Each button is disabled once it can't scroll
        any further.

        @param shouldBeVisible   true (the default) to show the buttons when they are useful
    */
    void setScrollButtonsVisible (bool shouldBeVisible);

    /** Returns true if the scroll buttons are shown when they are useful. */
    bool areScrollButtonsVisible() const noexcept { return scrollButtonsVisible; }

    /** Sets the size of the scroll buttons along the keyboard axis.

        The keys are laid out in the space left between the two buttons.

        @param widthOrHeight   the button size, 16 by default. It is limited to half the
                               length of the keyboard.
    */
    void setScrollButtonWidth (float widthOrHeight);

    /** Returns the size of the scroll buttons along the keyboard axis. */
    float getScrollButtonWidth() const noexcept { return scrollButtonWidth; }

    //==============================================================================
    /** Changes the orientation of the keyboard.

        Any notes currently held down are released and the keys are laid out again.
    */
    void setOrientation (Orientation newOrientation);

    /** Returns the orientation of the keyboard. */
    Orientation getOrientation() const noexcept { return orientation; }

    /** Sets the length of the black keys, as a proportion of the white keys length.

        @param proportion   the new proportion (0 to 1), 0.6 by default
    */
    void setBlackNoteLengthProportion (float proportion);

    /** Returns the length of the black keys, as a proportion of the white keys length. */
    float getBlackNoteLengthProportion() const noexcept { return blackNoteLengthProportion; }

    /** Sets the width of the black keys, as a proportion of the white keys width.

        @param proportion   the new proportion (0 to 1), 0.7 by default
    */
    void setBlackNoteWidthProportion (float proportion);

    /** Returns the width of the black keys, as a proportion of the white keys width. */
    float getBlackNoteWidthProportion() const noexcept { return blackNoteWidthProportion; }

    //==============================================================================
    /** Returns the span along the keyboard axis that is covered by keys.

        This is the whole width (or height, for vertical keyboards) of the component,
        minus the space taken by the scroll buttons when they are shown.
    */
    Range<float> getKeyStartRange() const;

    /** Returns the width of a white key along the keyboard axis. */
    float getKeyWidth() const;

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

    /** Returns the key at a given position, together with the velocity implied by it.

        @param position  the position to search
        @returns         the note (-1 if there's no key there) and the distance of the position
                         from the back edge of the key, as a proportion of its length
    */
    NoteAndVelocity getNoteAndVelocityAtPosition (Point<float> position) const;

    //==============================================================================
    /** Returns whether a given note is currently on, on any of the displayed channels.

        @param midiNoteNumber  the note to check

        @see setMidiChannelsToDisplay
    */
    bool isNoteOn (int midiNoteNumber) const;

    /** Returns whether a given note is a black key.

        @param midiNoteNumber  the note to check

        @returns true if the note is a black key, false otherwise
    */
    virtual bool isBlackKey (int midiNoteNumber) const;

    /** Returns the number of white keys in a given range.

        @param rangeStart  the start of the range (inclusive)
        @param rangeEnd    the end of the range (exclusive)

        @returns the number of white keys in the specified range
    */
    int getNumWhiteKeysInRange (int rangeStart, int rangeEnd) const;

    /** Returns the label drawn on a white key.

        The default names C keys with their octave (for example "C3", see setOctaveForMiddleC()),
        other white keys with their letter, and returns an empty string for black keys.

        @param midiNoteNumber  the note to name
    */
    virtual String getWhiteNoteText (int midiNoteNumber) const;

    //==============================================================================
    /** Called when a mouse button or a finger goes down on a key.

        @param midiNoteNumber  the key that was pressed
        @param e               the mouse or touch event
        @returns               true (the default) to play the note, false to suppress it
    */
    virtual bool mouseDownOnKey (int midiNoteNumber, const MouseEvent& e);

    /** Called when a mouse button or a finger held down is dragged onto a key.

        @param midiNoteNumber  the key under the pointer
        @param e               the mouse or touch event
        @returns               true (the default) to move the held note to this key, false to
                               keep the previous one
    */
    virtual bool mouseDraggedToKey (int midiNoteNumber, const MouseEvent& e);

    /** Called when a mouse button or a finger is released over a key.

        @param midiNoteNumber  the key under the pointer
        @param e               the mouse or touch event
    */
    virtual void mouseUpOnKey (int midiNoteNumber, const MouseEvent& e);

    //==============================================================================
    /** Color identifiers used by the midi keyboard component and its scroll buttons. */
    struct Style
    {
        static const Identifier whiteKeyColorId;              /**< Fill of an idle white key. */
        static const Identifier whiteKeyPressedColorId;       /**< Fill of a pressed white key, also blended in on hover. */
        static const Identifier whiteKeyShadowColorId;        /**< Shadow along the back edge of the keys. */
        static const Identifier blackKeyColorId;              /**< Fill of an idle black key. */
        static const Identifier blackKeyPressedColorId;       /**< Fill of a pressed black key, also blended in on hover. */
        static const Identifier blackKeyShadowColorId;        /**< Shadow of the black keys. */
        static const Identifier keyOutlineColorId;            /**< Separators between keys and the front edge line. */
        static const Identifier scrollButtonBackgroundColorId; /**< Background of the scroll buttons. */
        static const Identifier scrollButtonArrowColorId;     /**< Arrow of the scroll buttons. */
    };

    //==============================================================================
    /** @internal */
    void getKeyPosition (int midiNoteNumber, float keyWidth, Rectangle<float>& keyPos, bool& isBlack) const;

    /** @internal */
    bool isMouseOverNote (int midiNoteNumber) const { return midiNoteNumber == mouseOverNote; }

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
    void handleAsyncUpdate() override;
    /** @internal */
    void resized() override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void keyUp (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusLost() override;

private:
    //==============================================================================
    static constexpr int maxPointers = 32;

    void repaintNote (int midiNoteNumber);
    void updateNoteUnderMouse (Point<float> pos, bool isDown, int pointerSlot);
    void updateNoteUnderMouse (const MouseEvent& e, bool isDown);
    bool isNoteHeldByPointer (int midiNoteNumber) const;
    void resetAnyKeysInUse();
    void releaseKeyPressNotes();
    void updateShadowNoteUnderMouse (const MouseEvent& e);
    void updateScrollButtons();

    int getMidiNoteForKey (const KeyPress& key) const;
    void shiftVisibleRange (int semitones);
    void scrollByWhiteKeys (int numWhiteKeys);
    void zoomBy (float zoomDelta, int anchorNote);
    int whiteKeyToNote (int whiteKeyIndex) const;

    MidiKeyboardState& state;

    int midiChannel = 1;
    int midiInChannelMask = 0xffff;
    float velocity = 1.0f;
    bool useMousePositionForVelocity = true;

    int rangeStart = 0;
    int rangeEnd = 127;
    int lowestVisibleKey = 12;
    int highestVisibleKey = 96;
    int octaveNumForMiddleC = 3;
    int keyPressBaseOctave = 3;
    String keyboardKeys = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p";

    Orientation orientation;
    float blackNoteLengthProportion = 0.6f;
    float blackNoteWidthProportion = 0.7f;

    std::array<int, maxPointers> pointerDownNotes;
    Array<int> keyDownNotes;
    int mouseOverNote = -1;

    ScrollButton scrollDownButton;
    ScrollButton scrollUpButton;
    float scrollButtonWidth = 16.0f;
    bool scrollButtonsVisible = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiKeyboardComponent)
};

} // namespace yup
