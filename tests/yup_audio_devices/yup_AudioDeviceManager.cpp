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

#include <yup_audio_devices/yup_audio_devices.h>

using namespace yup;

namespace
{
struct Restartable
{
    virtual ~Restartable() = default;
    virtual void restart (double newSr, int newBs) = 0;
};

class MockDevice final : public AudioIODevice
    , private Restartable
{
public:
    MockDevice (ListenerList<Restartable>& l, String typeNameIn, String outNameIn, String inNameIn)
        : AudioIODevice ("mock", typeNameIn)
        , listeners (l)
        , outName (outNameIn)
        , inName (inNameIn)
    {
        listeners.add (this);
    }

    ~MockDevice() override
    {
        listeners.remove (this);
    }

    StringArray getOutputChannelNames() override { return { "o1", "o2", "o3" }; }

    StringArray getInputChannelNames() override { return { "i1", "i2", "i3" }; }

    Array<double> getAvailableSampleRates() override { return { 44100.0, 48000.0 }; }

    Array<int> getAvailableBufferSizes() override { return { 128, 256 }; }

    int getDefaultBufferSize() override { return 128; }

    String open (const BigInteger& inputs, const BigInteger& outputs, double sr, int bs) override
    {
        inChannels = inputs;
        outChannels = outputs;
        sampleRate = sr;
        blockSize = bs;
        on = true;
        return {};
    }

    void close() override { on = false; }

    bool isOpen() override { return on; }

    void start (AudioIODeviceCallback* c) override
    {
        callback = c;
        callback->audioDeviceAboutToStart (this);
        playing = true;
    }

    void stop() override
    {
        playing = false;
        callback->audioDeviceStopped();
    }

    bool isPlaying() override { return playing; }

    String getLastError() override { return {}; }

    int getCurrentBufferSizeSamples() override { return blockSize; }

    double getCurrentSampleRate() override { return sampleRate; }

    int getCurrentBitDepth() override { return 16; }

    BigInteger getActiveOutputChannels() const override { return outChannels; }

    BigInteger getActiveInputChannels() const override { return inChannels; }

    int getOutputLatencyInSamples() override { return 0; }

    int getInputLatencyInSamples() override { return 0; }

private:
    void restart (double newSr, int newBs) override
    {
        stop();
        close();
        open (inChannels, outChannels, newSr, newBs);
        start (callback);
    }

    ListenerList<Restartable>& listeners;
    AudioIODeviceCallback* callback = nullptr;
    String outName, inName;
    BigInteger outChannels, inChannels;
    double sampleRate = 0.0;
    int blockSize = 0;
    bool on = false, playing = false;
};

class MockDeviceType final : public AudioIODeviceType
{
public:
    explicit MockDeviceType (String kind)
        : MockDeviceType (std::move (kind), { "a", "b", "c" }, { "x", "y", "z" })
    {
    }

    MockDeviceType (String kind, StringArray inputNames, StringArray outputNames)
        : AudioIODeviceType (std::move (kind))
        , inNames (std::move (inputNames))
        , outNames (std::move (outputNames))
    {
    }

    ~MockDeviceType() override
    {
        // A Device outlived its DeviceType!
        jassert (listeners.isEmpty());
    }

    void scanForDevices() override {}

    StringArray getDeviceNames (bool isInput) const override
    {
        return getNames (isInput);
    }

    int getDefaultDeviceIndex (bool) const override { return 0; }

    int getIndexOfDevice (AudioIODevice* device, bool isInput) const override
    {
        return getNames (isInput).indexOf (device->getName());
    }

    bool hasSeparateInputsAndOutputs() const override { return true; }

    AudioIODevice* createDevice (const String& outputName, const String& inputName) override
    {
        if (inNames.contains (inputName) || outNames.contains (outputName))
            return new MockDevice (listeners, getTypeName(), outputName, inputName);

        return nullptr;
    }

    // Call this to emulate the device restarting itself with new settings.
    // This might happen e.g. when a user changes the ASIO settings.
    void restartDevices (double newSr, int newBs)
    {
        listeners.call ([&] (auto& l)
        {
            return l.restart (newSr, newBs);
        });
    }

private:
    const StringArray& getNames (bool isInput) const { return isInput ? inNames : outNames; }

    const StringArray inNames, outNames;
    ListenerList<Restartable> listeners;
};

class MockCallback final : public AudioIODeviceCallback
{
public:
    std::function<void()> callback;
    std::function<void()> aboutToStart;
    std::function<void()> stopped;
    std::function<void()> error;

    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const*,
                                           int,
                                           int,
                                           const AudioIODeviceCallbackContext&) override
    {
        NullCheckedInvocation::invoke (callback);
    }

