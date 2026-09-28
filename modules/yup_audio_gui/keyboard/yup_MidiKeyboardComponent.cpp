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

namespace
{

//==============================================================================

} // namespace

//==============================================================================
// Color identifiers
const Identifier MidiKeyboardComponent::Style::whiteKeyColorId ("midiKeyboardWhiteKey");
const Identifier MidiKeyboardComponent::Style::whiteKeyPressedColorId ("midiKeyboardWhiteKeyPressed");
const Identifier MidiKeyboardComponent::Style::whiteKeyShadowColorId ("midiKeyboardWhiteKeyShadow");
const Identifier MidiKeyboardComponent::Style::blackKeyColorId ("midiKeyboardBlackKey");
const Identifier MidiKeyboardComponent::Style::blackKeyPressedColorId ("midiKeyboardBlackKeyPressed");
const Identifier MidiKeyboardComponent::Style::blackKeyShadowColorId ("midiKeyboardBlackKeyShadow");
const Identifier MidiKeyboardComponent::Style::keyOutlineColorId ("midiKeyboardKeyOutline");
const Identifier MidiKeyboardComponent::Style::scrollButtonBackgroundColorId ("midiKeyboardScrollButtonBackground");
const Identifier MidiKeyboardComponent::Style::scrollButtonArrowColorId ("midiKeyboardScrollButtonArrow");

//==============================================================================
MidiKeyboardComponent::ScrollButton::ScrollButton (MidiKeyboardComponent& ownerToUse, bool shouldScrollUp)
    : Button (shouldScrollUp ? "ScrollUpButton" : "ScrollDownButton")
    , owner (ownerToUse)
    , scrollsUp (shouldScrollUp)
{
    // Clicking hands the focus to the keyboard instead, so it keeps playing from the computer keyboard.
    setWantsKeyboardFocus (false);
}

void MidiKeyboardComponent::ScrollButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================
MidiKeyboardComponent::MidiKeyboardComponent (MidiKeyboardState& stateToUse, Orientation orientationToUse)
    : state (stateToUse)
    , orientation (orientationToUse)
    , scrollDownButton (*this, false)
    , scrollUpButton (*this, true)
{
    state.addListener (this);
    setWantsKeyboardFocus (true);
    setMouseCursor (MouseCursor::Hand);
    //setMouseClickGrabsKeyboardFocus (true);

    pointerDownNotes.fill (-1);

    scrollDownButton.onClick = [this]
    {
        shiftVisibleRange (-12);
    };
    addChildComponent (scrollDownButton);

    scrollUpButton.onClick = [this]
    {
        shiftVisibleRange (12);
    };
    addChildComponent (scrollUpButton);

    updateScrollButtons();
}

MidiKeyboardComponent::~MidiKeyboardComponent()
{
    state.removeListener (this);
}

//==============================================================================
void MidiKeyboardComponent::setVelocity (float newVelocity)
{
    velocity = jlimit (0.0f, 1.0f, newVelocity);
}

void MidiKeyboardComponent::setMidiChannel (int midiChannelNumber)
{
    jassert (midiChannelNumber > 0 && midiChannelNumber <= 16);

    if (midiChannel != midiChannelNumber)
    {
        resetAnyKeysInUse();
        midiChannel = midiChannelNumber;
    }
}

void MidiKeyboardComponent::setMidiChannelsToDisplay (int midiChannelMask)
{
    if (midiInChannelMask != midiChannelMask)
    {
        midiInChannelMask = midiChannelMask;
        repaint();
    }
}

void MidiKeyboardComponent::setOctaveForMiddleC (int octaveNumber)
{
    if (octaveNumForMiddleC != octaveNumber)
    {
        octaveNumForMiddleC = octaveNumber;
        repaint();
    }
}

void MidiKeyboardComponent::setKeyPressBaseOctave (int octaveNumber)
{
    if (keyPressBaseOctave != octaveNumber)
    {
        // Release any held notes so a key held across the octave change can't
        // get stuck (keyUp can no longer resolve it to the same note number).
        resetAnyKeysInUse();

        keyPressBaseOctave = octaveNumber;
    }
}

