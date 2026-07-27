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

namespace MidiFileHelpers
{
static void writeVariableLengthInt (OutputStream& out, uint32 v)
{
    auto buffer = v & 0x7f;

    while ((v >>= 7) != 0)
    {
        buffer <<= 8;
        buffer |= ((v & 0x7f) | 0x80);
    }

    for (;;)
    {
        out.writeByte ((char) buffer);

        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }
}

template <typename Integral>
struct ReadTrait;

template <>
struct ReadTrait<uint32>
{
    static constexpr auto read = ByteOrder::bigEndianInt;
};

template <>
struct ReadTrait<uint16>
{
    static constexpr auto read = ByteOrder::bigEndianShort;
};

template <typename Integral>
std::optional<Integral> tryRead (const uint8*& data, size_t& remaining)
{
    using Trait = ReadTrait<Integral>;
    constexpr auto size = sizeof (Integral);

    if (remaining < size)
        return {};

    const std::optional<Integral> result { Trait::read (data) };

    data += size;
    remaining -= size;

    return result;
}

struct HeaderDetails
{
    size_t bytesRead = 0;
    short timeFormat = 0;
    short fileType = 0;
    short numberOfTracks = 0;
};

static std::optional<HeaderDetails> parseMidiHeader (const uint8* const initialData,
                                                     const size_t maxSize)
{
    auto* data = initialData;
    auto remaining = maxSize;

    auto ch = tryRead<uint32> (data, remaining);

    if (! ch.has_value())
        return {};

    if (*ch != ByteOrder::bigEndianInt ("MThd"))
    {
        auto ok = false;

        if (*ch == ByteOrder::bigEndianInt ("RIFF"))
        {
            for (int i = 0; i < 8; ++i)
            {
                ch = tryRead<uint32> (data, remaining);

                if (! ch.has_value())
                    return {};

                if (*ch == ByteOrder::bigEndianInt ("MThd"))
                {
                    ok = true;
                    break;
                }
            }
        }

        if (! ok)
            return {};
    }

    const auto bytesRemaining = tryRead<uint32> (data, remaining);

    if (! bytesRemaining.has_value() || *bytesRemaining > remaining)
        return {};

    const auto optFileType = tryRead<uint16> (data, remaining);

    if (! optFileType.has_value() || 2 < *optFileType)
        return {};

    const auto optNumTracks = tryRead<uint16> (data, remaining);

    if (! optNumTracks.has_value() || (*optFileType == 0 && *optNumTracks != 1))
        return {};

    const auto optTimeFormat = tryRead<uint16> (data, remaining);

    if (! optTimeFormat.has_value())
        return {};

    HeaderDetails result;

    result.fileType = (short) *optFileType;
    result.timeFormat = (short) *optTimeFormat;
    result.numberOfTracks = (short) *optNumTracks;
    result.bytesRead = maxSize - remaining;

    return { result };
}

static double convertTicksToSeconds (double time,
                                     const MidiMessageSequence& tempoEvents,
                                     int timeFormat)
{
    if (timeFormat < 0)
        return time / (-(timeFormat >> 8) * (timeFormat & 0xff));

    double lastTime = 0, correctedTime = 0;
    auto tickLen = 1.0 / (timeFormat & 0x7fff);
    auto secsPerTick = 0.5 * tickLen;
    auto numEvents = tempoEvents.getNumEvents();

    for (int i = 0; i < numEvents; ++i)
    {
        auto& m = tempoEvents.getEventPointer (i)->message;
        auto eventTime = m.getTimeStamp();

        if (eventTime >= time)
            break;

        correctedTime += (eventTime - lastTime) * secsPerTick;
        lastTime = eventTime;

        if (m.isTempoMetaEvent())
            secsPerTick = tickLen * m.getTempoSecondsPerQuarterNote();

        while (i + 1 < numEvents)
        {
            auto& m2 = tempoEvents.getEventPointer (i + 1)->message;

            if (! approximatelyEqual (m2.getTimeStamp(), eventTime))
                break;

            if (m2.isTempoMetaEvent())
                secsPerTick = tickLen * m2.getTempoSecondsPerQuarterNote();

            ++i;
        }
    }

    return correctedTime + (time - lastTime) * secsPerTick;
}

template <typename MethodType>
static void findAllMatchingEvents (const OwnedArray<MidiMessageSequence>& tracks,
                                   MidiMessageSequence& results,
                                   MethodType method)
{
    for (auto* track : tracks)
    {
        auto numEvents = track->getNumEvents();

        for (int j = 0; j < numEvents; ++j)
        {
            auto& m = track->getEventPointer (j)->message;

            if ((m.*method)())
                results.addEvent (m);
        }
    }
}

static MidiMessageSequence readTrack (const uint8* data, int size)
{
    double time = 0;
    uint8 lastStatusByte = 0;

    MidiMessageSequence result;

    while (size > 0)
    {
        const auto delay = MidiMessage::readVariableLengthValue (data, (int) size);

        if (! delay.isValid())
            break;

        data += delay.bytesUsed;
        size -= delay.bytesUsed;
        time += delay.value;

        if (size <= 0)
            break;

        int messSize = 0;
        const MidiMessage mm (data, size, messSize, lastStatusByte, time);

        if (messSize <= 0)
            break;

        size -= messSize;
        data += messSize;

        result.addEvent (mm);

        auto firstByte = *(mm.getRawData());

        if ((firstByte & 0xf0) != 0xf0)
            lastStatusByte = firstByte;
    }

    return result;
}
} // namespace MidiFileHelpers

