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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

namespace
{

// clang-format off
const std::vector<uint8> metaEvents[]
{
    // Format is 0xff, followed by a 'kind' byte, followed by a variable-length
    // 'data-length' value, followed by that many data bytes
    { 0xff, 0x00, 0x02, 0x00, 0x00 },                   // Sequence number
    { 0xff, 0x01, 0x00 },                               // Text event
    { 0xff, 0x02, 0x00 },                               // Copyright notice
    { 0xff, 0x03, 0x00 },                               // Track name
    { 0xff, 0x04, 0x00 },                               // Instrument name
    { 0xff, 0x05, 0x00 },                               // Lyric
    { 0xff, 0x06, 0x00 },                               // Marker
    { 0xff, 0x07, 0x00 },                               // Cue point
    { 0xff, 0x20, 0x01, 0x00 },                         // Channel prefix
    { 0xff, 0x2f, 0x00 },                               // End of track
    { 0xff, 0x51, 0x03, 0x01, 0x02, 0x03 },             // Set tempo
    { 0xff, 0x54, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05 }, // SMPTE offset
    { 0xff, 0x58, 0x04, 0x01, 0x02, 0x03, 0x04 },       // Time signature
    { 0xff, 0x59, 0x02, 0x01, 0x02 },                   // Key signature
    { 0xff, 0x7f, 0x00 },                               // Sequencer-specific
};
// clang-format on

} // namespace

TEST (MidiMessageTests, ReadVariableLengthValueShouldReturnCompatibleResults)
{
    using std::begin;
    using std::end;

    const std::vector<uint8> inputs[] {
        { 0x00 },
        { 0x40 },
        { 0x7f },
        { 0x81, 0x00 },
        { 0xc0, 0x00 },
        { 0xff, 0x7f },
        { 0x81, 0x80, 0x00 },
        { 0xc0, 0x80, 0x00 },
        { 0xff, 0xff, 0x7f },
        { 0x81, 0x80, 0x80, 0x00 },
        { 0xc0, 0x80, 0x80, 0x00 },
        { 0xff, 0xff, 0xff, 0x7f }
    };

    const int outputs[] {
        0x00,
        0x40,
        0x7f,
        0x80,
        0x2000,
        0x3fff,
        0x4000,
        0x100000,
        0x1fffff,
        0x200000,
        0x8000000,
        0xfffffff,
    };

    EXPECT_EQ (std::distance (begin (inputs), end (inputs)),
               std::distance (begin (outputs), end (outputs)));

    size_t index = 0;

    for (const auto& input : inputs)
    {
        auto copy = input;

        while (copy.size() < 16)
            copy.push_back (0);

        const auto result = MidiMessage::readVariableLengthValue (copy.data(),
                                                                  (int) copy.size());

        EXPECT_TRUE (result.isValid());
        EXPECT_EQ (result.value, outputs[index]);
        EXPECT_EQ (result.bytesUsed, (int) inputs[index].size());

        ++index;
    }
}

TEST (MidiMessageTests, ReadVariableLengthValueShouldReturnZeroWithTruncatedInput)
{
    for (size_t i = 0; i != 16; ++i)
    {
        std::vector<uint8> input;
        input.resize (i, 0xFF);

        const auto result = MidiMessage::readVariableLengthValue (input.data(),
                                                                  (int) input.size());

        EXPECT_TRUE (! result.isValid());
        EXPECT_EQ (result.value, 0);
        EXPECT_EQ (result.bytesUsed, 0);
    }
}

TEST (MidiMessageTests, DataConstructorWorksWithMetaEvents)
{
    const auto status = (uint8) 0x90;

    for (const auto& input : metaEvents)
    {
        int bytesUsed = 0;
        const MidiMessage msg (input.data(), (int) input.size(), bytesUsed, status);

        EXPECT_TRUE (msg.isMetaEvent());
        EXPECT_EQ (msg.getMetaEventLength(), (int) input.size() - 3);
        EXPECT_EQ (msg.getMetaEventType(), (int) input[1]);
    }
}

TEST (MidiMessageTests, DataConstructorWorksWithMalformedMetaEvents)
{
    const auto status = (uint8) 0x90;

    const auto runTest = [&] (const std::vector<uint8>& input)
    {
        int bytesUsed = 0;
        const MidiMessage msg (input.data(), (int) input.size(), bytesUsed, status);

        EXPECT_TRUE (msg.isMetaEvent());
        EXPECT_EQ (msg.getMetaEventLength(), jmax (0, (int) input.size() - 3));
        EXPECT_EQ (msg.getMetaEventType(), input.size() >= 2 ? input[1] : -1);
    };

    runTest ({ 0xff });

    for (const auto& input : metaEvents)
    {
        auto copy = input;
        copy[2] = 0x40; // Set the size of the message to more bytes than are present

        runTest (copy);
    }
}

//==============================================================================
// Constructor Tests
//==============================================================================
TEST (MidiMessageTests, DefaultConstructor)
{
    // Tests lines 121-126 (default constructor creates empty sysex)
    MidiMessage msg;

    EXPECT_TRUE (msg.isSysEx());
    EXPECT_EQ (msg.getRawDataSize(), 2);
    EXPECT_EQ (msg.getRawData()[0], 0xf0);
    EXPECT_EQ (msg.getRawData()[1], 0xf7);
}

TEST (MidiMessageTests, SingleByteConstructor)
{
    // Tests lines 139-147
    MidiMessage msg (0xf8, 1.5);

    EXPECT_EQ (msg.getRawDataSize(), 1);
    EXPECT_EQ (msg.getRawData()[0], 0xf8);
    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 1.5);
}

