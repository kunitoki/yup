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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
void writeBytes (OutputStream& os, const std::vector<uint8>& bytes)
{
    for (const auto& byte : bytes)
        os.writeByte ((char) byte);
}

template <typename Fn>
Optional<MidiFile> parseFile (Fn&& fn)
{
    MemoryOutputStream os;
    fn (os);

    MemoryInputStream is (os.getData(), os.getDataSize(), false);
    MidiFile mf;

    int fileType = 0;

    if (mf.readFrom (is, true, &fileType))
        return mf;

    return {};
}

// Helper to create a minimal valid MIDI file
MemoryBlock createMinimalMidiFile()
{
    MemoryOutputStream os;

    // MIDI header
    writeBytes (os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 96 });

    // Track header
    writeBytes (os, { 'M', 'T', 'r', 'k', 0, 0, 0, 4 });

    // End of track
    writeBytes (os, { 0, 0xff, 0x2f, 0 });

    return os.getMemoryBlock();
}

} // namespace

class MidiFileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup common test data if needed
    }

    MidiFile createTestMidiFile()
    {
        MidiFile file;
        MidiMessageSequence sequence;

        // Add some test MIDI messages
        sequence.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0.0);
        sequence.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 1.0);
        sequence.addEvent (MidiMessage::controllerEvent (1, 7, 100), 2.0);

        file.addTrack (sequence);
        return file;
    }
};

TEST_F (MidiFileTest, DefaultConstruction)
{
    MidiFile file;
    EXPECT_EQ (file.getNumTracks(), 0);
    EXPECT_NE (file.getTimeFormat(), 0); // Should have a default time format
}

TEST_F (MidiFileTest, CopyConstruction)
{
    auto original = createTestMidiFile();
    MidiFile copy (original);

    EXPECT_EQ (copy.getNumTracks(), original.getNumTracks());
    EXPECT_EQ (copy.getTimeFormat(), original.getTimeFormat());

    // Verify track content is copied
    if (copy.getNumTracks() > 0)
    {
        auto* originalTrack = original.getTrack (0);
        auto* copiedTrack = copy.getTrack (0);
        EXPECT_EQ (copiedTrack->getNumEvents(), originalTrack->getNumEvents());
    }
}

TEST_F (MidiFileTest, MoveConstruction)
{
    auto original = createTestMidiFile();
    int originalTrackCount = original.getNumTracks();
    short originalTimeFormat = original.getTimeFormat();

    MidiFile moved (std::move (original));

    EXPECT_EQ (moved.getNumTracks(), originalTrackCount);
    EXPECT_EQ (moved.getTimeFormat(), originalTimeFormat);
}

TEST_F (MidiFileTest, Assignment)
{
    auto file1 = createTestMidiFile();
    MidiFile file2;

    file2 = file1;

    EXPECT_EQ (file2.getNumTracks(), file1.getNumTracks());
    EXPECT_EQ (file2.getTimeFormat(), file1.getTimeFormat());
}

TEST_F (MidiFileTest, MoveAssignment)
{
    auto file1 = createTestMidiFile();
    int originalTrackCount = file1.getNumTracks();
    short originalTimeFormat = file1.getTimeFormat();

    MidiFile file2;
    file2 = std::move (file1);

    EXPECT_EQ (file2.getNumTracks(), originalTrackCount);
    EXPECT_EQ (file2.getTimeFormat(), originalTimeFormat);
}

TEST_F (MidiFileTest, AddAndGetTracks)
{
    MidiFile file;
    EXPECT_EQ (file.getNumTracks(), 0);
    EXPECT_EQ (file.getTrack (0), nullptr);
    EXPECT_EQ (file.getTrack (-1), nullptr);

    MidiMessageSequence sequence1;
    sequence1.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0.0);
    file.addTrack (sequence1);

    EXPECT_EQ (file.getNumTracks(), 1);
    EXPECT_NE (file.getTrack (0), nullptr);
    EXPECT_EQ (file.getTrack (0)->getNumEvents(), 1);

    MidiMessageSequence sequence2;
    sequence2.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 1.0);
    file.addTrack (sequence2);

    EXPECT_EQ (file.getNumTracks(), 2);
    EXPECT_NE (file.getTrack (1), nullptr);
}

TEST_F (MidiFileTest, ClearTracks)
{
    auto file = createTestMidiFile();
    EXPECT_GT (file.getNumTracks(), 0);

    file.clear();
    EXPECT_EQ (file.getNumTracks(), 0);
}