void MidiKeyboardComponent::setKeyboardKeys (const String& keys)
{
    auto newKeys = keys.toLowerCase();

    if (keyboardKeys != newKeys)
    {
        // Release any held notes so a key held across the mapping change can't
        // get stuck (keyUp can no longer resolve it to the same note number).
        resetAnyKeysInUse();

        keyboardKeys = newKeys;
    }
}

//==============================================================================
void MidiKeyboardComponent::setAvailableRange (int lowestNote, int highestNote)
{
    jassert (isPositiveAndBelow (lowestNote, 128));
    jassert (isPositiveAndBelow (highestNote, 128));
    jassert (lowestNote <= highestNote);

    rangeStart = jlimit (0, 127, lowestNote);
    rangeEnd = jlimit (rangeStart, 127, highestNote);

    setVisibleRange (rangeStart, rangeEnd);

    // The visible range may already cover it, but the buttons still depend on the new edges.
    updateScrollButtons();
    repaint();
}

void MidiKeyboardComponent::setVisibleRange (int lowestNote, int highestNote)
{
    const auto newLowest = jlimit (rangeStart, rangeEnd, lowestNote);
    const auto newHighest = jlimit (newLowest, rangeEnd, highestNote);

    if (newLowest == lowestVisibleKey && newHighest == highestVisibleKey)
        return;

    lowestVisibleKey = newLowest;
    highestVisibleKey = newHighest;

    updateScrollButtons();
    repaint();

    if (onVisibleRangeChanged)
        onVisibleRangeChanged();
}

void MidiKeyboardComponent::setLowestVisibleKey (int noteNumber)
{
    setVisibleRange (jmin (noteNumber, highestVisibleKey), highestVisibleKey);
}

void MidiKeyboardComponent::setHighestVisibleKey (int noteNumber)
{
    setVisibleRange (lowestVisibleKey, jmax (noteNumber, lowestVisibleKey));
}

//==============================================================================
void MidiKeyboardComponent::setScrollButtonsVisible (bool shouldBeVisible)
{
    if (scrollButtonsVisible != shouldBeVisible)
    {
        scrollButtonsVisible = shouldBeVisible;
        updateScrollButtons();
        repaint();
    }
}

void MidiKeyboardComponent::setScrollButtonWidth (float widthOrHeight)
{
    widthOrHeight = jmax (0.0f, widthOrHeight);

    if (scrollButtonWidth != widthOrHeight)
    {
        scrollButtonWidth = widthOrHeight;
        updateScrollButtons();
        repaint();
    }
}

//==============================================================================
void MidiKeyboardComponent::setOrientation (Orientation newOrientation)
{
    if (orientation != newOrientation)
    {
        resetAnyKeysInUse();
        orientation = newOrientation;
        updateScrollButtons();
        repaint();
    }
}

void MidiKeyboardComponent::setBlackNoteLengthProportion (float proportion)
{
    proportion = jlimit (0.0f, 1.0f, proportion);

    if (blackNoteLengthProportion != proportion)
    {
        blackNoteLengthProportion = proportion;
        repaint();
    }
}

void MidiKeyboardComponent::setBlackNoteWidthProportion (float proportion)
{
    proportion = jlimit (0.0f, 1.0f, proportion);

    if (blackNoteWidthProportion != proportion)
    {
        blackNoteWidthProportion = proportion;
        repaint();
    }
}

//==============================================================================
Rectangle<float> MidiKeyboardComponent::getRectangleForKey (int midiNoteNumber) const
{
    jassert (midiNoteNumber >= 0 && midiNoteNumber < 128);

    if (midiNoteNumber < lowestVisibleKey || midiNoteNumber > highestVisibleKey)
        return {};

    Rectangle<float> pos;
    bool isBlack;

    getKeyPosition (midiNoteNumber, getKeyWidth(), pos, isBlack);

    return pos;
}

int MidiKeyboardComponent::getNoteAtPosition (Point<float> position) const
{
    return getNoteAndVelocityAtPosition (position).note;
}

//==============================================================================
void MidiKeyboardComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================
bool MidiKeyboardComponent::mouseDownOnKey (int, const MouseEvent&)
{
    return true;
}

