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

DeviceTypeSelector::DeviceTypeSelector()
{
    setOpaque (false);

    typeLabel.setText ("Driver:", dontSendNotification);
    addAndMakeVisible (typeLabel);
    addAndMakeVisible (typeCombo);
}

void DeviceTypeSelector::populate (const OwnedArray<AudioIODeviceType>& types, const String& currentType)
{
    typeCombo.onSelectedItemChanged = nullptr;
    typeCombo.clear();

    int selectedId = 1;
    for (int i = 0; i < types.size(); ++i)
    {
        const String name = types[i]->getTypeName();
        typeCombo.addItem (name, i + 1);
        if (name == currentType)
            selectedId = i + 1;
    }

    typeCombo.setSelectedId (selectedId, dontSendNotification);

    typeCombo.onSelectedItemChanged = [this]
    {
        if (onTypeChanged != nullptr)
            onTypeChanged (typeCombo.getText());
    };
}

void DeviceTypeSelector::resized()
{
    auto bounds = getLocalBounds();
    typeLabel.setBounds (bounds.removeFromLeft (170).reduced (0, 10));
    typeCombo.setBounds (bounds.reduced (2, 4));
}

} // namespace yup