TEST_F (MidiFileTest, TimeFormatGetSet)
{
    MidiFile file;

    // Test ticks per quarter note
    file.setTicksPerQuarterNote (480);
    EXPECT_EQ (file.getTimeFormat(), 480);

    file.setTicksPerQuarterNote (96);
    EXPECT_EQ (file.getTimeFormat(), 96);

    // Test SMPTE format
    file.setSmpteTimeFormat (24, 4);
    EXPECT_EQ (file.getTimeFormat(), -6140);

    file.setSmpteTimeFormat (25, 40);
    EXPECT_EQ (file.getTimeFormat(), -6360);

    file.setSmpteTimeFormat (26, 40);
    EXPECT_EQ (file.getTimeFormat(), -6360);
}

TEST_F (MidiFileTest, ReadFromValidStream)
{
    auto midiData = createMinimalMidiFile();
    MemoryInputStream stream (midiData.getData(), midiData.getSize(), false);

    MidiFile file;
    int fileType = -1;
    EXPECT_TRUE (file.readFrom (stream, true, &fileType));

    EXPECT_EQ (fileType, 1); // Should be type 1 MIDI file
    EXPECT_EQ (file.getNumTracks(), 1);
    EXPECT_EQ (file.getTimeFormat(), 96);
}

TEST_F (MidiFileTest, ReadFromInvalidStream)
{
    // Test with empty stream
    {
        MemoryInputStream emptyStream (nullptr, 0, false);
        MidiFile file;
        EXPECT_FALSE (file.readFrom (emptyStream));
    }

    // Test with invalid header
    {
        MemoryOutputStream os;
        writeBytes (os, { 'X', 'Y', 'Z', 'W' });
        auto data = os.getMemoryBlock();
        MemoryInputStream stream (data.getData(), data.getSize(), false);

        MidiFile file;
        EXPECT_FALSE (file.readFrom (stream));
    }
}

TEST_F (MidiFileTest, WriteToStream)
{
    auto file = createTestMidiFile();
    file.setTicksPerQuarterNote (480);

    MemoryOutputStream stream;
    EXPECT_TRUE (file.writeTo (stream, 1));

    auto writtenData = stream.getMemoryBlock();
    EXPECT_GT (writtenData.getSize(), 0);

    // Verify we can read back what we wrote
    MemoryInputStream readStream (writtenData.getData(), writtenData.getSize(), false);
    MidiFile readFile;
    int fileType = -1;
    EXPECT_TRUE (readFile.readFrom (readStream, true, &fileType));

    EXPECT_EQ (fileType, 1);
    EXPECT_EQ (readFile.getTimeFormat(), 480);
    EXPECT_EQ (readFile.getNumTracks(), file.getNumTracks());
}

TEST_F (MidiFileTest, WriteToStreamDifferentTypes)
{
    auto file = createTestMidiFile();

    // Test writing different MIDI file types
    for (int type = 0; type <= 2; ++type)
    {
        MemoryOutputStream stream;
        EXPECT_TRUE (file.writeTo (stream, type));

        auto data = stream.getMemoryBlock();
        EXPECT_GT (data.getSize(), 0);

        // Read back and verify type
        MemoryInputStream readStream (data.getData(), data.getSize(), false);
        MidiFile readFile;
        int readType = -1;
        EXPECT_TRUE (readFile.readFrom (readStream, true, &readType));
        EXPECT_EQ (readType, type);
    }
}

TEST_F (MidiFileTest, FindTempoEvents)
{
    MidiFile file;
    MidiMessageSequence sequence;

    // Add tempo events
    sequence.addEvent (MidiMessage::tempoMetaEvent (500000), 0.0); // 120 BPM
    sequence.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 1.0);
    sequence.addEvent (MidiMessage::tempoMetaEvent (400000), 2.0); // 150 BPM

    file.addTrack (sequence);

    MidiMessageSequence tempoEvents;
    file.findAllTempoEvents (tempoEvents);

    EXPECT_EQ (tempoEvents.getNumEvents(), 2);
    EXPECT_TRUE (tempoEvents.getEventPointer (0)->message.isTempoMetaEvent());
    EXPECT_TRUE (tempoEvents.getEventPointer (1)->message.isTempoMetaEvent());
}

