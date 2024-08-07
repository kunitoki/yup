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

namespace juce
{

void AudioIODeviceCallback::audioDeviceIOCallbackWithContext ([[maybe_unused]] const float* const* inputChannelData,
                                                              [[maybe_unused]] int numInputChannels,
                                                              [[maybe_unused]] float* const* outputChannelData,
                                                              [[maybe_unused]] int numOutputChannels,
                                                              [[maybe_unused]] int numSamples,
                                                              [[maybe_unused]] const AudioIODeviceCallbackContext& context) {}

//==============================================================================
AudioIODevice::AudioIODevice (const String& deviceName, const String& deviceTypeName)
    : name (deviceName)
    , typeName (deviceTypeName)
{
}

AudioIODevice::~AudioIODevice() {}

void AudioIODeviceCallback::audioDeviceError (const String&) {}

bool AudioIODevice::setAudioPreprocessingEnabled (bool) { return false; }

bool AudioIODevice::hasControlPanel() const { return false; }

int AudioIODevice::getXRunCount() const noexcept { return -1; }

bool AudioIODevice::showControlPanel()
{
    jassertfalse; // this should only be called for devices which return true from
                  // their hasControlPanel() method.
    return false;
}

} // namespace juce
