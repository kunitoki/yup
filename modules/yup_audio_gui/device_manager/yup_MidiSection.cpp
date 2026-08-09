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

//==============================================================================
MidiSection::MidiInputRowComponent::MidiInputRowComponent()
{
    setOpaque (false);
    addAndMakeVisible (toggle);
    addAndMakeVisible (nameLabel);
}

void MidiSection::MidiInputRowComponent::setup (const String& name,
                                                bool enabled,
                                                std::function<void (bool)> onToggled)
{
    toggle.setButtonText ({});
    toggle.setToggleState (enabled, dontSendNotification);
    toggle.onClick = [this, onToggled]
    {
        if (onToggled != nullptr)
            onToggled (toggle.getToggleState());
    };
    nameLabel.setText (name, dontSendNotification);
}

void MidiSection::MidiInputRowComponent::resized()
{
    auto bounds = getLocalBounds().reduced (2, 2);
    const int toggleW = bounds.getHeight();
    toggle.setBounds (bounds.removeFromLeft (toggleW));
    bounds.removeFromLeft (4);
    nameLabel.setBounds (bounds);
}

//==============================================================================
void MidiSection::InputModel::setDevices (const Array<MidiDeviceInfo>& devices,
                                          const StringArray& ids)
{
    midiDevices = devices;
    enabledIds = ids;
}

StringArray MidiSection::InputModel::getEnabledIds() const
{
    return enabledIds;
}

int MidiSection::InputModel::getNumRows()
{
    return midiDevices.size();
}

Component* MidiSection::InputModel::refreshComponentForRow (int rowIndex, Component* existing)
{
    auto* row = dynamic_cast<MidiInputRowComponent*> (existing);
    if (row == nullptr)
        row = new MidiInputRowComponent();

    const auto& dev = midiDevices[rowIndex];
    const bool enabled = enabledIds.contains (dev.identifier);

    row->setup (dev.name, enabled, [this, id = dev.identifier] (bool on)
    {
        if (on)
        {
            if (! enabledIds.contains (id))
                enabledIds.add (id);
        }
        else
        {
            enabledIds.removeString (id);
        }

        if (onInputsChanged != nullptr)
            onInputsChanged (enabledIds);
    });

    return row;
}

//==============================================================================
MidiSection::MidiSection()
{
    setOpaque (false);

    inputsLabel.setText ("Active MIDI inputs:", dontSendNotification);
    outputLabel.setText ("MIDI Output:", dontSendNotification);

    inputsListBox.setModel (&inputsModel);

    addAndMakeVisible (inputsLabel);
    addAndMakeVisible (inputsListBox);
    addAndMakeVisible (outputLabel);
    addAndMakeVisible (outputCombo);
}

MidiSection::~MidiSection() = default;

void MidiSection::populateInputs (const Array<MidiDeviceInfo>& devices,
                                  const StringArray& enabledIds)
{
    inputsModel.onInputsChanged = [this] (const StringArray& ids)
    {
        if (onInputsChanged != nullptr)
            onInputsChanged (ids);
    };

    inputsModel.setDevices (devices, enabledIds);
    inputsListBox.updateContent();
}

void MidiSection::populateOutput (const Array<MidiDeviceInfo>& devices, const String& selectedId)
{
    outputDevices = devices;

    outputCombo.onSelectedItemChanged = nullptr;
    outputCombo.clear();
    outputCombo.addItem ("(none)", 1);

    int selectedComboId = 1;
    for (int i = 0; i < devices.size(); ++i)
    {
        outputCombo.addItem (devices[i].name, i + 2);
        if (devices[i].identifier == selectedId)
            selectedComboId = i + 2;
    }
    outputCombo.setSelectedId (selectedComboId, dontSendNotification);

    outputCombo.onSelectedItemChanged = [this]
    {
        if (onOutputChanged == nullptr)
            return;

        const int id = outputCombo.getSelectedId();
        if (id <= 1)
        {
            onOutputChanged ({});
        }
        else
        {
            const int devIdx = id - 2;
            if (devIdx >= 0 && devIdx < outputDevices.size())
                onOutputChanged (outputDevices[devIdx].identifier);
        }
    };
}

void MidiSection::resized()
{
    auto bounds = getLocalBounds();
    inputsLabel.setBounds (bounds.removeFromTop (20));
    inputsListBox.setBounds (bounds.removeFromTop (100).reduced (2, 2));

    auto outRow = bounds.removeFromTop (44);
    outputLabel.setBounds (outRow.removeFromLeft (170).reduced (0, 10));
    outputCombo.setBounds (outRow.reduced (2, 4));
}

} // namespace yup
