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

#include <gmock/gmock.h>

#include <yup_audio_devices/yup_audio_devices.h>

// ==============================================================================
// Mock yup::AudioIODeviceCallback
// ==============================================================================

class MockAudioIODeviceCallback : public yup::AudioIODeviceCallback
{
public:
    MOCK_METHOD (void, audioDeviceIOCallbackWithContext, (const float* const*, int, float* const*, int, int, const yup::AudioIODeviceCallbackContext&), (override));
    MOCK_METHOD (void, audioDeviceAboutToStart, (yup::AudioIODevice*), (override));
    MOCK_METHOD (void, audioDeviceStopped, (), (override));
    MOCK_METHOD (void, audioDeviceError, (const yup::String&), (override));
};

// ==============================================================================
// Mock yup::AudioIODevice
// ==============================================================================

class MockAudioIODevice : public yup::AudioIODevice
{
public:
    MOCK_METHOD (yup::String, open, (const yup::BigInteger&, const yup::BigInteger&, double, int), (override));
    MOCK_METHOD (void, close, (), (override));
    MOCK_METHOD (bool, isOpen, (), (override));
    MOCK_METHOD (void, start, (yup::AudioIODeviceCallback*), (override));
    MOCK_METHOD (void, stop, (), (override));
    MOCK_METHOD (bool, isPlaying, (), (override));
    MOCK_METHOD (yup::StringArray, getOutputChannelNames, (), (override));
    MOCK_METHOD (yup::StringArray, getInputChannelNames, (), (override));
    MOCK_METHOD (yup::Array<double>, getAvailableSampleRates, (), (override));
    MOCK_METHOD (yup::Array<int>, getAvailableBufferSizes, (), (override));
    MOCK_METHOD (int, getDefaultBufferSize, (), (override));
    MOCK_METHOD (int, getCurrentBufferSizeSamples, (), (override));
    MOCK_METHOD (double, getCurrentSampleRate, (), (override));
    MOCK_METHOD (int, getCurrentBitDepth, (), (override));
    MOCK_METHOD (yup::BigInteger, getActiveOutputChannels, (), (const, override));
    MOCK_METHOD (yup::BigInteger, getActiveInputChannels, (), (const, override));
    MOCK_METHOD (int, getOutputLatencyInSamples, (), (override));
    MOCK_METHOD (int, getInputLatencyInSamples, (), (override));
    MOCK_METHOD (yup::String, getLastError, (), (override));
};

// ==============================================================================
// Mock yup::AudioIODeviceType
// ==============================================================================

class MockAudioIODeviceType : public yup::AudioIODeviceType
{
public:
    MOCK_METHOD (yup::StringArray, getDeviceNames, (bool), (const, override));
    MOCK_METHOD (int, getDefaultDeviceIndex, (bool), (const, override));
    MOCK_METHOD (int, getIndexOfDevice, (yup::AudioIODevice*, bool), (const, override));
    MOCK_METHOD (yup::AudioIODevice*, createDevice, (const yup::String&, const yup::String&), (override));
    MOCK_METHOD (bool, hasSeparateInputsAndOutputs, (), (const, override));
    MOCK_METHOD (void, scanForDevices, (), (override));
};
