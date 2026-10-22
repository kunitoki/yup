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

    closeButton.setButtonText ("Close");
    closeButton.onClick = [this]
    {
        if (onClose != nullptr)
            onClose();
    };
    addChildComponent (closeButton);

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

    listsChanged();
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

    listsChanged();
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
    const auto background = ApplicationTheme::findComponentColor (*this, DocumentWindow::Style::backgroundColorId).value_or (Colors::dimgray);

    if (! hasShadow())
    {
        g.setFillColor (background);
        g.fillAll();
        return;
    }

    const auto card = getCardBounds();

    // A wide, soft key shadow under a tight contact shadow.
    g.setFillColor (Colors::black.withAlpha (0.35f));
    g.setFeather (24.0f);
    g.fillRoundedRect (card.translated (0.0f, 8.0f), cornerRadius);

    g.setFillColor (Colors::black.withAlpha (0.25f));
    g.setFeather (4.0f);
    g.fillRoundedRect (card.translated (0.0f, 1.0f), cornerRadius);

    g.setFeather (0.0f);
    g.setFillColor (background);
    g.fillRoundedRect (card, cornerRadius);
}

void AudioDeviceManagerPanel::resized()
{
    const auto card = getCardBounds();
    const auto listRows = fitListRows (card.getHeight());
    auto bounds = card.reduced (margin);

    closeButton.setVisible (onClose != nullptr);
    if (closeButton.isVisible())
    {
        closeButton.setBounds (bounds.removeFromBottom (itemHeight).removeFromRight (90.0f));
        bounds.removeFromBottom (itemGap);
    }

    const auto place = [&bounds] (Component& component, float height)
    {
        component.setBounds (bounds.removeFromTop (height));
        bounds.removeFromTop (itemGap);
    };

    place (deviceTypeSelector, itemHeight);
    place (deviceIOSelector, itemHeight * 2.0f + itemGap);
    place (outputChannelSection, itemHeight * static_cast<float> (1 + listRows[0]));
    place (inputChannelSection, itemHeight * static_cast<float> (1 + listRows[1]));
    place (rateBufferSelector, itemHeight * 2.0f + itemGap);
    place (midiSection, itemHeight * static_cast<float> (2 + listRows[2]) + itemGap);
}

void AudioDeviceManagerPanel::parentHierarchyChanged()
{
    setOpaque (! hasShadow());
    repaint();
}

float AudioDeviceManagerPanel::getPreferredHeight() const
{
    const auto listRows = getNaturalListRows();
    const auto shadowInsets = hasShadow() ? shadowInsetTop + shadowInsetBottom : 0.0f;

    return getFixedHeight() + itemHeight * static_cast<float> (listRows[0] + listRows[1] + listRows[2]) + shadowInsets;
}

void AudioDeviceManagerPanel::listsChanged()
{
    resized();

    if (onPreferredHeightChanged != nullptr)
        onPreferredHeightChanged();
}

std::array<int, 3> AudioDeviceManagerPanel::getNaturalListRows() const
{
    return { jmax (1, outputChannelSection.getNumChannels()),
             jmax (1, inputChannelSection.getNumChannels()),
             jmax (1, midiSection.getNumInputDevices()) };
}

std::array<int, 3> AudioDeviceManagerPanel::fitListRows (float height) const
{
    auto listRows = getNaturalListRows();
    const auto availableRows = static_cast<int> ((height - getFixedHeight()) / itemHeight);

    while (listRows[0] + listRows[1] + listRows[2] > availableRows)
    {
        auto longest = std::max_element (listRows.begin(), listRows.end());
        if (*longest <= 1)
            break;

        --*longest;
    }

    return listRows;
}

float AudioDeviceManagerPanel::getFixedHeight() const noexcept
{
    const auto closeRow = onClose != nullptr ? itemHeight + itemGap : 0.0f;

    return margin * 2.0f + itemHeight * static_cast<float> (numFixedItems) + itemGap * static_cast<float> (numGaps) + closeRow;
}

bool AudioDeviceManagerPanel::hasShadow() const noexcept
{
    return getParentComponent() != nullptr;
}

Rectangle<float> AudioDeviceManagerPanel::getCardBounds() const
{
    if (! hasShadow())
        return getLocalBounds();

    return getLocalBounds().reduced (shadowInsetSides, shadowInsetTop, shadowInsetSides, shadowInsetBottom);
}

} // namespace yup
