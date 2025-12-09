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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
class TestMidiCallback
{
public:
    struct ReceivedMessage
    {
        MidiMessage message;
        void* userData;

        bool operator== (const ReceivedMessage& other) const
        {
            return message.getDescription() == other.message.getDescription()
                && userData == other.userData;
        }
    };

    struct ReceivedPartialSysex
    {
        std::vector<uint8> data;
        double time;
        void* userData;
    };

    void handleIncomingMidiMessage (void* source, const MidiMessage& message)
    {
        receivedMessages.push_back ({ message, source });
    }

    void handlePartialSysexMessage (void* source, const uint8* messageData, int numBytesSoFar, double timestamp)
    {
        std::vector<uint8> data (messageData, messageData + numBytesSoFar);
        receivedPartialSysex.push_back ({ data, timestamp, source });
    }

    std::vector<ReceivedMessage> receivedMessages;
    std::vector<ReceivedPartialSysex> receivedPartialSysex;
};

//==============================================================================
class MidiDataConcatenatorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        concatenator = std::make_unique<MidiDataConcatenator> (256);
        callback = std::make_unique<TestMidiCallback>();
    }

    void TearDown() override
    {
        concatenator.reset();
        callback.reset();
    }

    void pushData (const std::vector<uint8>& data, double time = 0.0, void* userData = nullptr)
    {
        concatenator->pushMidiData (data.data(), (int) data.size(), time, userData, *callback);
    }

    std::unique_ptr<MidiDataConcatenator> concatenator;
    std::unique_ptr<TestMidiCallback> callback;
};

//==============================================================================
// Constructor tests
TEST_F (MidiDataConcatenatorTests, Constructor)
{
    EXPECT_NO_THROW (MidiDataConcatenator (256));
    EXPECT_NO_THROW (MidiDataConcatenator (0));
    EXPECT_NO_THROW (MidiDataConcatenator (1024));
}

//==============================================================================
// Reset tests
TEST_F (MidiDataConcatenatorTests, ResetClearsState)
{
    // Send partial message
    pushData ({ 0x90, 0x3c }, 1.0);
    EXPECT_EQ (callback->receivedMessages.size(), 0);

    concatenator->reset();

    // After reset, previous partial message should be forgotten
    pushData ({ 0x64 }, 2.0);
    EXPECT_EQ (callback->receivedMessages.size(), 0);
}

TEST_F (MidiDataConcatenatorTests, ResetClearsPendingSysex)
{
    // Start sysex but don't complete it
    pushData ({ 0xf0, 0x43, 0x12 }, 1.0);

    concatenator->reset();

    // After reset, pending sysex should be cleared
    // Send a complete note-on message
    pushData ({ 0x90, 0x3c, 0x64 }, 2.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
}

//==============================================================================
// Simple message tests
TEST_F (MidiDataConcatenatorTests, NoteOnMessage)
{
    pushData ({ 0x90, 0x3c, 0x64 }, 1.5);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[0].message.getChannel(), 1);
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
    EXPECT_EQ (callback->receivedMessages[0].message.getVelocity(), 100);
    EXPECT_DOUBLE_EQ (callback->receivedMessages[0].message.getTimeStamp(), 1.5);
}

TEST_F (MidiDataConcatenatorTests, NoteOffMessage)
{
    pushData ({ 0x80, 0x3c, 0x40 }, 2.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOff());
    EXPECT_EQ (callback->receivedMessages[0].message.getChannel(), 1);
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
}

TEST_F (MidiDataConcatenatorTests, ControllerMessage)
{
    pushData ({ 0xb0, 0x07, 0x7f }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isController());
    EXPECT_EQ (callback->receivedMessages[0].message.getControllerNumber(), 7);
    EXPECT_EQ (callback->receivedMessages[0].message.getControllerValue(), 127);
}

TEST_F (MidiDataConcatenatorTests, ProgramChangeMessage)
{
    pushData ({ 0xc0, 0x2a }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isProgramChange());
    EXPECT_EQ (callback->receivedMessages[0].message.getProgramChangeNumber(), 42);
}

