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

//==============================================================================
MidiKeyboardComponent::MidiKeyboardComponent (MidiKeyboardState& stateToUse, Orientation orientationToUse)
    : state (stateToUse)
    , orientation (orientationToUse)
{
    state.addListener (this);
    setWantsKeyboardFocus (true);
    //setMouseClickGrabsKeyboardFocus (true);

    octaveDownButton = std::make_unique<TextButton> ("-");
    octaveDownButton->setClickingGrabFocus (false);
    octaveDownButton->onClick = [this]
    {
        setOctaveForMiddleC (octaveNumForMiddleC - 1);
    };
    addAndMakeVisible (*octaveDownButton);

    octaveUpButton = std::make_unique<TextButton> ("+");
    octaveUpButton->setClickingGrabFocus (false);
    octaveUpButton->onClick = [this]
    {
        setOctaveForMiddleC (octaveNumForMiddleC + 1);
    };
    addAndMakeVisible (*octaveUpButton);

    octaveLabel = std::make_unique<Label> ("OctaveLabel");
    octaveLabel->setText (String (octaveNumForMiddleC), dontSendNotification);
    octaveLabel->setJustification (Justification::center);
    octaveLabel->setWantsMouseEvents (false, false); // clicks pass through to the buttons either side
    addAndMakeVisible (*octaveLabel);
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

void MidiKeyboardComponent::setOctaveForMiddleC (int octaveNumber)
{
    if (octaveNumForMiddleC != octaveNumber)
    {
        // Release any held notes so a key held across the octave change can't
        // get stuck (keyUp can no longer resolve it to the same note number).
        resetAnyKeysInUse();

        octaveNumForMiddleC = octaveNumber;
        octaveLabel->setText (String (octaveNumForMiddleC), dontSendNotification);
        repaint();
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

void MidiKeyboardComponent::setLowestVisibleKey (int noteNumber)
{
    setAvailableRange (noteNumber, rangeEnd);
}

void MidiKeyboardComponent::setAvailableRange (int lowestNote, int highestNote)
{
    jassert (isPositiveAndBelow (lowestNote, 128));
    jassert (isPositiveAndBelow (highestNote, 128));
    jassert (lowestNote <= highestNote);

    if (rangeStart != lowestNote || rangeEnd != highestNote)
    {
        rangeStart = jlimit (0, 127, lowestNote);
        rangeEnd = jlimit (0, 127, highestNote);
        repaint();
    }
}

//==============================================================================
Rectangle<float> MidiKeyboardComponent::getRectangleForKey (int midiNoteNumber) const
{
    jassert (midiNoteNumber >= 0 && midiNoteNumber < 128);

    if (midiNoteNumber < rangeStart || midiNoteNumber > rangeEnd)
        return {};

    auto keyWidth = getKeyStartRange().getLength() / getNumWhiteKeysInRange (rangeStart, rangeEnd + 1);
    Rectangle<float> pos;
    bool isBlack;

    getKeyPosition (midiNoteNumber, keyWidth, pos, isBlack);

    return pos;
}

int MidiKeyboardComponent::getNoteAtPosition (Point<float> position) const
{
    float mousePositionVelocity;
    return remappedXYToNote (position, mousePositionVelocity);
}

//==============================================================================
void MidiKeyboardComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================
void MidiKeyboardComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    updateNoteUnderMouse (e, true);
    shouldCheckState = true;
}

void MidiKeyboardComponent::mouseDrag (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    updateNoteUnderMouse (e, true);
}

void MidiKeyboardComponent::mouseUp (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    // Always release all notes that were triggered by mouse interaction
    for (auto noteDown : mouseDownNotes)
        state.noteOff (midiChannel, noteDown, velocity);

    mouseDownNotes.clear();

    // Update visual state to show keys are no longer pressed
    updateNoteUnderMouse (e, false);
    updateShadowNoteUnderMouse (e);
    shouldCheckState = true;
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

    // If we're entering while dragging, trigger the note under the mouse
    if (e.isAnyButtonDown())
    {
        updateNoteUnderMouse (e, true);
    }
}

void MidiKeyboardComponent::mouseExit (const MouseEvent& e)
{
    updateShadowNoteUnderMouse (e);

    // If we're dragging and leaving the component, release all notes
    if (e.isAnyButtonDown() && ! mouseDownNotes.isEmpty())
    {
        for (auto noteDown : mouseDownNotes)
            state.noteOff (midiChannel, noteDown, velocity);

        mouseDownNotes.clear();
    }
}

void MidiKeyboardComponent::mouseWheel (const MouseEvent& event, const MouseWheelData& wheel)
{
    const auto modifiers = event.getModifiers();

    // Ctrl/Cmd + wheel zooms in and out, clamped to a minimum span of a single
    // octave and a maximum span covering the whole 0-127 note range.
    if (modifiers.isControlDown() || modifiers.isCommandDown())
    {
        auto zoomDelta = wheel.getDeltaY() != 0.0f ? wheel.getDeltaY() : wheel.getDeltaX();

        if (zoomDelta != 0.0f)
        {
            auto anchorNote = getNoteAtPosition (event.getPosition());

            if (anchorNote < 0)
                anchorNote = rangeStart + (rangeEnd - rangeStart) / 2;

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
    shouldCheckState = true;

    constexpr float octaveButtonSize = 18.0f;
    constexpr float octaveLabelWidth = 22.0f;

    auto octaveSelectorBounds = Rectangle<float> (2.0f, 2.0f, octaveButtonSize * 2.0f + octaveLabelWidth, octaveButtonSize);
    octaveDownButton->setBounds (octaveSelectorBounds.removeFromLeft (octaveButtonSize));
    octaveLabel->setBounds (octaveSelectorBounds.removeFromLeft (octaveLabelWidth));
    octaveUpButton->setBounds (octaveSelectorBounds);
}

void MidiKeyboardComponent::keyDown (const KeyPress& key, const Point<float>&)
{
    auto midiNote = getMidiNoteForKey (key);

    if (midiNote >= 0)
    {
        midiNote += 12 * octaveNumForMiddleC;

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
        midiNote += 12 * octaveNumForMiddleC;

        if (keyDownNotes.removeFirstMatchingValue (midiNote) >= 0)
            state.noteOff (midiChannel, midiNote, 0.0f);
    }
}

void MidiKeyboardComponent::focusLost()
{
    resetAnyKeysInUse();
}

//==============================================================================
int MidiKeyboardComponent::getMidiNoteForKey (const KeyPress& key) const
{
    auto character = CharacterFunctions::toLowerCase (static_cast<yup_wchar> (key.getKey()));

    return keyboardKeys.indexOfChar (character);
}

void MidiKeyboardComponent::scrollByWhiteKeys (int numWhiteKeys)
{
    if (numWhiteKeys == 0)
        return;

    const auto span = rangeEnd - rangeStart;

    auto newStart = whiteKeyToNote (getNumWhiteKeysInRange (0, rangeStart) + numWhiteKeys);

    if (newStart < 0)
        return;

    auto newEnd = jmin (127, newStart + span);
    newStart = newEnd - span;

    if (newStart != rangeStart || newEnd != rangeEnd)
        setAvailableRange (newStart, newEnd);
}

void MidiKeyboardComponent::zoomBy (float zoomDelta, int anchorNote)
{
    if (zoomDelta == 0.0f)
        return;

    const auto span = rangeEnd - rangeStart;

    if (span == 0)
        return;

    // Each wheel notch multiplies the span by a fixed factor, clamped so that
    // zooming in can never go below a single octave and zooming out can never
    // exceed the full 0-127 note range.
    auto newSpan = (zoomDelta > 0.0f) ? roundToInt (span / 1.25f)
                                      : roundToInt (span * 1.25f);

    newSpan = jlimit (12, 127, newSpan);

    if (newSpan == span)
        return;

    // Keep the note under the mouse at the same relative position within the range.
    const auto anchorPos = (float) (anchorNote - rangeStart) / (float) span;

    auto newStart = roundToInt (anchorNote - anchorPos * newSpan);
    auto newEnd = newStart + newSpan;

    if (newStart < 0)
    {
        newStart = 0;
        newEnd = newSpan;
    }
    else if (newEnd > 127)
    {
        newEnd = 127;
        newStart = 127 - newSpan;
    }

    if (newStart != rangeStart || newEnd != rangeEnd)
        setAvailableRange (newStart, newEnd);
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

String MidiKeyboardComponent::getWhiteNoteText (int midiNoteNumber)
{
    if (isBlackKey (midiNoteNumber))
        return {};

    static const char* const noteNames[] = { "C", "", "D", "", "E", "F", "", "G", "", "A", "", "B" };

    return String (noteNames[midiNoteNumber % 12]);
}

void MidiKeyboardComponent::getKeyPosition (int midiNoteNumber, float keyWidth, Rectangle<float>& keyPos, bool& isBlack) const
{
    jassert (midiNoteNumber >= 0 && midiNoteNumber < 128);

    // Fixed black key offsets for proper positioning
    // static const float blackKeyOffsets[] = { 0.0f, 0.25f, 0.0f, 0.35f, 0.0f, 0.0f, 0.25f, 0.0f, 0.3f, 0.0f, 0.35f, 0.0f };
    static const float blackKeyOffsets[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    auto octave = midiNoteNumber / 12;
    auto note = midiNoteNumber % 12;

    auto numWhiteKeysBefore = 0;
    auto notePos = 0;

    for (int i = 0; i < note; ++i)
    {
        if (! isBlackKey (i))
            ++numWhiteKeysBefore;
    }

    for (int i = rangeStart; i < midiNoteNumber; ++i)
    {
        if (! isBlackKey (i))
            ++notePos;
    }

    isBlack = isBlackKey (midiNoteNumber);

    auto x = notePos * keyWidth;
    auto w = keyWidth;

    if (isBlack)
    {
        auto blackKeyWidth = keyWidth * 0.7f;
        x = x - (blackKeyWidth * 0.5f) + (keyWidth * blackKeyOffsets[note]);
        w = blackKeyWidth;
    }

    switch (orientation)
    {
        case horizontalKeyboard:
            keyPos = Rectangle<float> (x, 0.0f, w, (float) getHeight());
            break;

        case verticalKeyboardFacingLeft:
            keyPos = Rectangle<float> ((float) getWidth() - ((isBlack ? 0.7f : 1.0f) * (float) getWidth()),
                                       x,
                                       (isBlack ? 0.7f : 1.0f) * (float) getWidth(),
                                       w);
            break;

        case verticalKeyboardFacingRight:
            keyPos = Rectangle<float> (0.0f, (float) getHeight() - x - w, (isBlack ? 0.7f : 1.0f) * (float) getWidth(), w);
            break;

        default:
            break;
    }

    if (isBlack)
    {
        switch (orientation)
        {
            case horizontalKeyboard:
                keyPos = keyPos.withHeight (keyPos.getHeight() * 0.6f);
                break;
            case verticalKeyboardFacingLeft:
                keyPos = keyPos.withWidth (keyPos.getWidth() * 0.6f);
                break;
            case verticalKeyboardFacingRight:
                keyPos = keyPos.withX (keyPos.getX() + keyPos.getWidth() * 0.4f)
                             .withWidth (keyPos.getWidth() * 0.6f);
                break;
            default:
                break;
        }
    }
}

Range<float> MidiKeyboardComponent::getKeyStartRange() const
{
    return (orientation == horizontalKeyboard) ? Range<float> (0.0f, (float) getWidth())
                                               : Range<float> (0.0f, (float) getHeight());
}

int MidiKeyboardComponent::xyToNote (Point<float> pos, float& mousePositionVelocity)
{
    return remappedXYToNote (pos, mousePositionVelocity);
}

int MidiKeyboardComponent::remappedXYToNote (Point<float> pos, float& mousePositionVelocity) const
{
    auto keyWidth = getKeyStartRange().getLength() / getNumWhiteKeysInRange (rangeStart, rangeEnd + 1);

    auto coord = (orientation == horizontalKeyboard) ? pos.getX() : pos.getY();
    auto otherCoord = (orientation == horizontalKeyboard) ? pos.getY() : pos.getX();

    auto blackKeyDepth = 0.7f;

    switch (orientation)
    {
        case horizontalKeyboard:
            blackKeyDepth = getHeight() * 0.6f;
            break;
        case verticalKeyboardFacingLeft:
            blackKeyDepth = getWidth() * 0.6f;
            break;
        case verticalKeyboardFacingRight:
            blackKeyDepth = getWidth() * 0.6f;
            break;
        default:
            break;
    }

    // First try black keys
    for (int note = rangeStart; note <= rangeEnd; ++note)
    {
        if (isBlackKey (note))
        {
            Rectangle<float> area;
            bool isBlack;
            getKeyPosition (note, keyWidth, area, isBlack);

            if (area.contains (pos))
            {
                mousePositionVelocity = jlimit (0.0f, 1.0f, otherCoord / area.getHeight());
                return note;
            }
        }
    }

    // Then try white keys
    for (int note = rangeStart; note <= rangeEnd; ++note)
    {
        if (! isBlackKey (note))
        {
            Rectangle<float> area;
            bool isBlack;
            getKeyPosition (note, keyWidth, area, isBlack);

            if (area.contains (pos))
            {
                mousePositionVelocity = jlimit (0.0f, 1.0f, otherCoord / area.getHeight());
                return note;
            }
        }
    }

    mousePositionVelocity = velocity;
    return -1;
}

void MidiKeyboardComponent::repaintNote (int midiNoteNumber)
{
    if (midiNoteNumber >= rangeStart && midiNoteNumber <= rangeEnd)
        repaint (getRectangleForKey (midiNoteNumber).roundToInt().enlarged (1)); // getSmallestIntegerContainer
}

void MidiKeyboardComponent::updateNoteUnderMouse (Point<float> pos, bool isDown, int fingerNum)
{
    float mousePositionVelocity;
    auto newNote = xyToNote (pos, mousePositionVelocity);
    auto oldNote = mouseOverNote;

    // Always update hover visual state when the note under mouse changes
    if (oldNote != newNote)
    {
        repaintNote (oldNote);
        repaintNote (newNote);
        mouseOverNote = newNote;
    }

    if (isDown)
    {
        // Handle note triggering - this should work regardless of hover state

        // First, release any previously pressed notes that are no longer under the mouse
        for (int i = mouseDownNotes.size(); --i >= 0;)
        {
            auto pressedNote = mouseDownNotes.getUnchecked (i);
            if (pressedNote != newNote)
            {
                state.noteOff (midiChannel, pressedNote, mousePositionVelocity);
                mouseDownNotes.remove (i);
            }
        }

        // Then, trigger the new note if it's valid and not already pressed
        if (newNote >= 0 && ! mouseDownNotes.contains (newNote))
        {
            state.noteOn (midiChannel, newNote, mousePositionVelocity);
            mouseDownNotes.add (newNote);
        }
    }
}

void MidiKeyboardComponent::updateNoteUnderMouse (const MouseEvent& e, bool isDown)
{
    updateNoteUnderMouse (e.getPosition(), isDown, 0);
}

void MidiKeyboardComponent::resetAnyKeysInUse()
{
    if (! mouseDownNotes.isEmpty())
    {
        for (auto noteDown : mouseDownNotes)
            state.noteOff (midiChannel, noteDown, velocity);

        mouseDownNotes.clear();
    }

    if (! keyDownNotes.isEmpty())
    {
        for (auto noteDown : keyDownNotes)
            state.noteOff (midiChannel, noteDown, 0.0f);

        keyDownNotes.clear();
    }

    mouseOverNote = -1;
}

void MidiKeyboardComponent::updateShadowNoteUnderMouse (const MouseEvent& e)
{
    auto note = getNoteAtPosition (e.getPosition());

    if (note != mouseOverNote)
    {
        repaintNote (mouseOverNote);
        mouseOverNote = note;
        repaintNote (mouseOverNote);
    }
}

} // namespace yup