//==============================================================================
MidiFile::MidiFile()
    : timeFormat ((short) (unsigned short) 0xe728)
{
}

MidiFile::MidiFile (const MidiFile& other)
    : timeFormat (other.timeFormat)
{
    tracks.addCopiesOf (other.tracks);
}

MidiFile& MidiFile::operator= (const MidiFile& other)
{
    tracks.clear();
    tracks.addCopiesOf (other.tracks);
    timeFormat = other.timeFormat;
    return *this;
}

MidiFile::MidiFile (MidiFile&& other)
    : tracks (std::move (other.tracks))
    , timeFormat (other.timeFormat)
{
}

MidiFile& MidiFile::operator= (MidiFile&& other)
{
    tracks = std::move (other.tracks);
    timeFormat = other.timeFormat;
    return *this;
}

void MidiFile::clear()
{
    tracks.clear();
}

//==============================================================================
int MidiFile::getNumTracks() const noexcept
{
    return tracks.size();
}

const MidiMessageSequence* MidiFile::getTrack (int index) const noexcept
{
    return tracks[index];
}

void MidiFile::addTrack (const MidiMessageSequence& trackSequence)
{
    tracks.add (new MidiMessageSequence (trackSequence));
}

//==============================================================================
short MidiFile::getTimeFormat() const noexcept
{
    return timeFormat;
}

void MidiFile::setTicksPerQuarterNote (int ticks) noexcept
{
    timeFormat = (short) ticks;
}

void MidiFile::setSmpteTimeFormat (int framesPerSecond, int subframeResolution) noexcept
{
    switch (framesPerSecond)
    {
        case 24:
        case 25:
        case 29:
        case 30:
            break;

        default:
            framesPerSecond = 25;
            break;
    }

    const int8 smpteByte = static_cast<int8> (-framesPerSecond);

    timeFormat = static_cast<int16> ((static_cast<uint8> (smpteByte) << 8) | static_cast<uint8> (subframeResolution));
}

//==============================================================================
void MidiFile::findAllTempoEvents (MidiMessageSequence& results) const
{
    MidiFileHelpers::findAllMatchingEvents (tracks, results, &MidiMessage::isTempoMetaEvent);
}

void MidiFile::findAllTimeSigEvents (MidiMessageSequence& results) const
{
    MidiFileHelpers::findAllMatchingEvents (tracks, results, &MidiMessage::isTimeSignatureMetaEvent);
}

void MidiFile::findAllKeySigEvents (MidiMessageSequence& results) const
{
    MidiFileHelpers::findAllMatchingEvents (tracks, results, &MidiMessage::isKeySignatureMetaEvent);
}

double MidiFile::getLastTimestamp() const
{
    double t = 0.0;

    for (auto* ms : tracks)
        t = jmax (t, ms->getEndTime());

    return t;
}

//==============================================================================
bool MidiFile::readFrom (InputStream& sourceStream,
                         bool createMatchingNoteOffs,
                         int* fileType)
{
    clear();
    MemoryBlock data;

    const int maxSensibleMidiFileSize = 200 * 1024 * 1024;

    // (put a sanity-check on the file size, as midi files are generally small)
    if (! sourceStream.readIntoMemoryBlock (data, maxSensibleMidiFileSize))
        return false;

    auto size = data.getSize();
    auto d = static_cast<const uint8*> (data.getData());

    const auto optHeader = MidiFileHelpers::parseMidiHeader (d, size);

    if (! optHeader.has_value())
        return false;

    const auto header = *optHeader;
    timeFormat = header.timeFormat;

    d += header.bytesRead;
    size -= (size_t) header.bytesRead;

    for (int track = 0; track < header.numberOfTracks; ++track)
    {
        const auto optChunkType = MidiFileHelpers::tryRead<uint32> (d, size);

        if (! optChunkType.has_value())
            return false;

        const auto optChunkSize = MidiFileHelpers::tryRead<uint32> (d, size);

        if (! optChunkSize.has_value())
            return false;

        const auto chunkSize = *optChunkSize;

        if (size < chunkSize)
            return false;

        if (*optChunkType == ByteOrder::bigEndianInt ("MTrk"))
            readNextTrack (d, (int) chunkSize, createMatchingNoteOffs);

        size -= chunkSize;
        d += chunkSize;
    }

    const auto successful = (size == 0);

    if (successful && fileType != nullptr)
        *fileType = header.fileType;

    return successful;
}