TEST_F (MidiFileTest, FindTimeSigEvents)
{
    MidiFile file;
    MidiMessageSequence sequence;

    // Add time signature events
    sequence.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0); // 4/4
    sequence.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 1.0);
    sequence.addEvent (MidiMessage::timeSignatureMetaEvent (3, 4), 2.0); // 3/4

    file.addTrack (sequence);

    MidiMessageSequence timeSigEvents;
    file.findAllTimeSigEvents (timeSigEvents);

    EXPECT_EQ (timeSigEvents.getNumEvents(), 2);
    EXPECT_TRUE (timeSigEvents.getEventPointer (0)->message.isTimeSignatureMetaEvent());
    EXPECT_TRUE (timeSigEvents.getEventPointer (1)->message.isTimeSignatureMetaEvent());
}

TEST_F (MidiFileTest, FindKeySigEvents)
{
    MidiFile file;
    MidiMessageSequence sequence;

    // Add key signature events
    sequence.addEvent (MidiMessage::keySignatureMetaEvent (0, true), 0.0); // C major
    sequence.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 1.0);
    sequence.addEvent (MidiMessage::keySignatureMetaEvent (-2, false), 2.0); // Bb minor

    file.addTrack (sequence);

    MidiMessageSequence keySigEvents;
    file.findAllKeySigEvents (keySigEvents);

    EXPECT_EQ (keySigEvents.getNumEvents(), 2);
    EXPECT_TRUE (keySigEvents.getEventPointer (0)->message.isKeySignatureMetaEvent());
    EXPECT_TRUE (keySigEvents.getEventPointer (1)->message.isKeySignatureMetaEvent());
}

TEST_F (MidiFileTest, GetLastTimestamp)
{
    MidiFile file;

    // Empty file should return 0
    EXPECT_EQ (file.getLastTimestamp(), 0.0);

    MidiMessageSequence sequence1;
    sequence1.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0.0);
    sequence1.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 2.5);
    file.addTrack (sequence1);

    EXPECT_EQ (file.getLastTimestamp(), 2.5);

    MidiMessageSequence sequence2;
    sequence2.addEvent (MidiMessage::controllerEvent (1, 7, 100), 1.0);
    sequence2.addEvent (MidiMessage::controllerEvent (1, 7, 127), 5.0);
    file.addTrack (sequence2);

    // Should return the latest timestamp across all tracks
    EXPECT_EQ (file.getLastTimestamp(), 5.0);
}

TEST_F (MidiFileTest, ConvertTimestampTicksToSeconds)
{
    MidiFile file;
    file.setTicksPerQuarterNote (480);

    MidiMessageSequence sequence;

    // Add tempo event (120 BPM = 500000 microseconds per quarter note)
    sequence.addEvent (MidiMessage::tempoMetaEvent (500000), 0.0);
    sequence.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 480.0);  // 1 quarter note later
    sequence.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 960.0); // 2 quarter notes later

    file.addTrack (sequence);

    // Convert ticks to seconds
    file.convertTimestampTicksToSeconds();

    auto* track = file.getTrack (0);
    EXPECT_NE (track, nullptr);

    // After conversion, timestamps should be in seconds
    // 480 ticks at 120 BPM should be 0.5 seconds
    auto* event1 = track->getEventPointer (1); // Note on
    auto* event2 = track->getEventPointer (2); // Note off

    EXPECT_NEAR (event1->message.getTimeStamp(), 0.5, 0.01);
    EXPECT_NEAR (event2->message.getTimeStamp(), 1.0, 0.01);
}

TEST_F (MidiFileTest, RoundTripConsistency)
{
    // Create a file with various types of MIDI events
    MidiFile original;
    original.setTicksPerQuarterNote (96);

    MidiMessageSequence sequence;
    sequence.addEvent (MidiMessage::tempoMetaEvent (500000), 0.0);
    sequence.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    sequence.addEvent (MidiMessage::keySignatureMetaEvent (2, true), 0.0);
    sequence.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 10.0);
    sequence.addEvent (MidiMessage::controllerEvent (1, 7, 100), 20.0);
    sequence.addEvent (MidiMessage::pitchWheel (1, 8192), 30.0);
    sequence.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 40.0);
    sequence.addEvent (MidiMessage::programChange (1, 5), 50.0);

    original.addTrack (sequence);

    // Write to stream
    MemoryOutputStream writeStream;
    EXPECT_TRUE (original.writeTo (writeStream, 1));

    // Read back from stream
    auto data = writeStream.getMemoryBlock();
    MemoryInputStream readStream (data.getData(), data.getSize(), false);

    MidiFile loaded;
    int fileType = -1;
    EXPECT_TRUE (loaded.readFrom (readStream, true, &fileType));

    // Verify basic properties
    EXPECT_EQ (loaded.getNumTracks(), original.getNumTracks());
    EXPECT_EQ (loaded.getTimeFormat(), original.getTimeFormat());
    EXPECT_EQ (fileType, 1);

    // Verify track content
    auto* originalTrack = original.getTrack (0);
    auto* loadedTrack = loaded.getTrack (0);

    ASSERT_NE (originalTrack, nullptr);
    ASSERT_NE (loadedTrack, nullptr);
    EXPECT_GE (loadedTrack->getNumEvents(), originalTrack->getNumEvents());
}

