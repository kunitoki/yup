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

#include <yup_audio_basics/yup_audio_basics.h>

// ==============================================================================
// Mock yup::AudioSource
// ==============================================================================

class MockAudioSource : public yup::AudioSource
{
public:
    MOCK_METHOD (void, prepareToPlay, (int, double), (override));
    MOCK_METHOD (void, releaseResources, (), (override));
    MOCK_METHOD (void, getNextAudioBlock, (const yup::AudioSourceChannelInfo&), (override));
};

// ==============================================================================
// Mock yup::PositionableAudioSource
// ==============================================================================

class MockPositionableAudioSource : public yup::PositionableAudioSource
{
public:
    MOCK_METHOD (void, prepareToPlay, (int, double), (override));
    MOCK_METHOD (void, releaseResources, (), (override));
    MOCK_METHOD (void, getNextAudioBlock, (const yup::AudioSourceChannelInfo&), (override));
    MOCK_METHOD (void, setNextReadPosition, (yup::int64), (override));
    MOCK_METHOD (yup::int64, getNextReadPosition, (), (const, override));
    MOCK_METHOD (yup::int64, getTotalLength, (), (const, override));
    MOCK_METHOD (bool, isLooping, (), (const, override));
    MOCK_METHOD (void, setLooping, (bool), (override));
};

// ==============================================================================
// Mock yup::SynthesiserVoice
// ==============================================================================

class MockSynthesiserVoice : public yup::SynthesiserVoice
{
public:
    MOCK_METHOD (bool, canPlaySound, (yup::SynthesiserSound*), (override));
    MOCK_METHOD (void, startNote, (int, float, yup::SynthesiserSound*, int), (override));
    MOCK_METHOD (void, stopNote, (float, bool), (override));
    MOCK_METHOD (void, pitchWheelMoved, (int), (override));
    MOCK_METHOD (void, controllerMoved, (int, int), (override));
    MOCK_METHOD (void, renderNextBlock, (yup::AudioBuffer<float>&, int, int), (override));
};

// ==============================================================================
// Mock yup::MidiKeyboardState::Listener
// ==============================================================================

class MockMidiKeyboardStateListener : public yup::MidiKeyboardState::Listener
{
public:
    MOCK_METHOD (void, handleNoteOn, (yup::MidiKeyboardState*, int, int, float), (override));
    MOCK_METHOD (void, handleNoteOff, (yup::MidiKeyboardState*, int, int, float), (override));
};
