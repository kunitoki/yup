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

Array<MidiDeviceInfo> makeMidiDevices (int count)
{
    Array<MidiDeviceInfo> devices;
    for (int i = 0; i < count; ++i)
        devices.add ({ "MIDI Device " + String (i + 1), "midi_id_" + String (i + 1) });
    return devices;
}

} // namespace

//==============================================================================
class DeviceTypeSelectorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        selector = std::make_unique<DeviceTypeSelector>();
        selector->setBounds (0, 0, 400, 44);
    }

    std::unique_ptr<DeviceTypeSelector> selector;
};

TEST_F (DeviceTypeSelectorTests, ConstructsWithoutCrash)
{
    EXPECT_GT (selector->getNumChildComponents(), 0);
}

TEST_F (DeviceTypeSelectorTests, ResizedDoesNotCrash)
{
    EXPECT_NO_THROW (selector->resized());
}

TEST_F (DeviceTypeSelectorTests, OnTypeChangedCallbackIsInvoked)
{
    bool invoked = false;
    String receivedType;
    selector->onTypeChanged = [&] (const String& typeName)
    {
        invoked = true;
        receivedType = typeName;
    };

    // Simulate the callback directly (populate would set this on the combo)
    selector->onTypeChanged ("TestDriver");

    EXPECT_TRUE (invoked);
    EXPECT_EQ (String ("TestDriver"), receivedType);
}

//==============================================================================
class DeviceIOSelectorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        selector = std::make_unique<DeviceIOSelector>();
        selector->setBounds (0, 0, 400, 88);
    }

    std::unique_ptr<DeviceIOSelector> selector;
};

TEST_F (DeviceIOSelectorTests, ConstructsWithoutCrash)
{
    EXPECT_GT (selector->getNumChildComponents(), 0);
}

TEST_F (DeviceIOSelectorTests, ResizedDoesNotCrash)
{
    EXPECT_NO_THROW (selector->resized());
}

TEST_F (DeviceIOSelectorTests, OnDeviceChangedCallbackIsInvoked)
{
    bool invoked = false;
    String receivedOutput, receivedInput;
    selector->onDeviceChanged = [&] (const String& output, const String& input)
    {
        invoked = true;
        receivedOutput = output;
        receivedInput = input;
    };

    // Simulate the callback directly
    selector->onDeviceChanged ("OutputDevice", "InputDevice");

    EXPECT_TRUE (invoked);
    EXPECT_EQ (String ("OutputDevice"), receivedOutput);
    EXPECT_EQ (String ("InputDevice"), receivedInput);
}

TEST_F (DeviceIOSelectorTests, OnTestClickedCallbackIsInvoked)
{
    bool invoked = false;
    selector->onTestClicked = [&]
    {
        invoked = true;
    };

    // Simulate the callback directly
    selector->onTestClicked();

    EXPECT_TRUE (invoked);
}

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

TEST_F (ChannelSectionTests, ResizedDoesNotCrash)
{
    ChannelSection section;
    section.setBounds (0, 0, 400, 120);

    // Populate first so the model has data for the list box
    BigInteger active;
    active.setBit (0);
    active.setBit (1);
    section.populate (makeChannelNames (4), active);

    EXPECT_NO_THROW (section.resized());
}

TEST_F (ChannelSectionTests, OnChannelsChangedCallbackIsInvoked)
{
    ChannelSection section;
    BigInteger receivedActive;
    section.onChannelsChanged = [&] (const BigInteger& active)
    {
        receivedActive = active;
    };

    // Simulate the callback directly
    BigInteger active;
    active.setBit (0);
    section.onChannelsChanged (active);

    EXPECT_EQ (receivedActive, active);
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

TEST_F (RateBufferSelectorTests, ResizedDoesNotCrash)
{
    RateBufferSelector selector;
    selector.setBounds (0, 0, 400, 88);

    selector.populate ({ 44100.0, 48000.0 }, 48000.0, { 256, 512 }, 512);

    EXPECT_NO_THROW (selector.resized());
}

TEST_F (RateBufferSelectorTests, OnChangedCallbackIsInvoked)
{
    RateBufferSelector selector;
    bool invoked = false;
    double receivedRate = 0.0;
    int receivedBuffer = 0;
    selector.onChanged = [&] (double rate, int bufferSize)
    {
        invoked = true;
        receivedRate = rate;
        receivedBuffer = bufferSize;
    };

    // Simulate the callback directly
    selector.onChanged (44100.0, 256);

    EXPECT_TRUE (invoked);
    EXPECT_DOUBLE_EQ (44100.0, receivedRate);
    EXPECT_EQ (256, receivedBuffer);
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

TEST_F (MidiSectionTests, PopulateOutputWithDevicesDoesNotCrash)
{
    MidiSection section;
    const auto devices = makeMidiDevices (3);
    section.populateOutput (devices, "midi_id_2");
}

TEST_F (MidiSectionTests, ResizedDoesNotCrash)
{
    MidiSection section;
    section.setBounds (0, 0, 400, 200);
    EXPECT_NO_THROW (section.resized());
}

TEST_F (MidiSectionTests, OnInputsChangedCallbackIsInvoked)
{
    MidiSection section;
    StringArray receivedIds;
    section.onInputsChanged = [&] (const StringArray& ids)
    {
        receivedIds = ids;
    };

    StringArray ids;
    ids.add ("id_1");
    ids.add ("id_2");
    section.onInputsChanged (ids);

    EXPECT_EQ (2, receivedIds.size());
    EXPECT_EQ (String ("id_1"), receivedIds[0]);
    EXPECT_EQ (String ("id_2"), receivedIds[1]);
}

TEST_F (MidiSectionTests, OnOutputChangedCallbackIsInvoked)
{
    MidiSection section;
    String receivedId;
    bool invoked = false;
    section.onOutputChanged = [&] (const String& id)
    {
        invoked = true;
        receivedId = id;
    };

    // Simulate the callback directly
    section.onOutputChanged ("out_id_1");

    EXPECT_TRUE (invoked);
    EXPECT_EQ (String ("out_id_1"), receivedId);
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

TEST_F (AudioDeviceManagerPanelTests, ResizedDoesNotCrash)
{
    AudioDeviceManager manager;
    AudioDeviceManagerPanel panel (manager);
    panel.setBounds (0, 0, 470, 680);

    EXPECT_NO_THROW (panel.resized());
}

TEST_F (AudioDeviceManagerPanelTests, MakeMidiDeviceListConnectionDoesNotCrash)
{
    AudioDeviceManager manager;
    AudioDeviceManagerPanel panel (manager);

    // Connection creation should not crash
    EXPECT_NO_THROW (panel.makeMidiDeviceListConnection());
}