bool MidiKeyboardComponent::mouseDraggedToKey (int, const MouseEvent&)
{
    return true;
}

void MidiKeyboardComponent::mouseUpOnKey (int, const MouseEvent&)
{
}

//==============================================================================
void MidiKeyboardComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    const auto note = getNoteAtPosition (e.getPosition());

    if (note >= 0 && mouseDownOnKey (note, e))
        updateNoteUnderMouse (e, true);
}

void MidiKeyboardComponent::mouseDrag (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    // Dragging off the keys releases the note held by this pointer.
    const auto note = getNoteAtPosition (e.getPosition());

    if (note < 0 || mouseDraggedToKey (note, e))
        updateNoteUnderMouse (e, true);
}

void MidiKeyboardComponent::mouseUp (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    updateNoteUnderMouse (e, false);

    const auto note = getNoteAtPosition (e.getPosition());

    if (note >= 0)
        mouseUpOnKey (note, e);
}

void MidiKeyboardComponent::mouseMove (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    updateShadowNoteUnderMouse (e);
}

void MidiKeyboardComponent::mouseEnter (const MouseEvent& e)
{
    updateShadowNoteUnderMouse (e);

    // If we're entering while dragging, trigger the note under the pointer
    if (e.isAnyButtonDown())
        mouseDrag (e);
}

void MidiKeyboardComponent::mouseExit (const MouseEvent& e)
{
    // Leaving the component releases the note held by this pointer only
    updateNoteUnderMouse (e, false);
}

void MidiKeyboardComponent::mouseWheel (const MouseEvent& event, const MouseWheelData& wheel)
{
    const auto modifiers = event.getModifiers();

    // Ctrl/Cmd + wheel zooms in and out, clamped to a minimum span of a single
    // octave and a maximum span covering the whole available range.
    if (modifiers.isControlDown() || modifiers.isCommandDown())
    {
        auto zoomDelta = wheel.getDeltaY() != 0.0f ? wheel.getDeltaY() : wheel.getDeltaX();

        if (zoomDelta != 0.0f)
        {
            auto anchorNote = getNoteAtPosition (event.getPosition());

            if (anchorNote < 0)
                anchorNote = lowestVisibleKey + (highestVisibleKey - lowestVisibleKey) / 2;

            zoomBy (zoomDelta, anchorNote);
        }

        return;
    }

    // Plain wheel scrolls along the keyboard axis, one white key per notch.
    auto scrollDelta = (orientation == horizontalKeyboard) ? wheel.getDeltaX() : wheel.getDeltaY();

    if (scrollDelta == 0.0f)
        scrollDelta = (orientation == horizontalKeyboard) ? wheel.getDeltaY() : wheel.getDeltaX();

    if (scrollDelta != 0.0f)
        scrollByWhiteKeys (roundToInt (scrollDelta));
}

//==============================================================================
void MidiKeyboardComponent::handleNoteOn (MidiKeyboardState*, int midiChannelNumber, int, float)
{
    if (midiInChannelMask & (1 << (midiChannelNumber - 1)))
        triggerAsyncUpdate();
}

void MidiKeyboardComponent::handleNoteOff (MidiKeyboardState*, int midiChannelNumber, int, float)
{
    if (midiInChannelMask & (1 << (midiChannelNumber - 1)))
        triggerAsyncUpdate();
}

void MidiKeyboardComponent::handleAsyncUpdate()
{
    repaint();
}

//==============================================================================
void MidiKeyboardComponent::resized()
{
    updateScrollButtons();
}

void MidiKeyboardComponent::keyDown (const KeyPress& key, const Point<float>&)
{
    auto midiNote = getMidiNoteForKey (key);

    if (midiNote >= 0)
    {
        midiNote += 12 * keyPressBaseOctave;

        if (midiNote >= 0 && midiNote < 128 && ! keyDownNotes.contains (midiNote))
        {
            state.noteOn (midiChannel, midiNote, velocity);
            keyDownNotes.add (midiNote);
        }
    }
}