    void audioDeviceAboutToStart (AudioIODevice*) override { NullCheckedInvocation::invoke (aboutToStart); }

    void audioDeviceStopped() override { NullCheckedInvocation::invoke (stopped); }

    void audioDeviceError (const String&) override { NullCheckedInvocation::invoke (error); }
};
} // namespace

class AudioDeviceManagerTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        MessageManager::getInstance()->setCurrentThreadAsMessageThread();
    }

    void TearDown() override
    {
        MessageManager::deleteInstance();
    }

    void initialiseWithDefaultDevices (AudioDeviceManager& manager)
    {
        manager.initialiseWithDefaultDevices (2, 2);
        const auto& setup = manager.getAudioDeviceSetup();

        EXPECT_EQ (setup.inputChannels.countNumberOfSetBits(), 2);
        EXPECT_EQ (setup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (setup.useDefaultInputChannels);
        EXPECT_TRUE (setup.useDefaultOutputChannels);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void disableInputChannelsButLeaveDeviceOpen (AudioDeviceManager& manager)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.inputChannels.clear();
        setup.useDefaultInputChannels = false;

        EXPECT_TRUE (manager.setAudioDeviceSetup (setup, true).isEmpty());

        const auto newSetup = manager.getAudioDeviceSetup();
        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 0);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (! newSetup.useDefaultInputChannels);
        EXPECT_TRUE (newSetup.useDefaultOutputChannels);

        EXPECT_EQ (newSetup.inputDeviceName, setup.inputDeviceName);
        EXPECT_EQ (newSetup.outputDeviceName, setup.outputDeviceName);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void selectANewInputDevice (AudioDeviceManager& manager)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.inputDeviceName = "b";

        EXPECT_TRUE (manager.setAudioDeviceSetup (setup, true).isEmpty());

        const auto newSetup = manager.getAudioDeviceSetup();
        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 0);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (! newSetup.useDefaultInputChannels);
        EXPECT_TRUE (newSetup.useDefaultOutputChannels);

        EXPECT_EQ (newSetup.inputDeviceName, setup.inputDeviceName);
        EXPECT_EQ (newSetup.outputDeviceName, setup.outputDeviceName);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void disableInputDevice (AudioDeviceManager& manager)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.inputDeviceName = "";

        EXPECT_TRUE (manager.setAudioDeviceSetup (setup, true).isEmpty());

        const auto newSetup = manager.getAudioDeviceSetup();
        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 0);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (! newSetup.useDefaultInputChannels);
        EXPECT_TRUE (newSetup.useDefaultOutputChannels);

        EXPECT_EQ (newSetup.inputDeviceName, setup.inputDeviceName);
        EXPECT_EQ (newSetup.outputDeviceName, setup.outputDeviceName);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void reenableInputDeviceWithNoChannels (AudioDeviceManager& manager)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.inputDeviceName = "a";

        EXPECT_TRUE (manager.setAudioDeviceSetup (setup, true).isEmpty());

        const auto newSetup = manager.getAudioDeviceSetup();
        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 0);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (! newSetup.useDefaultInputChannels);
        EXPECT_TRUE (newSetup.useDefaultOutputChannels);

        EXPECT_EQ (newSetup.inputDeviceName, setup.inputDeviceName);
        EXPECT_EQ (newSetup.outputDeviceName, setup.outputDeviceName);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void enableInputChannels (AudioDeviceManager& manager)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.inputDeviceName = manager.getCurrentDeviceTypeObject()->getDeviceNames (true)[0];
        setup.inputChannels = 3;
        setup.useDefaultInputChannels = false;

        EXPECT_TRUE (manager.setAudioDeviceSetup (setup, true).isEmpty());

        const auto newSetup = manager.getAudioDeviceSetup();
        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (! newSetup.useDefaultInputChannels);
        EXPECT_TRUE (newSetup.useDefaultOutputChannels);

        EXPECT_EQ (newSetup.inputDeviceName, setup.inputDeviceName);
        EXPECT_EQ (newSetup.outputDeviceName, setup.outputDeviceName);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void switchDeviceType (AudioDeviceManager& manager)
    {
        const auto oldSetup = manager.getAudioDeviceSetup();

        EXPECT_EQ (manager.getCurrentAudioDeviceType(), String (mockAName));

        manager.setCurrentAudioDeviceType (mockBName, true);

        EXPECT_EQ (manager.getCurrentAudioDeviceType(), String (mockBName));

        const auto newSetup = manager.getAudioDeviceSetup();

        EXPECT_TRUE (newSetup.outputDeviceName.isNotEmpty());
        // We had no channels enabled, which means we don't need to open a new input device
        EXPECT_TRUE (newSetup.inputDeviceName.isEmpty());

        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 0);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
    }

    void closeDeviceByRequestingEmptyNames (AudioDeviceManager& manager)
    {
        auto setup = manager.getAudioDeviceSetup();
        setup.inputDeviceName = "";
        setup.outputDeviceName = "";

        EXPECT_TRUE (manager.setAudioDeviceSetup (setup, true).isEmpty());

        const auto newSetup = manager.getAudioDeviceSetup();
        EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);
        EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);

        EXPECT_TRUE (newSetup.inputDeviceName.isEmpty());
        EXPECT_TRUE (newSetup.outputDeviceName.isEmpty());

        EXPECT_TRUE (manager.getCurrentAudioDevice() == nullptr);
    }

    const String mockAName = "mockA";
    const String mockBName = "mockB";
    const String emptyName = "empty";

    void initialiseManager (AudioDeviceManager& manager)
    {
        manager.addAudioDeviceType (std::make_unique<MockDeviceType> (mockAName));
        manager.addAudioDeviceType (std::make_unique<MockDeviceType> (mockBName));
    }

    void initialiseManagerWithEmptyDeviceType (AudioDeviceManager& manager)
    {
        manager.addAudioDeviceType (std::make_unique<MockDeviceType> (emptyName, StringArray {}, StringArray {}));
        initialiseManager (manager);
    }

    void initialiseManagerWithDifferentDeviceNames (AudioDeviceManager& manager)
    {
        manager.addAudioDeviceType (std::make_unique<MockDeviceType> ("foo",
                                                                      StringArray { "foo in a", "foo in b" },
                                                                      StringArray { "foo out a", "foo out b" }));

        manager.addAudioDeviceType (std::make_unique<MockDeviceType> ("bar",
                                                                      StringArray { "bar in a", "bar in b" },
                                                                      StringArray { "bar out a", "bar out b" }));
    }
};

