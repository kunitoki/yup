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

#pragma once

#if ! YUP_MODULE_AVAILABLE_yup_audio_devices
#error This binding file requires adding the yup_audio_devices module in the project
#else
#include <yup_audio_devices/yup_audio_devices.h>
#endif

#include "yup_YupCore_bindings.h"
#include "yup_YupEvents_bindings.h"
#include "yup_YupAudioBasics_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings
{

//==============================================================================

void registerYupAudioDevicesBindings (pybind11::module_& m);

//==============================================================================

struct PyAudioIODeviceCallback : AudioIODeviceCallback
{
    // NOTE: audioDeviceIOCallbackWithContext uses raw float* pointer arrays
    // that pybind11 cannot marshal.  Python subclasses should use AudioSource +
    // AudioSourcePlayer for custom audio processing instead.  This method falls
    // through to the C++ default (no-op).
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        AudioIODeviceCallback::audioDeviceIOCallbackWithContext (
            inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples, context);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        PYBIND11_OVERRIDE_PURE (void, AudioIODeviceCallback, audioDeviceAboutToStart, device);
    }

    void audioDeviceStopped() override
    {
        PYBIND11_OVERRIDE_PURE (void, AudioIODeviceCallback, audioDeviceStopped);
    }

    void audioDeviceError (const String& errorMessage) override
    {
        PYBIND11_OVERRIDE (void, AudioIODeviceCallback, audioDeviceError, errorMessage);
    }
};

} // namespace yup::Bindings