TEST (MidiMessageTests, TwoByteConstructor)
{
    // Tests lines 149-158
    MidiMessage msg (0xc0, 64, 2.5);

    EXPECT_EQ (msg.getRawDataSize(), 2);
    EXPECT_EQ (msg.getRawData()[0], 0xc0);
    EXPECT_EQ (msg.getRawData()[1], 64);
    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 2.5);
}

TEST (MidiMessageTests, ThreeByteConstructor)
{
    // Tests lines 160-170
    MidiMessage msg (0x90, 60, 100, 3.5);

    EXPECT_EQ (msg.getRawDataSize(), 3);
    EXPECT_EQ (msg.getRawData()[0], 0x90);
    EXPECT_EQ (msg.getRawData()[1], 60);
    EXPECT_EQ (msg.getRawData()[2], 100);
    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 3.5);
}

TEST (MidiMessageTests, CopyConstructor)
{
    // Tests lines 172-180
    MidiMessage original (0x90, 60, 100, 1.0);
    MidiMessage copy (original);

    EXPECT_EQ (copy.getRawDataSize(), original.getRawDataSize());
    EXPECT_EQ (copy.getRawData()[0], original.getRawData()[0]);
    EXPECT_EQ (copy.getRawData()[1], original.getRawData()[1]);
    EXPECT_EQ (copy.getRawData()[2], original.getRawData()[2]);
    EXPECT_DOUBLE_EQ (copy.getTimeStamp(), original.getTimeStamp());
}

TEST (MidiMessageTests, CopyConstructorWithNewTimestamp)
{
    // Tests lines 182-190
    MidiMessage original (0x90, 60, 100, 1.0);
    MidiMessage copy (original, 5.0);

    EXPECT_EQ (copy.getRawDataSize(), original.getRawDataSize());
    EXPECT_EQ (copy.getRawData()[0], original.getRawData()[0]);
    EXPECT_DOUBLE_EQ (copy.getTimeStamp(), 5.0);
}

TEST (MidiMessageTests, MoveConstructor)
{
    // Tests lines 316-322
    MidiMessage original (0x90, 60, 100, 1.0);
    MidiMessage moved (std::move (original));

    EXPECT_EQ (moved.getRawDataSize(), 3);
    EXPECT_EQ (moved.getRawData()[0], 0x90);
    EXPECT_EQ (original.getRawDataSize(), 0);
}

TEST (MidiMessageTests, CopyAssignment)
{
    // Tests lines 285-314
    MidiMessage msg1 (0x90, 60, 100);
    MidiMessage msg2 (0x80, 64, 0);

    msg2 = msg1;

    EXPECT_EQ (msg2.getRawDataSize(), msg1.getRawDataSize());
    EXPECT_EQ (msg2.getRawData()[0], msg1.getRawData()[0]);
}

TEST (MidiMessageTests, MoveAssignment)
{
    // Tests lines 324-331
    MidiMessage msg1 (0x90, 60, 100);
    MidiMessage msg2 (0x80, 64, 0);

    msg2 = std::move (msg1);

    EXPECT_EQ (msg2.getRawDataSize(), 3);
    EXPECT_EQ (msg1.getRawDataSize(), 0);
}

//==============================================================================
// Helper Function Tests
//==============================================================================
TEST (MidiMessageTests, FloatValueToMidiByte)
{
    // Tests lines 57-63
    EXPECT_EQ (MidiMessage::floatValueToMidiByte (0.0f), 0);
    EXPECT_EQ (MidiMessage::floatValueToMidiByte (0.5f), 64);
    EXPECT_EQ (MidiMessage::floatValueToMidiByte (1.0f), 127);
}

TEST (MidiMessageTests, PitchbendToPitchwheelPos)
{
    // Tests lines 65-74
    EXPECT_EQ (MidiMessage::pitchbendToPitchwheelPos (0.0f, 2.0f), 8192);
    EXPECT_EQ (MidiMessage::pitchbendToPitchwheelPos (2.0f, 2.0f), 16383);
    EXPECT_EQ (MidiMessage::pitchbendToPitchwheelPos (-2.0f, 2.0f), 0);
}

TEST (MidiMessageTests, GetMessageLengthFromFirstByte)
{
    // Tests lines 102-118
    EXPECT_EQ (MidiMessage::getMessageLengthFromFirstByte (0x80), 3); // Note off
    EXPECT_EQ (MidiMessage::getMessageLengthFromFirstByte (0x90), 3); // Note on
    EXPECT_EQ (MidiMessage::getMessageLengthFromFirstByte (0xc0), 2); // Program change
    EXPECT_EQ (MidiMessage::getMessageLengthFromFirstByte (0xe0), 3); // Pitch wheel
    EXPECT_EQ (MidiMessage::getMessageLengthFromFirstByte (0xf1), 2); // Quarter frame
    EXPECT_EQ (MidiMessage::getMessageLengthFromFirstByte (0xf8), 1); // Clock
}

//==============================================================================
// Timestamp Tests
//==============================================================================
TEST (MidiMessageTests, GetSetTimeStamp)
{
    MidiMessage msg (0x90, 60, 100);

    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 0.0);

    msg.setTimeStamp (5.5);
    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 5.5);
}

TEST (MidiMessageTests, AddToTimeStamp)
{
    MidiMessage msg (0x90, 60, 100, 1.0);

    msg.addToTimeStamp (2.5);
    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 3.5);
}

TEST (MidiMessageTests, WithTimeStamp)
{
    // Tests line 393-396
    MidiMessage msg (0x90, 60, 100, 1.0);
    MidiMessage newMsg = msg.withTimeStamp (5.0);

    EXPECT_DOUBLE_EQ (msg.getTimeStamp(), 1.0);
    EXPECT_DOUBLE_EQ (newMsg.getTimeStamp(), 5.0);
}

