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

class MidiRPNDetectorTests : public ::testing::Test
{
};

TEST_F (MidiRPNDetectorTests, IndividualMSBIsParsedAs7Bit)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (2, 101, 0));
    EXPECT_FALSE (detector.tryParse (2, 100, 7));

    auto parsed = detector.tryParse (2, 6, 42);
    EXPECT_TRUE (parsed.has_value());

    EXPECT_EQ (parsed->channel, 2);
    EXPECT_EQ (parsed->parameterNumber, 7);
    EXPECT_EQ (parsed->value, 42);
    EXPECT_FALSE (parsed->isNRPN);
    EXPECT_FALSE (parsed->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, LSBWithoutPrecedingMSBIsIgnored)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (2, 101, 0));
    EXPECT_FALSE (detector.tryParse (2, 100, 7));
    EXPECT_FALSE (detector.tryParse (2, 38, 42));
}

TEST_F (MidiRPNDetectorTests, LSBFollowingMSBIsParsedAs14Bit)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (1, 101, 2));
    EXPECT_FALSE (detector.tryParse (1, 100, 44));

    EXPECT_TRUE (detector.tryParse (1, 6, 1).has_value());

    auto lsbParsed = detector.tryParse (1, 38, 94);
    EXPECT_TRUE (lsbParsed.has_value());

    EXPECT_EQ (lsbParsed->channel, 1);
    EXPECT_EQ (lsbParsed->parameterNumber, 300);
    EXPECT_EQ (lsbParsed->value, 222);
    EXPECT_FALSE (lsbParsed->isNRPN);
    EXPECT_TRUE (lsbParsed->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, MultipleLSBFollowingMSBReuseTheMSB)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (1, 101, 2));
    EXPECT_FALSE (detector.tryParse (1, 100, 43));

    EXPECT_TRUE (detector.tryParse (1, 6, 1).has_value());

    EXPECT_TRUE (detector.tryParse (1, 38, 94).has_value());
    EXPECT_TRUE (detector.tryParse (1, 38, 95).has_value());
    EXPECT_TRUE (detector.tryParse (1, 38, 96).has_value());

    auto lsbParsed = detector.tryParse (1, 38, 97);
    EXPECT_TRUE (lsbParsed.has_value());

    EXPECT_EQ (lsbParsed->channel, 1);
    EXPECT_EQ (lsbParsed->parameterNumber, 299);
    EXPECT_EQ (lsbParsed->value, 225);
    EXPECT_FALSE (lsbParsed->isNRPN);
    EXPECT_TRUE (lsbParsed->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, SendingNewMSBResetsTheLSB)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (1, 101, 3));
    EXPECT_FALSE (detector.tryParse (1, 100, 43));

    EXPECT_TRUE (detector.tryParse (1, 6, 1).has_value());
    EXPECT_TRUE (detector.tryParse (1, 38, 94).has_value());

    auto newMsb = detector.tryParse (1, 6, 2);
    EXPECT_TRUE (newMsb.has_value());

    EXPECT_EQ (newMsb->channel, 1);
    EXPECT_EQ (newMsb->parameterNumber, 427);
    EXPECT_EQ (newMsb->value, 2);
    EXPECT_FALSE (newMsb->isNRPN);
    EXPECT_FALSE (newMsb->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, RPNsOnMultipleChannelsSimultaneously)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (1, 100, 44));
    EXPECT_FALSE (detector.tryParse (2, 101, 0));
    EXPECT_FALSE (detector.tryParse (1, 101, 2));
    EXPECT_FALSE (detector.tryParse (2, 100, 7));
    EXPECT_TRUE (detector.tryParse (1, 6, 1).has_value());

    auto channelTwo = detector.tryParse (2, 6, 42);
    EXPECT_TRUE (channelTwo.has_value());

    EXPECT_EQ (channelTwo->channel, 2);
    EXPECT_EQ (channelTwo->parameterNumber, 7);
    EXPECT_EQ (channelTwo->value, 42);
    EXPECT_FALSE (channelTwo->isNRPN);
    EXPECT_FALSE (channelTwo->is14BitValue);

    auto channelOne = detector.tryParse (1, 38, 94);
    EXPECT_TRUE (channelOne.has_value());

    EXPECT_EQ (channelOne->channel, 1);
    EXPECT_EQ (channelOne->parameterNumber, 300);
    EXPECT_EQ (channelOne->value, 222);
    EXPECT_FALSE (channelOne->isNRPN);
    EXPECT_TRUE (channelOne->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, RPNWithValueWithin7BitRange)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (16, 100, 0));
    EXPECT_FALSE (detector.tryParse (16, 101, 0));
    EXPECT_TRUE (detector.tryParse (16, 6, 0).has_value());

    auto parsed = detector.tryParse (16, 38, 3);
    EXPECT_TRUE (parsed.has_value());

    EXPECT_EQ (parsed->channel, 16);
    EXPECT_EQ (parsed->parameterNumber, 0);
    EXPECT_EQ (parsed->value, 3);
    EXPECT_FALSE (parsed->isNRPN);
    EXPECT_TRUE (parsed->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, InvalidRPNWrongOrder)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (2, 6, 42));
    EXPECT_FALSE (detector.tryParse (2, 101, 0));
    EXPECT_FALSE (detector.tryParse (2, 100, 7));
}

