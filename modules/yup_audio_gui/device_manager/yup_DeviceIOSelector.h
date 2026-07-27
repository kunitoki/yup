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
/** Output and input device ComboBoxes with a Test button. */
class YUP_API DeviceIOSelector : public Component
{
public:
    //==============================================================================
    DeviceIOSelector();

    //==============================================================================
    /** Populates the output/input ComboBoxes from a device type.
        @param type          The current AudioIODeviceType (will be scanned for devices).
        @param currentOutput The currently selected output device name.
        @param currentInput  The currently selected input device name.
    */
    void populate (AudioIODeviceType& type,
                   const String& currentOutput,
                   const String& currentInput);

    //==============================================================================
    /** Called when the user picks a different output or input device. */
    std::function<void (const String& output, const String& input)> onDeviceChanged;

    /** Called when the user clicks the Test button. */
    std::function<void()> onTestClicked;

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    void fireDeviceChanged();

    Label outputLabel;
    ComboBox outputCombo;
    TextButton testButton;
    Label inputLabel;
    ComboBox inputCombo;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceIOSelector)
};

} // namespace yup