//==============================================================================
// Channel Tests
//==============================================================================
TEST (MidiMessageTests, GetChannel)
{
    // Tests lines 398-406
    MidiMessage msg1 (0x90, 60, 100); // Channel 1
    EXPECT_EQ (msg1.getChannel(), 1);

    MidiMessage msg2 (0x95, 60, 100); // Channel 6
    EXPECT_EQ (msg2.getChannel(), 6);

    MidiMessage msg3 (0xf0); // System message
    EXPECT_EQ (msg3.getChannel(), 0);
}

TEST (MidiMessageTests, IsForChannel)
{
    // Tests lines 408-416
    MidiMessage msg (0x90, 60, 100); // Channel 1

    EXPECT_TRUE (msg.isForChannel (1));
    EXPECT_FALSE (msg.isForChannel (2));
}

TEST (MidiMessageTests, SetChannel)
{
    // Tests lines 418-427
    MidiMessage msg (0x90, 60, 100); // Channel 1

    msg.setChannel (5);
    EXPECT_EQ (msg.getChannel(), 5);
}

//==============================================================================
// Note On/Off Tests
//==============================================================================
TEST (MidiMessageTests, IsNoteOn)
{
    // Tests lines 429-435
    MidiMessage noteOn (0x90, 60, 100);
    EXPECT_TRUE (noteOn.isNoteOn());
    EXPECT_TRUE (noteOn.isNoteOn (true));

    MidiMessage noteOnZeroVel (0x90, 60, 0);
    EXPECT_FALSE (noteOnZeroVel.isNoteOn());
    EXPECT_TRUE (noteOnZeroVel.isNoteOn (true));
}

TEST (MidiMessageTests, IsNoteOff)
{
    // Tests lines 437-443
    MidiMessage noteOff (0x80, 60, 0);
    EXPECT_TRUE (noteOff.isNoteOff());

    MidiMessage noteOnZeroVel (0x90, 60, 0);
    EXPECT_TRUE (noteOnZeroVel.isNoteOff (true));
    EXPECT_FALSE (noteOnZeroVel.isNoteOff (false));
}

TEST (MidiMessageTests, IsNoteOnOrOff)
{
    // Tests lines 445-449
    MidiMessage noteOn (0x90, 60, 100);
    MidiMessage noteOff (0x80, 60, 0);
    MidiMessage controller (0xb0, 7, 100);

    EXPECT_TRUE (noteOn.isNoteOnOrOff());
    EXPECT_TRUE (noteOff.isNoteOnOrOff());
    EXPECT_FALSE (controller.isNoteOnOrOff());
}

TEST (MidiMessageTests, GetNoteNumber)
{
    // Tests lines 451-454
    MidiMessage msg (0x90, 60, 100);
    EXPECT_EQ (msg.getNoteNumber(), 60);
}

TEST (MidiMessageTests, SetNoteNumber)
{
    // Tests lines 456-460
    MidiMessage msg (0x90, 60, 100);
    msg.setNoteNumber (64);
    EXPECT_EQ (msg.getNoteNumber(), 64);
}

TEST (MidiMessageTests, GetVelocity)
{
    // Tests lines 462-468
    MidiMessage noteOn (0x90, 60, 100);
    EXPECT_EQ (noteOn.getVelocity(), 100);

    MidiMessage controller (0xb0, 7, 100);
    EXPECT_EQ (controller.getVelocity(), 0);
}

TEST (MidiMessageTests, GetFloatVelocity)
{
    // Tests lines 470-473
    MidiMessage msg (0x90, 60, 127);
    EXPECT_FLOAT_EQ (msg.getFloatVelocity(), 1.0f);

    MidiMessage msg2 (0x90, 60, 64);
    EXPECT_NEAR (msg2.getFloatVelocity(), 0.5039f, 0.01f);
}

TEST (MidiMessageTests, SetVelocity)
{
    // Tests lines 475-479
    MidiMessage msg (0x90, 60, 100);
    msg.setVelocity (0.5f);
    EXPECT_EQ (msg.getVelocity(), 64);
}

TEST (MidiMessageTests, MultiplyVelocity)
{
    // Tests lines 481-488
    MidiMessage msg (0x90, 60, 100);
    msg.multiplyVelocity (0.5f);
    EXPECT_EQ (msg.getVelocity(), 50);
}

TEST (MidiMessageTests, NoteOnFactoryFloat)
{
    // Tests lines 628-631
    MidiMessage msg = MidiMessage::noteOn (1, 60, 0.5f);
    EXPECT_TRUE (msg.isNoteOn());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getNoteNumber(), 60);
    EXPECT_EQ (msg.getVelocity(), 64);
}

TEST (MidiMessageTests, NoteOnFactoryUint8)
{
    // Tests lines 618-626
    MidiMessage msg = MidiMessage::noteOn (1, 60, (uint8) 100);
    EXPECT_TRUE (msg.isNoteOn());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getNoteNumber(), 60);
    EXPECT_EQ (msg.getVelocity(), 100);
}

TEST (MidiMessageTests, NoteOffFactoryFloat)
{
    // Tests lines 643-646
    MidiMessage msg = MidiMessage::noteOff (1, 60, 0.5f);
    EXPECT_TRUE (msg.isNoteOff());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getNoteNumber(), 60);
}

TEST (MidiMessageTests, NoteOffFactoryUint8)
{
    // Tests lines 633-641
    MidiMessage msg = MidiMessage::noteOff (1, 60, (uint8) 64);
    EXPECT_TRUE (msg.isNoteOff());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getNoteNumber(), 60);
    EXPECT_EQ (msg.getVelocity(), 64);
}

TEST (MidiMessageTests, NoteOffFactoryNoVelocity)
{
    // Tests lines 648-654
    MidiMessage msg = MidiMessage::noteOff (1, 60);
    EXPECT_TRUE (msg.isNoteOff());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getNoteNumber(), 60);
    EXPECT_EQ (msg.getVelocity(), 0);
}

