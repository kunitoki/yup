/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

MPEChannelAssigner::MPEChannelAssigner (MPEZoneLayout::Zone zoneToUse)
    : zone (new MPEZoneLayout::Zone (zoneToUse))
    , channelIncrement (zone->isLowerZone() ? 1 : -1)
    , numChannels (zone->numMemberChannels)
    , firstChannel (zone->getFirstMemberChannel())
    , lastChannel (zone->getLastMemberChannel())
    , midiChannelLastAssigned (firstChannel - channelIncrement)
{
    // must be an active MPE zone!
    jassert (numChannels > 0);
}

MPEChannelAssigner::MPEChannelAssigner (Range<int> channelRange)
    : isLegacy (true)
    , channelIncrement (1)
    , numChannels (channelRange.getLength())
    , firstChannel (channelRange.getStart())
    , lastChannel (channelRange.getEnd() - 1)
    , midiChannelLastAssigned (firstChannel - channelIncrement)
{
    // must have at least one channel!
    jassert (! channelRange.isEmpty());
}

int MPEChannelAssigner::findMidiChannelForNewNote (int noteNumber) noexcept
{
    if (numChannels <= 1)
        return firstChannel;

    for (int ch = firstChannel; (isLegacy || zone->isLowerZone() ? ch <= lastChannel : ch >= lastChannel); ch += channelIncrement)
    {
        if (midiChannels[(size_t) ch].isFree() && midiChannels[(size_t) ch].lastNotePlayed == noteNumber)
        {
            midiChannelLastAssigned = ch;
            midiChannels[(size_t) ch].notes.add (noteNumber);
            return ch;
        }
    }

    for (int ch = midiChannelLastAssigned + channelIncrement;; ch += channelIncrement)
    {
        if (ch == lastChannel + channelIncrement) // loop wrap-around
            ch = firstChannel;

        if (midiChannels[(size_t) ch].isFree())
        {
            midiChannelLastAssigned = ch;
            midiChannels[(size_t) ch].notes.add (noteNumber);
            return ch;
        }

        if (ch == midiChannelLastAssigned)
            break; // no free channels!
    }

    midiChannelLastAssigned = findMidiChannelPlayingClosestNonequalNote (noteNumber);
    midiChannels[(size_t) midiChannelLastAssigned].notes.add (noteNumber);

    return midiChannelLastAssigned;
}

int MPEChannelAssigner::findMidiChannelForExistingNote (int noteNumber) noexcept
{
    const auto iter = std::find_if (midiChannels.cbegin(), midiChannels.cend(), [&] (auto& ch)
    {
        return std::find (ch.notes.begin(), ch.notes.end(), noteNumber) != ch.notes.end();
    });

    return iter != midiChannels.cend() ? (int) std::distance (midiChannels.cbegin(), iter) : -1;
}

void MPEChannelAssigner::noteOff (int noteNumber, int midiChannel)
{
    const auto removeNote = [] (MidiChannel& ch, int noteNum)
    {
        if (ch.notes.removeAllInstancesOf (noteNum) > 0)
        {
            ch.lastNotePlayed = noteNum;
            return true;
        }

        return false;
    };

    if (midiChannel >= 0 && midiChannel <= 16)
    {
        removeNote (midiChannels[(size_t) midiChannel], noteNumber);
        return;
    }

    for (auto& ch : midiChannels)
    {
        if (removeNote (ch, noteNumber))
            return;
    }
}

void MPEChannelAssigner::allNotesOff()
{
    for (auto& ch : midiChannels)
    {
        if (ch.notes.size() > 0)
            ch.lastNotePlayed = ch.notes.getLast();

        ch.notes.clear();
    }
}

int MPEChannelAssigner::findMidiChannelPlayingClosestNonequalNote (int noteNumber) noexcept
{
    auto channelWithClosestNote = firstChannel;
    int closestNoteDistance = 127;

    for (int ch = firstChannel; (isLegacy || zone->isLowerZone() ? ch <= lastChannel : ch >= lastChannel); ch += channelIncrement)
    {
        for (auto note : midiChannels[(size_t) ch].notes)
        {
            auto noteDistance = std::abs (note - noteNumber);

            if (noteDistance > 0 && noteDistance < closestNoteDistance)
            {
                closestNoteDistance = noteDistance;
                channelWithClosestNote = ch;
            }
        }
    }

    return channelWithClosestNote;
}