TEST_F (AudioDeviceManagerTests, InitialiseNonEmptyDeviceName)
{
    AudioDeviceManager manager;
    initialiseManager (manager);

    EXPECT_EQ (manager.getAvailableDeviceTypes().size(), 2);

    AudioDeviceManager::AudioDeviceSetup setup;
    setup.outputDeviceName = "z";
    setup.inputDeviceName = "c";

    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, String {}, &setup).isEmpty());

    const auto& newSetup = manager.getAudioDeviceSetup();

    EXPECT_EQ (newSetup.outputDeviceName, setup.outputDeviceName);
    EXPECT_EQ (newSetup.inputDeviceName, setup.inputDeviceName);

    EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);
    EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);
}

TEST_F (AudioDeviceManagerTests, InitialiseNonEmptyDeviceNamePickSuitableDefaultDevice)
{
    AudioDeviceManager manager;
    initialiseManager (manager);

    AudioDeviceManager::AudioDeviceSetup setup;

    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, String {}, &setup).isEmpty());

    const auto& newSetup = manager.getAudioDeviceSetup();

    EXPECT_EQ (newSetup.outputDeviceName, String ("x"));
    EXPECT_EQ (newSetup.inputDeviceName, String ("a"));

    EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);
    EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);
}

TEST_F (AudioDeviceManagerTests, WhenPreferredDeviceNameMatchesAnInputAndOutputOnSameTypeThatTypeIsUsed)
{
    AudioDeviceManager manager;
    initialiseManagerWithDifferentDeviceNames (manager);

    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, "bar *").isEmpty());

    EXPECT_EQ (manager.getCurrentAudioDeviceType(), String ("bar"));

    const auto& newSetup = manager.getAudioDeviceSetup();

    EXPECT_EQ (newSetup.outputDeviceName, String ("bar out a"));
    EXPECT_EQ (newSetup.inputDeviceName, String ("bar in a"));

    EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);
    EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);

    EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
}

TEST_F (AudioDeviceManagerTests, WhenPreferredDeviceNameMatchesEitherAnInputAndAnOutputButNotBothThatTypeIsUsed)
{
    AudioDeviceManager manager;
    initialiseManagerWithDifferentDeviceNames (manager);

    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, "bar out b").isEmpty());

    EXPECT_EQ (manager.getCurrentAudioDeviceType(), String ("bar"));

    const auto& newSetup = manager.getAudioDeviceSetup();

    EXPECT_EQ (newSetup.outputDeviceName, String ("bar out b"));
    EXPECT_EQ (newSetup.inputDeviceName, String ("bar in a"));

    EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);
    EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);

    EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
}