void MidiKeyboardComponent::keyUp (const KeyPress& key, const Point<float>&)
{
    auto midiNote = getMidiNoteForKey (key);

    if (midiNote >= 0)
    {
        midiNote += 12 * keyPressBaseOctave;

        if (keyDownNotes.removeFirstMatchingValue (midiNote) >= 0)
            state.noteOff (midiChannel, midiNote, 0.0f);
    }
}

void MidiKeyboardComponent::focusLost()
{
    // Pointer notes are released by their own mouse up: focus moves on every
    // finger down, so touching another component must not cut a held chord.
    releaseKeyPressNotes();
}

//==============================================================================
int MidiKeyboardComponent::getMidiNoteForKey (const KeyPress& key) const
{
    auto character = CharacterFunctions::toLowerCase (static_cast<yup_wchar> (key.getKey()));

    return keyboardKeys.indexOfChar (character);
}

void MidiKeyboardComponent::shiftVisibleRange (int semitones)
{
    const auto span = highestVisibleKey - lowestVisibleKey;
    const auto newStart = jlimit (rangeStart, rangeEnd - span, lowestVisibleKey + semitones);

    setVisibleRange (newStart, newStart + span);
}

void MidiKeyboardComponent::scrollByWhiteKeys (int numWhiteKeys)
{
    if (numWhiteKeys == 0)
        return;

    auto newStart = whiteKeyToNote (getNumWhiteKeysInRange (0, lowestVisibleKey) + numWhiteKeys);

    if (newStart < 0)
        newStart = (numWhiteKeys < 0) ? rangeStart : rangeEnd;

    shiftVisibleRange (newStart - lowestVisibleKey);
}

void MidiKeyboardComponent::zoomBy (float zoomDelta, int anchorNote)
{
    if (zoomDelta == 0.0f)
        return;

    const auto span = highestVisibleKey - lowestVisibleKey;
    const auto availableSpan = rangeEnd - rangeStart;

    if (span == 0)
        return;

    // Each wheel notch multiplies the span by a fixed factor, clamped so that
    // zooming in can never go below a single octave and zooming out can never
    // exceed the available range.
    auto newSpan = (zoomDelta > 0.0f) ? roundToInt (span / 1.25f)
                                      : roundToInt (span * 1.25f);

    newSpan = jlimit (jmin (12, availableSpan), availableSpan, newSpan);

    if (newSpan == span)
        return;

    // Keep the note under the mouse at the same relative position within the range.
    const auto anchorPos = (float) (anchorNote - lowestVisibleKey) / (float) span;
    const auto newStart = jlimit (rangeStart, rangeEnd - newSpan, roundToInt (anchorNote - anchorPos * newSpan));

    setVisibleRange (newStart, newStart + newSpan);
}

int MidiKeyboardComponent::whiteKeyToNote (int whiteKeyIndex) const
{
    auto count = 0;

    for (auto note = 0; note < 128; ++note)
    {
        if (! isBlackKey (note))
        {
            if (count == whiteKeyIndex)
                return note;

            ++count;
        }
    }

    return -1;
}

//==============================================================================
bool MidiKeyboardComponent::isNoteOn (int midiNoteNumber) const
{
    return state.isNoteOnForChannels (midiInChannelMask, midiNoteNumber);
}

//==============================================================================
bool MidiKeyboardComponent::isBlackKey (int midiNoteNumber) const
{
    return MidiMessage::isMidiNoteBlack (midiNoteNumber);
}

int MidiKeyboardComponent::getNumWhiteKeysInRange (int rangeStart, int rangeEnd) const
{
    int numWhiteKeys = 0;

    for (int i = rangeStart; i < rangeEnd; ++i)
        if (! isBlackKey (i))
            ++numWhiteKeys;

    return numWhiteKeys;
}

String MidiKeyboardComponent::getWhiteNoteText (int midiNoteNumber) const
{
    if (isBlackKey (midiNoteNumber))
        return {};

    return MidiMessage::getMidiNoteName (midiNoteNumber, true, midiNoteNumber % 12 == 0, octaveNumForMiddleC);
}

