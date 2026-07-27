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

DeviceIOSelector::DeviceIOSelector()
{
    setOpaque (false);

    outputLabel.setText ("Output:", dontSendNotification);
    inputLabel.setText ("Input:", dontSendNotification);
    testButton.setButtonText ("Test");

    testButton.onClick = [this]
    {
        if (onTestClicked != nullptr)
            onTestClicked();
    };

    addAndMakeVisible (outputLabel);
    addAndMakeVisible (outputCombo);
    addAndMakeVisible (testButton);
    addAndMakeVisible (inputLabel);
    addAndMakeVisible (inputCombo);
}

void DeviceIOSelector::populate (AudioIODeviceType& type,
                                 const String& currentOutput,
                                 const String& currentInput)
{
    outputCombo.onSelectedItemChanged = nullptr;
    inputCombo.onSelectedItemChanged = nullptr;

    type.scanForDevices();

    outputCombo.clear();
    const StringArray outputNames = type.getDeviceNames (false);
    for (int i = 0; i < outputNames.size(); ++i)
        outputCombo.addItem (outputNames[i], i + 1);

    const int outIdx = outputNames.indexOf (currentOutput);
    outputCombo.setSelectedId (outIdx >= 0 ? outIdx + 1 : 1, dontSendNotification);

    inputCombo.clear();
    const StringArray inputNames = type.getDeviceNames (true);
    for (int i = 0; i < inputNames.size(); ++i)
        inputCombo.addItem (inputNames[i], i + 1);

    const int inIdx = inputNames.indexOf (currentInput);
    inputCombo.setSelectedId (inIdx >= 0 ? inIdx + 1 : 1, dontSendNotification);

    outputCombo.onSelectedItemChanged = [this]
    {
        fireDeviceChanged();
    };
    inputCombo.onSelectedItemChanged = [this]
    {
        fireDeviceChanged();
    };
}

void DeviceIOSelector::fireDeviceChanged()
{
    if (onDeviceChanged != nullptr)
        onDeviceChanged (outputCombo.getText(), inputCombo.getText());
}

void DeviceIOSelector::resized()
{
    auto bounds = getLocalBounds();
    const int rowH = bounds.getHeight() / 2;
    const int labelW = 170;
    const int testW = 60;

    auto outRow = bounds.removeFromTop (rowH);
    outputLabel.setBounds (outRow.removeFromLeft (labelW).reduced (0, 10));
    testButton.setBounds (outRow.removeFromRight (testW).reduced (2, 4));
    outputCombo.setBounds (outRow.reduced (2, 4));

    inputLabel.setBounds (bounds.removeFromLeft (labelW).reduced (0, 10));
    bounds.removeFromRight (testW);
    inputCombo.setBounds (bounds.reduced (2, 4));
}

} // namespace yup