TEST_F (AudioDeviceManagerTests, WhenPreferredDeviceNameDoesNotMatchAnyInputsOrOutputsDefaultsAreUsed)
{
    AudioDeviceManager manager;
    initialiseManagerWithDifferentDeviceNames (manager);

    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, "unmatchable").isEmpty());

    EXPECT_EQ (manager.getCurrentAudioDeviceType(), String ("foo"));

    const auto& newSetup = manager.getAudioDeviceSetup();

    EXPECT_EQ (newSetup.outputDeviceName, String ("foo out a"));
    EXPECT_EQ (newSetup.inputDeviceName, String ("foo in a"));

    EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);
    EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);

    EXPECT_TRUE (manager.getCurrentAudioDevice() != nullptr);
}

TEST_F (AudioDeviceManagerTests, WhenFirstDeviceHAsNoDevicesDeviceWithDevicesIsUsedInstead)
{
    AudioDeviceManager manager;
    initialiseManagerWithEmptyDeviceType (manager);

    AudioDeviceManager::AudioDeviceSetup setup;

    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, {}, &setup).isEmpty());

    const auto& newSetup = manager.getAudioDeviceSetup();

    EXPECT_EQ (newSetup.outputDeviceName, String ("x"));
    EXPECT_EQ (newSetup.inputDeviceName, String ("a"));

    EXPECT_EQ (newSetup.outputChannels.countNumberOfSetBits(), 2);
    EXPECT_EQ (newSetup.inputChannels.countNumberOfSetBits(), 2);
}

TEST_F (AudioDeviceManagerTests, ExplicitSetOfDeviceWithDevicesInitialisationShouldRespectTheChoice)
{
    AudioDeviceManager manager;
    initialiseManagerWithEmptyDeviceType (manager);
    manager.setCurrentAudioDeviceType (mockBName, true);

    AudioDeviceManager::AudioDeviceSetup setup;
    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, {}, &setup).isEmpty());

    EXPECT_EQ (manager.getCurrentAudioDeviceType(), mockBName);
}

TEST_F (AudioDeviceManagerTests, ExplicitSetOfDeviceWithoutDevicesInitialisationShouldPickDeviceWithDevices)
{
    AudioDeviceManager manager;
    initialiseManagerWithEmptyDeviceType (manager);
    manager.setCurrentAudioDeviceType (emptyName, true);

    AudioDeviceManager::AudioDeviceSetup setup;
    EXPECT_TRUE (manager.initialise (2, 2, nullptr, true, {}, &setup).isEmpty());

    EXPECT_EQ (manager.getCurrentAudioDeviceType(), mockAName);
}

TEST_F (AudioDeviceManagerTests, CarryOutLongSequenceOfConfigChanges)
{
    AudioDeviceManager manager;
    initialiseManagerWithEmptyDeviceType (manager);
    initialiseWithDefaultDevices (manager);
    disableInputChannelsButLeaveDeviceOpen (manager);
    selectANewInputDevice (manager);
    disableInputDevice (manager);
    reenableInputDeviceWithNoChannels (manager);
    enableInputChannels (manager);
    disableInputChannelsButLeaveDeviceOpen (manager);
    switchDeviceType (manager);
    enableInputChannels (manager);
    closeDeviceByRequestingEmptyNames (manager);
}

TEST_F (AudioDeviceManagerTests, AudioDeviceManagerUpdatesSettingsBeforeNotifyingCallbacksWhenDeviceRestarts)
{
    AudioDeviceManager manager;
    auto deviceType = std::make_unique<MockDeviceType> ("foo",
                                                        StringArray { "foo in a", "foo in b" },
                                                        StringArray { "foo out a", "foo out b" });
    auto* ptr = deviceType.get();
    manager.addAudioDeviceType (std::move (deviceType));

    AudioDeviceManager::AudioDeviceSetup setup;
    setup.sampleRate = 48000.0;
    setup.bufferSize = 256;
    setup.inputDeviceName = "foo in a";
    setup.outputDeviceName = "foo out a";
    setup.useDefaultInputChannels = true;
    setup.useDefaultOutputChannels = true;
    manager.setAudioDeviceSetup (setup, true);

    const auto currentSetup = manager.getAudioDeviceSetup();
    EXPECT_EQ (currentSetup.sampleRate, setup.sampleRate);
    EXPECT_EQ (currentSetup.bufferSize, setup.bufferSize);

    MockCallback callback;
    manager.addAudioCallback (&callback);

    constexpr auto newSr = 10000.0;
    constexpr auto newBs = 1024;
    auto numCalls = 0;

    // Compilers disagree about whether newSr and newBs need to be captured
    callback.aboutToStart = [&]
    {
        ++numCalls;
        const auto current = manager.getAudioDeviceSetup();
        EXPECT_EQ (current.sampleRate, newSr);
        EXPECT_EQ (current.bufferSize, newBs);
    };

    ptr->restartDevices (newSr, newBs);
    EXPECT_EQ (numCalls, 1);
}
