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

#include <gtest/gtest.h>

#include <yup_audio_gui/yup_audio_gui.h>

using namespace yup;

namespace
{

StringArray makeChannelNames (int count)
{
    StringArray names;
    for (int i = 0; i < count; ++i)
        names.add ("Channel " + String (i + 1));
    return names;
}

} // namespace

//==============================================================================
class ChannelSectionTests : public ::testing::Test
{
};

TEST_F (ChannelSectionTests, ConstructsWithoutCrash)
{
    ChannelSection section;
    EXPECT_EQ (section.getActiveChannels(), BigInteger {});
}

TEST_F (ChannelSectionTests, PopulateReturnsCorrectActiveMask)
{
    ChannelSection section;
    BigInteger active;
    active.setBit (0);
    active.setBit (2);

    section.populate (makeChannelNames (4), active);

    EXPECT_EQ (section.getActiveChannels(), active);
}

TEST_F (ChannelSectionTests, PopulateWithEmptyChannelsDoesNotCrash)
{
    ChannelSection section;
    section.populate ({}, {});
    EXPECT_EQ (section.getActiveChannels(), BigInteger {});
}

TEST_F (ChannelSectionTests, SetTextDoesNotCrash)
{
    ChannelSection section;
    section.setText ("Active output channels:", dontSendNotification);
}

//==============================================================================
class RateBufferSelectorTests : public ::testing::Test
{
};

TEST_F (RateBufferSelectorTests, ConstructsWithoutCrash)
{
    RateBufferSelector selector;
    EXPECT_EQ (selector.getNumChildComponents(), 4);
}

TEST_F (RateBufferSelectorTests, PopulateWithEmptyArraysDoesNotCrash)
{
    RateBufferSelector selector;
    selector.populate ({}, 0.0, {}, 0);
}

TEST_F (RateBufferSelectorTests, PopulateWithValidDataDoesNotCrash)
{
    RateBufferSelector selector;
    selector.populate ({ 44100.0, 48000.0 }, 48000.0, { 256, 512 }, 512);
}

//==============================================================================
class MidiSectionTests : public ::testing::Test
{
};

TEST_F (MidiSectionTests, ConstructsWithoutCrash)
{
    MidiSection section;
}

TEST_F (MidiSectionTests, PopulateInputsWithEmptyListDoesNotCrash)
{
    MidiSection section;
    section.populateInputs ({}, {});
}

TEST_F (MidiSectionTests, PopulateOutputWithNoneSelectedDoesNotCrash)
{
    MidiSection section;
    section.populateOutput ({}, {});
}

//==============================================================================
class AudioDeviceManagerPanelTests : public ::testing::Test
{
};

TEST_F (AudioDeviceManagerPanelTests, ConstructsWithUnopenedManagerWithoutCrash)
{
    AudioDeviceManager manager;
    AudioDeviceManagerPanel panel (manager);
    EXPECT_GT (panel.getNumChildComponents(), 0);
}

TEST_F (AudioDeviceManagerPanelTests, HasExpectedChildComponentCount)
{
    AudioDeviceManager manager;
    AudioDeviceManagerPanel panel (manager);
    // 6 direct children: type selector, IO selector, 2x channel section,
    // rate+buffer selector, MIDI section
    EXPECT_EQ (panel.getNumChildComponents(), 6);
}
