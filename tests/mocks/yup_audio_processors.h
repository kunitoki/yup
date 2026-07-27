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

#include <yup_audio_processors/yup_audio_processors.h>

// ==============================================================================
// Mock yup::AudioProcessorBase
//
// AudioProcessorBase has no pure virtual methods of its own; its nested
// Listener class does. For tests that need a concrete AudioProcessorBase,
// use the real class directly.
// ==============================================================================

// ==============================================================================
// Mock yup::AudioProcessorBase::Listener
// ==============================================================================

class MockAudioProcessorBaseListener : public yup::AudioProcessorBase::Listener
{
public:
    MOCK_METHOD (void, audioProcessorChanged, (yup::AudioProcessorBase*, const yup::AudioProcessorBase::ChangeDetails&), (override));
};

// ==============================================================================
// Mock yup::AudioProcessor
//
// YUP's AudioProcessor extends DomainProcessor<AudioProcessContext, AudioSpec>.
// Only the methods that are virtual (pure or overrideable) in YUP are mocked.
// ==============================================================================

class MockAudioProcessor : public yup::AudioProcessor
{
public:
    using yup::AudioProcessor::AudioProcessor;

    // DomainProcessor pure virtuals
    MOCK_METHOD (void, prepareToPlay, (const yup::AudioSpec&), (override));
    MOCK_METHOD (void, releaseResources, (), (override));
    MOCK_METHOD (void, processBlock, (yup::AudioProcessContext<float>&), (override));

    // AudioProcessor virtuals
    MOCK_METHOD (bool, hasEditor, (), (const, override));
    MOCK_METHOD (yup::AudioProcessorEditor*, createEditor, (), (override));
    MOCK_METHOD (bool, acceptsMidi, (), (const, noexcept, override));
    MOCK_METHOD (bool, producesMidi, (), (const, noexcept, override));
};

// ==============================================================================
// Mock yup::AudioParameter::Listener
// ==============================================================================

class MockAudioParameterListener : public yup::AudioParameter::Listener
{
public:
    MOCK_METHOD (void, parameterValueChanged, (const yup::AudioParameter::Ptr&, int), (override));
    MOCK_METHOD (void, parameterGestureBegin, (const yup::AudioParameter::Ptr&, int), (override));
    MOCK_METHOD (void, parameterGestureEnd, (const yup::AudioParameter::Ptr&, int), (override));
};