//==============================================================================
// Controller Tests
//==============================================================================
TEST (MidiMessageTests, IsController)
{
    // Tests lines 585-588
    MidiMessage controller (0xb0, 7, 100);
    EXPECT_TRUE (controller.isController());

    MidiMessage noteOn (0x90, 60, 100);
    EXPECT_FALSE (noteOn.isController());
}

TEST (MidiMessageTests, IsControllerOfType)
{
    // Tests lines 590-594
    MidiMessage controller (0xb0, 7, 100);
    EXPECT_TRUE (controller.isControllerOfType (7));
    EXPECT_FALSE (controller.isControllerOfType (10));
}

TEST (MidiMessageTests, GetControllerNumber)
{
    // Tests lines 596-600
    MidiMessage controller (0xb0, 7, 100);
    EXPECT_EQ (controller.getControllerNumber(), 7);
}

TEST (MidiMessageTests, GetControllerValue)
{
    // Tests lines 602-606
    MidiMessage controller (0xb0, 7, 100);
    EXPECT_EQ (controller.getControllerValue(), 100);
}

TEST (MidiMessageTests, ControllerEventFactory)
{
    // Tests lines 608-616
    MidiMessage msg = MidiMessage::controllerEvent (1, 7, 100);
    EXPECT_TRUE (msg.isController());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getControllerNumber(), 7);
    EXPECT_EQ (msg.getControllerValue(), 100);
}

TEST (MidiMessageTests, IsSustainPedalOn)
{
    // Tests line 533
    MidiMessage msg = MidiMessage::controllerEvent (1, 0x40, 64);
    EXPECT_TRUE (msg.isSustainPedalOn());
    EXPECT_FALSE (msg.isSustainPedalOff());
}

TEST (MidiMessageTests, IsSustainPedalOff)
{
    // Tests line 535
    MidiMessage msg = MidiMessage::controllerEvent (1, 0x40, 63);
    EXPECT_TRUE (msg.isSustainPedalOff());
    EXPECT_FALSE (msg.isSustainPedalOn());
}

TEST (MidiMessageTests, IsSostenutoPedalOn)
{
    // Tests line 537
    MidiMessage msg = MidiMessage::controllerEvent (1, 0x42, 64);
    EXPECT_TRUE (msg.isSostenutoPedalOn());
}

TEST (MidiMessageTests, IsSostenutoPedalOff)
{
    // Tests line 539
    MidiMessage msg = MidiMessage::controllerEvent (1, 0x42, 63);
    EXPECT_TRUE (msg.isSostenutoPedalOff());
}

TEST (MidiMessageTests, IsSoftPedalOn)
{
    // Tests line 541
    MidiMessage msg = MidiMessage::controllerEvent (1, 0x43, 64);
    EXPECT_TRUE (msg.isSoftPedalOn());
}

TEST (MidiMessageTests, IsSoftPedalOff)
{
    // Tests line 543
    MidiMessage msg = MidiMessage::controllerEvent (1, 0x43, 63);
    EXPECT_TRUE (msg.isSoftPedalOff());
}

TEST (MidiMessageTests, AllNotesOff)
{
    // Tests lines 656-659, 661-665
    MidiMessage msg = MidiMessage::allNotesOff (1);
    EXPECT_TRUE (msg.isAllNotesOff());
    EXPECT_EQ (msg.getControllerNumber(), 123);
}

TEST (MidiMessageTests, AllSoundOff)
{
    // Tests lines 667-670, 672-676
    MidiMessage msg = MidiMessage::allSoundOff (1);
    EXPECT_TRUE (msg.isAllSoundOff());
    EXPECT_EQ (msg.getControllerNumber(), 120);
}

TEST (MidiMessageTests, IsResetAllControllers)
{
    // Tests lines 678-682
    MidiMessage msg = MidiMessage::controllerEvent (1, 121, 0);
    EXPECT_TRUE (msg.isResetAllControllers());
}

TEST (MidiMessageTests, AllControllersOff)
{
    // Tests lines 684-687
    MidiMessage msg = MidiMessage::allControllersOff (1);
    EXPECT_TRUE (msg.isResetAllControllers());
}

//==============================================================================
// Program Change Tests
//==============================================================================
TEST (MidiMessageTests, IsProgramChange)
{
    // Tests lines 545-548
    MidiMessage msg (0xc0, 64);
    EXPECT_TRUE (msg.isProgramChange());
}

TEST (MidiMessageTests, GetProgramChangeNumber)
{
    // Tests lines 550-554
    MidiMessage msg (0xc0, 64);
    EXPECT_EQ (msg.getProgramChangeNumber(), 64);
}

TEST (MidiMessageTests, ProgramChangeFactory)
{
    // Tests lines 556-561
    MidiMessage msg = MidiMessage::programChange (1, 64);
    EXPECT_TRUE (msg.isProgramChange());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getProgramChangeNumber(), 64);
}

//==============================================================================
// Pitch Wheel Tests
//==============================================================================
TEST (MidiMessageTests, IsPitchWheel)
{
    // Tests lines 563-566
    MidiMessage msg (0xe0, 0, 64);
    EXPECT_TRUE (msg.isPitchWheel());
}

TEST (MidiMessageTests, GetPitchWheelValue)
{
    // Tests lines 568-573
    MidiMessage msg (0xe0, 0, 64);
    EXPECT_EQ (msg.getPitchWheelValue(), 8192);
}

TEST (MidiMessageTests, PitchWheelFactory)
{
    // Tests lines 575-583
    MidiMessage msg = MidiMessage::pitchWheel (1, 8192);
    EXPECT_TRUE (msg.isPitchWheel());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getPitchWheelValue(), 8192);
}

