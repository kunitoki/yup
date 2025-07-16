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

    updateNoteUnderMouse (e, false);

    for (int i = mouseDownNotes.size(); --i >= 0;)
    {
        const int noteDown = mouseDownNotes.getUnchecked (i);

        if (mouseDraggedToKey (noteDown, e))
        {
            mouseDownNotes.remove (i);
        }
        else
        {
            state.noteOff (midiChannel, noteDown, velocity);
            mouseDownNotes.remove (i);
        }
    }

    updateShadowNoteUnderMouse (e);
    shouldCheckState = true;
}

void MidiKeyboardComponent::mouseEnter (const MouseEvent& e)
{
    updateShadowNoteUnderMouse (e);
}

void MidiKeyboardComponent::mouseExit (const MouseEvent& e)
{
    updateShadowNoteUnderMouse (e);
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

void MidiKeyboardComponent::keyDown (const KeyPress& key, const Point<float>& position)
{
    int midiNote = -1;

    if (key == KeyPress ('z')) midiNote = 0;
    else if (key == KeyPress ('s')) midiNote = 1;
    else if (key == KeyPress ('x')) midiNote = 2;
    else if (key == KeyPress ('d')) midiNote = 3;
    else if (key == KeyPress ('c')) midiNote = 4;
    else if (key == KeyPress ('v')) midiNote = 5;
    else if (key == KeyPress ('g')) midiNote = 6;
    else if (key == KeyPress ('b')) midiNote = 7;
    else if (key == KeyPress ('h')) midiNote = 8;
    else if (key == KeyPress ('n')) midiNote = 9;
    else if (key == KeyPress ('j')) midiNote = 10;
    else if (key == KeyPress ('m')) midiNote = 11;
    else if (key == KeyPress (',')) midiNote = 12;
    else if (key == KeyPress ('l')) midiNote = 13;
    else if (key == KeyPress ('.')) midiNote = 14;
    else if (key == KeyPress (';')) midiNote = 15;
    else if (key == KeyPress ('/')) midiNote = 16;

    if (midiNote >= 0)
    {
        midiNote += 12 * octaveNumForMiddleC;

        if (midiNote >= 0 && midiNote < 128)
            state.noteOn (midiChannel, midiNote, velocity);
    }
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

    static const float blackKeyOffsets[] = { 0.0f, 0.6f, 0.0f, 0.7f, 0.0f, 0.0f, 0.6f, 0.0f, 0.65f, 0.0f, 0.7f, 0.0f };

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

    if (oldNote != newNote)
    {
        repaintNote (oldNote);
        repaintNote (newNote);
        mouseOverNote = newNote;
    }

    if (isDown)
    {
        if (newNote != oldNote)
        {
            if (oldNote >= 0)
            {
                mouseDownNotes.removeFirstMatchingValue (oldNote);
                state.noteOff (midiChannel, oldNote, mousePositionVelocity);
            }

            if (newNote >= 0 && ! mouseDownNotes.contains (newNote))
            {
                state.noteOn (midiChannel, newNote, mousePositionVelocity);
                mouseDownNotes.add (newNote);
            }
        }
    }
}

void MidiKeyboardComponent::updateNoteUnderMouse (const MouseEvent& e, bool isDown)
{
    updateNoteUnderMouse (e.getPosition(), isDown, 0);
}

bool MidiKeyboardComponent::mouseDraggedToKey (int midiNoteNumber, const MouseEvent& e)
{
    jassert (midiNoteNumber >= 0 && midiNoteNumber < 128);

    return getRectangleForKey (midiNoteNumber).contains (e.getPosition());
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