void MidiKeyboardComponent::getKeyPosition (int midiNoteNumber, float keyWidth, Rectangle<float>& keyPos, bool& isBlack) const
{
    jassert (midiNoteNumber >= 0 && midiNoteNumber < 128);

    isBlack = isBlackKey (midiNoteNumber);

    // Offset along the keyboard axis, from its low end. Black keys sit centered on the
    // boundary between their neighbours.
    auto x = getNumWhiteKeysInRange (lowestVisibleKey, midiNoteNumber) * keyWidth;
    auto w = keyWidth;

    if (isBlack)
    {
        w = keyWidth * blackNoteWidthProportion;
        x -= w * 0.5f;
    }

    // Keys hang from their back edge, so black keys are anchored there.
    const auto keyStart = getKeyStartRange();
    const auto depth = (orientation == horizontalKeyboard) ? getHeight() : getWidth();
    const auto length = isBlack ? depth * blackNoteLengthProportion : depth;

    switch (orientation)
    {
        case horizontalKeyboard:
            keyPos = Rectangle<float> (keyStart.getStart() + x, 0.0f, w, length);
            break;

        case verticalKeyboardFacingLeft:
            keyPos = Rectangle<float> (depth - length, keyStart.getStart() + x, length, w);
            break;

        case verticalKeyboardFacingRight:
            keyPos = Rectangle<float> (0.0f, keyStart.getEnd() - x - w, length, w);
            break;

        default:
            break;
    }
}

Range<float> MidiKeyboardComponent::getKeyStartRange() const
{
    const auto length = (orientation == horizontalKeyboard) ? getWidth() : getHeight();
    const auto buttonWidth = scrollDownButton.isVisible() ? jmin (scrollButtonWidth, length * 0.5f) : 0.0f;

    return { buttonWidth, length - buttonWidth };
}

float MidiKeyboardComponent::getKeyWidth() const
{
    return getKeyStartRange().getLength() / (float) jmax (1, getNumWhiteKeysInRange (lowestVisibleKey, highestVisibleKey + 1));
}

MidiKeyboardComponent::NoteAndVelocity MidiKeyboardComponent::getNoteAndVelocityAtPosition (Point<float> position) const
{
    // Black keys at the visible edges overhang the key area, keep them off the scroll buttons.
    if (! getKeyStartRange().contains ((orientation == horizontalKeyboard) ? position.getX() : position.getY()))
        return {};

    const auto keyWidth = getKeyWidth();

    // Black keys lie on top of the white ones, so they are hit first.
    for (const auto blackKeys : { true, false })
    {
        for (int note = lowestVisibleKey; note <= highestVisibleKey; ++note)
        {
            Rectangle<float> area;
            bool isBlack;
            getKeyPosition (note, keyWidth, area, isBlack);

            if (isBlack != blackKeys || ! area.contains (position))
                continue;

            float distanceFromBack = 0.0f;

            switch (orientation)
            {
                case horizontalKeyboard:
                    distanceFromBack = (position.getY() - area.getY()) / area.getHeight();
                    break;

                case verticalKeyboardFacingLeft:
                    distanceFromBack = (area.getRight() - position.getX()) / area.getWidth();
                    break;

                case verticalKeyboardFacingRight:
                    distanceFromBack = (position.getX() - area.getX()) / area.getWidth();
                    break;

                default:
                    break;
            }

            return { note, jlimit (0.0f, 1.0f, distanceFromBack) };
        }
    }

    return {};
}

void MidiKeyboardComponent::repaintNote (int midiNoteNumber)
{
    if (midiNoteNumber >= lowestVisibleKey && midiNoteNumber <= highestVisibleKey)
        repaint (getRectangleForKey (midiNoteNumber).roundToInt().enlarged (1)); // getSmallestIntegerContainer
}