//==============================================================================
// Aftertouch Tests
//==============================================================================
TEST (MidiMessageTests, IsAftertouch)
{
    // Tests lines 490-493
    MidiMessage msg (0xa0, 60, 64);
    EXPECT_TRUE (msg.isAftertouch());
}

TEST (MidiMessageTests, GetAfterTouchValue)
{
    // Tests lines 495-499
    MidiMessage msg (0xa0, 60, 64);
    EXPECT_EQ (msg.getAfterTouchValue(), 64);
}

TEST (MidiMessageTests, AftertouchChangeFactory)
{
    // Tests lines 501-512
    MidiMessage msg = MidiMessage::aftertouchChange (1, 60, 64);
    EXPECT_TRUE (msg.isAftertouch());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getNoteNumber(), 60);
    EXPECT_EQ (msg.getAfterTouchValue(), 64);
}

//==============================================================================
// Channel Pressure Tests
//==============================================================================
TEST (MidiMessageTests, IsChannelPressure)
{
    // Tests lines 514-517
    MidiMessage msg (0xd0, 64);
    EXPECT_TRUE (msg.isChannelPressure());
}

TEST (MidiMessageTests, GetChannelPressureValue)
{
    // Tests lines 519-523
    MidiMessage msg (0xd0, 64);
    EXPECT_EQ (msg.getChannelPressureValue(), 64);
}

TEST (MidiMessageTests, ChannelPressureChangeFactory)
{
    // Tests lines 525-531
    MidiMessage msg = MidiMessage::channelPressureChange (1, 64);
    EXPECT_TRUE (msg.isChannelPressure());
    EXPECT_EQ (msg.getChannel(), 1);
    EXPECT_EQ (msg.getChannelPressureValue(), 64);
}

//==============================================================================
// SysEx Tests
//==============================================================================
TEST (MidiMessageTests, IsSysEx)
{
    // Tests lines 697-700
    MidiMessage msg;
    EXPECT_TRUE (msg.isSysEx());

    MidiMessage noteOn (0x90, 60, 100);
    EXPECT_FALSE (noteOn.isSysEx());
}

TEST (MidiMessageTests, CreateSysExMessage)
{
    // Tests lines 702-711
    uint8 data[] = { 0x01, 0x02, 0x03 };
    MidiMessage msg = MidiMessage::createSysExMessage (data, 3);

    EXPECT_TRUE (msg.isSysEx());
    EXPECT_EQ (msg.getSysExDataSize(), 3);
}

TEST (MidiMessageTests, CreateSysExMessageFromSpan)
{
    // Tests lines 713-716
    std::byte data[] = { std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0x03 } };
    Span<const std::byte> span (data, 3);
    MidiMessage msg = MidiMessage::createSysExMessage (span);

    EXPECT_TRUE (msg.isSysEx());
    EXPECT_EQ (msg.getSysExDataSize(), 3);
}

TEST (MidiMessageTests, GetSysExData)
{
    // Tests lines 718-721
    uint8 data[] = { 0x01, 0x02, 0x03 };
    MidiMessage msg = MidiMessage::createSysExMessage (data, 3);

    const uint8* sysexData = msg.getSysExData();
    EXPECT_NE (sysexData, nullptr);
    EXPECT_EQ (sysexData[0], 0x01);
    EXPECT_EQ (sysexData[1], 0x02);
    EXPECT_EQ (sysexData[2], 0x03);
}

//==============================================================================
// Meta Event Tests
//==============================================================================
TEST (MidiMessageTests, IsMetaEvent)
{
    // Tests line 729
    MidiMessage msg (0xff, 0x03, 0x00);
    EXPECT_TRUE (msg.isMetaEvent());
}

TEST (MidiMessageTests, IsActiveSense)
{
    // Tests line 731
    MidiMessage msg (0xfe);
    EXPECT_TRUE (msg.isActiveSense());
}

TEST (MidiMessageTests, GetMetaEventType)
{
    // Tests lines 733-737
    MidiMessage msg (0xff, 0x03, 0x00);
    EXPECT_EQ (msg.getMetaEventType(), 0x03);
}

TEST (MidiMessageTests, IsTrackMetaEvent)
{
    // Tests line 761
    MidiMessage msg (0xff, 0x00, 0x02, 0x00, 0x00);
    EXPECT_TRUE (msg.isTrackMetaEvent());
}

TEST (MidiMessageTests, IsEndOfTrackMetaEvent)
{
    // Tests lines 763, 949-952
    MidiMessage msg = MidiMessage::endOfTrack();
    EXPECT_TRUE (msg.isEndOfTrackMetaEvent());
}

TEST (MidiMessageTests, IsTextMetaEvent)
{
    // Tests lines 765-769
    MidiMessage msg (0xff, 0x01, 0x00);
    EXPECT_TRUE (msg.isTextMetaEvent());
}

TEST (MidiMessageTests, TextMetaEvent)
{
    // Tests lines 779-808
    MidiMessage msg = MidiMessage::textMetaEvent (1, "Test");
    EXPECT_TRUE (msg.isTextMetaEvent());
    EXPECT_EQ (msg.getMetaEventType(), 1);
}

TEST (MidiMessageTests, GetTextFromTextMetaEvent)
{
    // Tests lines 771-777
    MidiMessage msg = MidiMessage::textMetaEvent (1, "Hello");
    String text = msg.getTextFromTextMetaEvent();
    EXPECT_TRUE (text == "Hello");
}

TEST (MidiMessageTests, IsTrackNameEvent)
{
    // Tests lines 810-814
    MidiMessage msg = MidiMessage::textMetaEvent (3, "Track1");
    EXPECT_TRUE (msg.isTrackNameEvent());
}

