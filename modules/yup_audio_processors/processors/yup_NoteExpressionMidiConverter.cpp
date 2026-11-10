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

NoteExpressionMidiConverter::NoteExpressionMidiConverter (Mode mode)
    : mode (mode)
    , pitchBendRange (mode == Mode::mpe ? 48.0f : 2.0f)
{
    activeNotes.reserve (128);
}

void NoteExpressionMidiConverter::setMode (Mode newMode)
{
    mode = newMode;
    pitchBendRange = (mode == Mode::mpe ? 48.0f : 2.0f);
    reset();
}

void NoteExpressionMidiConverter::setPitchBendRange (float semitones) noexcept
{
    pitchBendRange = jmax (0.01f, semitones);
}

void NoteExpressionMidiConverter::prepare (int maxMidiBytesPerBlock)
{
    scratch.ensureSize (static_cast<size_t> (jmax (0, maxMidiBytesPerBlock)));
}

void NoteExpressionMidiConverter::reset() noexcept
{
    activeNotes.clear();
    notesPerChannel.fill (0);
    nextMemberChannel = 2;
    needsMpeConfiguration = true;
}

//==============================================================================

void NoteExpressionMidiConverter::process (MidiBuffer& midi, const NoteExpressionBuffer& expressions)
{
    const bool remapsMidi = (mode == Mode::mpe);

    if (expressions.isEmpty() && ! remapsMidi)
        return;

    scratch.clear();

    if (remapsMidi && needsMpeConfiguration)
    {
        // MPE configuration message: lower zone with 15 member channels.
        scratch.addEvent (MidiMessage::controllerEvent (1, 101, 0), 0);
        scratch.addEvent (MidiMessage::controllerEvent (1, 100, 6), 0);
        scratch.addEvent (MidiMessage::controllerEvent (1, 6, 15), 0);
        needsMpeConfiguration = false;
    }

    auto expression = expressions.begin();

    for (const auto metadata : midi)
    {
        for (; expression != expressions.end() && expression->sampleOffset < metadata.samplePosition; ++expression)
            processExpression (*expression);

        if (remapsMidi)
            processMidi (metadata.getMessage(), metadata.samplePosition);
        else
            scratch.addEvent (metadata.data, metadata.numBytes, metadata.samplePosition);
    }

    for (; expression != expressions.end(); ++expression)
        processExpression (*expression);

    midi.swapWith (scratch);
}

//==============================================================================

void NoteExpressionMidiConverter::processMidi (const MidiMessage& message, int samplePosition)
{
    auto remapped = message;

    if (message.isNoteOn())
    {
        const auto memberChannel = allocateMemberChannel();

        if (activeNotes.size() < activeNotes.capacity())
        {
            activeNotes.push_back ({ message.getChannel(), message.getNoteNumber(), memberChannel });
            ++notesPerChannel[static_cast<size_t> (memberChannel)];
        }

        remapped.setChannel (memberChannel);
    }
    else if (message.isNoteOff())
    {
        const auto memberChannel = findMemberChannel (message.getChannel(), message.getNoteNumber());
        remapped.setChannel (memberChannel > 0 ? memberChannel : 1);
        releaseNote (message.getChannel(), message.getNoteNumber());
    }
    else if (message.getChannel() > 0)
    {
        remapped.setChannel (1);
    }

    scratch.addEvent (remapped, samplePosition);
}

void NoteExpressionMidiConverter::processExpression (const NoteExpression& expression)
{
    if (mode == Mode::channelWide)
    {
        addExpressionMessages (expression, expression.channel > 0 ? expression.channel : 1);
        return;
    }

    for (const auto& note : activeNotes)
    {
        if ((expression.channel <= 0 || expression.channel == note.sourceChannel)
            && (expression.key < 0 || expression.key == note.key))
        {
            addExpressionMessages (expression, note.memberChannel);
        }
    }
}

void NoteExpressionMidiConverter::addExpressionMessages (const NoteExpression& expression, int channel)
{
    const auto toMidi7 = [] (float value)
    {
        return jlimit (0, 127, roundToInt (value * 127.0f));
    };

    switch (expression.type)
    {
        case NoteExpression::Type::tuning:
        {
            const auto bend = 8192.0f + (expression.value / pitchBendRange) * 8192.0f;
            scratch.addEvent (MidiMessage::pitchWheel (channel, jlimit (0, 16383, roundToInt (bend))), expression.sampleOffset);
            break;
        }

        case NoteExpression::Type::pressure:
            scratch.addEvent (MidiMessage::channelPressureChange (channel, toMidi7 (expression.value)), expression.sampleOffset);
            break;

        case NoteExpression::Type::brightness:
            scratch.addEvent (MidiMessage::controllerEvent (channel, 74, toMidi7 (expression.value)), expression.sampleOffset);
            break;

        case NoteExpression::Type::volume:
        case NoteExpression::Type::pan:
        case NoteExpression::Type::vibrato:
        case NoteExpression::Type::expression:
            break;
    }
}

//==============================================================================

int NoteExpressionMidiConverter::allocateMemberChannel() noexcept
{
    // Prefer the next free member channel in round-robin order, otherwise share the least busy one.
    int bestChannel = nextMemberChannel;

    for (int offset = 0; offset < 15; ++offset)
    {
        const auto channel = 2 + (nextMemberChannel - 2 + offset) % 15;

        if (notesPerChannel[static_cast<size_t> (channel)] == 0)
        {
            bestChannel = channel;
            break;
        }

        if (notesPerChannel[static_cast<size_t> (channel)] < notesPerChannel[static_cast<size_t> (bestChannel)])
            bestChannel = channel;
    }

    nextMemberChannel = 2 + (bestChannel - 1) % 15;
    return bestChannel;
}

int NoteExpressionMidiConverter::findMemberChannel (int sourceChannel, int key) const noexcept
{
    for (const auto& note : activeNotes)
    {
        if (note.sourceChannel == sourceChannel && note.key == key)
            return note.memberChannel;
    }

    return 0;
}

void NoteExpressionMidiConverter::releaseNote (int sourceChannel, int key) noexcept
{
    for (auto it = activeNotes.begin(); it != activeNotes.end(); ++it)
    {
        if (it->sourceChannel == sourceChannel && it->key == key)
        {
            --notesPerChannel[static_cast<size_t> (it->memberChannel)];
            activeNotes.erase (it);
            return;
        }
    }
}

} // namespace yup
