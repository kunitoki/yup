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
const Identifier MidiKeyboardComponent::Style::whiteKeyColorId          ("midiKeyboardWhiteKey");
const Identifier MidiKeyboardComponent::Style::whiteKeyPressedColorId   ("midiKeyboardWhiteKeyPressed");
const Identifier MidiKeyboardComponent::Style::whiteKeyShadowColorId    ("midiKeyboardWhiteKeyShadow");
const Identifier MidiKeyboardComponent::Style::blackKeyColorId          ("midiKeyboardBlackKey");
const Identifier MidiKeyboardComponent::Style::blackKeyPressedColorId   ("midiKeyboardBlackKeyPressed");
const Identifier MidiKeyboardComponent::Style::blackKeyShadowColorId    ("midiKeyboardBlackKeyShadow");
const Identifier MidiKeyboardComponent::Style::keyOutlineColorId        ("midiKeyboardKeyOutline");

//==============================================================================
MidiKeyboardComponent::MidiKeyboardComponent (MidiKeyboardState& stateToUse, Orientation orientationToUse)
    : state (stateToUse),
      orientation (orientationToUse)
{
    state.addListener (this);
    setWantsKeyboardFocus (true);
    //setMouseClickGrabsKeyboardFocus (true);
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
    octaveNumForMiddleC = octaveNumber;
    repaint();
}

void MidiKeyboardComponent::setLowestVisibleKey (int noteNumber)
{
    setAvailableRange (noteNumber, rangeEnd);
}

void MidiKeyboardComponent::setAvailableRange (int lowestNote, int highestNote)
{
    jassert (lowestNote >= 0 && lowestNote <= 127);
    jassert (highestNote >= 0 && highestNote <= 127);
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

    takeKeyboardFocus();

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

void MidiKeyboardComponent::mouseWheel (const MouseEvent&, const MouseWheelData& wheel)
{
    const auto amount = (orientation == horizontalKeyboard && wheel.getDeltaX() != 0)
        ? wheel.getDeltaX()
        : (orientation != horizontalKeyboard && wheel.getDeltaY() != 0)
            ? wheel.getDeltaY() : wheel.getDeltaX();

    setLowestVisibleKey (rangeStart + roundToInt (amount * 5.0f));
}

//==============================================================================
void MidiKeyboardComponent::handleNoteOn (MidiKeyboardState*, int midiChannelNumber, int midiNoteNumber, float)
{
    if (midiInChannelMask & (1 << (midiChannelNumber - 1)))
        repaintNote (midiNoteNumber);
}

void MidiKeyboardComponent::handleNoteOff (MidiKeyboardState*, int midiChannelNumber, int midiNoteNumber, float)
{
    if (midiInChannelMask & (1 << (midiChannelNumber - 1)))
        repaintNote (midiNoteNumber);
}

//==============================================================================
void MidiKeyboardComponent::resized()
{
    shouldCheckState = true;
}

int MidiKeyboardComponent::getNoteForKeyPress (const KeyPress& key) const
{
    // Compares key codes only: KeyPress::operator== also compares the
    // scancode, which is always zero on a literal like KeyPress ('z') but set
    // on real events, so full KeyPress comparisons can never match here.
    int midiNote = -1;

    switch (key.getKey())
    {
        case 'z': midiNote = 0; break;
        case 's': midiNote = 1; break;
        case 'x': midiNote = 2; break;
        case 'd': midiNote = 3; break;
        case 'c': midiNote = 4; break;
        case 'v': midiNote = 5; break;
        case 'g': midiNote = 6; break;
        case 'b': midiNote = 7; break;
        case 'h': midiNote = 8; break;
        case 'n': midiNote = 9; break;
        case 'j': midiNote = 10; break;
        case 'm': midiNote = 11; break;
        case ',': midiNote = 12; break;
        case 'l': midiNote = 13; break;
        case '.': midiNote = 14; break;
        case ';': midiNote = 15; break;
        case '/': midiNote = 16; break;
        default: break;
    }

    if (midiNote < 0)
        return -1;

    midiNote += 12 * octaveNumForMiddleC;

    return midiNote >= 0 && midiNote < 128 ? midiNote : -1;
}

void MidiKeyboardComponent::keyDown (const KeyPress& key, const Point<float>& position)
{
    const auto midiNote = getNoteForKeyPress (key);

    // Ignore held-key auto-repeat: the note is already sounding.
    if (midiNote >= 0 && ! state.isNoteOn (midiChannel, midiNote))
        state.noteOn (midiChannel, midiNote, velocity);
}

void MidiKeyboardComponent::keyUp (const KeyPress& key, const Point<float>& position)
{
    const auto midiNote = getNoteForKeyPress (key);

    if (midiNote >= 0)
        state.noteOff (midiChannel, midiNote, 0.0f);
}

void MidiKeyboardComponent::focusLost()
{
    resetAnyKeysInUse();
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

    return String (noteNames [midiNoteNumber % 12]);
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
                                       x, (isBlack ? 0.7f : 1.0f) * (float) getWidth(), w);
            break;

        case verticalKeyboardFacingRight:
            keyPos = Rectangle<float> (0.0f, (float) getHeight() - x - w,
                                       (isBlack ? 0.7f : 1.0f) * (float) getWidth(), w);
            break;

        default:
            break;
    }

    if (isBlack)
    {
        switch (orientation)
        {
            case horizontalKeyboard:           keyPos = keyPos.withHeight (keyPos.getHeight() * 0.6f); break;
            case verticalKeyboardFacingLeft:   keyPos = keyPos.withWidth (keyPos.getWidth() * 0.6f); break;
            case verticalKeyboardFacingRight:  keyPos = keyPos.withX (keyPos.getX() + keyPos.getWidth() * 0.4f)
                                                              .withWidth (keyPos.getWidth() * 0.6f); break;
            default: break;
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
        case horizontalKeyboard:           blackKeyDepth = getHeight() * 0.6f; break;
        case verticalKeyboardFacingLeft:   blackKeyDepth = getWidth() * 0.6f; break;
        case verticalKeyboardFacingRight:  blackKeyDepth = getWidth() * 0.6f; break;
        default: break;
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