TEST_F (MidiDataConcatenatorTests, PitchWheelMessage)
{
    pushData ({ 0xe0, 0x00, 0x40 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isPitchWheel());
}

TEST_F (MidiDataConcatenatorTests, ChannelPressureMessage)
{
    pushData ({ 0xd0, 0x50 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isChannelPressure());
    EXPECT_EQ (callback->receivedMessages[0].message.getChannelPressureValue(), 80);
}

TEST_F (MidiDataConcatenatorTests, AftertouchMessage)
{
    pushData ({ 0xa0, 0x3c, 0x64 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isAftertouch());
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
}

//==============================================================================
// Realtime message tests
TEST_F (MidiDataConcatenatorTests, TimingClockMessage)
{
    pushData ({ 0xf8 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiClock());
}

TEST_F (MidiDataConcatenatorTests, StartMessage)
{
    pushData ({ 0xfa }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiStart());
}

TEST_F (MidiDataConcatenatorTests, ContinueMessage)
{
    pushData ({ 0xfb }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiContinue());
}

TEST_F (MidiDataConcatenatorTests, StopMessage)
{
    pushData ({ 0xfc }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiStop());
}

TEST_F (MidiDataConcatenatorTests, ActiveSensingMessage)
{
    pushData ({ 0xfe }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isActiveSense());
}

TEST_F (MidiDataConcatenatorTests, RealtimeMessageEmbeddedInNormalMessage)
{
    // Clock embedded between status and data bytes
    pushData ({ 0x90, 0xf8, 0x3c, 0x64 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiClock());
    EXPECT_TRUE (callback->receivedMessages[1].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[1].message.getNoteNumber(), 60);
}

//==============================================================================
// Running status tests
TEST_F (MidiDataConcatenatorTests, RunningStatusSameChannel)
{
    // Send complete message then use running status
    pushData ({ 0x90, 0x3c, 0x64 }, 1.0);
    pushData ({ 0x40, 0x50 }, 1.5);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
    EXPECT_TRUE (callback->receivedMessages[1].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[1].message.getNoteNumber(), 64);
}

TEST_F (MidiDataConcatenatorTests, RunningStatusInterruptedByNewStatus)
{
    pushData ({ 0x90, 0x3c, 0x64 }, 1.0);
    pushData ({ 0xb0, 0x07, 0x7f }, 1.5);
    pushData ({ 0x10, 0x50 }, 2.0); // Should use controller running status

    EXPECT_EQ (callback->receivedMessages.size(), 3);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_TRUE (callback->receivedMessages[1].message.isController());
    EXPECT_TRUE (callback->receivedMessages[2].message.isController());
    EXPECT_EQ (callback->receivedMessages[2].message.getControllerNumber(), 16);
}

//==============================================================================
// Fragmented message tests
TEST_F (MidiDataConcatenatorTests, MessageSplitAcrossMultipleCalls)
{
    pushData ({ 0x90 }, 1.0);
    pushData ({ 0x3c }, 1.0);
    pushData ({ 0x64 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
}

TEST_F (MidiDataConcatenatorTests, TwoByteMessageSplitAcrossMultipleCalls)
{
    pushData ({ 0xc0 }, 1.0);
    pushData ({ 0x2a }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isProgramChange());
    EXPECT_EQ (callback->receivedMessages[0].message.getProgramChangeNumber(), 42);
}

TEST_F (MidiDataConcatenatorTests, MultipleMessagesInOneCall)
{
    pushData ({ 0x90, 0x3c, 0x64, 0x80, 0x3c, 0x40, 0xb0, 0x07, 0x7f }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 3);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_TRUE (callback->receivedMessages[1].message.isNoteOff());
    EXPECT_TRUE (callback->receivedMessages[2].message.isController());
}

//==============================================================================
// SysEx message tests
TEST_F (MidiDataConcatenatorTests, CompleteSysExMessage)
{
    pushData ({ 0xf0, 0x43, 0x12, 0x00, 0x01, 0xf7 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isSysEx());

    auto* data = callback->receivedMessages[0].message.getSysExData();
    EXPECT_EQ (data[0], 0x43);
    EXPECT_EQ (data[1], 0x12);
    EXPECT_EQ (data[2], 0x00);
    EXPECT_EQ (data[3], 0x01);
}

TEST_F (MidiDataConcatenatorTests, SysExSplitAcrossMultipleCalls)
{
    pushData ({ 0xf0, 0x43 }, 1.0);
    pushData ({ 0x12, 0x00 }, 1.0);
    pushData ({ 0x01, 0xf7 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isSysEx());

    auto* data = callback->receivedMessages[0].message.getSysExData();
    EXPECT_EQ (data[0], 0x43);
    EXPECT_EQ (data[1], 0x12);
}

TEST_F (MidiDataConcatenatorTests, PartialSysExWithoutTerminator)
{
    pushData ({ 0xf0, 0x43, 0x12, 0x00 }, 1.5);

    EXPECT_EQ (callback->receivedMessages.size(), 0);
    EXPECT_EQ (callback->receivedPartialSysex.size(), 1);
    EXPECT_DOUBLE_EQ (callback->receivedPartialSysex[0].time, 1.5);
    EXPECT_EQ (callback->receivedPartialSysex[0].data.size(), 4);
    EXPECT_EQ (callback->receivedPartialSysex[0].data[0], 0xf0);
    EXPECT_EQ (callback->receivedPartialSysex[0].data[1], 0x43);
}

TEST_F (MidiDataConcatenatorTests, PartialSysExCompletedLater)
{
    pushData ({ 0xf0, 0x43, 0x12 }, 1.0);
    EXPECT_EQ (callback->receivedPartialSysex.size(), 1);

    pushData ({ 0x00, 0x01, 0xf7 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isSysEx());
}

TEST_F (MidiDataConcatenatorTests, LargeSysExMessage)
{
    std::vector<uint8> sysexData { 0xf0 };
    for (int i = 0; i < 1000; ++i)
        sysexData.push_back (i & 0x7f);
    sysexData.push_back (0xf7);

    pushData (sysexData, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isSysEx());
    EXPECT_EQ (callback->receivedMessages[0].message.getSysExDataSize(), 1000);
}

TEST_F (MidiDataConcatenatorTests, SysExInterruptedByRealtimeMessage)
{
    pushData ({ 0xf0, 0x43, 0xf8, 0x12, 0x00, 0xf7 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiClock());
    EXPECT_TRUE (callback->receivedMessages[1].message.isSysEx());

    // Clock should not be part of sysex data
    auto* data = callback->receivedMessages[1].message.getSysExData();
    EXPECT_EQ (data[0], 0x43);
    EXPECT_EQ (data[1], 0x12);
    EXPECT_EQ (data[2], 0x00);
}

TEST_F (MidiDataConcatenatorTests, SysExInterruptedByNonRealtimeMessage)
{
    // SysEx interrupted by note-on
    pushData ({ 0xf0, 0x43, 0x12, 0x90, 0x3c, 0x64 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
}

TEST_F (MidiDataConcatenatorTests, MultipleSysExMessages)
{
    pushData ({ 0xf0, 0x43, 0x12, 0xf7 }, 1.0);
    pushData ({ 0xf0, 0x7e, 0x00, 0xf7 }, 2.0);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_TRUE (callback->receivedMessages[0].message.isSysEx());
    EXPECT_TRUE (callback->receivedMessages[1].message.isSysEx());
}

//==============================================================================
// Invalid data tests
TEST_F (MidiDataConcatenatorTests, InvalidDataByte)
{
    // Send data byte without status
    pushData ({ 0x3c }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 0);
}

TEST_F (MidiDataConcatenatorTests, MessageTooLong)
{
    // Try to send 4 bytes for a 3-byte message
    pushData ({ 0x90, 0x3c, 0x64, 0x70 }, 1.0);

    // Should get one complete message, then treat 0x70 as invalid data
    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_EQ (callback->receivedMessages[0].message.getNoteNumber(), 60);
}

TEST_F (MidiDataConcatenatorTests, StatusByteWithoutData)
{
    pushData ({ 0x90 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 0);
}

TEST_F (MidiDataConcatenatorTests, IncompleteMessageFollowedByNewStatus)
{
    pushData ({ 0x90, 0x3c }, 1.0);       // Incomplete note-on
    pushData ({ 0xb0, 0x07, 0x7f }, 1.5); // Complete controller

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isController());
}

//==============================================================================
// User data tests
TEST_F (MidiDataConcatenatorTests, UserDataPassedThrough)
{
    int myData = 42;
    pushData ({ 0x90, 0x3c, 0x64 }, 1.0, &myData);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_EQ (callback->receivedMessages[0].userData, &myData);
}

TEST_F (MidiDataConcatenatorTests, DifferentUserDataForDifferentMessages)
{
    int data1 = 1;
    int data2 = 2;

    pushData ({ 0x90, 0x3c, 0x64 }, 1.0, &data1);
    pushData ({ 0x80, 0x3c, 0x40 }, 1.5, &data2);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_EQ (callback->receivedMessages[0].userData, &data1);
    EXPECT_EQ (callback->receivedMessages[1].userData, &data2);
}

TEST_F (MidiDataConcatenatorTests, UserDataForSysEx)
{
    int myData = 99;
    pushData ({ 0xf0, 0x43, 0x12, 0xf7 }, 1.0, &myData);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_EQ (callback->receivedMessages[0].userData, &myData);
}

//==============================================================================
// Timestamp tests
TEST_F (MidiDataConcatenatorTests, DifferentTimestamps)
{
    pushData ({ 0x90, 0x3c, 0x64 }, 1.0);
    pushData ({ 0x80, 0x3c, 0x40 }, 2.5);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_DOUBLE_EQ (callback->receivedMessages[0].message.getTimeStamp(), 1.0);
    EXPECT_DOUBLE_EQ (callback->receivedMessages[1].message.getTimeStamp(), 2.5);
}

TEST_F (MidiDataConcatenatorTests, TimestampForFragmentedMessage)
{
    pushData ({ 0x90 }, 1.0);
    pushData ({ 0x3c }, 2.0);
    pushData ({ 0x64 }, 3.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    // Should use timestamp from final byte
    EXPECT_DOUBLE_EQ (callback->receivedMessages[0].message.getTimeStamp(), 3.0);
}

TEST_F (MidiDataConcatenatorTests, TimestampForSysExPreserved)
{
    pushData ({ 0xf0, 0x43 }, 1.5);
    pushData ({ 0x12, 0xf7 }, 2.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    // Should use timestamp from when sysex started
    EXPECT_DOUBLE_EQ (callback->receivedMessages[0].message.getTimeStamp(), 1.5);
}

//==============================================================================
// Edge case tests
TEST_F (MidiDataConcatenatorTests, EmptyData)
{
    pushData ({}, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 0);
}

TEST_F (MidiDataConcatenatorTests, NullData)
{
    struct CustomData
    {
    } customData;

    concatenator->pushMidiData (nullptr, 0, 1.0, &customData, *callback);

    EXPECT_EQ (callback->receivedMessages.size(), 0);
}

TEST_F (MidiDataConcatenatorTests, ZeroBytes)
{
    struct CustomData
    {
    } customData;

    std::vector<uint8> data { 0x90, 0x3c, 0x64 };
    concatenator->pushMidiData (data.data(), 0, 1.0, &customData, *callback);

    EXPECT_EQ (callback->receivedMessages.size(), 0);
}

TEST_F (MidiDataConcatenatorTests, SingleByte)
{
    pushData ({ 0xf8 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
    EXPECT_TRUE (callback->receivedMessages[0].message.isMidiClock());
}

TEST_F (MidiDataConcatenatorTests, ResetBetweenMessages)
{
    pushData ({ 0x90, 0x3c, 0x64 }, 1.0);

    concatenator->reset();

    pushData ({ 0x80, 0x40, 0x40 }, 2.0);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_TRUE (callback->receivedMessages[1].message.isNoteOff());
}

TEST_F (MidiDataConcatenatorTests, MultipleResetsInARow)
{
    concatenator->reset();
    concatenator->reset();
    concatenator->reset();

    pushData ({ 0x90, 0x3c, 0x64 }, 1.0);

    EXPECT_EQ (callback->receivedMessages.size(), 1);
}

//==============================================================================
// Complex scenarios
TEST_F (MidiDataConcatenatorTests, RealisticMidiStream)
{
    // Note on
    pushData ({ 0x90, 0x3c, 0x64 }, 0.0);

    // Clock messages (typical during playback)
    pushData ({ 0xf8 }, 0.02);
    pushData ({ 0xf8 }, 0.04);

    // Controller change
    pushData ({ 0xb0, 0x07, 0x7f }, 0.05);

    // More clock
    pushData ({ 0xf8 }, 0.06);

    // Note off using running status
    pushData ({ 0x80, 0x3c, 0x40 }, 0.1);

    EXPECT_EQ (callback->receivedMessages.size(), 6);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_TRUE (callback->receivedMessages[1].message.isMidiClock());
    EXPECT_TRUE (callback->receivedMessages[2].message.isMidiClock());
    EXPECT_TRUE (callback->receivedMessages[3].message.isController());
    EXPECT_TRUE (callback->receivedMessages[4].message.isMidiClock());
    EXPECT_TRUE (callback->receivedMessages[5].message.isNoteOff());
}

TEST_F (MidiDataConcatenatorTests, MixedFragmentedAndCompleteMessages)
{
    pushData ({ 0x90 }, 0.0);
    pushData ({ 0x3c, 0x64, 0xb0, 0x07 }, 0.01);
    pushData ({ 0x7f }, 0.02);

    EXPECT_EQ (callback->receivedMessages.size(), 2);
    EXPECT_TRUE (callback->receivedMessages[0].message.isNoteOn());
    EXPECT_TRUE (callback->receivedMessages[1].message.isController());
}

TEST_F (MidiDataConcatenatorTests, AllChannels)
{
    for (int ch = 0; ch < 16; ++ch)
    {
        pushData ({ static_cast<uint8> (0x90 | ch), 0x3c, 0x64 }, 0.0);
    }

    EXPECT_EQ (callback->receivedMessages.size(), 16);
    for (int ch = 0; ch < 16; ++ch)
    {
        EXPECT_EQ (callback->receivedMessages[ch].message.getChannel(), ch + 1);
    }
}