void MidiFile::readNextTrack (const uint8* data, int size, bool createMatchingNoteOffs)
{
    auto sequence = MidiFileHelpers::readTrack (data, size);

    // sort so that we put all the note-offs before note-ons that have the same time
    std::stable_sort (sequence.list.begin(), sequence.list.end(), [] (const MidiMessageSequence::MidiEventHolder* a, const MidiMessageSequence::MidiEventHolder* b)
    {
        auto t1 = a->message.getTimeStamp();
        auto t2 = b->message.getTimeStamp();

        if (t1 < t2)
            return true;
        if (t2 < t1)
            return false;

        return a->message.isNoteOff() && b->message.isNoteOn();
    });

    if (createMatchingNoteOffs)
        sequence.updateMatchedPairs();

    addTrack (sequence);
}

//==============================================================================
void MidiFile::convertTimestampTicksToSeconds()
{
    MidiMessageSequence tempoEvents;
    findAllTempoEvents (tempoEvents);
    findAllTimeSigEvents (tempoEvents);

    if (timeFormat != 0)
    {
        for (auto* ms : tracks)
        {
            for (int j = ms->getNumEvents(); --j >= 0;)
            {
                auto& m = ms->getEventPointer (j)->message;
                m.setTimeStamp (MidiFileHelpers::convertTicksToSeconds (m.getTimeStamp(), tempoEvents, timeFormat));
            }
        }
    }
}

//==============================================================================
bool MidiFile::writeTo (OutputStream& out, int midiFileType) const
{
    jassert (midiFileType >= 0 && midiFileType <= 2);

    if (! out.writeIntBigEndian ((int) ByteOrder::bigEndianInt ("MThd")))
        return false;
    if (! out.writeIntBigEndian (6))
        return false;
    if (! out.writeShortBigEndian ((short) midiFileType))
        return false;
    if (! out.writeShortBigEndian ((short) tracks.size()))
        return false;
    if (! out.writeShortBigEndian (timeFormat))
        return false;

    for (auto* ms : tracks)
        if (! writeTrack (out, *ms))
            return false;

    out.flush();
    return true;
}

bool MidiFile::writeTrack (OutputStream& mainOut, const MidiMessageSequence& ms) const
{
    MemoryOutputStream out;

    int lastTick = 0;
    uint8 lastStatusByte = 0;
    bool endOfTrackEventWritten = false;

    for (int i = 0; i < ms.getNumEvents(); ++i)
    {
        auto& mm = ms.getEventPointer (i)->message;

        if (mm.isEndOfTrackMetaEvent())
            endOfTrackEventWritten = true;

        auto tick = roundToInt (mm.getTimeStamp());
        auto delta = jmax (0, tick - lastTick);
        MidiFileHelpers::writeVariableLengthInt (out, (uint32) delta);
        lastTick = tick;

        auto* data = mm.getRawData();
        auto dataSize = mm.getRawDataSize();
        auto statusByte = data[0];

        if (statusByte == lastStatusByte
            && (statusByte & 0xf0) != 0xf0
            && dataSize > 1
            && i > 0)
        {
            ++data;
            --dataSize;
        }
        else if (statusByte == 0xf0) // Write sysex message with length bytes.
        {
            out.writeByte ((char) statusByte);

            ++data;
            --dataSize;

            MidiFileHelpers::writeVariableLengthInt (out, (uint32) dataSize);
        }

        out.write (data, (size_t) dataSize);
        lastStatusByte = statusByte;
    }

    if (! endOfTrackEventWritten)
    {
        out.writeByte (0); // (tick delta)
        auto m = MidiMessage::endOfTrack();
        out.write (m.getRawData(), (size_t) m.getRawDataSize());
    }

    if (! mainOut.writeIntBigEndian ((int) ByteOrder::bigEndianInt ("MTrk")))
        return false;
    if (! mainOut.writeIntBigEndian ((int) out.getDataSize()))
        return false;

    mainOut << out;

    return true;
}

} // namespace yup