void MidiKeyboardComponent::updateNoteUnderMouse (Point<float> pos, bool isDown, int pointerSlot)
{
    const auto noteInfo = getNoteAndVelocityAtPosition (pos);
    const auto newNote = noteInfo.note;
    const auto eventVelocity = useMousePositionForVelocity ? noteInfo.velocity * velocity : velocity;

    // Only the real mouse hovers: a lifted finger must not leave its last key tinted
    if (pointerSlot == 0 && newNote != mouseOverNote)
    {
        repaintNote (mouseOverNote);
        repaintNote (newNote);
        mouseOverNote = newNote;
    }

    const auto oldNoteDown = pointerDownNotes[(size_t) pointerSlot];
    const auto newNoteDown = isDown ? newNote : -1;

    if (oldNoteDown == newNoteDown)
        return;

    // A note shared by several pointers sounds once, and stops when the last one leaves it
    pointerDownNotes[(size_t) pointerSlot] = -1;

    if (oldNoteDown >= 0 && ! isNoteHeldByPointer (oldNoteDown))
        state.noteOff (midiChannel, oldNoteDown, eventVelocity);

    if (newNoteDown >= 0 && ! isNoteHeldByPointer (newNoteDown))
        state.noteOn (midiChannel, newNoteDown, eventVelocity);

    pointerDownNotes[(size_t) pointerSlot] = newNoteDown;
}

void MidiKeyboardComponent::updateNoteUnderMouse (const MouseEvent& e, bool isDown)
{
    // The mouse (touch index -1) takes slot 0, each finger the slots after it
    const auto pointerSlot = e.getTouchIndex() + 1;

    if (! isPositiveAndBelow (pointerSlot, maxPointers))
    {
        jassertfalse; // More simultaneous pointers than the keyboard can track
        return;
    }

    updateNoteUnderMouse (e.getPosition(), isDown, pointerSlot);
}

bool MidiKeyboardComponent::isNoteHeldByPointer (int midiNoteNumber) const
{
    return std::find (pointerDownNotes.begin(), pointerDownNotes.end(), midiNoteNumber) != pointerDownNotes.end();
}

void MidiKeyboardComponent::resetAnyKeysInUse()
{
    for (auto& noteDown : pointerDownNotes)
    {
        const auto note = std::exchange (noteDown, -1);

        if (note >= 0 && ! isNoteHeldByPointer (note))
            state.noteOff (midiChannel, note, velocity);
    }

    releaseKeyPressNotes();

    mouseOverNote = -1;
}

void MidiKeyboardComponent::releaseKeyPressNotes()
{
    for (auto noteDown : keyDownNotes)
        state.noteOff (midiChannel, noteDown, 0.0f);

    keyDownNotes.clear();
}

void MidiKeyboardComponent::updateShadowNoteUnderMouse (const MouseEvent& e)
{
    if (e.isTouch())
        return;

    auto note = getNoteAtPosition (e.getPosition());

    if (note != mouseOverNote)
    {
        repaintNote (mouseOverNote);
        mouseOverNote = note;
        repaintNote (mouseOverNote);
    }
}

void MidiKeyboardComponent::updateScrollButtons()
{
    const auto canScrollDown = lowestVisibleKey > rangeStart;
    const auto canScrollUp = highestVisibleKey < rangeEnd;
    const auto showButtons = scrollButtonsVisible && (canScrollDown || canScrollUp);

    scrollDownButton.setEnabled (canScrollDown);
    scrollUpButton.setEnabled (canScrollUp);
    scrollDownButton.setVisible (showButtons);
    scrollUpButton.setVisible (showButtons);

    if (! showButtons)
        return;

    auto bounds = getLocalBounds();

    if (orientation == horizontalKeyboard)
    {
        const auto buttonWidth = jmin (scrollButtonWidth, bounds.getWidth() * 0.5f);

        scrollDownButton.setBounds (bounds.removeFromLeft (buttonWidth));
        scrollUpButton.setBounds (bounds.removeFromRight (buttonWidth));
        return;
    }

    // Each button sits at the end of the keys it scrolls towards.
    const auto buttonHeight = jmin (scrollButtonWidth, bounds.getHeight() * 0.5f);
    const auto topButton = bounds.removeFromTop (buttonHeight);
    const auto bottomButton = bounds.removeFromBottom (buttonHeight);
    const auto lowNotesAtTop = orientation == verticalKeyboardFacingLeft;

    scrollDownButton.setBounds (lowNotesAtTop ? topButton : bottomButton);
    scrollUpButton.setBounds (lowNotesAtTop ? bottomButton : topButton);
}

} // namespace yup