//==============================================================================
MPEChannelRemapper::MPEChannelRemapper (MPEZoneLayout::Zone zoneToRemap)
    : zone (zoneToRemap)
    , channelIncrement (zone.isLowerZone() ? 1 : -1)
    , firstChannel (zone.getFirstMemberChannel())
    , lastChannel (zone.getLastMemberChannel())
{
    // must be an active MPE zone!
    jassert (zone.numMemberChannels > 0);
    zeroArrays();
}

void MPEChannelRemapper::remapMidiChannelIfNeeded (MidiMessage& message, uint32 mpeSourceID) noexcept
{
    auto channel = message.getChannel();

    if (! zone.isUsingChannelAsMemberChannel (channel))
        return;

    if (channel == zone.getMasterChannel() && (message.isResetAllControllers() || message.isAllNotesOff()))
    {
        clearSource (mpeSourceID);
        return;
    }

    auto sourceAndChannelID = (((uint32) mpeSourceID << 5) | (uint32) (channel));

    if (messageIsNoteData (message))
    {
        ++counter;

        // fast path - no remap
        if (applyRemapIfExisting (channel, sourceAndChannelID, message))
            return;

        // find existing remap
        for (int chan = firstChannel; (zone.isLowerZone() ? chan <= lastChannel : chan >= lastChannel); chan += channelIncrement)
            if (applyRemapIfExisting (chan, sourceAndChannelID, message))
                return;

        // no remap necessary
        if (sourceAndChannel[channel] == notMPE)
        {
            lastUsed[channel] = counter;
            sourceAndChannel[channel] = sourceAndChannelID;
            return;
        }

        // remap source & channel to new channel
        auto chan = getBestChanToReuse();

        sourceAndChannel[chan] = sourceAndChannelID;
        lastUsed[chan] = counter;
        message.setChannel (chan);
    }
}

void MPEChannelRemapper::reset() noexcept
{
    for (auto& s : sourceAndChannel)
        s = notMPE;
}

void MPEChannelRemapper::clearChannel (int channel) noexcept
{
    sourceAndChannel[channel] = notMPE;
}

void MPEChannelRemapper::clearSource (uint32 mpeSourceID)
{
    for (auto& s : sourceAndChannel)
    {
        if (uint32 (s >> 5) == mpeSourceID)
        {
            s = notMPE;
            return;
        }
    }
}

bool MPEChannelRemapper::applyRemapIfExisting (int channel, uint32 sourceAndChannelID, MidiMessage& m) noexcept
{
    if (sourceAndChannel[channel] == sourceAndChannelID)
    {
        if (m.isNoteOff())
            sourceAndChannel[channel] = notMPE;
        else
            lastUsed[channel] = counter;

        m.setChannel (channel);
        return true;
    }

    return false;
}

int MPEChannelRemapper::getBestChanToReuse() const noexcept
{
    for (int chan = firstChannel; (zone.isLowerZone() ? chan <= lastChannel : chan >= lastChannel); chan += channelIncrement)
        if (sourceAndChannel[chan] == notMPE)
            return chan;

    auto bestChan = firstChannel;
    auto bestLastUse = counter;

    for (int chan = firstChannel; (zone.isLowerZone() ? chan <= lastChannel : chan >= lastChannel); chan += channelIncrement)
    {
        if (lastUsed[chan] < bestLastUse)
        {
            bestLastUse = lastUsed[chan];
            bestChan = chan;
        }
    }

    return bestChan;
}

void MPEChannelRemapper::zeroArrays()
{
    for (int i = 0; i < 17; ++i)
    {
        sourceAndChannel[i] = 0;
        lastUsed[i] = 0;
    }
}

} // namespace yup