TEST (MidiMessageTests, IsTempoMetaEvent)
{
    // Tests lines 816-820
    MidiMessage msg = MidiMessage::tempoMetaEvent (500000);
    EXPECT_TRUE (msg.isTempoMetaEvent());
}

TEST (MidiMessageTests, TempoMetaEvent)
{
    // Tests lines 882-885
    MidiMessage msg = MidiMessage::tempoMetaEvent (500000);
    EXPECT_TRUE (msg.isTempoMetaEvent());
    EXPECT_NEAR (msg.getTempoSecondsPerQuarterNote(), 0.5, 0.001);
}

TEST (MidiMessageTests, GetTempoSecondsPerQuarterNote)
{
    // Tests lines 834-845
    MidiMessage msg = MidiMessage::tempoMetaEvent (500000);
    EXPECT_NEAR (msg.getTempoSecondsPerQuarterNote(), 0.5, 0.001);
}

TEST (MidiMessageTests, GetTempoMetaEventTickLength)
{
    // Tests lines 847-880
    MidiMessage msg = MidiMessage::tempoMetaEvent (500000);
    double tickLength = msg.getTempoMetaEventTickLength (480);
    EXPECT_GT (tickLength, 0.0);
}

TEST (MidiMessageTests, IsMidiChannelMetaEvent)
{
    // Tests lines 822-826
    MidiMessage msg = MidiMessage::midiChannelMetaEvent (1);
    EXPECT_TRUE (msg.isMidiChannelMetaEvent());
}

TEST (MidiMessageTests, MidiChannelMetaEvent)
{
    // Tests lines 922-925
    MidiMessage msg = MidiMessage::midiChannelMetaEvent (5);
    EXPECT_TRUE (msg.isMidiChannelMetaEvent());
    EXPECT_EQ (msg.getMidiChannelMetaEventChannel(), 5);
}

TEST (MidiMessageTests, GetMidiChannelMetaEventChannel)
{
    // Tests lines 828-832
    MidiMessage msg = MidiMessage::midiChannelMetaEvent (5);
    EXPECT_EQ (msg.getMidiChannelMetaEventChannel(), 5);
}

TEST (MidiMessageTests, IsTimeSignatureMetaEvent)
{
    // Tests lines 887-891
    MidiMessage msg = MidiMessage::timeSignatureMetaEvent (4, 4);
    EXPECT_TRUE (msg.isTimeSignatureMetaEvent());
}

TEST (MidiMessageTests, TimeSignatureMetaEvent)
{
    // Tests lines 908-920
    MidiMessage msg = MidiMessage::timeSignatureMetaEvent (3, 4);
    EXPECT_TRUE (msg.isTimeSignatureMetaEvent());

    int num, denom;
    msg.getTimeSignatureInfo (num, denom);
    EXPECT_EQ (num, 3);
    EXPECT_EQ (denom, 4);
}

TEST (MidiMessageTests, GetTimeSignatureInfo)
{
    // Tests lines 893-906
    MidiMessage msg = MidiMessage::timeSignatureMetaEvent (6, 8);

    int numerator, denominator;
    msg.getTimeSignatureInfo (numerator, denominator);

    EXPECT_EQ (numerator, 6);
    EXPECT_EQ (denominator, 8);
}

TEST (MidiMessageTests, IsKeySignatureMetaEvent)
{
    // Tests lines 927-930
    MidiMessage msg = MidiMessage::keySignatureMetaEvent (2, false);
    EXPECT_TRUE (msg.isKeySignatureMetaEvent());
}

TEST (MidiMessageTests, KeySignatureMetaEvent)
{
    // Tests lines 942-947
    MidiMessage msg = MidiMessage::keySignatureMetaEvent (2, false);
    EXPECT_TRUE (msg.isKeySignatureMetaEvent());
    EXPECT_EQ (msg.getKeySignatureNumberOfSharpsOrFlats(), 2);
    EXPECT_TRUE (msg.isKeySignatureMajorKey());
}

TEST (MidiMessageTests, GetKeySignatureNumberOfSharpsOrFlats)
{
    // Tests lines 932-935
    MidiMessage msg = MidiMessage::keySignatureMetaEvent (-3, true);
    EXPECT_EQ (msg.getKeySignatureNumberOfSharpsOrFlats(), -3);
}

TEST (MidiMessageTests, IsKeySignatureMajorKey)
{
    // Tests lines 937-940
    MidiMessage major = MidiMessage::keySignatureMetaEvent (2, false);
    EXPECT_TRUE (major.isKeySignatureMajorKey());

    MidiMessage minor = MidiMessage::keySignatureMetaEvent (2, true);
    EXPECT_FALSE (minor.isKeySignatureMajorKey());
}

//==============================================================================
// System Real-Time Tests
//==============================================================================
TEST (MidiMessageTests, IsSongPositionPointer)
{
    // Tests line 955
    MidiMessage msg (0xf2, 0, 0);
    EXPECT_TRUE (msg.isSongPositionPointer());
}

TEST (MidiMessageTests, GetSongPositionPointerMidiBeat)
{
    // Tests lines 957-961
    MidiMessage msg (0xf2, 0, 64);
    EXPECT_EQ (msg.getSongPositionPointerMidiBeat(), 8192);
}

TEST (MidiMessageTests, SongPositionPointerFactory)
{
    // Tests lines 963-968
    MidiMessage msg = MidiMessage::songPositionPointer (1024);
    EXPECT_TRUE (msg.isSongPositionPointer());
    EXPECT_EQ (msg.getSongPositionPointerMidiBeat(), 1024);
}

TEST (MidiMessageTests, IsMidiStart)
{
    // Tests lines 970, 972
    MidiMessage msg = MidiMessage::midiStart();
    EXPECT_TRUE (msg.isMidiStart());
}

