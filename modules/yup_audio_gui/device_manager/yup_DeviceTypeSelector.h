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
/** A labelled ComboBox for selecting the audio driver type. */
class YUP_API DeviceTypeSelector : public Component
{
public:
    //==============================================================================
    DeviceTypeSelector();

    //==============================================================================
    /** Populates the ComboBox from the available device types.
        @param types        The list of available device types (from AudioDeviceManager).
        @param currentType  The name of the currently active device type.
    */
    void populate (const OwnedArray<AudioIODeviceType>& types, const String& currentType);

    //==============================================================================
    /** Called when the user selects a different driver type. */
    std::function<void (const String& typeName)> onTypeChanged;

    //==============================================================================
    /** @internal */
    void resized() override;

private:
    Label typeLabel;
    ComboBox typeCombo;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceTypeSelector)
};

} // namespace yup