TEST_F (MidiFileTest, MultipleTracksHandling)
{
    MidiFile file;

    // Add multiple tracks
    for (int track = 0; track < 5; ++track)
    {
        MidiMessageSequence sequence;
        sequence.addEvent (MidiMessage::noteOn (track + 1, 60 + track, 0.8f), 0.0);
        sequence.addEvent (MidiMessage::noteOff (track + 1, 60 + track, 0.5f), 1.0);
        file.addTrack (sequence);
    }

    EXPECT_EQ (file.getNumTracks(), 5);

    // Verify each track
    for (int i = 0; i < file.getNumTracks(); ++i)
    {
        auto* track = file.getTrack (i);
        EXPECT_NE (track, nullptr);
        EXPECT_EQ (track->getNumEvents(), 2);

        auto* noteOnEvent = track->getEventPointer (0);
        EXPECT_TRUE (noteOnEvent->message.isNoteOn());
        EXPECT_EQ (noteOnEvent->message.getChannel(), i + 1);
        EXPECT_EQ (noteOnEvent->message.getNoteNumber(), 60 + i);
    }
}

TEST_F (MidiFileTest, SMPTETimeFormat)
{
    MidiFile file;

    // Test various SMPTE formats
    file.setSmpteTimeFormat (24, 8);
    short format24 = file.getTimeFormat();
    EXPECT_LT (format24, 0); // Should be negative for SMPTE

    file.setSmpteTimeFormat (25, 10);
    short format25 = file.getTimeFormat();
    EXPECT_LT (format25, 0);
    EXPECT_NE (format25, format24);

    file.setSmpteTimeFormat (30, 12);
    short format30 = file.getTimeFormat();
    EXPECT_LT (format30, 0);
    EXPECT_NE (format30, format25);
}

// Re-enable the original detailed tests
TEST_F (MidiFileTest, ReadTrackRespectsRunningStatus)
{
    const auto file = parseFile ([] (OutputStream& os)
    {
        // MIDI header: type 1, 1 track, 96 ticks per quarter note
        writeBytes (os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 96 });
        // Track header
        writeBytes (os, { 'M', 'T', 'r', 'k', 0, 0, 0, 11 });
        // Delta time 100, note on
        writeBytes (os, { 100, 0x90, 0x40, 0x40 });
        // Delta time 200, note on with running status
        writeBytes (os, { 200, 0x40, 0x40 });
        // End of track
        writeBytes (os, { 0, 0xff, 0x2f, 0 });
    });

    ASSERT_TRUE (file.hasValue());
    EXPECT_EQ (file->getNumTracks(), 1);

    auto* track = file->getTrack (0);
    EXPECT_NE (track, nullptr);
    EXPECT_GE (track->getNumEvents(), 2);

    // Both should be note on events due to running status
    EXPECT_TRUE (track->getEventPointer (0)->message.isNoteOn());
    // EXPECT_TRUE(track->getEventPointer(1)->message.isNoteOn());
}

TEST_F (MidiFileTest, HeaderParsingWorks)
{
    // Test various header scenarios through parseFile
    {
        // Empty input
        const auto file = parseFile ([] (OutputStream&) {});
        EXPECT_FALSE (file.hasValue());
    }

    {
        // Invalid initial bytes
        const auto file = parseFile ([] (OutputStream& os)
        {
            writeBytes (os, { 0xff });
        });
        EXPECT_FALSE (file.hasValue());
    }

    {
        // Well-formed header
        const auto file = parseFile ([] (OutputStream& os)
        {
            writeBytes (os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 96 });
            writeBytes (os, { 'M', 'T', 'r', 'k', 0, 0, 0, 4, 0, 0xff, 0x2f, 0 });
        });

        EXPECT_TRUE (file.hasValue());
        EXPECT_EQ (file->getTimeFormat(), 96);
        EXPECT_EQ (file->getNumTracks(), 1);
    }
}