TEST (MidiMessageTests, IsMidiContinue)
{
    // Tests lines 974, 976
    MidiMessage msg = MidiMessage::midiContinue();
    EXPECT_TRUE (msg.isMidiContinue());
}

TEST (MidiMessageTests, IsMidiStop)
{
    // Tests lines 978, 980
    MidiMessage msg = MidiMessage::midiStop();
    EXPECT_TRUE (msg.isMidiStop());
}

TEST (MidiMessageTests, IsMidiClock)
{
    // Tests lines 982, 984
    MidiMessage msg = MidiMessage::midiClock();
    EXPECT_TRUE (msg.isMidiClock());
}

//==============================================================================
// SMPTE/MTC Tests
//==============================================================================
TEST (MidiMessageTests, IsQuarterFrame)
{
    // Tests line 986
    MidiMessage msg (0xf1, 0x00);
    EXPECT_TRUE (msg.isQuarterFrame());
}

TEST (MidiMessageTests, GetQuarterFrameSequenceNumber)
{
    // Tests line 988
    MidiMessage msg (0xf1, 0x35);
    EXPECT_EQ (msg.getQuarterFrameSequenceNumber(), 3);
}

TEST (MidiMessageTests, GetQuarterFrameValue)
{
    // Tests line 990
    MidiMessage msg (0xf1, 0x35);
    EXPECT_EQ (msg.getQuarterFrameValue(), 5);
}

TEST (MidiMessageTests, QuarterFrameFactory)
{
    // Tests lines 992-995
    MidiMessage msg = MidiMessage::quarterFrame (3, 5);
    EXPECT_TRUE (msg.isQuarterFrame());
    EXPECT_EQ (msg.getQuarterFrameSequenceNumber(), 3);
    EXPECT_EQ (msg.getQuarterFrameValue(), 5);
}

TEST (MidiMessageTests, IsFullFrame)
{
    // Tests lines 997-1006
    MidiMessage msg = MidiMessage::fullFrame (1, 2, 3, 4, MidiMessage::fps24);
    EXPECT_TRUE (msg.isFullFrame());
}

TEST (MidiMessageTests, FullFrameFactory)
{
    // Tests lines 1020-1023
    MidiMessage msg = MidiMessage::fullFrame (1, 30, 45, 10, MidiMessage::fps25);
    EXPECT_TRUE (msg.isFullFrame());

    int hours, minutes, seconds, frames;
    MidiMessage::SmpteTimecodeType timecode;
    msg.getFullFrameParameters (hours, minutes, seconds, frames, timecode);

    EXPECT_EQ (hours, 1);
    EXPECT_EQ (minutes, 30);
    EXPECT_EQ (seconds, 45);
    EXPECT_EQ (frames, 10);
    EXPECT_EQ (timecode, MidiMessage::fps25);
}

TEST (MidiMessageTests, GetFullFrameParameters)
{
    // Tests lines 1008-1018
    MidiMessage msg = MidiMessage::fullFrame (2, 15, 30, 20, MidiMessage::fps30);

    int hours, minutes, seconds, frames;
    MidiMessage::SmpteTimecodeType timecode;
    msg.getFullFrameParameters (hours, minutes, seconds, frames, timecode);

    EXPECT_EQ (hours, 2);
    EXPECT_EQ (minutes, 15);
    EXPECT_EQ (seconds, 30);
    EXPECT_EQ (frames, 20);
}

//==============================================================================
// MIDI Machine Control Tests
//==============================================================================
TEST (MidiMessageTests, IsMidiMachineControlMessage)
{
    // Tests lines 1025-1033
    MidiMessage msg = MidiMessage::midiMachineControlCommand (MidiMessage::mmc_stop);
    EXPECT_TRUE (msg.isMidiMachineControlMessage());
}

TEST (MidiMessageTests, GetMidiMachineControlCommand)
{
    // Tests lines 1035-1040
    MidiMessage msg = MidiMessage::midiMachineControlCommand (MidiMessage::mmc_play);
    EXPECT_EQ (msg.getMidiMachineControlCommand(), MidiMessage::mmc_play);
}

TEST (MidiMessageTests, MidiMachineControlCommandFactory)
{
    // Tests lines 1042-1045
    MidiMessage msg = MidiMessage::midiMachineControlCommand (MidiMessage::mmc_stop);
    EXPECT_TRUE (msg.isMidiMachineControlMessage());
    EXPECT_EQ (msg.getMidiMachineControlCommand(), MidiMessage::mmc_stop);
}

TEST (MidiMessageTests, IsMidiMachineControlGoto)
{
    // Tests lines 1048-1069
    MidiMessage msg = MidiMessage::midiMachineControlGoto (1, 30, 45, 10);

    int hours, minutes, seconds, frames;
    EXPECT_TRUE (msg.isMidiMachineControlGoto (hours, minutes, seconds, frames));
    EXPECT_EQ (hours, 1);
    EXPECT_EQ (minutes, 30);
    EXPECT_EQ (seconds, 45);
    EXPECT_EQ (frames, 10);
}

TEST (MidiMessageTests, MidiMachineControlGotoFactory)
{
    // Tests lines 1071-1074
    MidiMessage msg = MidiMessage::midiMachineControlGoto (2, 15, 30, 20);

    int hours, minutes, seconds, frames;
    EXPECT_TRUE (msg.isMidiMachineControlGoto (hours, minutes, seconds, frames));
    EXPECT_EQ (hours, 2);
}

