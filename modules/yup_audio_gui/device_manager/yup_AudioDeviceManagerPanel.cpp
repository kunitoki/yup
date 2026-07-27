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

AudioDeviceManagerPanel::AudioDeviceManagerPanel (AudioDeviceManager& m)
    : manager (m)
{
    outputChannelSection.setText ("Active output channels:", dontSendNotification);
    inputChannelSection.setText ("Active input channels:", dontSendNotification);

    addAndMakeVisible (deviceTypeSelector);
    addAndMakeVisible (deviceIOSelector);
    addAndMakeVisible (outputChannelSection);
    addAndMakeVisible (inputChannelSection);
    addAndMakeVisible (rateBufferSelector);
    addAndMakeVisible (midiSection);

    deviceTypeSelector.onTypeChanged = [this] (const String& typeName)
    {
        manager.setCurrentAudioDeviceType (typeName, true);
        populateFromManager();
    };

    deviceIOSelector.onDeviceChanged = [this] (const String& output, const String& input)
    {
        stagedSetup.outputDeviceName = output;
        stagedSetup.inputDeviceName = input;
        repopulateDeviceDependent();
        applyToManager();
    };

    deviceIOSelector.onTestClicked = [this]
    {
        manager.playTestSound();
    };

    outputChannelSection.onChannelsChanged = [this] (const BigInteger& active)
    {
        stagedSetup.outputChannels = active;
        stagedSetup.useDefaultOutputChannels = false;
        triggerAsyncUpdate();
    };

    inputChannelSection.onChannelsChanged = [this] (const BigInteger& active)
    {
        stagedSetup.inputChannels = active;
        stagedSetup.useDefaultInputChannels = false;
        triggerAsyncUpdate();
    };

    rateBufferSelector.onChanged = [this] (double rate, int bufferSize)
    {
        stagedSetup.sampleRate = rate;
        stagedSetup.bufferSize = bufferSize;
        applyToManager();
    };

    midiSection.onInputsChanged = [this] (const StringArray& ids)
    {
        stagedMidiInputIds = ids;
        triggerAsyncUpdate();
    };

    midiSection.onOutputChanged = [this] (const String& id)
    {
        stagedMidiOutputId = id;
        applyToManager();
    };

    populateFromManager();
}

void AudioDeviceManagerPanel::populateFromManager()
{
    stagedSetup = manager.getAudioDeviceSetup();

    deviceTypeSelector.populate (manager.getAvailableDeviceTypes(),
                                 manager.getCurrentAudioDeviceType());

    const auto& types = manager.getAvailableDeviceTypes();
    for (int i = 0; i < types.size(); ++i)
    {
        if (types[i]->getTypeName() == manager.getCurrentAudioDeviceType())
        {
            deviceIOSelector.populate (*types[i],
                                       stagedSetup.outputDeviceName,
                                       stagedSetup.inputDeviceName);
            break;
        }
    }

    repopulateDeviceDependent();

    const auto midiInputDevices = MidiInput::getAvailableDevices();
    stagedMidiInputIds.clear();
    for (const auto& dev : midiInputDevices)
    {
        if (manager.isMidiInputDeviceEnabled (dev.identifier))
            stagedMidiInputIds.add (dev.identifier);
    }
    midiSection.populateInputs (midiInputDevices, stagedMidiInputIds);
    midiSection.populateOutput (MidiOutput::getAvailableDevices(),
                                manager.getDefaultMidiOutputIdentifier());
    stagedMidiOutputId = manager.getDefaultMidiOutputIdentifier();
}

void AudioDeviceManagerPanel::repopulateDeviceDependent()
{
    AudioIODevice* deviceToQuery = nullptr;
    std::unique_ptr<AudioIODevice> tempDevice;

    // Fast path: use the currently open device when it matches the staged setup
    if (auto* current = manager.getCurrentAudioDevice())
    {
        const auto appliedSetup = manager.getAudioDeviceSetup();
        if (stagedSetup.outputDeviceName == appliedSetup.outputDeviceName
            && stagedSetup.inputDeviceName == appliedSetup.inputDeviceName)
        {
            deviceToQuery = current;
        }
    }

    // Staged device differs — create a temporary device to query its properties
    if (deviceToQuery == nullptr)
    {
        const auto& types = manager.getAvailableDeviceTypes();
        for (int i = 0; i < types.size(); ++i)
        {
            if (types[i]->getTypeName() != manager.getCurrentAudioDeviceType())
                continue;

            tempDevice.reset (types[i]->createDevice (stagedSetup.outputDeviceName,
                                                      stagedSetup.inputDeviceName));
            deviceToQuery = tempDevice.get();
            break;
        }
    }

    if (deviceToQuery != nullptr)
    {
        outputChannelSection.populate (deviceToQuery->getOutputChannelNames(), stagedSetup.outputChannels);
        inputChannelSection.populate (deviceToQuery->getInputChannelNames(), stagedSetup.inputChannels);
        rateBufferSelector.populate (deviceToQuery->getAvailableSampleRates(), stagedSetup.sampleRate, deviceToQuery->getAvailableBufferSizes(), stagedSetup.bufferSize);
    }
    else
    {
        outputChannelSection.populate ({}, {});
        inputChannelSection.populate ({}, {});
        rateBufferSelector.populate ({}, 0.0, {}, 0);
    }
}

void AudioDeviceManagerPanel::applyToManager()
{
    manager.setAudioDeviceSetup (stagedSetup, true);

    const auto midiInputDevices = MidiInput::getAvailableDevices();
    for (const auto& dev : midiInputDevices)
        manager.setMidiInputDeviceEnabled (dev.identifier,
                                           stagedMidiInputIds.contains (dev.identifier));

    manager.setDefaultMidiOutputDevice (stagedMidiOutputId);

    // Refresh: the device may have clamped sample rate or buffer size
    populateFromManager();
}

void AudioDeviceManagerPanel::handleAsyncUpdate()
{
    applyToManager();
}

MidiDeviceListConnection AudioDeviceManagerPanel::makeMidiDeviceListConnection()
{
    return MidiDeviceListConnection::make ([this]
    {
        populateFromManager();
    });
}

void AudioDeviceManagerPanel::paint (Graphics& g)
{
    g.setFillColor (findColor (DocumentWindow::Style::backgroundColorId).value_or (Colors::dimgray));
    g.fillAll();
}

void AudioDeviceManagerPanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);
    const int rowH = 44;
    const int gap = 4;
    const int listH = 100;

    deviceTypeSelector.setBounds (bounds.removeFromTop (rowH));
    bounds.removeFromTop (gap);

    deviceIOSelector.setBounds (bounds.removeFromTop (rowH * 2));
    bounds.removeFromTop (gap);

    outputChannelSection.setBounds (bounds.removeFromTop (20 + listH));
    bounds.removeFromTop (gap);

    inputChannelSection.setBounds (bounds.removeFromTop (20 + listH));
    bounds.removeFromTop (gap);

    rateBufferSelector.setBounds (bounds.removeFromTop (rowH * 2));
    bounds.removeFromTop (gap);

    midiSection.setBounds (bounds.removeFromTop (20 + listH + rowH));
}

} // namespace yup