TEST_F (MidiRPNDetectorTests, RPNInterspersedWithUnrelatedCCMessages)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (16, 3, 80));
    EXPECT_FALSE (detector.tryParse (16, 100, 0));
    EXPECT_FALSE (detector.tryParse (16, 4, 81));
    EXPECT_FALSE (detector.tryParse (16, 101, 0));
    EXPECT_FALSE (detector.tryParse (16, 5, 82));
    EXPECT_FALSE (detector.tryParse (16, 5, 83));
    EXPECT_TRUE (detector.tryParse (16, 6, 0).has_value());
    EXPECT_FALSE (detector.tryParse (16, 4, 84).has_value());
    EXPECT_FALSE (detector.tryParse (16, 3, 85).has_value());

    auto parsed = detector.tryParse (16, 38, 3);
    EXPECT_TRUE (parsed.has_value());

    EXPECT_EQ (parsed->channel, 16);
    EXPECT_EQ (parsed->parameterNumber, 0);
    EXPECT_EQ (parsed->value, 3);
    EXPECT_FALSE (parsed->isNRPN);
    EXPECT_TRUE (parsed->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, NRPNTest)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (1, 98, 44));
    EXPECT_FALSE (detector.tryParse (1, 99, 2));
    EXPECT_TRUE (detector.tryParse (1, 6, 1).has_value());

    auto parsed = detector.tryParse (1, 38, 94);
    EXPECT_TRUE (parsed.has_value());

    EXPECT_EQ (parsed->channel, 1);
    EXPECT_EQ (parsed->parameterNumber, 300);
    EXPECT_EQ (parsed->value, 222);
    EXPECT_TRUE (parsed->isNRPN);
    EXPECT_TRUE (parsed->is14BitValue);
}

TEST_F (MidiRPNDetectorTests, ResetTest)
{
    MidiRPNDetector detector;
    EXPECT_FALSE (detector.tryParse (2, 101, 0));
    detector.reset();
    EXPECT_FALSE (detector.tryParse (2, 100, 7));
    EXPECT_FALSE (detector.tryParse (2, 6, 42));
}

// Generator tests
class MidiRPNGeneratorTests : public ::testing::Test
{
protected:
    void expectContainsRPN (const MidiBuffer& midiBuffer,
                            int channel,
                            int parameterNumber,
                            int value,
                            bool isNRPN,
                            bool is14BitValue)
    {
        MidiRPNMessage expected = { channel, parameterNumber, value, isNRPN, is14BitValue };
        expectContainsRPN (midiBuffer, expected);
    }

    void expectContainsRPN (const MidiBuffer& midiBuffer, MidiRPNMessage expected)
    {
        std::optional<MidiRPNMessage> result;
        MidiRPNDetector detector;

        for (const auto metadata : midiBuffer)
        {
            const auto midiMessage = metadata.getMessage();

            result = detector.tryParse (midiMessage.getChannel(),
                                        midiMessage.getControllerNumber(),
                                        midiMessage.getControllerValue());
        }

        EXPECT_TRUE (result.has_value());
        EXPECT_EQ (result->channel, expected.channel);
        EXPECT_EQ (result->parameterNumber, expected.parameterNumber);
        EXPECT_EQ (result->value, expected.value);
        EXPECT_EQ (result->isNRPN, expected.isNRPN);
        EXPECT_EQ (result->is14BitValue, expected.is14BitValue);
    }
};

TEST_F (MidiRPNGeneratorTests, GeneratingRPNAndNRPN)
{
    {
        MidiBuffer buffer = MidiRPNGenerator::generate (1, 23, 1337, true, true);
        expectContainsRPN (buffer, 1, 23, 1337, true, true);
    }
    {
        MidiBuffer buffer = MidiRPNGenerator::generate (16, 101, 34, false, false);
        expectContainsRPN (buffer, 16, 101, 34, false, false);
    }
    {
        MidiRPNMessage message = { 16, 101, 34, false, false };
        MidiBuffer buffer = MidiRPNGenerator::generate (message);
        expectContainsRPN (buffer, message);
    }
}