//==============================================================================
// Note Name and Frequency Tests
//==============================================================================
TEST (MidiMessageTests, GetMidiNoteName)
{
    // Tests lines 1077-1094
    EXPECT_EQ (MidiMessage::getMidiNoteName (60, true, true, 3), "C3");
    EXPECT_EQ (MidiMessage::getMidiNoteName (61, true, true, 3), "C#3");
    EXPECT_EQ (MidiMessage::getMidiNoteName (61, false, true, 3), "Db3");
    EXPECT_EQ (MidiMessage::getMidiNoteName (60, true, false, 3), "C");
}

TEST (MidiMessageTests, GetMidiNoteInHertz)
{
    // Tests lines 1096-1099
    double freq = MidiMessage::getMidiNoteInHertz (69, 440.0);
    EXPECT_NEAR (freq, 440.0, 0.01);

    freq = MidiMessage::getMidiNoteInHertz (60, 440.0);
    EXPECT_NEAR (freq, 261.63, 0.01);
}

TEST (MidiMessageTests, IsMidiNoteBlack)
{
    // Tests lines 1101-1104
    EXPECT_FALSE (MidiMessage::isMidiNoteBlack (60)); // C
    EXPECT_TRUE (MidiMessage::isMidiNoteBlack (61));  // C#
    EXPECT_FALSE (MidiMessage::isMidiNoteBlack (62)); // D
    EXPECT_TRUE (MidiMessage::isMidiNoteBlack (63));  // D#
    EXPECT_FALSE (MidiMessage::isMidiNoteBlack (64)); // E
}

//==============================================================================
// Master Volume Test
//==============================================================================
TEST (MidiMessageTests, MasterVolume)
{
    // Tests lines 689-694
    MidiMessage msg = MidiMessage::masterVolume (0.5f);
    EXPECT_TRUE (msg.isSysEx());
    EXPECT_EQ (msg.getRawDataSize(), 8);
}

//==============================================================================
// Description Tests
//==============================================================================
TEST (MidiMessageTests, GetDescriptionNoteOn)
{
    // Tests lines 351-392 (getDescription)
    MidiMessage msg (0x90, 60, 100);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Note on"));
    EXPECT_TRUE (desc.contains ("Channel 1"));
}

TEST (MidiMessageTests, GetDescriptionNoteOff)
{
    MidiMessage msg (0x80, 60, 64);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Note off"));
}

TEST (MidiMessageTests, GetDescriptionProgramChange)
{
    MidiMessage msg = MidiMessage::programChange (1, 10);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Program change"));
}

TEST (MidiMessageTests, GetDescriptionPitchWheel)
{
    MidiMessage msg = MidiMessage::pitchWheel (1, 8192);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Pitch wheel"));
}

TEST (MidiMessageTests, GetDescriptionAftertouch)
{
    MidiMessage msg = MidiMessage::aftertouchChange (1, 60, 64);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Aftertouch"));
}

TEST (MidiMessageTests, GetDescriptionChannelPressure)
{
    MidiMessage msg = MidiMessage::channelPressureChange (1, 64);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Channel pressure"));
}

TEST (MidiMessageTests, GetDescriptionController)
{
    MidiMessage msg = MidiMessage::controllerEvent (1, 7, 100);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Controller"));
}

TEST (MidiMessageTests, GetDescriptionAllNotesOff)
{
    MidiMessage msg = MidiMessage::allNotesOff (1);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("All notes off"));
}

TEST (MidiMessageTests, GetDescriptionAllSoundOff)
{
    MidiMessage msg = MidiMessage::allSoundOff (1);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("All sound off"));
}

TEST (MidiMessageTests, GetDescriptionMetaEvent)
{
    MidiMessage msg (0xff, 0x03, 0x00);
    String desc = msg.getDescription();
    EXPECT_TRUE (desc.contains ("Meta event"));
}

//==============================================================================
// GM Instrument Name Tests
//==============================================================================
TEST (MidiMessageTests, GetGMInstrumentName)
{
    // Tests lines 1106-1243
    EXPECT_STREQ (MidiMessage::getGMInstrumentName (0), "Acoustic Grand Piano");
    EXPECT_STREQ (MidiMessage::getGMInstrumentName (24), "Acoustic Guitar (nylon)");
    EXPECT_NE (MidiMessage::getGMInstrumentName (127), nullptr);
    EXPECT_EQ (MidiMessage::getGMInstrumentName (128), nullptr);
}

TEST (MidiMessageTests, GetGMInstrumentBankName)
{
    // Tests lines 1245-1270
    EXPECT_STREQ (MidiMessage::getGMInstrumentBankName (0), "Piano");
    EXPECT_STREQ (MidiMessage::getGMInstrumentBankName (3), "Guitar");
    EXPECT_NE (MidiMessage::getGMInstrumentBankName (15), nullptr);
    EXPECT_EQ (MidiMessage::getGMInstrumentBankName (16), nullptr);
}

TEST (MidiMessageTests, GetRhythmInstrumentName)
{
    // Tests lines 1272-1328
    EXPECT_STREQ (MidiMessage::getRhythmInstrumentName (35), "Acoustic Bass Drum");
    EXPECT_STREQ (MidiMessage::getRhythmInstrumentName (42), "Closed Hi-Hat");
    EXPECT_NE (MidiMessage::getRhythmInstrumentName (81), nullptr);
    EXPECT_EQ (MidiMessage::getRhythmInstrumentName (34), nullptr);
    EXPECT_EQ (MidiMessage::getRhythmInstrumentName (82), nullptr);
}

TEST (MidiMessageTests, GetControllerName)
{
    // Tests lines 1330-1467
    EXPECT_STREQ (MidiMessage::getControllerName (0), "Bank Select");
    EXPECT_STREQ (MidiMessage::getControllerName (7), "Volume (coarse)");
    EXPECT_STREQ (MidiMessage::getControllerName (64), "Hold Pedal (on/off)");
    EXPECT_EQ (MidiMessage::getControllerName (3), nullptr);
}
